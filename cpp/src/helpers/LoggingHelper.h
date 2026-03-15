#pragma once

#include <cstring>
#include <string>

#include "../../../abi/AbiBindings.h"

namespace AresWasmWorker {
    inline void abiLog(const std::string& message) {
        const auto len = static_cast<uint32_t>(message.size() + 1);
        const uint32_t ptr = alloc(len);
        if (!ptr) {
            return;
        }

        std::memcpy(reinterpret_cast<void*>(ptr), message.c_str(), len);
        abi_log(ptr);
        free_mem(ptr, len);
    }
}