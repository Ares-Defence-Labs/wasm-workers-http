#pragma once

namespace AresWasmWorker {

    enum class HttpHeader
    {
        ACCEPT,
        ACCEPT_ENCODING,
        ACCEPT_LANGUAGE,
        AUTHORIZATION,
        CACHE_CONTROL,
        CONNECTION,
        CONTENT_TYPE,
        CONTENT_LENGTH,
        COOKIE,
        HOST,
        ORIGIN,
        REFERER,
        USER_AGENT
    };

    inline const char* to_string_header(HttpHeader header)
    {
        switch (header)
        {
            case HttpHeader::ACCEPT: return "Accept";
            case HttpHeader::ACCEPT_ENCODING: return "Accept-Encoding";
            case HttpHeader::ACCEPT_LANGUAGE: return "Accept-Language";
            case HttpHeader::AUTHORIZATION: return "Authorization";
            case HttpHeader::CACHE_CONTROL: return "Cache-Control";
            case HttpHeader::CONNECTION: return "Connection";
            case HttpHeader::CONTENT_TYPE: return "Content-Type";
            case HttpHeader::CONTENT_LENGTH: return "Content-Length";
            case HttpHeader::COOKIE: return "Cookie";
            case HttpHeader::HOST: return "Host";
            case HttpHeader::ORIGIN: return "Origin";
            case HttpHeader::REFERER: return "Referer";
            case HttpHeader::USER_AGENT: return "User-Agent";
        }

        return "";
    }

}