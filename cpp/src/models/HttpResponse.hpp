#pragma once

#include <map>
#include <string>
#include <concepts>
#include <memory>
#include <utility>

#include "HttpRequest.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

template<typename Res>
concept ResponseChecker = std::is_class_v<Res>;

template<typename Res>
concept RequestCheck =
        std::is_class_v<Res> &&
        std::derived_from<Res, AresWasmWorker::HttpRequest>;

namespace AresWasmWorker {
    template<ResponseChecker BodyType>
    struct HttpResponse {
        uint32_t status = 0;
        std::shared_ptr<std::map<std::string, std::string> > headers;
        BodyType body;

        explicit HttpResponse(
            uint32_t status,
            std::shared_ptr<std::map<std::string, std::string> > headers,
            BodyType body
        )
            : status(status),
              headers(std::move(headers)),
              body(std::move(body)) {
        }

        HttpResponse() = default;

        virtual ~HttpResponse() = default;
    };
}
