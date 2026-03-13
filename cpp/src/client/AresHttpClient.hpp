#pragma once

#include <format>

#include "../models/HttpResponse.hpp"
#include<map>

#include "../../../abi/AbiBindings.h"
#include "constants/HttpHeaders.h"
#include "enums/MethodType.hpp"
#include "enums/Mimes.h"
#include "extensions/JsonExtension.h"
#include "models/HttpRequest.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace AresWasmWorker {
    class AresHttpClient {
        std::unique_ptr<std::string> baseAddress;
        std::unique_ptr<std::map<std::string, std::string> > headers;

        std::string configureAddress(std::string apiName) const {
            return std::format("{}/{}", *baseAddress, apiName);
        }

        void configureBaseHeaders() {
            auto baseHeaders = std::map<std::string, std::string>({
                {
                    to_string_header(HttpHeader::CONTENT_TYPE),
                    to_string_mime(AresWasmWorker::MimeType::JSON)
                },
                {
                    to_string_header(HttpHeader::ACCEPT),
                    to_string_mime(AresWasmWorker::MimeType::JSON)
                },
                {
                    to_string_header(HttpHeader::ACCEPT_ENCODING),
                    "gzip, br"
                },
                {
                    to_string_header(HttpHeader::ACCEPT_LANGUAGE),
                    "en-US,en;q=0.9"
                },
                {
                    to_string_header(HttpHeader::CONNECTION),
                    "keep-alive"
                },
                {
                    // user agent must come from the runtime (cloudflare worker)
                    to_string_header(HttpHeader::USER_AGENT),
                    reinterpret_cast<const char *>(abi_http_get_user_agent_name())
                },
                {
                    to_string_header(HttpHeader::CACHE_CONTROL),
                    "no-cache"
                }
            });

            headers = std::make_unique<std::map<std::string, std::string> >(baseHeaders);
        }

        void verifyIfBaseAddress() const {
            if (!baseAddress) {
                throw std::runtime_error("base address has not been configured");
            }
        }

        template<ResponseChecker BodyType>
         std::unique_ptr<HttpResponse<BodyType>> makeRequestCall(
             const std::string apiAddress,
             const HttpMethod methodType
         ) {
            auto targetAddress = configureAddress(apiAddress);
            HttpRequest request(
                targetAddress,
                to_string_method(methodType),
                *headers,
                std::nullopt
            );

            auto jsonData = request.toJson();
            auto newCopy = alloc(jsonData.size() + 1);
            std::memcpy(reinterpret_cast<void*>(newCopy), jsonData.c_str(), jsonData.size() + 1);

            auto response = reinterpret_cast<char*>(abi_http_fetch_blocking(newCopy));
            auto responseObj = JsonExtensions::getResponseFromJson<BodyType>(response);

            return std::make_unique<HttpResponse<BodyType>>(responseObj);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
        std::unique_ptr<HttpResponse<BodyType>> makeRequestCall(
            const std::string apiAddress,
            const HttpMethod methodType,
            const RequestBody requestBody
        ) {
            auto targetAddress = configureAddress(apiAddress);

            HttpRequest request(
                targetAddress,
                to_string_method(methodType),
                *headers,
                requestBody.toJson()
            );

            auto jsonData = request.toJson();
            auto newCopy = alloc(jsonData.size() + 1);
            std::memcpy(reinterpret_cast<void*>(newCopy), jsonData.c_str(), jsonData.size() + 1);

            auto response = reinterpret_cast<char*>(abi_http_fetch_blocking(newCopy));
            auto responseObj = JsonExtensions::getResponseFromJson<BodyType>(response);

            return std::make_unique<HttpResponse<BodyType>>(responseObj);
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
        std::unique_ptr<HttpResponse<BodyType> > get(const std::string apiAddress) {
            verifyIfBaseAddress();

            return makeRequestCall<BodyType>(apiAddress, HttpMethod::GET);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
        std::unique_ptr<HttpResponse<BodyType> > post(std::string apiAddress, RequestBody requestBody) {
            verifyIfBaseAddress();

            return makeRequestCall<BodyType>(apiAddress, HttpMethod::POST, requestBody);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
        std::unique_ptr<HttpResponse<BodyType> > delete_(std::string apiAddress, RequestBody requestBody) {
            verifyIfBaseAddress();

            return makeRequestCall<BodyType>(apiAddress, HttpMethod::DELETE_, requestBody);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
        std::unique_ptr<HttpResponse<BodyType> > patch(std::string apiAddress) {
            verifyIfBaseAddress();

            return makeRequestCall<BodyType>(apiAddress, HttpMethod::PATCH);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
        std::unique_ptr<HttpResponse<BodyType> > put(std::string apiAddress) {
            verifyIfBaseAddress();

            return makeRequestCall<BodyType>(apiAddress, HttpMethod::PUT);
        }

        template<ResponseChecker BodyType>
        std::unique_ptr<HttpResponse<BodyType> > head(std::string apiAddress) {
            verifyIfBaseAddress();
            return makeRequestCall<BodyType>(apiAddress, HttpMethod::HEAD);
        }

        template<ResponseChecker BodyType, RequestCheck RequestBody>
        std::unique_ptr<HttpResponse<BodyType> > options(std::string apiAddress) {
            verifyIfBaseAddress();

            return makeRequestCall<BodyType>(apiAddress, HttpMethod::OPTIONS);
        }
    };
}
