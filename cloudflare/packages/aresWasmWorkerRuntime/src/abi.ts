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
        ares_abi: {
            abi_log(messagePtr: number): void {
                const instance = getInstanceOrThrow(state);
                const message = readCString(instance.exports.memory, messagePtr);

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

                return writeCString(
                    instance.exports.memory,
                    instance.exports.alloc,
                    userAgent
                );
            },

            async abi_http_fetch_blocking(requestJsonCstrPtr: number): Promise<number> {
                const instance = getInstanceOrThrow(state);

                const requestJson = readCString(
                    instance.exports.memory,
                    requestJsonCstrPtr
                );

                const outbound = JSON.parse(requestJson) as OutboundHttpRequestEnvelope;

                const response = await fetch(outbound.url, {
                    method: outbound.method ?? "GET",
                    headers: outbound.headers,
                    body: outbound.body ?? undefined
                });

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
                const bodyText = getResponseByIdOrEmpty(state, responseId).bodyText;

                return writeBytesIntoMemory(
                    instance.exports.memory,
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
                const key = readCString(
                    instance.exports.memory,
                    keyCstrPtr
                ).toLowerCase();

                const value =
                    getResponseByIdOrEmpty(state, responseId).headers[key] ?? "";

                return writeBytesIntoMemory(
                    instance.exports.memory,
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