const encoder = new TextEncoder();
const decoder = new TextDecoder();

export function encodeUtf8(value: string): Uint8Array {
    return encoder.encode(value);
}

export function utf8ByteLength(value: string): number {
    return encoder.encode(value).length >>> 0;
}

export function readCString(
    memory: WebAssembly.Memory,
    ptr: number,
    maxLen = 10_000_000
): string {
    if (!ptr) return "";

    const bytes = new Uint8Array(memory.buffer);
    const start = ptr >>> 0;

    if (start >= bytes.length) {
        throw new Error(`CString pointer out of bounds: ${ptr}`);
    }

    let end = start;
    const limit = Math.min(bytes.length, start + maxLen);

    while (end < limit && bytes[end] !== 0) {
        end++;
    }

    if (end >= limit) {
        throw new Error("CString terminator not found within safe bounds");
    }

    return decoder.decode(bytes.subarray(start, end));
}

export function writeCString(
    memory: WebAssembly.Memory,
    alloc: (size: number) => number,
    value: string
): number {
    const encoded = encoder.encode(value);
    const ptr = alloc(encoded.length + 1);

    if (!ptr) {
        throw new Error(`alloc failed for ${encoded.length + 1} bytes`);
    }

    const bytes = new Uint8Array(memory.buffer);
    bytes.set(encoded, ptr);
    bytes[ptr + encoded.length] = 0;

    return ptr >>> 0;
}

export function writeJsonCString(
    memory: WebAssembly.Memory,
    alloc: (size: number) => number,
    value: unknown
): number {
    return writeCString(memory, alloc, JSON.stringify(value));
}

export function writeBytesIntoMemory(
    memory: WebAssembly.Memory,
    outPtr: number,
    src: Uint8Array,
    maxLen: number
): number {
    if (!outPtr || maxLen <= 0) return 0;

    const bytes = new Uint8Array(memory.buffer);
    const start = outPtr >>> 0;

    if (start >= bytes.length) {
        return 0;
    }

    const writeLen = Math.min(src.length, maxLen, bytes.length - start);
    if (writeLen <= 0) {
        return 0;
    }

    bytes.set(src.subarray(0, writeLen), start);
    return writeLen >>> 0;
}

export function headersToObject(headers: Headers): Record<string, string> {
    return Object.fromEntries(headers.entries());
}