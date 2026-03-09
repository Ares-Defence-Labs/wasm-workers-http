import type { RuntimeState } from "./types";
import { readCString, writeCString } from "./memory";

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
            }
        }
    };
}