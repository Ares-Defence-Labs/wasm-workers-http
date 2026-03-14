cmake_minimum_required(VERSION 3.21)

project(wasm_workers_http_cpp LANGUAGES CXX)

add_library(wasm_workers_http INTERFACE
        src/models/RawHttpHostResponse.hpp
        src/helpers/ABIHelpers.h
        src/helpers/LoggingHelper.h
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

if (EMSCRIPTEN)
    find_program(EMCC_PATH emcc REQUIRED)
    get_filename_component(EMCC_REAL "${EMCC_PATH}" REALPATH)
    get_filename_component(EMCC_BIN_DIR "${EMCC_REAL}" DIRECTORY)
    get_filename_component(EMSCRIPTEN_ROOT "${EMCC_BIN_DIR}/../libexec" ABSOLUTE)
    set(EMSCRIPTEN_SYSTEM_INCLUDE "${EMSCRIPTEN_ROOT}/system/include")

    if (NOT EXISTS "${EMSCRIPTEN_SYSTEM_INCLUDE}/emscripten/emscripten.h")
        message(FATAL_ERROR
                "Could not find emscripten headers at: ${EMSCRIPTEN_SYSTEM_INCLUDE}\n"
                "Expected: ${EMSCRIPTEN_SYSTEM_INCLUDE}/emscripten/emscripten.h"
        )
    endif()

    target_include_directories(wasm_workers_http
            INTERFACE
            $<BUILD_INTERFACE:${EMSCRIPTEN_SYSTEM_INCLUDE}>
    )
endif()