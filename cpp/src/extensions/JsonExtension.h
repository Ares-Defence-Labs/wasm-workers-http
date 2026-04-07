#pragma once

#include <string>

#include "../models/HttpResponse.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace AresWasmWorker {
    struct JsonExtensions {
        template<ResponseChecker BodyType>
        static BodyType getResponseFromJson(const std::string &responseJson) {
            if (responseJson.empty()) {
                return BodyType{}; 
            }
            return nlohmann::json::parse(responseJson).get<BodyType>();
        }
    };
}
