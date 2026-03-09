#pragma once

#include "models/HttpResponse.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace AresWasmWorker {
    struct JsonExtensions {
        template<ResponseChecker BodyType>
        static HttpResponse<BodyType> getResponseFromJson(const std::string responseJson) {
            auto j = nlohmann::json::parse(responseJson);
            auto status = j.at("status").get<uint32_t>();

            auto headers = std::make_shared<std::map<std::string, std::string> >(
                j.at("headers").get<std::map<std::string, std::string> >()
            );

            auto body = j.at("body").get<BodyType>();

            return nlohmann::json::parse(responseJson).get<HttpResponse>(status, HttpMethod::GET, headers, body);
        }

        template<ResponseChecker BodyType>
        static HttpResponse<BodyType> getResponseBodyFromJson(std::string body) {
            auto responseBody = nlohmann::json::parse(body).get<BodyType>();
            return HttpResponse(0, {{}}, responseBody);
        }
    };
}
