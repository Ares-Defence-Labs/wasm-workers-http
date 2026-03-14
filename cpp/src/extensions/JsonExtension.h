#pragma once

#include <string>

#include "../models/HttpResponse.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace AresWasmWorker {
    struct JsonExtensions {
        template<ResponseChecker BodyType>
        static BodyType getResponseFromJson(const std::string& responseJson) {
            return nlohmann::json::parse(responseJson).get<BodyType>();
        }

        template<ResponseChecker BodyType>
        static BodyType getResponseBodyFromJson(const std::string& bodyJson) {
            return nlohmann::json::parse(bodyJson).get<BodyType>();
        }
    };
}