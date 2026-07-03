#pragma once

#include <string>
#include <map>
#include <cstdint>
#include "nlohmann/json.hpp"

namespace AresWasmWorker {

    enum class ServiceBusDispatchMode {
        Local,
        Remote
    };

    inline std::string toString(ServiceBusDispatchMode mode) {
        switch (mode) {
            case ServiceBusDispatchMode::Local:
                return "local";
            case ServiceBusDispatchMode::Remote:
                return "remote";
            default:
                return "remote";
        }
    }

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
        ServiceBusDispatchMode dispatchMode = ServiceBusDispatchMode::Remote;
        ServiceBusTargetRequest target;

        uint32_t maxImmediateAttempts = 3;
        uint32_t retryAfterSeconds = 10800;

        ServiceBusRequest() = default;
        virtual ~ServiceBusRequest() = default;

        [[nodiscard]]
        virtual std::string toJson() const {
            nlohmann::json json;

            if (!idempotencyKey.empty()) {
                json["idempotencyKey"] = idempotencyKey;
            }

            json["dispatchMode"] = toString(dispatchMode);

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