cmake_minimum_required(VERSION 3.30)
project(hw02 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(hw02 src/weather.c)

set(CURL_ROOT "third_party/curl")
include_directories(${CURL_ROOT}/include)
link_directories(${CURL_ROOT}/lib)
target_link_libraries(hw02 PRIVATE curl)

add_subdirectory(third_party/cJSON)
target_link_libraries(hw02 PRIVATE cjson)

target_compile_options(hw02 PRIVATE -Wall -Wextra -Wpedantic -std=c11)

if(NOT BUILD_SHARED_LIBS)
    target_link_libraries(hw02 PRIVATE ssl crypto z)
endif()