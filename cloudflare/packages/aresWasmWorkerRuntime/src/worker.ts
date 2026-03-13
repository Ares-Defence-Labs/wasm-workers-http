import type { RuntimeOptions, WorkerExecutionContext } from "./types";
import { AresWorkerRuntime } from "./runtime";

export function createAresWorkerHandler<Env = unknown>(
    wasm: WebAssembly.Module,
    options?: Omit<RuntimeOptions<Env>, "wasm">
) {
    const runtime = new AresWorkerRuntime<Env>({
        wasm,
        ...options
    });

    const ready = runtime.init();

    return {
        async fetch(request: Request, env: Env, ctx: WorkerExecutionContext) {
            await ready;
            return runtime.handleFetch(request, env, ctx);
        }
    };
}