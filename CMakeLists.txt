cmake_minimum_required(VERSION 3.21)

project(wasm_workers_http LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
message(STATUS "VCPKG_TARGET_TRIPLET = ${VCPKG_TARGET_TRIPLET}")

add_executable(wasm_workers_http
        cpp/src/main.cpp
        cpp/src/constants/HttpHeaders.h
        cpp/src/extensions/JsonExtension.h
)

target_include_directories(wasm_workers_http
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp/src
)

find_package(nlohmann_json CONFIG REQUIRED)

get_target_property(JSON_INCLUDES nlohmann_json::nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "nlohmann_json includes = ${JSON_INCLUDES}")

target_link_libraries(wasm_workers_http
        PRIVATE
        nlohmann_json::nlohmann_json
)