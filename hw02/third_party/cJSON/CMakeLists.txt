cmake_minimum_required(VERSION 3.30)
project(cJSON C)

# Опция для сборки динамических библиотек
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Добавляем исходный файл cJSON.c для создания библиотеки
add_library(cjson SHARED
        cJSON.c
        cJSON_Utils.c
)

# Указываем директорию с заголовочными файлами для публичного использования
target_include_directories(cjson PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})