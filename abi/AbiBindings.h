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

    uint32_t malloc(uint32_t size);
    void free_mem(uint32_t ptr, uint32_t size);
}