cmake_minimum_required(VERSION 3.30)
project(homework_c C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(homework_c hw01/rar_jpeg.c)

target_compile_options(homework_c PRIVATE -Wall -Wextra -Wpedantic -std=c11)
