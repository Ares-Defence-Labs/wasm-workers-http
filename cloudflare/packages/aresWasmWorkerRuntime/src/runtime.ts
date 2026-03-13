import type {
    RequestEnvelope,
    RequestScopedContext,
    ResponseEnvelope,
    RuntimeOptions,
    RuntimeState,
    WasmInstanceWithExports,
    WorkerExecutionContext
} from "./types";
import { createAresAbiImports } from "./abi";
import { headersToObject, readCString, writeJsonCString } from "./memory";

const DEFAULT_MAX_HTTP_RESPONSE_HANDLES = 32;
const DEFAULT_MAX_TOTAL_BRIDGE_BYTES = 8 * 1024 * 1024;
const DEFAULT_MAX_RESPONSE_BODY_BYTES = 512 * 1024;
const DEFAULT_RESPONSE_TTL_MS = 30_000;

const NOOP_LOGGER = (_: string): void => {};

function withDefaults<Env>(options: RuntimeOptions<Env>): RuntimeState<Env>["options"] {
    return {
        wasm: options.wasm,
        debug: options.debug ?? false,
        onLog: options.onLog ?? NOOP_LOGGER,
        env: options.env as Env,
        maxHttpResponseHandles:
            options.maxHttpResponseHandles ?? DEFAULT_MAX_HTTP_RESPONSE_HANDLES,
        maxTotalBridgeBytes:
            options.maxTotalBridgeBytes ?? DEFAULT_MAX_TOTAL_BRIDGE_BYTES,
        maxResponseBodyBytes:
            options.maxResponseBodyBytes ?? DEFAULT_MAX_RESPONSE_BODY_BYTES,
        responseTtlMs: options.responseTtlMs ?? DEFAULT_RESPONSE_TTL_MS
    };
}

function safeSetU32(memory: WebAssembly.Memory, ptr: number, value: number): void {
    if (!ptr) return;
    const start = ptr >>> 0;
    if (start + 4 > memory.buffer.byteLength) return;
    new DataView(memory.buffer).setUint32(start, value >>> 0, true);
}

function safeSetU64Zero(memory: WebAssembly.Memory, ptr: number): void {
    if (!ptr) return;
    const start = ptr >>> 0;
    if (start + 8 > memory.buffer.byteLength) return;
    const view = new DataView(memory.buffer);
    view.setUint32(start, 0, true);
    view.setUint32(start + 4, 0, true);
}

export class AresWorkerRuntime<Env = unknown> {
    private readonly state: RuntimeState<Env>;

    constructor(options: RuntimeOptions<Env>) {
        this.state = {
            instance: null,
            options: withDefaults(options),
            requestContext: null,
            initPromise: null,
            httpResponses: new Map(),
            nextHttpResponseId: 1
        };
    }

    async init(): Promise<void> {
        if (this.state.instance) {
            return;
        }

        if (this.state.initPromise) {
            return this.state.initPromise;
        }

        this.state.initPromise = (async () => {
            const abiImports = createAresAbiImports(this.state);

            const imports: WebAssembly.Imports = {
                env: {
                    ...abiImports.env,
                    emscripten_notify_memory_growth: () => {},
                    emscripten_date_now: () => Date.now(),
                    emscripten_get_now: () => performance.now(),
                    abort: (msg?: unknown, file?: unknown, line?: unknown, col?: unknown) => {
                        throw new Error(
                            `abort: ${String(msg)} @ ${String(file)}:${String(line)}:${String(col)}`
                        );
                    }
                },
                wasi_snapshot_preview1: {
                    fd_write: (
                        _fd: number,
                        _iovs: number,
                        _iovsLen: number,
                        nwritten: number
                    ) => {
                        const memory = this.state.instance
                            ? getMemoryOrThrow(this.state.instance)
                            : null;
                        if (memory) {
                            safeSetU32(memory, nwritten, 0);
                        }
                        return 0;
                    },
                    fd_read: (
                        _fd: number,
                        _iovs: number,
                        _iovsLen: number,
                        nread: number
                    ) => {
                        const memory = this.state.instance
                            ? getMemoryOrThrow(this.state.instance)
                            : null;
                        if (memory) {
                            safeSetU32(memory, nread, 0);
                        }
                        return 0;
                    },
                    fd_seek: (
                        _fd: number,
                        _offsetLow: number,
                        _offsetHigh: number,
                        _whence: number,
                        newOffset: number
                    ) => {
                        const memory = this.state.instance
                            ? getMemoryOrThrow(this.state.instance)
                            : null;
                        if (memory) {
                            safeSetU64Zero(memory, newOffset);
                        }
                        return 0;
                    },
                    fd_close: (_fd: number) => 0
                }
            };

            const instantiated = await WebAssembly.instantiate(
                this.state.options.wasm,
                imports
            );

            const instance =
                instantiated &&
                typeof instantiated === "object" &&
                "instance" in instantiated
                    ? (instantiated.instance as WasmInstanceWithExports)
                    : (instantiated as WasmInstanceWithExports);

            this.state.instance = instance;

            getMemoryOrThrow(instance);
            getAllocOrThrow(instance);

            if (typeof instance.exports.app_init === "function") {
                const rc = await Promise.resolve(instance.exports.app_init());
                if (rc !== 0) {
                    throw new Error(`Wasm app_init() failed with rc=${rc}`);
                }
            }
        })();

        return this.state.initPromise;
    }

