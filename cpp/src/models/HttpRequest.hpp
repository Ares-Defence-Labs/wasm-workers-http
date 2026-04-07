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

        HttpRequest() = default;

        virtual ~HttpRequest() = default;

        explicit HttpRequest(std::string url, std::string method = "GET",
                             std::map<std::string, std::string> headers = {},
                             std::optional<std::string> body = std::nullopt): url(url), method(method),
                                                                              headers(headers), body(body) {
        };

        std::string toJson() const {
            nlohmann::json j;
            j["url"] = url;
            j["method"] = method;
            j["headers"] = headers;
            j["body"] = body.has_value() ? nlohmann::json(*body) : nlohmann::json(nullptr);
            return j.dump();
        }

        virtual std::string toJsonCustom() = 0;
    };
}
