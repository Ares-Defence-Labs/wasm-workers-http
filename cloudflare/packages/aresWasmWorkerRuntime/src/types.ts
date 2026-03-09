export interface WorkerExecutionContext {
    waitUntil(promise: Promise<unknown>): void;
    passThroughOnException?(): void;
}

export type WasmExports = {
    memory: WebAssembly.Memory;
    alloc: (size: number) => number;
    free_mem?: (ptr: number, size: number) => void;
    app_init?: () => number;
    handle_http?: (requestJsonPtr: number) => number;
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

export type RuntimeOptions<Env = unknown> = {
    wasm: WebAssembly.Module;
    debug?: boolean;
    onLog?: (message: string) => void;
    env?: Env;
};

export type RequestScopedContext<Env = unknown> = {
    request: Request;
    env: Env;
    ctx: WorkerExecutionContext;
};

export type RuntimeState<Env = unknown> = {
    instance: WasmInstanceWithExports | null;
    options: RuntimeOptions<Env>;
    requestContext: RequestScopedContext<Env> | null;
};