    setRequestContext(context: RequestScopedContext<Env>): void {
        this.state.requestContext = context;
    }

    clearRequestContext(): void {
        this.state.requestContext = null;
    }

    get instance(): WasmInstanceWithExports {
        if (!this.state.instance) {
            throw new Error("Runtime not initialised. Call init() first.");
        }
        return this.state.instance;
    }

    private freeIfNeeded(ptr: number, size = 0): void {
        if (!ptr) return;

        try {
            const free = getFreeOrThrow(this.instance);
            free(ptr, size);
        } catch {
            // ignore cleanup failures
        }
    }

    private jsonErrorResponse(message: string, status = 500): Response {
        return new Response(
            JSON.stringify({
                error: "worker_wasm_failure",
                message
            }),
            {
                status,
                headers: {
                    "content-type": "application/json; charset=utf-8"
                }
            }
        );
    }

    async handleFetch(
        request: Request,
        env: Env,
        ctx: WorkerExecutionContext
    ): Promise<Response> {
        let requestPtr = 0;
        let requestSize = 0;
        let responsePtr = 0;

        await this.init();
        this.setRequestContext({ request, env, ctx });

        try {
            const handleHttp =
                this.instance.exports.handle_http ??
                this.instance.exports.handle_http_json;

            if (typeof handleHttp !== "function") {
                throw new Error(
                    "Wasm export handle_http() / handle_http_json() was not found."
                );
            }

            const requestBody = await request.text();

            const inboundEnvelope: RequestEnvelope = {
                url: request.url,
                method: request.method,
                headers: headersToObject(request.headers),
                body: requestBody
            };

            const memory = getMemoryOrThrow(this.instance);
            const alloc = getAllocOrThrow(this.instance);

            const requestJson = JSON.stringify(inboundEnvelope);
            requestSize = new TextEncoder().encode(requestJson).length + 1;

            requestPtr = writeJsonCString(memory, alloc, inboundEnvelope);

            const responsePtrOrPromise = handleHttp(requestPtr);
            responsePtr = await Promise.resolve(responsePtrOrPromise);

            if (!responsePtr) {
                throw new Error("Wasm returned a null response pointer.");
            }

            const responseJson = readCString(memory, responsePtr);
            const responseEnvelope = JSON.parse(responseJson) as ResponseEnvelope;

            return new Response(
                typeof responseEnvelope.body === "string"
                    ? responseEnvelope.body
                    : JSON.stringify(responseEnvelope.body ?? ""),
                {
                    status: responseEnvelope.status || 200,
                    headers:
                        responseEnvelope.headers || {
                            "content-type": "application/json; charset=utf-8"
                        }
                }
            );
        } catch (error) {
            const message =
                error instanceof Error ? error.message : String(error);
            return this.jsonErrorResponse(message);
        } finally {
            this.freeIfNeeded(requestPtr, requestSize);
            this.freeIfNeeded(responsePtr);
            this.clearRequestContext();
        }
    }
}

export function getMemoryOrThrow(instance: WebAssembly.Instance & {
    exports: Record<string, unknown>;
}): WebAssembly.Memory {
    const memory = instance.exports.memory;
    if (!(memory instanceof WebAssembly.Memory)) {
        throw new Error("Wasm module does not export memory");
    }
    return memory;
}

export function getAllocOrThrow(instance: WebAssembly.Instance & {
    exports: Record<string, unknown>;
}): (size: number) => number {
    const candidates = [
        instance.exports.alloc,
        instance.exports._alloc,
        instance.exports.malloc,
        instance.exports._malloc
    ];

    for (const fn of candidates) {
        if (typeof fn === "function") {
            return fn as (size: number) => number;
        }
    }

    throw new Error(
        "Wasm module does not export an allocator (alloc/_alloc/malloc/_malloc)"
    );
}

export function getFreeOrThrow(instance: WebAssembly.Instance & {
    exports: Record<string, unknown>;
}): (ptr: number, size?: number) => void {
    const freeMem = instance.exports.free_mem ?? instance.exports._free_mem;
    if (typeof freeMem === "function") {
        return freeMem as (ptr: number, size?: number) => void;
    }

    const free = instance.exports.free ?? instance.exports._free;
    if (typeof free === "function") {
        return free as (ptr: number) => void;
    }

    throw new Error(
        "Wasm module does not export a deallocator (free_mem/_free_mem/free/_free)"
    );
}