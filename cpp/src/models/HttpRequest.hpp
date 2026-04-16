#pragma once

#include <string>
#include<map>

#include "constants/HttpHeaders.h"
#include "enums/Mimes.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"

namespace AresWasmWorker {
    struct HttpRequest {
        std::string url;
        std::string method = "GET";
        std::map<std::string, std::string> headers = {};

        HttpRequest() = default;
        virtual ~HttpRequest() = default;
    };
}
