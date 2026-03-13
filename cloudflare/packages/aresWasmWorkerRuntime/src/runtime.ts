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
const DEFAULT_MAX_TOTAL_BRIDGE_BYTES = 8 * 1024 * 1024; // 8 MB total pool
const DEFAULT_MAX_RESPONSE_BODY_BYTES = 512 * 1024; // 512 KB per response
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
            const imports = createAresAbiImports(this.state);

            const instantiated = await WebAssembly.instantiate(
                this.state.options.wasm,
                imports as WebAssembly.Imports
            );

            const instance =
                instantiated &&
                typeof instantiated === "object" &&
                "instance" in instantiated
                    ? (instantiated.instance as WasmInstanceWithExports)
                    : (instantiated as WasmInstanceWithExports);

            this.state.instance = instance;

            if (!(instance.exports.memory instanceof WebAssembly.Memory)) {
                throw new Error("Wasm export memory was not found.");
            }

            if (typeof instance.exports.alloc !== "function") {
                throw new Error("Wasm export alloc() was not found.");
            }

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
        if (typeof this.instance.exports.free_mem === "function") {
            this.instance.exports.free_mem(ptr, size);
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
        let responsePtr = 0;

        await this.init();
        this.setRequestContext({ request, env, ctx });

        try {
            if (typeof this.instance.exports.handle_http_json !== "function") {
                throw new Error("Wasm export handle_http_json() was not found.");
            }

            const requestBody = await request.text();

            const inboundEnvelope: RequestEnvelope = {
                url: request.url,
                method: request.method,
                headers: headersToObject(request.headers),
                body: requestBody
            };

            requestPtr = writeJsonCString(
                this.instance.exports.memory,
                this.instance.exports.alloc,
                inboundEnvelope
            );

            const responsePtrOrPromise =
                this.instance.exports.handle_http_json(requestPtr);

            responsePtr = await Promise.resolve(responsePtrOrPromise);

            if (!responsePtr) {
                throw new Error("Wasm returned a null response pointer.");
            }

            const responseJson = readCString(
                this.instance.exports.memory,
                responsePtr
            );

            const responseEnvelope = JSON.parse(responseJson) as ResponseEnvelope;

            return new Response(
                typeof responseEnvelope.body === "string"
                    ? responseEnvelope.body
                    : JSON.stringify(responseEnvelope.body),
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
            this.freeIfNeeded(requestPtr);
            this.freeIfNeeded(responsePtr);
            this.clearRequestContext();
        }
    }
}