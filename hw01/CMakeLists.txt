cmake_minimum_required(VERSION 3.30)
project(hw01 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

#add_executable(hw01 hw01/rar_jpeg.c)
add_executable(hw01 encodings.c
        give_me_utf8.c
        give_me_utf8.h)

target_compile_options(hw01 PRIVATE -Wall -Wextra -Wpedantic -std=c11)
