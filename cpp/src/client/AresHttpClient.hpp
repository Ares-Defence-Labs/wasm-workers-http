#pragma once

#include <format>

#include "../models/HttpResponse.hpp"
#include<map>

#include "../../../abi/AbiBindings.h"
#include "../constants/HttpHeaders.h"
#include "../enums/MethodType.hpp"
#include "../enums/Mimes.h"
#include "../extensions/JsonExtension.h"
#include "../helpers/ABIHelpers.h"
#include "../helpers/LoggingHelper.h"
#include "../models/HttpRequest.hpp"
#include "nlohmann/json.hpp"

#include <emscripten/emscripten.h>

using json = nlohmann::json;

namespace AresWasmWorker {
    class AresHttpClient {
        std::unique_ptr<std::string> baseAddress;

        [[nodiscard]] std::string configureAddress(std::string apiName) const {
            const auto baseAddr = *baseAddress;
            if (baseAddr.empty()) {
                return apiName;
            }

            return std::format("{}/{}", baseAddr, apiName);
        }

        template<ResponseChecker BodyType, RequestCheck Request>
        std::unique_ptr<HttpResponse<BodyType> > makeRequestCall(
            const std::string &apiAddress,
            const HttpMethod methodType,
            const Request &request
        ) {
            request.method = to_string_method(methodType);

            auto targetAddress = configureAddress(apiAddress);

            auto jsonData = request.toJson();
            auto requestPtr = alloc(static_cast<uint32_t>(jsonData.size() + 1));
            if (!requestPtr) {
                throw std::runtime_error("alloc failed for request json");
            }

            std::memcpy(
                reinterpret_cast<void *>(requestPtr),
                jsonData.c_str(),
                jsonData.size() + 1
            );

            const auto responseId = abi_http_fetch_blocking_async(requestPtr);
            free_mem(requestPtr, static_cast<uint32_t>(jsonData.size() + 1));

            if (!responseId) {
                throw std::runtime_error("abi_http_fetch_blocking failed");
            }

            ScopedHostResponse scoped(responseId);
            auto rawResponse = AbiHttpHelpers::readResponse(scoped.id());

            BodyType responseObj =
                    JsonExtensions::getResponseFromJson<BodyType>(rawResponse.body);

            return std::make_unique<HttpResponse<BodyType> >(
                rawResponse.status,
                std::make_shared<std::map<std::string, std::string> >(std::move(rawResponse.headers)),
                std::move(responseObj)
            );
        }

    public:
        AresHttpClient *configureBaseAddress(std::string& _baseAddress) {
            baseAddress = std::make_unique<std::string>(_baseAddress);
            return this;
        }

        template<ResponseChecker BodyType>
        std::unique_ptr<HttpResponse<BodyType> > get(const std::string apiAddress) {
            return makeRequestCall<BodyType>(apiAddress, HttpMethod::GET);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
        std::unique_ptr<HttpResponse<BodyType> > post(std::string apiAddress, RequestBody requestBody) {
            return makeRequestCall<BodyType>(apiAddress, HttpMethod::POST, requestBody);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
        std::unique_ptr<HttpResponse<BodyType> > delete_(std::string apiAddress, RequestBody requestBody) {
            return makeRequestCall<BodyType>(apiAddress, requestBody);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
        std::unique_ptr<HttpResponse<BodyType> > patch(std::string apiAddress, RequestBody requestBody) {
            return makeRequestCall<BodyType>(apiAddress, HttpMethod::PATCH, requestBody);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
        std::unique_ptr<HttpResponse<BodyType> > put(std::string apiAddress, RequestBody requestBody) {
            return makeRequestCall<BodyType>(apiAddress, HttpMethod::PUT, requestBody);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
         std::unique_ptr<HttpResponse<BodyType> > head(std::string apiAddress, RequestBody requestBody) {
            return makeRequestCall<BodyType>(apiAddress, HttpMethod::HEAD, requestBody);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
         std::unique_ptr<HttpResponse<BodyType> > options(std::string apiAddress, RequestBody requestBody) {
            return makeRequestCall<BodyType>(apiAddress, HttpMethod::OPTIONS, requestBody);
        }
    };
}
