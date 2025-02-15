#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <cJSON.h>

enum ERRORS {
    NOT_ENOUGH_ARGUMENTS = 1,
    NULL_CITY_STR,
    CURL_ERROR,
    BAD_RESPONSE_CODE,
};

// Структура для хранения ответа
struct Memory {
    char *data;
    size_t size;
};

struct Weather {
    char *weatherDesc;
    char *temp_C;
    char *FeelsLikeC;
    char *windspeedKmph;
    char *winddir16Point;
};

const char *base_url = "https://wttr.in/";
const char *format_url = "?lang=ru&format=j1";

// Функция для обработки ответа curl
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t real_size = size * nmemb;
    struct Memory *mem = (struct Memory *) userdata;

    // Расширяем буфер
    char *temp = realloc(mem->data, mem->size + real_size + 1);
    if (temp == NULL) {
        fprintf(stderr, "Ошибка перевыделения памяти для буфера ответа\n");
        return 0; // Прерываем запрос
    }

    mem->data = temp;
    memcpy(&(mem->data[mem->size]), ptr, real_size);
    mem->size += real_size;
    mem->data[mem->size] = '\0'; // Добавляем терминальный символ

    return real_size;
}

char *build_url(char *city) {
    size_t city_size = strlen(city);
    size_t base_url_size = strlen(base_url);
    size_t format_url_size = strlen(format_url);
    size_t url_size = base_url_size + city_size;

    char *url = malloc(url_size);
    strncat(url, base_url, base_url_size);
    strncat(url, city, city_size);
    strncat(url, format_url, format_url_size);

    return url;
}

char *get_weather_json(char *city) {
    if (city == NULL) {
        perror("Получена некорректная строка с названием города!");
        exit(NULL_CITY_STR);
    }

    CURL *curl;
    CURLcode res;

    struct Memory response;
    response.data = malloc(1); // Изначально пустой буфер
    response.size = 0;

    if (response.data == NULL) {
        fprintf(stderr, "Ошибка выделения памяти для буфера ответа\n");
        return NULL;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        char *url = build_url(city);
        curl_easy_setopt(curl, CURLOPT_URL, url); // "); // Устанавливаем URL
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "Ошибка запроса: %s\n", curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            free(url);
            exit(CURL_ERROR);
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code != 200) {
            fprintf(stderr, "Ошибка: сервер вернул HTTP %ld\n", http_code);
            curl_easy_cleanup(curl);
            free(response.data);
            free(url);
            exit(BAD_RESPONSE_CODE);
        }

        curl_easy_cleanup(curl);
        free(url);
    }

    curl_global_cleanup();

    return response.data;
}

cJSON *cJSON_GetFirstArrayItem(cJSON *json, char *value) {
    if (json == NULL) {
        fprintf(stderr, "Передан некорректный JSON объект для получения значения %s\n", value);
        return NULL;
    }

    cJSON *array = cJSON_GetObjectItemCaseSensitive(json, value);
    if (array == NULL) {
        fprintf(stderr, "Не удалось получить значение %s из %s\n", value, json->string);
        return NULL;
    }

    printf("Анализ элемента: %s\n", array->string);

    if (cJSON_IsArray(array) && cJSON_GetArraySize(array) > 0) {
        cJSON *item = cJSON_GetArrayItem(array, 0);

        return item;
    }

    fprintf(stderr, "Элемент \'%s\' не является массивом\n", json->string);
    return NULL;
}

char *cJSON_GetStringFromItem(cJSON *json, char *value) {
    if (json == NULL) {
        fprintf(stderr, "Передан некорректный JSON объект для получения значения %s\n", value);
        return NULL;
    }

    cJSON *item = cJSON_GetObjectItemCaseSensitive(json, value);

    if (item == NULL) {
        fprintf(stderr, "Не удалось получить значение %s из %s\n", value, json->string);
        return NULL;
    }

    printf("Анализ элемента: %s\n", item->string);

    if (!cJSON_IsString(item) || (item->valuestring == NULL)) {
        fprintf(stderr, "Значение %s из %s не является строкой\n", value, json->string);
        return NULL;
    }

    return item->valuestring;
}

