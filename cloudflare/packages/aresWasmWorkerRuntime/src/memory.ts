const encoder = new TextEncoder();
const decoder = new TextDecoder();

export function readCString(memory: WebAssembly.Memory, ptr: number): string {
    const bytes = new Uint8Array(memory.buffer);

    let end = ptr;
    while (bytes[end] !== 0) {
        end++;
    }

    return decoder.decode(bytes.subarray(ptr, end));
}

export function writeCString(
    memory: WebAssembly.Memory,
    alloc: (size: number) => number,
    value: string
): number {
    const encoded = encoder.encode(value);
    const ptr = alloc(encoded.length + 1);

    const bytes = new Uint8Array(memory.buffer);
    bytes.set(encoded, ptr);
    bytes[ptr + encoded.length] = 0;

    return ptr;
}

export function writeJsonCString(
    memory: WebAssembly.Memory,
    alloc: (size: number) => number,
    value: unknown
): number {
    return writeCString(memory, alloc, JSON.stringify(value));
}

export function headersToObject(headers: Headers): Record<string, string> {
    return Object.fromEntries(headers.entries());
}