#include <stdio.h>     // Для printf, fprintf, perror, fopen, fread, fclose
#include <stdint.h>    // Для uint32_t, uint8_t
#include <stdlib.h>    // Для malloc, free, exit

// 1 mb
#define CHUNK_SIZE (1024 * 1024)
// Стандартный полином CRC32 IEEE 802.3
#define CRC32_POLYNOMIAL 0xEDB88320
// Таблица для ускорения расчёта CRC32, заполняется один раз при запуске
static uint32_t crc32_table[256];
// Генерирует таблицу CRC32 на основе указанного полинома
void generate_crc32_table(void) {
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (int j = 0; j < 8; ++j) {
            // Если младший бит установлен, применяем полином, иначе просто сдвиг
            if (crc & 1)
                crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
            else
                crc >>= 1;
        }
        crc32_table[i] = crc;
    }
}
// Обновляет CRC32 значением из переданных данных
// @param crc    текущая контрольная сумма
// @param data   указатель на буфер с данными
// @param len    количество байт в буфере
// @return       новая контрольная сумма
uint32_t update_crc32(uint32_t crc, const uint8_t *data, size_t len) {
    crc = ~crc;  // Инвертируем начальное значение
    while (len--) {
        // Используем таблицу и байт данных, чтобы обновить значение
        crc = (crc >> 8) ^ crc32_table[(crc ^ *data++) & 0xFF];
    }
    return ~crc; // Инвертируем обратно
}
// Вычисляет CRC32 для файла, читая его последовательно блоками
// @param file   дескриптор открытого файла
// @return       финальная CRC32 контрольная сумма
uint32_t crc32_file(FILE *file) {
    uint8_t *buffer = malloc(CHUNK_SIZE);  // Аллоцируем буфер для чтения данных
    if (!buffer) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    uint32_t crc = 0;                      // Начальное значение CRC
    size_t bytesRead;
    // Читаем файл последовательно, блоками
    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        crc = update_crc32(crc, buffer, bytesRead);  // Обновляем CRC по данным
    }
    // Проверяем, была ли ошибка при чтении
    if (ferror(file)) {
        perror("fread");
        free(buffer);
        exit(EXIT_FAILURE);
    }
    free(buffer);  // Освобождаем буфер
    return crc;
}

// Точка входа в программу
int main(int argc, char *argv[]) {
    // Проверяем количество аргументов
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    generate_crc32_table();  // Заполняем таблицу CRC
    // Открываем файл в бинарном режиме
    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("fopen");
        return EXIT_FAILURE;
    }
    uint32_t crc = crc32_file(file);  // Считаем CRC32
    fclose(file);                     // Закрываем файл
    // Печатаем контрольную сумму в hex-формате с ведущими нулями
    printf("CRC32: %08X\n", crc);
    return EXIT_SUCCESS;
}
