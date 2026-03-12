cmake_minimum_required(VERSION 3.21)

project(wasm_workers_http LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(WASM_WORKERS_HTTP_BUILD_EXAMPLE "Build wasm_workers_http example executable" OFF)

message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
message(STATUS "VCPKG_TARGET_TRIPLET = ${VCPKG_TARGET_TRIPLET}")

add_subdirectory(cpp)

if (WASM_WORKERS_HTTP_BUILD_EXAMPLE)
    add_executable(wasm_workers_http_example
            cpp/src/main.cpp
    )

    target_link_libraries(wasm_workers_http_example
            PRIVATE
            ares::wasm-workers-http
    )
endif()