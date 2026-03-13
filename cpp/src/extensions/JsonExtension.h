#pragma once

#include <map>
#include <memory>
#include <string>

#include "models/HttpResponse.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace AresWasmWorker {
    struct JsonExtensions {
        template<ResponseChecker BodyType>
        static HttpResponse<BodyType> getResponseFromJson(const std::string& responseJson) {
            auto j = nlohmann::json::parse(responseJson);
            auto status = j.at("status").get<uint32_t>();

            auto headers = std::make_shared<std::map<std::string, std::string>>(
                j.at("headers").get<std::map<std::string, std::string>>()
            );

            auto body = j.at("body").get<BodyType>();

            return HttpResponse<BodyType>(status, headers, body);
        }

        template<ResponseChecker BodyType>
        static HttpResponse<BodyType> getResponseBodyFromJson(const std::string& bodyJson) {
            auto responseBody = nlohmann::json::parse(bodyJson).get<BodyType>();

            auto headers = std::make_shared<std::map<std::string, std::string>>();
            return HttpResponse<BodyType>(0, headers, responseBody);
        }
    };
}