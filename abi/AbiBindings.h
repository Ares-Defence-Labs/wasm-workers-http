#pragma once
#include <cstdint>

extern "C" {
    uint32_t abi_http_fetch_blocking_async(uint32_t request_json_cstr_ptr);
    void abi_http_fetch_non_blocking_async(uint32_t request_json_cstr_ptr);

    uint32_t abi_http_response_get_status(uint32_t response_id);
    uint32_t abi_http_response_get_body_len(uint32_t response_id);
    uint32_t abi_http_response_copy_body(uint32_t response_id, uint32_t out_ptr, uint32_t max_len);
    uint32_t abi_http_response_copy_header(uint32_t response_id, uint32_t key_cstr_ptr, uint32_t out_ptr, uint32_t max_len);
    void abi_http_response_free(uint32_t response_id);

    uint32_t abi_http_get_user_agent_name();
    void abi_log(uint32_t message_cstr_ptr);

    uint32_t alloc(uint32_t size);
    uint32_t get_value_from_key(uint32_t key);
    void free_mem(uint32_t ptr, uint32_t size);

    // Enqueue JSON payload to durable service bus.
    // Returns 1 = accepted, 0 = failed.
    uint32_t abi_service_bus_enqueue_json(uint32_t payload_json_cstr_ptr);
}