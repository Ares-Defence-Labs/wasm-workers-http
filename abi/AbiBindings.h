#pragma once

#include <cstdint>

extern "C" {
    __attribute__((import_module("env"), import_name("abi_log")))
    void abi_log(uint32_t message_cstr_ptr);

    __attribute__((import_module("env"), import_name("abi_http_fetch_blocking_async")))
    uint32_t abi_http_fetch_blocking_async(uint32_t request_json_cstr_ptr);

    __attribute__((import_module("env"), import_name("abi_http_response_get_status")))
    uint32_t abi_http_response_get_status(uint32_t response_id);

    __attribute__((import_module("env"), import_name("abi_http_response_get_body_len")))
    uint32_t abi_http_response_get_body_len(uint32_t response_id);

    __attribute__((import_module("env"), import_name("abi_http_response_copy_body")))
    uint32_t abi_http_response_copy_body(uint32_t response_id, uint32_t out_ptr, uint32_t max_len);

    __attribute__((import_module("env"), import_name("abi_http_response_copy_header")))
    uint32_t abi_http_response_copy_header(uint32_t response_id, uint32_t key_cstr_ptr, uint32_t out_ptr, uint32_t max_len);

    __attribute__((import_module("env"), import_name("abi_http_response_free")))
    void abi_http_response_free(uint32_t response_id);

    __attribute__((import_module("env"), import_name("abi_http_get_user_agent_name")))
    uint32_t abi_http_get_user_agent_name();

    uint32_t alloc(uint32_t size);
    void free_mem(uint32_t ptr, uint32_t size);

}