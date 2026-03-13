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

    inline uint32_t alloc(uint32_t size) {
        return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(std::malloc(size)));
    }

   inline  void free_mem(uint32_t ptr, uint32_t size) {
        std::free(reinterpret_cast<void*>(ptr));
    }
}