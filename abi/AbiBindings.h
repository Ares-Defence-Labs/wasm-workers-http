#pragma once

#include <cstdint>

extern "C" {
    // Imported from JS
    uint32_t abi_http_fetch_blocking(uint32_t request_json_cstr_ptr);
    uint32_t abi_http_get_last_status();
    uint32_t abi_http_get_user_agent_name();
    uint32_t abi_http_get_last_body_len();
    uint32_t abi_http_copy_last_body(uint32_t out_ptr, uint32_t max_len);
    uint32_t abi_http_copy_last_header(uint32_t key_cstr_ptr, uint32_t out_ptr, uint32_t max_len);
    void abi_log(uint32_t message_cstr_ptr);

    // Exported from Wasm
    uint32_t alloc(uint32_t size);
    void free_mem(uint32_t ptr, uint32_t size);
    uint32_t app_init();
    uint32_t handle_http_json(uint32_t request_json_cstr_ptr);
    uint32_t get_response_len();
}