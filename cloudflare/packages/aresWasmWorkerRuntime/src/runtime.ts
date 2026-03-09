import type {
    RequestEnvelope,
    ResponseEnvelope,
    RequestScopedContext,
    RuntimeOptions,
    RuntimeState,
    WasmInstanceWithExports,
    WorkerExecutionContext
} from "./types";
import { createAresAbiImports } from "./abi";
import { headersToObject, readCString, writeJsonCString } from "./memory";

export class AresWorkerRuntime<Env = unknown> {
    private readonly state: RuntimeState<Env>;

    constructor(options: RuntimeOptions<Env>) {
        this.state = {
            instance: null,
            options,
            requestContext: null
        };
    }

    async init(): Promise<void> {
        if (this.state.instance) {
            return;
        }

        const imports = createAresAbiImports(this.state);
        const instance = await WebAssembly.instantiate(
            this.state.options.wasm,
            imports
        ) as WasmInstanceWithExports;

        this.state.instance = instance;

        if (instance.exports.app_init) {
            instance.exports.app_init();
        }
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

    async handleFetch(
        request: Request,
        env: Env,
        ctx: WorkerExecutionContext
    ): Promise<Response> {
        await this.init();
        this.setRequestContext({ request, env, ctx });

        try {
            if (!this.instance.exports.handle_http) {
                throw new Error("Wasm export handle_http() was not found.");
            }

            const requestBody = await request.text();

            const inboundEnvelope: RequestEnvelope = {
                url: request.url,
                method: request.method,
                headers: headersToObject(request.headers),
                body: requestBody
            };

            const requestPtr = writeJsonCString(
                this.instance.exports.memory,
                this.instance.exports.alloc,
                inboundEnvelope
            );

            const responsePtr = this.instance.exports.handle_http(requestPtr);
            const responseJson = readCString(this.instance.exports.memory, responsePtr);
            const responseEnvelope = JSON.parse(responseJson) as ResponseEnvelope;

            return new Response(
                typeof responseEnvelope.body === "string"
                    ? responseEnvelope.body
                    : JSON.stringify(responseEnvelope.body),
                {
                    status: responseEnvelope.status,
                    headers: responseEnvelope.headers
                }
            );
        } finally {
            this.clearRequestContext();
        }
    }
}