#pragma once

#include <cstdint>

extern "C" {
    // Imported from JS host
    uint32_t abi_http_fetch_blocking(uint32_t request_json_cstr_ptr);

    uint32_t abi_http_response_get_status(uint32_t response_id);
    uint32_t abi_http_response_get_body_len(uint32_t response_id);
    uint32_t abi_http_response_copy_body(uint32_t response_id, uint32_t out_ptr, uint32_t max_len);
    uint32_t abi_http_response_copy_header(uint32_t response_id, uint32_t key_cstr_ptr, uint32_t out_ptr, uint32_t max_len);
    void abi_http_response_free(uint32_t response_id);

    uint32_t abi_http_get_user_agent_name();
    void abi_log(uint32_t message_cstr_ptr);

    // Exported from Wasm
    uint32_t alloc(uint32_t size);
    void free_mem(uint32_t ptr, uint32_t size);
    uint32_t app_init();
    uint32_t handle_http_json(uint32_t request_json_cstr_ptr);
}

inline std::string abi_copy_last_body_string() {
    const uint32_t len = abi_http_get_last_body_len();
    if (len == 0) {
        return {};
    }

    auto* buffer = reinterpret_cast<char*>(std::malloc(len + 1));
    if (!buffer) {
        return {};
    }

    const uint32_t copied = abi_http_copy_last_body(
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(buffer)),
        len
    );

    buffer[copied] = '\0';
    std::string result(buffer, copied);
    std::free(buffer);
    return result;
}

inline std::string abi_copy_last_header_string(const char* key) {
    const size_t keyLen = std::strlen(key);
    auto* keyBuf = reinterpret_cast<char*>(std::malloc(keyLen + 1));
    std::memcpy(keyBuf, key, keyLen + 1);

    constexpr uint32_t maxLen = 8192;
    auto* valueBuf = reinterpret_cast<char*>(std::malloc(maxLen + 1));

    const uint32_t copied = abi_http_copy_last_header(
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(keyBuf)),
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(valueBuf)),
        maxLen
    );

    valueBuf[copied] = '\0';

    std::string result(valueBuf, copied);

    std::free(keyBuf);
    std::free(valueBuf);

    return result;
}