#pragma once

namespace AresWasmWorker {
    enum class MimeType
    {
        JSON,
        XML,
        TEXT,
        HTML,
        FORM_URLENCODED,
        MULTIPART_FORM_DATA,
        OCTET_STREAM
    };

    inline const char* to_string_mime(MimeType mime)
    {
        switch (mime)
        {
            case MimeType::JSON: return "application/json";
            case MimeType::XML: return "application/xml";
            case MimeType::TEXT: return "text/plain";
            case MimeType::HTML: return "text/html";
            case MimeType::FORM_URLENCODED: return "application/x-www-form-urlencoded";
            case MimeType::MULTIPART_FORM_DATA: return "multipart/form-data";
            case MimeType::OCTET_STREAM: return "application/octet-stream";
        }

        return "application/octet-stream";
    }

}