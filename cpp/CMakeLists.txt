cmake_minimum_required(VERSION 3.21)

project(wasm_workers_http_cpp LANGUAGES CXX)

add_library(wasm_workers_http INTERFACE
        src/models/RawResponseModel.hpp
        src/helpers/ABIHelpers.h)
add_library(ares::wasm-workers-http ALIAS wasm_workers_http)

target_compile_features(wasm_workers_http INTERFACE cxx_std_20)

target_include_directories(wasm_workers_http
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(wasm_workers_http
        INTERFACE
        nlohmann_json::nlohmann_json
)