#include <iso646.h>
#include <string.h>
#include "give_me_utf8.h"

enum ERRORS {
    NOT_ENOUGH_ARGUMENTS = 1, // NOT_ENOUGH_ARGUMENTS -   1
    FILE_OPEN_ERROR, // FILE_OPEN_ERROR      -   2
    GET_BUFFER_ERROR, // GET_BUFFER_ERROR     -   3
    FILE_READ_ERROR, // FILE_READ_ERROR      -   4
    FILE_WRITE_ERROR, // FILE_WRITE_ERROR     -   5
};

typedef unsigned char *buffer;

void safe_free_buffer(buffer ptr) {
    if (ptr != NULL) {
        free(ptr);
    }
}

void print_success_file_open_message(char *filename, char *mode) {
    char mode_desc[128] = "";

    // Определяем базовый режим (с учётом возможного режима обновления)
    if (mode[0] == 'r') {
        if (strchr(mode, '+') != NULL)
            strcpy(mode_desc, "чтения и записи");
        else
            strcpy(mode_desc, "чтения");
    } else if (mode[0] == 'w') {
        if (strchr(mode, '+') != NULL)
            strcpy(mode_desc, "чтения и записи");
        else
            strcpy(mode_desc, "записи");
    } else if (mode[0] == 'a') {
        if (strchr(mode, '+') != NULL)
            strcpy(mode_desc, "чтения и дозаписи");
        else
            strcpy(mode_desc, "дозаписи");
    } else {
        strcpy(mode_desc, "неизвестного режима");
    }

    // Уточняем, текстовый или двоичный формат
    if (strchr(mode, 'b') != NULL)
        strcat(mode_desc, " в двоичном формате");
    else
        strcat(mode_desc, " в текстовом формате");

    // Выводим сообщение об успешном открытии файла
    printf("Файл %s открыт в режиме %s.\n", filename, mode_desc);
}

size_t get_string_size(char *str) {
    int i = 0;
    size_t str_size = 0;

    while (str[i] != '\0') {
        str_size += sizeof(str[i]);
        i++;
    }

    return str_size;
}

FILE *open_file(char *filename, char *mode) {
    FILE *file = fopen(filename, mode);
    if (!file) {
        size_t msg_size = sizeof("Ошибка открытия файла ") + get_string_size(filename);
        char *msg = malloc(msg_size);
        snprintf(msg, msg_size, "Ошибка открытия файла %s", filename);
        perror(msg);
        exit(FILE_OPEN_ERROR);
    }

    print_success_file_open_message(filename, mode);

    return file;
}

int write_file(FILE *file, buffer content, char *filename) {
    if (file == NULL || content == NULL) {
    }

    // Вычисляем длину строки (без учета завершающего нуля)
    size_t len = strlen((const char *) content);

    // Пытаемся записать строку в файл
    size_t written = fwrite(content, sizeof(unsigned char), len, file);

    // Проверяем, что все байты успешно записаны
    if (written != len) {
        size_t msg_size = sizeof("Ошибка записи в файл ") + get_string_size(filename);
        char *msg = malloc(msg_size);
        snprintf(msg, msg_size, "Ошибка записи в файл %s", filename);
        perror(msg);
        return FILE_WRITE_ERROR;
    }

    return 0;
}

void close_file(FILE *file, char *filename) {
    fclose(file);
    printf("Файл %s закрыт\n\n", filename);
}

long get_file_size(FILE *file) {
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    return file_size;
}

void get_buffer(buffer *content, size_t file_size, FILE *file) {
    *content = malloc(file_size);
    if (!*content) {
        perror("Ошибка выделения памяти буфера");
        fclose(file);
        exit(GET_BUFFER_ERROR);
    }
}

size_t read_file_to_buffer(buffer buf, size_t file_size, FILE *file) {
    if (!file) return 0;

    size_t bytes_read = fread(buf, 1, file_size, file);
    if (bytes_read != file_size) {
        perror("Ошибка чтения файла");
        fclose(file);
        safe_free_buffer(buf);
        exit(FILE_READ_ERROR);
    }

    return bytes_read;
}

long check_for_jpeg_sign(buffer buf, size_t file_size) {
    if (file_size < 2) return -1;

    // JPEG_EOI: 0xFF, 0xD9
    unsigned char eoi[2] = {0xFF, 0xD9};

    for (long i = file_size - 2; i >= 0; i--) {
        if (memcmp(buf + i, eoi, sizeof(eoi)) == 0)
            return i;
    }

    return -1;
}

int main(int argc, char *argv[]) {
    int exit_code = 0;

    if (argc < 4) {
        perror("Должен быть передано 3 параметра: имя исходного файла, исходная кодировка и имя выходного файла");
        exit(NOT_ENOUGH_ARGUMENTS);
    }

    char *input_filename = argv[1];
    printf("Имя исходного файла: %s\n", input_filename);

    char *codename = argv[2];
    printf("Исходная кодировка: \'%s\'\n", codename);

    char *output_filename = argv[3];
    printf("Имя выходного файла: %s\n", output_filename);

    printf("\n");

    FILE *input_file = open_file(input_filename, "rb");

    FILE *output_file = open_file(output_filename, "wb");

    long file_size = get_file_size(input_file);
    printf("Размер исходного файла: %ld байт\n", file_size);

    buffer input_content = NULL;
    get_buffer(&input_content, file_size, input_file);
    printf("Выделен буфер для чтения исходного файла\n");

    size_t content_size = read_file_to_buffer(input_content, file_size, input_file);
    printf("Считано содержимое исходного файла в размере %ld байт\n\n", content_size);

    close_file(input_file, input_filename);

    buffer output_content = NULL;

    if (strcmp(codename, "cp1251") == 0 or strcmp(codename, "CP1251") == 0) {
        output_content = cp1251_to_utf8(input_content, file_size);
    }

    if (strcmp(codename, "iso-8859-5") == 0 or strcmp(codename, "ISO-8859-5") == 0) {
        output_content = iso8859_5_to_utf8(input_content, file_size);
    }

    if (strcmp(codename, "koi8-r") == 0 or strcmp(codename, "KOI8-R") == 0) {
        output_content = koi8r_to_utf8(input_content, file_size);
    }

    if (output_content == NULL) {
        size_t msg_size = sizeof("Обнаружена неподдерживаемая кодировка: ") + get_string_size(codename) + 2;
        char *msg = malloc(msg_size);
        snprintf(msg, msg_size, "Обнаружена неподдерживаемая кодировка: \'%s\'", codename);
        perror(msg);

        close_file(output_file, output_filename);
        safe_free_buffer(input_content);
        safe_free_buffer(output_content);
    }

    printf("Содержимое файла %s преобразовано в UTF-8 в размере %ld байт\n", input_filename,
           strlen((const char *) output_content));

    exit_code = write_file(output_file, output_content, output_filename);

    close_file(output_file, output_filename);

    safe_free_buffer(input_content);
    safe_free_buffer(output_content);
    return exit_code;
}
