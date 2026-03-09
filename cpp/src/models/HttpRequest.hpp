#pragma once

#include <string>
#include<map>

#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"

namespace AresWasmWorker {
    struct HttpRequest {
        std::string url;
        std::string method = "GET";
        std::map<std::string, std::string> headers;
        std::optional<std::string> body;

        explicit HttpRequest(std::string url, std::string method = "GET", std::map<std::string, std::string> headers,
                             std::optional<std::string> body = std::nullopt): url(url), method(method), headers(headers), body(body) {
        };

        std::string toJson() {
            return nlohmann::json::parse(this);
        }
    };
}
