#pragma once

namespace AresWasmWorker {
    enum class HttpMethod {
        GET,
        POST,
        PUT,
        PATCH,
        DELETE_,
        HEAD,
        OPTIONS
    };

    inline const char *to_string_method(HttpMethod method) {
        switch (method) {
            case HttpMethod::GET: return "GET";
            case HttpMethod::POST: return "POST";
            case HttpMethod::PUT: return "PUT";
            case HttpMethod::PATCH: return "PATCH";
            case HttpMethod::DELETE_: return "DELETE";
            case HttpMethod::HEAD: return "HEAD";
            case HttpMethod::OPTIONS: return "OPTIONS";
        }

        return "GET";
    }
}
