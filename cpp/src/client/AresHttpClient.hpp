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
        std::unique_ptr<std::map<std::string, std::string> > headers;

        [[nodiscard]] std::string configureAddress(std::string apiName) const {
            const auto baseAddr = *baseAddress;
            if (baseAddr.empty()) {
                return apiName;
            }

            return std::format("{}/{}", baseAddr, apiName);
        }

        void configureBaseHeaders() {
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

            headers = std::make_unique<std::map<std::string, std::string> >(baseHeaders);
        }

        // void verifyIfBaseAddress() const {
        //     if (!baseAddress) {
        //         throw std::runtime_error("base address has not been configured");
        //     }
        // }

        template<ResponseChecker BodyType>
        std::unique_ptr<HttpResponse<BodyType> > makeRequestCall(
            const std::string &apiAddress,
            const HttpMethod methodType,
            const std::map<std::string, std::string> &customHeaders = {}
        ) {
            AresWasmWorker::abiLog("AresHttpClient: starting makeRequestCall");
            auto targetAddress = configureAddress(apiAddress);
            AresWasmWorker::abiLog(std::string("AresHttpClient: targetAddress = ") + targetAddress);

            auto requestHeaders = *headers;
            requestHeaders.insert(customHeaders.begin(), customHeaders.end());
            HttpRequest request(
                targetAddress,
                to_string_method(methodType),
                requestHeaders,
                std::nullopt
            );

            auto jsonData = request.toJson();
            AresWasmWorker::abiLog(std::string("AresHttpClient: requestJson = ") + jsonData);

            const uint32_t requestPtr = alloc(static_cast<uint32_t>(jsonData.size() + 1));
            if (!requestPtr) {
                AresWasmWorker::abiLog("AresHttpClient: alloc failed for request json");
                throw std::runtime_error("alloc failed for request json");
            }

            std::memcpy(
                reinterpret_cast<void *>(requestPtr),
                jsonData.c_str(),
                jsonData.size() + 1
            );

            AresWasmWorker::abiLog("AresHttpClient: before abi_http_fetch_blocking");
            const auto responseId = abi_http_fetch_blocking_async(requestPtr);
            AresWasmWorker::abiLog("AresHttpClient: after abi_http_fetch_blocking");

            free_mem(requestPtr, static_cast<uint32_t>(jsonData.size() + 1));

            if (!responseId) {
                AresWasmWorker::abiLog("AresHttpClient: abi_http_fetch_blocking returned 0");
                throw std::runtime_error("abi_http_fetch_blocking failed");
            }

            AresWasmWorker::abiLog(std::string("AresHttpClient: responseId = ") + std::to_string(responseId));

            ScopedHostResponse scoped(responseId);
            auto rawResponse = AbiHttpHelpers::readResponse(scoped.id());

            AresWasmWorker::abiLog(
                std::string("AresHttpClient: raw status = ") + std::to_string(rawResponse.status)
            );
            AresWasmWorker::abiLog(
                std::string("AresHttpClient: raw body = ") + rawResponse.body
            );

            BodyType responseObj =
                    JsonExtensions::getResponseFromJson<BodyType>(rawResponse.body);

            AresWasmWorker::abiLog("AresHttpClient: JSON parse success");

            return std::make_unique<HttpResponse<BodyType> >(
                rawResponse.status,
                std::make_shared<std::map<std::string, std::string> >(std::move(rawResponse.headers)),
                std::move(responseObj)
            );
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
        std::unique_ptr<HttpResponse<BodyType> > makeRequestCall(
            const std::string &apiAddress,
            const HttpMethod methodType,
            const RequestBody &requestBody,
            const std::map<std::string, std::string> &customHeaders = {},
            const std::optional<std::string> &jsonBody = std::nullopt
        ) {
            auto targetAddress = configureAddress(apiAddress);

            auto requestHeaders = *headers;
            requestHeaders.insert(customHeaders.begin(), customHeaders.end());
            HttpRequest request(
                targetAddress,
                to_string_method(methodType),
                requestHeaders,
                jsonBody.value_or(requestBody.toJson())
            );

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
        AresHttpClient() {
            configureBaseHeaders();
        }

        AresHttpClient *configureBaseAddress(std::string _baseAddress) {
            baseAddress = std::make_unique<std::string>(_baseAddress);
            return this;
        }

        AresHttpClient *configureHeaders(std::map<std::string, std::string> _headers) {
            headers->insert(_headers.begin(), _headers.end());
            return this;
        }

        template<ResponseChecker BodyType>
        std::unique_ptr<HttpResponse<BodyType> > get(const std::string apiAddress,
                                                     const std::map<std::string, std::string> &customHeaders = {}) {
            //  verifyIfBaseAddress();

            return makeRequestCall<BodyType>(apiAddress, HttpMethod::GET, customHeaders);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
        std::unique_ptr<HttpResponse<BodyType> > post(std::string apiAddress, RequestBody requestBody,

                                                      const std::map<std::string, std::string> &customHeaders = {},
                                                      const std::optional<std::string> jsonBody = std::nullopt) {
            //verifyIfBaseAddress();
            return makeRequestCall<BodyType>(apiAddress, HttpMethod::POST, requestBody, customHeaders, jsonBody);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
        std::unique_ptr<HttpResponse<BodyType> > delete_(std::string apiAddress, RequestBody requestBody,
                                                         const std::map<std::string, std::string> &customHeaders = {}) {
            //  verifyIfBaseAddress();

            return makeRequestCall<BodyType>(apiAddress, HttpMethod::DELETE_, requestBody, customHeaders);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
        std::unique_ptr<HttpResponse<BodyType> > patch(std::string apiAddress,
                                                       const std::map<std::string, std::string> &customHeaders = {}) {
            //  verifyIfBaseAddress();

            return makeRequestCall<BodyType>(apiAddress, HttpMethod::PATCH, customHeaders);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
        std::unique_ptr<HttpResponse<BodyType> > put(std::string apiAddress,
                                                     const std::map<std::string, std::string> &customHeaders = {}) {
            // verifyIfBaseAddress();

            return makeRequestCall<BodyType>(apiAddress, HttpMethod::PUT, customHeaders);
        }

        template<ResponseChecker BodyType>
        std::unique_ptr<HttpResponse<BodyType> > head(std::string apiAddress,
                                                      const std::map<std::string, std::string> &customHeaders = {}) {
            //  verifyIfBaseAddress();
            return makeRequestCall<BodyType>(apiAddress, HttpMethod::HEAD, customHeaders);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
        std::unique_ptr<HttpResponse<BodyType> > options(std::string apiAddress,
                                                         const std::map<std::string, std::string> &customHeaders = {}) {
            //  verifyIfBaseAddress();

            return makeRequestCall<BodyType>(apiAddress, HttpMethod::OPTIONS, customHeaders);
        }
    };
}
