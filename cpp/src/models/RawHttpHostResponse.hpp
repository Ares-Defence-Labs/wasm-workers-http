#pragma once

#include <map>
#include <stdexcept>
#include <string>
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
}