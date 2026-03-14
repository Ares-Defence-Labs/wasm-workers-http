#pragma once

#include <cstdint>

#define WASM_IMPORT(module, name) __attribute__((import_module(module), import_name(name)))

extern "C" {
    uint32_t abi_http_fetch_blocking_async(uint32_t request_json_cstr_ptr)
        WASM_IMPORT("env", "abi_http_fetch_blocking_async");

    uint32_t abi_http_response_get_status(uint32_t response_id)
        WASM_IMPORT("env", "abi_http_response_get_status");

    uint32_t abi_http_response_get_body_len(uint32_t response_id)
        WASM_IMPORT("env", "abi_http_response_get_body_len");

    uint32_t abi_http_response_copy_body(uint32_t response_id, uint32_t out_ptr, uint32_t max_len)
        WASM_IMPORT("env", "abi_http_response_copy_body");

    uint32_t abi_http_response_copy_header(uint32_t response_id, uint32_t key_cstr_ptr, uint32_t out_ptr, uint32_t max_len)
        WASM_IMPORT("env", "abi_http_response_copy_header");

    void abi_http_response_free(uint32_t response_id)
        WASM_IMPORT("env", "abi_http_response_free");

    uint32_t abi_http_get_user_agent_name()
        WASM_IMPORT("env", "abi_http_get_user_agent_name");

    void abi_log(uint32_t message_cstr_ptr)
        WASM_IMPORT("env", "abi_log");

    uint32_t alloc(uint32_t size)
        WASM_IMPORT("env", "alloc");

    void free_mem(uint32_t ptr, uint32_t size)
        WASM_IMPORT("env", "free_mem");
}