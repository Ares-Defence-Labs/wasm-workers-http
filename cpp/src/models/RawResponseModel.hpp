#pragma once

#include <cstdint>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../../abi/AbiBindings.h"

namespace AresWasmWorker {

    struct RawHttpHostResponse {
        uint32_t responseId = 0;
        uint32_t status = 0;
        std::map<std::string, std::string> headers;
        std::string body;
    };

    class ScopedHostResponse {
        uint32_t responseId_;

    public:
        explicit ScopedHostResponse(uint32_t responseId) : responseId_(responseId) {}

        ~ScopedHostResponse() {
            if (responseId_ != 0) {
                abi_http_response_free(responseId_);
            }
        }

        ScopedHostResponse(const ScopedHostResponse&) = delete;
        ScopedHostResponse& operator=(const ScopedHostResponse&) = delete;

        ScopedHostResponse(ScopedHostResponse&& other) noexcept
            : responseId_(other.responseId_) {
            other.responseId_ = 0;
        }

        ScopedHostResponse& operator=(ScopedHostResponse&& other) noexcept {
            if (this != &other) {
                if (responseId_ != 0) {
                    abi_http_response_free(responseId_);
                }
                responseId_ = other.responseId_;
                other.responseId_ = 0;
            }
            return *this;
        }

        [[nodiscard]] uint32_t id() const {
            return responseId_;
        }
    };

    namespace AbiHttpHelpers {

        inline std::string copyResponseBody(uint32_t responseId) {
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

        inline std::string copyResponseHeader(
            uint32_t responseId,
            const std::string& key,
            uint32_t maxLen = 8192
        ) {
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
                auto value = copyResponseHeader(responseId, key);
                if (!value.empty()) {
                    out[key] = value;
                }
            }

            return out;
        }

        inline RawHttpHostResponse readResponse(uint32_t responseId) {
            RawHttpHostResponse response;
            response.responseId = responseId;
            response.status = abi_http_response_get_status(responseId);
            response.body = copyResponseBody(responseId);
            response.headers = copyKnownHeaders(responseId);
            return response;
        }
    }
}