#pragma once

#include <string>
#include <map>
#include "nlohmann/json.hpp"

namespace AresWasmWorker {

    struct ServiceBusTargetRequest {
        std::string url;
        std::string method = "POST";
        std::map<std::string, std::string> headers = {
            {"content-type", "application/json"}
        };
        nlohmann::json body = nlohmann::json::object();

        [[nodiscard]]
        std::string toJson() const {
            nlohmann::json json;
            json["url"] = url;
            json["method"] = method;
            json["headers"] = headers;
            json["body"] = body;
            return json.dump();
        }
    };

    struct ServiceBusRequest {
        std::string idempotencyKey;
        ServiceBusTargetRequest target;

        uint32_t maxImmediateAttempts = 3;
        uint32_t retryAfterSeconds = 10800; // 3 hours

        ServiceBusRequest() = default;
        virtual ~ServiceBusRequest() = default;

        [[nodiscard]]
        virtual std::string toJson() const {
            nlohmann::json json;

            if (!idempotencyKey.empty()) {
                json["idempotencyKey"] = idempotencyKey;
            }

            json["target"] = {
                {"url", target.url},
                {"method", target.method},
                {"headers", target.headers},
                {"body", target.body}
            };

            json["maxImmediateAttempts"] = maxImmediateAttempts;
            json["retryAfterSeconds"] = retryAfterSeconds;

            return json.dump();
        }
    };

}