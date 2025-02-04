#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma pack(1)

// Определения сигнатур
#define JPEG_EOI 0xD9FF
#define ZIP_EOCD 0x06054b50
#define CENTRAL_DIR 0x02014B50

enum ERRORS {
    FILE_NOT_JPEG = 0,
    NULL_FILENAME = 1,
    FILE_OPEN_ERROR,
    GET_BUFFER_ERROR,
    FILE_READ_ERROR,
};
typedef struct {
    uint32_t signature;               // 4 байта: 0x06054B50
    uint16_t diskNumber;              // 2 байта
    uint16_t startDiskNumber;         // 2 байта
    uint16_t numberCentralDirectoryRecord; // 2 байта
    uint16_t totalCentralDirectoryRecord;    // 2 байта
    uint32_t sizeOfCentralDirectory;  // 4 байта
    uint32_t centralDirectoryOffset;  // 4 байта
    uint16_t commentLength;           // 2 байта
    uint8_t comment[];                // Гибкий массив для комментария
} EOCD;

struct cfh {
    uint16_t made_by_ver;    /* Version made by. */
    uint16_t extract_ver;    /* Version needed to extract. */
    uint16_t gp_flag;        /* General purpose bit flag. */
    uint16_t method;         /* Compression method. */
    uint16_t mod_time;       /* Modification time. */
    uint16_t mod_date;       /* Modification date. */
    uint32_t crc32;          /* CRC-32 checksum. */
    uint32_t comp_size;      /* Compressed size. */
    uint32_t uncomp_size;    /* Uncompressed size. */
    uint16_t name_len;       /* Filename length. */
    uint16_t extra_len;      /* Extra data length. */
    uint16_t comment_len;    /* Comment length. */
    uint16_t disk_nbr_start; /* Disk nbr. where file begins. */
    uint16_t int_attrs;      /* Internal file attributes. */
    uint32_t ext_attrs;      /* External file attributes. */
    uint32_t lfh_offset;     /* Local File Header offset. */
    const uint8_t *name;     /* Filename. */
    const uint8_t *extra;    /* Extra data. */
    const uint8_t *comment;  /* File comment. */
};

typedef struct {
    uint32_t signature; // Сигнатура центрального каталога, должна равняться 0x02014B50
    uint16_t versionMadeBy;
    uint16_t versionNeeded;
    uint16_t generalPurpose;
    uint16_t compressionMethod;
    uint16_t lastModTime;
    uint16_t lastModDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t fileNameLength;
    uint16_t extraFieldLength;
    uint16_t fileCommentLength;
    uint16_t diskNumberStart;
    uint16_t internalFileAttributes;
    uint32_t externalFileAttributes;
    uint32_t localHeaderOffset;
} CentralDirectoryHeader;

typedef unsigned char *buffer;

void safe_free_buffer(buffer ptr) {
    if (ptr != NULL) {
        free(ptr);
    }
}

FILE *open_file(char *filename) {
    FILE *file = fopen(filename, "rb");

    if (!file) {
        perror("Ошибка открытия файла");
        exit(FILE_OPEN_ERROR);
    }

    return file;
}

long get_file_size(FILE *file) {
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    return file_size;
}

unsigned char *load_file_to_buffer(size_t file_size, FILE *file) {
    unsigned char *b = malloc(file_size);
    if (!b) {
        perror("Ошибка выделения памяти буфера");
        fclose(file);
        exit(GET_BUFFER_ERROR);
    }

    return b;
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
    unsigned char eoi[2] = { 0xFF, 0xD9 };

    for (long i = file_size - 2; i >= 0; i--) {
        if (memcmp(buf + i, eoi, sizeof(eoi)) == 0)
            return i;
    }

    return -1;
}

long check_for_zip_sign(buffer buf, size_t file_size) {
    if (file_size < 4) return -1;

    unsigned int zip_eocd = ZIP_EOCD;

    for (long i = file_size - 4; i >= 0; i--) {
        if (memcmp(buf + i, &zip_eocd, sizeof(zip_eocd)) == 0)
            return i;
    }
    return -1;
}

void list_zip_files(unsigned char *buf, uint32_t centralDirOffset, uint16_t numFiles) {

    unsigned char *ptr = buf + centralDirOffset;

    for (uint16_t i = 0; i < numFiles; i++) {
        CentralDirectoryHeader *cdh = (CentralDirectoryHeader *) ptr;

        if (cdh->signature != CENTRAL_DIR) {
            fprintf(stderr, "Неверная сигнатура центрального каталога: 0x%X\n", cdh->signature);
            return;
        }

        // Считываем длины переменных полей: имя файла, extra-поле и комментарий
        uint16_t name_len = cdh->fileNameLength;
        uint16_t extra_len = cdh->extraFieldLength;
        uint16_t comment_len = cdh->fileCommentLength;

        // Имя файла находится сразу после фиксированной части
        char *filename = malloc(name_len + 1);
        if (!filename) {
            perror("Ошибка выделения памяти для имени файла");
            exit(EXIT_FAILURE);
        }
        memcpy(filename, ptr + sizeof(CentralDirectoryHeader), name_len);
        filename[name_len] = '\0';

        printf("Файл: %s\n", filename);
        free(filename);

        // Переход к следующей записи центрального каталога:
        ptr += sizeof(CentralDirectoryHeader) + name_len + extra_len + comment_len;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        perror("Должен быть передан параметр с именем файла");
        exit(NULL_FILENAME);
    }

    char *filename = argv[1];
    printf("Имя файла: %s\n", filename);

    FILE *file = open_file(filename);

    long file_size = get_file_size(file);
    printf("Размер файла: %ld байт\n", file_size);

    buffer file_content = NULL;
    file_content = load_file_to_buffer(file_size, file);
    printf("Выделен буфер для чтения файла\n");

    size_t content_size = read_file_to_buffer(file_content, file_size, file);
    printf("Считано содержимое файла в размере %ld байт\n\n", content_size);

    // Определяем смещения
    long jpeg_eoi_offset = check_for_jpeg_sign(file_content, file_size);
    if (jpeg_eoi_offset < 0) {
        fprintf(stderr, "JPEG_EOI не найден.\n");
        fclose(file);
        printf("Файл %s закрыт\n", filename);
        safe_free_buffer(file_content);
        exit(FILE_NOT_JPEG);
    }
    printf("Обнаружено смещение JPEG_EOI: %lX\n", jpeg_eoi_offset);

    // Найдем EOCD
    long eocd_offset = check_for_zip_sign(file_content, file_size);
    if (eocd_offset >= 0) {
        EOCD *eocd = (EOCD *) (file_content + eocd_offset);
        printf("Обнаружено смещение EOCD: %lX\n", eocd_offset);

        long centralDirOffset = eocd_offset - eocd->sizeOfCentralDirectory;
        printf("Обнаружено смещение centralDirectory: %lX\n", centralDirOffset);

        uint16_t numFiles = eocd->numberCentralDirectoryRecord;
        printf("Обнаружено файлов в архиве: %d\n", numFiles);

        list_zip_files(file_content, centralDirOffset, numFiles);
    } else {
        printf("Файл %s НЕ является ZipJPEG'ом\n", filename);
    }

    fclose(file);
    printf("Файл %s закрыт\n", filename);

    safe_free_buffer(file_content);
    return 0;
}
