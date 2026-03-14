import type {
    HttpBridgeResponseCache,
    OutboundHttpRequestEnvelope,
    RuntimeState
} from "./types";
import {
    encodeUtf8,
    readCString,
    utf8ByteLength,
    writeBytesIntoMemory,
    writeCString
} from "./memory";
import {
    getAllocOrThrow,
    getFreeOrThrow,
    getMemoryOrThrow
} from "./runtime";

function getInstanceOrThrow<Env>(state: RuntimeState<Env>) {
    if (!state.instance) {
        throw new Error("Wasm instance has not been initialised yet.");
    }
    return state.instance;
}

function getRequestContextOrThrow<Env>(state: RuntimeState<Env>) {
    if (!state.requestContext) {
        throw new Error("No active request context.");
    }
    return state.requestContext;
}

function normalizeHeaders(headers: Headers): Record<string, string> {
    const out: Record<string, string> = {};
    for (const [key, value] of headers.entries()) {
        out[key.toLowerCase()] = value;
    }
    return out;
}

function estimateResponseBytes(resp: {
    bodyText: string;
    headers: Record<string, string>;
}): number {
    let total = utf8ByteLength(resp.bodyText);
    for (const [k, v] of Object.entries(resp.headers)) {
        total += utf8ByteLength(k);
        total += utf8ByteLength(v);
    }
    return total >>> 0;
}

function sweepExpiredResponses<Env>(state: RuntimeState<Env>): void {
    const now = Date.now();
    const ttl = state.options.responseTtlMs;

    for (const [id, entry] of state.httpResponses.entries()) {
        if (now - entry.createdAtMs > ttl) {
            state.httpResponses.delete(id);
        }
    }
}

function getCurrentBridgeBytes<Env>(state: RuntimeState<Env>): number {
    let total = 0;
    for (const entry of state.httpResponses.values()) {
        total += entry.estimatedBytes;
    }
    return total >>> 0;
}

function assertBridgeCapacity<Env>(
    state: RuntimeState<Env>,
    nextEntryBytes: number
): void {
    sweepExpiredResponses(state);

    if (state.httpResponses.size >= state.options.maxHttpResponseHandles) {
        throw new Error("Too many outstanding HTTP response handles");
    }

    const totalAfterInsert = getCurrentBridgeBytes(state) + nextEntryBytes;
    if (totalAfterInsert > state.options.maxTotalBridgeBytes) {
        throw new Error("HTTP response bridge memory budget exceeded");
    }
}

function getResponseByIdOrEmpty<Env>(
    state: RuntimeState<Env>,
    responseId: number
): HttpBridgeResponseCache {
    sweepExpiredResponses(state);

    return (
        state.httpResponses.get(responseId) ?? {
            status: 0,
            bodyText: "",
            headers: {},
            createdAtMs: 0,
            estimatedBytes: 0
        }
    );
}

export function createAresAbiImports<Env>(state: RuntimeState<Env>) {
    return {
        env: {
            emscripten_notify_memory_growth: () => {},
            emscripten_date_now: () => Date.now(),
            emscripten_get_now: () => performance.now(),
            abort: (
                msg?: unknown,
                file?: unknown,
                line?: number,
                col?: number
            ): never => {
                throw new Error(
                    `abort: ${String(msg)} @ ${String(file)}:${String(line)}:${String(col)}`
                );
            },

            alloc(size: number): number {
                const instance = getInstanceOrThrow(state);
                const alloc = getAllocOrThrow(instance);
                return alloc(size) >>> 0;
            },

            free_mem(ptr: number, size: number): void {
                const instance = getInstanceOrThrow(state);
                const free = getFreeOrThrow(instance);
                free(ptr >>> 0, size >>> 0);
            },

            abi_log(messagePtr: number): void {
                const instance = getInstanceOrThrow(state);
                const memory = getMemoryOrThrow(instance);
                const message = readCString(memory, messagePtr);

                if (state.options.onLog) {
                    state.options.onLog(message);
                } else if (state.options.debug) {
                    console.log("[AresWasm]", message);
                }
            },

            abi_http_get_user_agent_name(): number {
                const instance = getInstanceOrThrow(state);
                const ctx = getRequestContextOrThrow(state);

                const userAgent =
                    ctx.request.headers.get("user-agent") ?? "Cloudflare-Worker";

                const memory = getMemoryOrThrow(instance);
                const alloc = getAllocOrThrow(instance);

                return writeCString(memory, alloc, userAgent);
            },

            __asyncjs__abi_http_fetch_blocking_async: async (requestJsonCstrPtr: number): Promise<number> => {
                const instance = getInstanceOrThrow(state);
                const memory = getMemoryOrThrow(instance);

                try {
                    const requestJson = readCString(memory, requestJsonCstrPtr);
                    const outbound = JSON.parse(requestJson) as OutboundHttpRequestEnvelope;

                    if (state.options.debug) {
                        console.log("[AresWasm] abi_http_fetch_blocking outbound =", outbound);
                    }

                    const response = await fetch(outbound.url, {
                        method: outbound.method ?? "GET",
                        headers: outbound.headers,
                        body: outbound.body ?? undefined
                    });

                    console.log("[AresWasm] abi_http_fetch_blocking fetch completed, status =", response.status);
                    const bodyText = await response.text();
                    const bodyBytes = utf8ByteLength(bodyText);

                    if (bodyBytes > state.options.maxResponseBodyBytes) {
                        throw new Error(
                            `HTTP response body too large: ${bodyBytes} bytes exceeds maxResponseBodyBytes=${state.options.maxResponseBodyBytes}`
                        );
                    }

                    const headers = normalizeHeaders(response.headers);
                    const estimatedBytes = estimateResponseBytes({
                        bodyText,
                        headers
                    });

                    assertBridgeCapacity(state, estimatedBytes);

                    const responseId = state.nextHttpResponseId++;
                    state.httpResponses.set(responseId, {
                        status: response.status,
                        bodyText,
                        headers,
                        createdAtMs: Date.now(),
                        estimatedBytes
                    });

                    return responseId >>> 0;
                } catch (error) {
                    console.error("[AresWasm] abi_http_fetch_blocking failed:", error);
                    return 0;
                }
            },

            abi_http_response_get_status(responseId: number): number {
                return getResponseByIdOrEmpty(state, responseId).status >>> 0;
            },

            abi_http_response_get_body_len(responseId: number): number {
                return utf8ByteLength(
                    getResponseByIdOrEmpty(state, responseId).bodyText
                );
            },

            abi_http_response_copy_body(
                responseId: number,
                outPtr: number,
                maxLen: number
            ): number {
                const instance = getInstanceOrThrow(state);
                const memory = getMemoryOrThrow(instance);
                const bodyText = getResponseByIdOrEmpty(state, responseId).bodyText;

                return writeBytesIntoMemory(
                    memory,
                    outPtr,
                    encodeUtf8(bodyText),
                    maxLen
                );
            },

            abi_http_response_copy_header(
                responseId: number,
                keyCstrPtr: number,
                outPtr: number,
                maxLen: number
            ): number {
                const instance = getInstanceOrThrow(state);
                const memory = getMemoryOrThrow(instance);
                const key = readCString(memory, keyCstrPtr).toLowerCase();

                const value =
                    getResponseByIdOrEmpty(state, responseId).headers[key] ?? "";

                return writeBytesIntoMemory(
                    memory,
                    outPtr,
                    encodeUtf8(value),
                    maxLen
                );
            },

            abi_http_response_free(responseId: number): void {
                state.httpResponses.delete(responseId);
            }
        }
    };
}