char *get_full_winddir(char *winddir16Point) {
    size_t buffer_size = 50;
    char *buffer = calloc(buffer_size, sizeof(char));

    if (strncmp(winddir16Point, "NNW", 3) == 0) { strncpy(buffer, "с севера на северо-запад", buffer_size); } else if (
        strncmp(winddir16Point, "NNE", 3) == 0) { strncpy(buffer, "с севера на северо-восток", buffer_size); } else if (
        strncmp(winddir16Point, "SSW", 3) == 0) { strncpy(buffer, "с юга на юго-запад", buffer_size); } else if (
        strncmp(winddir16Point, "SSE", 3) == 0) { strncpy(buffer, "с юга на юго-восток", buffer_size); } else if (
        strncmp(winddir16Point, "WNW", 3) == 0) { strncpy(buffer, "с запада на северо-запад", buffer_size); } else if (
        strncmp(winddir16Point, "WSW", 3) == 0) { strncpy(buffer, "с запада на юго-запад", buffer_size); } else if (
        strncmp(winddir16Point, "ENE", 3) == 0) { strncpy(buffer, "с востока на северо-восток", buffer_size); } else if
    (strncmp(winddir16Point, "ESE", 3) == 0) { strncpy(buffer, "с востока на юго-восток", buffer_size); } else {
        snprintf(buffer, buffer_size, "Направление не определено: %s", winddir16Point);
    }

    return buffer;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        perror("Для работы программы нужно указать название города в аргументах! (Например, \'weather Moscow\'");
        exit(NOT_ENOUGH_ARGUMENTS);
    }

    char *city = argv[1];
    printf("Получено название города: \'%s\'\n", city);

    char *json_string = get_weather_json(city);

    FILE *log = fopen("json.log", "w");
    fwrite(json_string, strlen(json_string), sizeof(char), log);
    fclose(log);

    struct Weather weather;

    cJSON *json = cJSON_Parse(json_string);
    cJSON *current_condition = cJSON_GetFirstArrayItem(json, "current_condition");

    cJSON *weatherDesc = cJSON_GetFirstArrayItem(current_condition, "lang_ru");

    weather.weatherDesc = cJSON_GetStringFromItem(weatherDesc, "value");
    weather.temp_C = cJSON_GetStringFromItem(current_condition, "temp_C");
    weather.FeelsLikeC = cJSON_GetStringFromItem(current_condition, "FeelsLikeC");
    weather.windspeedKmph = cJSON_GetStringFromItem(current_condition, "windspeedKmph");
    weather.winddir16Point = cJSON_GetStringFromItem(current_condition, "winddir16Point");

    weather.weatherDesc = weather.weatherDesc == NULL ? "Empty" : weather.weatherDesc;
    weather.temp_C = weather.temp_C == NULL ? "Empty" : weather.temp_C;
    weather.FeelsLikeC = weather.FeelsLikeC == NULL ? "Empty" : weather.FeelsLikeC;
    weather.windspeedKmph = weather.windspeedKmph == NULL ? "Empty" : weather.windspeedKmph;
    weather.winddir16Point = weather.winddir16Point == NULL ? "Empty" : get_full_winddir(weather.winddir16Point);

    printf("\nПогода в г. %s:\n", city);
    printf("%s\n", weather.weatherDesc);
    printf("Температура: %s˚C\n", weather.temp_C);
    printf("Ощущается как: %s °С\n", weather.FeelsLikeC);
    printf("Скорость ветра: %s км/ч\n", weather.windspeedKmph);
    printf("Направление ветра: %s\n", weather.winddir16Point);

    cJSON_Delete(json);
    free(weather.winddir16Point);
    free(json_string);
    return 0;
}
