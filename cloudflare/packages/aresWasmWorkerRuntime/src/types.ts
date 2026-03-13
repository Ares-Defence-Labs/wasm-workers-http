export interface WorkerExecutionContext {
    waitUntil(promise: Promise<unknown>): void;
    passThroughOnException?(): void;
}

export type WasmExports = {
    memory: WebAssembly.Memory;
    alloc: (size: number) => number;
    free_mem?: (ptr: number, size: number) => void;
    app_init?: () => number | Promise<number>;
    handle_http_json?: (requestJsonPtr: number) => number | Promise<number>;
};

export type WasmInstanceWithExports = WebAssembly.Instance & {
    exports: WasmExports;
};

export type RequestEnvelope = {
    url: string;
    method: string;
    headers: Record<string, string>;
    body?: string | null;
};

export type ResponseEnvelope = {
    status: number;
    headers: Record<string, string>;
    body: unknown;
};

export type OutboundHttpRequestEnvelope = {
    url: string;
    method?: string;
    headers?: Record<string, string>;
    body?: string | null;
};

export type HttpBridgeResponseCache = {
    status: number;
    bodyText: string;
    headers: Record<string, string>;
    createdAtMs: number;
    estimatedBytes: number;
};

export type RuntimeOptions<Env = unknown> = {
    wasm: WebAssembly.Module;
    debug?: boolean;
    onLog?: (message: string) => void;
    env?: Env;

    maxHttpResponseHandles?: number;
    maxTotalBridgeBytes?: number;
    maxResponseBodyBytes?: number;
    responseTtlMs?: number;
};

export type RequestScopedContext<Env = unknown> = {
    request: Request;
    env: Env;
    ctx: WorkerExecutionContext;
};

export type RuntimeState<Env = unknown> = {
    instance: WasmInstanceWithExports | null;
    options: Required<
        Pick<
            RuntimeOptions<Env>,
            | "wasm"
            | "debug"
            | "onLog"
            | "env"
            | "maxHttpResponseHandles"
            | "maxTotalBridgeBytes"
            | "maxResponseBodyBytes"
            | "responseTtlMs"
        >
    >;
    requestContext: RequestScopedContext<Env> | null;
    initPromise: Promise<void> | null;

    httpResponses: Map<number, HttpBridgeResponseCache>;
    nextHttpResponseId: number;
};