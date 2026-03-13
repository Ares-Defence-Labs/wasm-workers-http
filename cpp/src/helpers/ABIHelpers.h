#pragma once

#include <cstdint>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../../abi/AbiBindings.h"
#include "../models/RawHttpHostResponse.hpp"

namespace AresWasmWorker::AbiHttpHelpers {

    inline std::string copyBody(uint32_t responseId) {
        const uint32_t len = abi_http_response_get_body_len(responseId);
        if (len == 0) {
            return {};
        }

        std::string result;
        result.resize(len);

        const auto copied = abi_http_response_copy_body(
            responseId,
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(result.data())),
            len
        );

        if (copied > len) {
            throw std::runtime_error("abi_http_response_copy_body copied more than expected");
        }

        result.resize(copied);
        return result;
    }

    inline std::string copyHeader(
        uint32_t responseId,
        const std::string& key,
        uint32_t maxLen = 8192
    ) {
        if (key.empty()) {
            return {};
        }

        const uint32_t keyPtr = alloc(static_cast<uint32_t>(key.size() + 1));
        if (!keyPtr) {
            throw std::runtime_error("alloc failed for header key");
        }

        std::memcpy(reinterpret_cast<void*>(keyPtr), key.c_str(), key.size() + 1);

        std::string value;
        value.resize(maxLen);

        const auto copied = abi_http_response_copy_header(
            responseId,
            keyPtr,
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(value.data())),
            maxLen
        );

        free_mem(keyPtr, static_cast<uint32_t>(key.size() + 1));

        if (copied > maxLen) {
            throw std::runtime_error("abi_http_response_copy_header copied more than maxLen");
        }

        value.resize(copied);
        return value;
    }

    inline std::map<std::string, std::string> copyKnownHeaders(uint32_t responseId) {
        std::map<std::string, std::string> out;

        static const std::vector<std::string> knownHeaders = {
            "content-type",
            "content-length",
            "cache-control",
            "etag",
            "location",
            "authorization",
            "x-request-id",
            "x-correlation-id"
        };

        for (const auto& key : knownHeaders) {
            auto value = copyHeader(responseId, key);
            if (!value.empty()) {
                out[key] = value;
            }
        }

        return out;
    }

    inline RawHttpHostResponse readResponse(uint32_t responseId) {
        RawHttpHostResponse response;
        response.status = abi_http_response_get_status(responseId);
        response.body = copyBody(responseId);
        response.headers = copyKnownHeaders(responseId);
        return response;
    }
}