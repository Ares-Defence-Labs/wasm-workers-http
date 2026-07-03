#pragma once

#include <string>
#include "../../../abi/AbiBindings.h"
#include "../models/ServiceBusRequest.h"

namespace AresWasmWorker {

    class ServiceBus {
    public:
        ServiceBus() = delete;

        [[nodiscard]]
        static bool enqueue(const ServiceBusRequest& request) {
            const std::string json = request.toJson();

            return abi_service_bus_enqueue_json(
                reinterpret_cast<uint32_t>(json.c_str())
            ) != 0;
        }
    };

}