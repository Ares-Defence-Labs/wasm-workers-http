#pragma once

#include <string>
#include<map>

#include "constants/HttpHeaders.h"
#include "enums/Mimes.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"

namespace AresWasmWorker {
    struct HttpRequest {
        std::string url;
        std::string method = "GET";
        std::map<std::string, std::string> headers;

        HttpRequest() = default;

        virtual ~HttpRequest() = default;

        explicit HttpRequest(std::string url,
                             std::map<std::string, std::string> headers = {}): url(std::move(url)),
                                                                               method(std::move(method)),
                                                                               headers(std::move(headers)) {
            auto baseHeaders = std::map<std::string, std::string>({
                {
                    to_string_header(HttpHeader::ACCEPT),
                    to_string_mime(AresWasmWorker::MimeType::JSON)
                },

                // Security headers
                {
                    "X-Content-Type-Options",
                    "nosniff"
                },
                {
                    "X-Frame-Options",
                    "DENY"
                },
                {
                    "Content-Security-Policy",
                    "default-src 'none'"
                },

                // Explicit content type
                {
                    "Content-Type",
                    to_string_mime(AresWasmWorker::MimeType::JSON)
                }
            });

            headers.insert(baseHeaders.begin(), baseHeaders.end());
        };
    };
}
