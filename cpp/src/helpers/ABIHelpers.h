#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../../abi/AbiBindings.h"
#include "../models/RawHttpHostResponse.hpp"
#include "models/RawResponseModel.hpp"

namespace AresWasmWorker::AbiHttpHelpers {

    inline std::string copyLastBody() {
        const uint32_t len = abi_http_get_last_body_len();
        if (len == 0) {
            return {};
        }

        std::string result;
        result.resize(len);

        const auto copied = abi_http_copy_last_body(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(result.data())),
            len
        );

        if (copied > len) {
            throw std::runtime_error("abi_http_copy_last_body copied more than expected");
        }

        result.resize(copied);
        return result;
    }

    inline std::string copyLastHeader(const std::string& key, uint32_t maxLen = 8192) {
        if (key.empty()) {
            return {};
        }

        auto* keyPtr = alloc(static_cast<uint32_t>(key.size() + 1));
        if (!keyPtr) {
            throw std::runtime_error("alloc failed for header key");
        }

        std::memcpy(reinterpret_cast<void*>(keyPtr), key.c_str(), key.size() + 1);

        std::string value;
        value.resize(maxLen);

        const auto copied = abi_http_copy_last_header(
            keyPtr,
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(value.data())),
            maxLen
        );

        free_mem(keyPtr, static_cast<uint32_t>(key.size() + 1));

        if (copied > maxLen) {
            throw std::runtime_error("abi_http_copy_last_header copied more than maxLen");
        }

        value.resize(copied);
        return value;
    }

    inline std::map<std::string, std::string> copyKnownHeaders() {
        std::map<std::string, std::string> out;

        // Add whichever headers you care about reading back.
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
            auto value = copyLastHeader(key);
            if (!value.empty()) {
                out[key] = value;
            }
        }

        return out;
    }

    inline RawHttpHostResponse getLastResponse() {
        RawHttpHostResponse response;
        response.status = abi_http_get_last_status();
        response.body = copyLastBody();
        response.headers = copyKnownHeaders();
        return response;
    }
}
