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
#include "../models/HttpRequest.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using Headers = std::map<std::string, std::string>;

namespace AresWasmWorker {
    class AresHttpClient {
        template<RequestCheck Request>
void appendDefaultHeaders(
    Request &request,
    const Headers &headers = {}
) const {
    Headers baseHeaders = {
        { to_string_header(HttpHeader::ACCEPT), to_string_mime(MimeType::JSON) },
        { "X-Content-Type-Options", "nosniff" },
        { "X-Frame-Options", "DENY" },
        { "Content-Security-Policy", "default-src 'none'" },
        { "Content-Type", to_string_mime(MimeType::JSON) }
    };

    request.headers.insert(baseHeaders.begin(), baseHeaders.end());

     for (const auto& [key, value] : headers) {
        request.headers.insert_or_assign(key, value);
    }
}

        template<ResponseChecker BodyType, RequestCheck Request>
        std::unique_ptr<HttpResponse<BodyType> > makeRequestCall(
            const std::string &targetAddress,
            const HttpMethod methodType,
            Request &request,
 			const Headers &headers = {}
        ) {
            request.method = to_string_method(methodType);
            request.url = targetAddress;
            appendDefaultHeaders(request, headers);

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
AresWasmWorker::abiLog(std::format(
    "HTTP status = {}",
    rawResponse.status));

AresWasmWorker::abiLog(std::format(
    "HTTP body = {}",
    rawResponse.body));

            BodyType responseObj =
                    JsonExtensions::getResponseFromJson<BodyType>(rawResponse.body);

            return std::make_unique<HttpResponse<BodyType> >(
                rawResponse.status,
                std::make_shared<std::map<std::string, std::string> >(std::move(rawResponse.headers)),
                std::move(responseObj)
            );
        }

        template<RequestCheck Request>
        void makeRequestCallFireAndForget(
            const std::string &targetAddress,
            const HttpMethod methodType,
            Request &request,
 			const Headers &headers = {}
        ) {
            request.method = to_string_method(methodType);
            request.url = targetAddress;
            appendDefaultHeaders(request, headers);

            auto jsonData = request.toJson();
            auto requestSize = static_cast<uint32_t>(jsonData.size() + 1);

            auto requestPtr = alloc(requestSize);
            if (!requestPtr) {
                throw std::runtime_error("alloc failed for request json");
            }

            std::memcpy(
                reinterpret_cast<void *>(requestPtr),
                jsonData.c_str(),
                requestSize
            );

            try {
                abi_http_fetch_non_blocking_async(requestPtr);
            } catch (...) {
                free_mem(requestPtr, requestSize);
                throw;
            }

            free_mem(requestPtr, requestSize);
        }

    public:
std::unique_ptr<HttpResponse<std::string>> getRaw(
    const std::string& apiAddress,
    const Headers& headers = {}
) {
    HttpRequest request;

    request.method = to_string_method(HttpMethod::GET);
    request.url = apiAddress;

    appendDefaultHeaders(request, headers);

    auto jsonData = request.toJson();

    auto requestPtr = alloc(static_cast<uint32_t>(jsonData.size() + 1));

    std::memcpy(
        reinterpret_cast<void*>(requestPtr),
        jsonData.c_str(),
        jsonData.size() + 1
    );

    const auto responseId = abi_http_fetch_blocking_async(requestPtr);

    free_mem(
        requestPtr,
        static_cast<uint32_t>(jsonData.size() + 1)
    );

    if (!responseId) {
        throw std::runtime_error("abi_http_fetch_blocking failed");
    }

    ScopedHostResponse scoped(responseId);

    auto rawResponse = AbiHttpHelpers::readResponse(scoped.id());

AresWasmWorker::abiLog(std::format(
    "HTTP status = {}",
    rawResponse.status));

AresWasmWorker::abiLog(std::format(
    "HTTP body = {}",
    rawResponse.body));

    return std::make_unique<HttpResponse<std::string>>(
        rawResponse.status,
        std::make_shared<std::map<std::string, std::string>>(
            std::move(rawResponse.headers)
        ),
        std::move(rawResponse.body)
    );
}

      template<ResponseChecker BodyType>
std::unique_ptr<HttpResponse<BodyType>> get(
    const std::string &apiAddress,
    const Headers &headers = {}
) {
    HttpRequest request;
    return makeRequestCall<BodyType>(apiAddress, HttpMethod::GET, request, headers);
}

template<ResponseChecker BodyType, RequestCheck RequestBody>
std::unique_ptr<HttpResponse<BodyType>> post(
    const std::string &apiAddress,
    RequestBody requestBody,
    const Headers &headers = {}
) {
    return makeRequestCall<BodyType>(apiAddress, HttpMethod::POST, requestBody, headers);
}

template<RequestCheck RequestBody>
void postFireAndForget(
    const std::string &apiAddress,
    RequestBody requestBody,
    const Headers &headers = {}
) {
    makeRequestCallFireAndForget(apiAddress, HttpMethod::POST, requestBody, headers);
}

template<ResponseChecker BodyType, RequestCheck RequestBody>
std::unique_ptr<HttpResponse<BodyType>> delete_(
    const std::string &apiAddress,
    RequestBody requestBody,
    const Headers &headers = {}
) {
    return makeRequestCall<BodyType>(apiAddress, HttpMethod::DELETE_, requestBody, headers);
}

template<ResponseChecker BodyType, RequestCheck RequestBody>
std::unique_ptr<HttpResponse<BodyType>> patch(
    const std::string &apiAddress,
    RequestBody requestBody,
    const Headers &headers = {}
) {
    return makeRequestCall<BodyType>(apiAddress, HttpMethod::PATCH, requestBody, headers);
}

template<ResponseChecker BodyType, RequestCheck RequestBody>
std::unique_ptr<HttpResponse<BodyType>> put(
    const std::string &apiAddress,
    RequestBody requestBody,
    const Headers &headers = {}
) {
    return makeRequestCall<BodyType>(apiAddress, HttpMethod::PUT, requestBody, headers);
}

template<ResponseChecker BodyType, RequestCheck RequestBody>
std::unique_ptr<HttpResponse<BodyType>> head(
    const std::string &apiAddress,
    RequestBody requestBody,
    const Headers &headers = {}
) {
    return makeRequestCall<BodyType>(apiAddress, HttpMethod::HEAD, requestBody, headers);
}

template<ResponseChecker BodyType, RequestCheck RequestBody>
std::unique_ptr<HttpResponse<BodyType>> options(
    const std::string &apiAddress,
    RequestBody requestBody,
    const Headers &headers = {}
) {
    return makeRequestCall<BodyType>(apiAddress, HttpMethod::OPTIONS, requestBody, headers);
}
    };
}
