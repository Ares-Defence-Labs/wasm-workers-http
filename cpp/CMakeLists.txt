cmake_minimum_required(VERSION 3.21)

project(wasm_workers_http_cpp LANGUAGES CXX)

add_library(wasm_workers_http INTERFACE

        #models
        src/models/RawHttpHostResponse.hpp
        src/models/HttpResponse.hpp
        src/models/HttpRequest.hpp

        # helpers
        src/helpers/ABIHelpers.h
        src/helpers/LoggingHelper.h

        src/client/AresHttpClient.hpp
        src/constants/HttpHeaders.h

        src/enums/MethodType.hpp
        src/enums/Mimes.h
        src/enums/ResponseCode.hpp

        src/extensions/JsonExtension.h
)

add_library(ares::wasm-workers-http ALIAS wasm_workers_http)

target_compile_features(wasm_workers_http INTERFACE cxx_std_20)

find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(wasm_workers_http
        INTERFACE
        nlohmann_json::nlohmann_json
)

target_include_directories(wasm_workers_http
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)