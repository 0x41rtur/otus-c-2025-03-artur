#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>

/* Формат времени */
#define LOCAL_TIME_FMT "%H:%M:%S %Y-%m-%d" // 18:39:50 2025-05-17

/* Цвета */
#define COLOR_PURPLE "\033[35m" // DEBUG
#define COLOR_CYAN   "\033[36m" // INFO
#define COLOR_RED    "\033[31m" // ERROR
#define COLOR_YELLOW "\033[33m" // WARNING
#define COLOR_RESET  "\033[0m"

/* Строка цвета по уровню */
#define GET_COLOR_BY_LEVEL(level)                                              \
        ((level) == DEBUG     ? COLOR_PURPLE                                   \
         : (level) == INFO    ? COLOR_CYAN                                     \
         : (level) == WARNING ? COLOR_YELLOW                                   \
         : (level) == ERROR   ? COLOR_RED                                      \
                              : COLOR_RESET)

/* Строковое представление уровня */
#define GET_LEVEL_STR(level)                                                   \
        ((level) == DEBUG     ? "DEBUG"                                        \
         : (level) == INFO    ? "INFO"                                         \
         : (level) == WARNING ? "WARNING"                                      \
         : (level) == ERROR   ? "ERROR"                                        \
                              : "UNKNOWN")

/* Структура конфигурации */
struct console_cfg
{
        const char    *out_file;  // файл логов
        unsigned short duplicate; // дублировать ли в консоль
        unsigned short buf_size;  // размер буфера (не используется пока)
};

static struct console_cfg *config = NULL;

/* Уровни логов */
enum CONSOLE_LEVEL
{
        DEBUG = 0,
        INFO,
        WARNING,
        ERROR
};

/* Получение локального времени */
static const struct tm *
ltime(void)
{
        static struct tm result;
        struct timeval   t;

        if (gettimeofday(&t, NULL) == -1)
        {
                perror("gettimeofday failed");
                return NULL;
        }

        struct tm *tmp = localtime(&t.tv_sec);
        if (!tmp)
        {
                perror("localtime failed");
                return NULL;
        }

        result = *tmp;
        return &result;
}

/* Генерация префикса времени */
static void
get_time_source(char *dest_buff, size_t size)
{
        const struct tm *t = ltime();
        if (t)
        {
                char timebuff[64];
                if (strftime(timebuff, sizeof(timebuff), LOCAL_TIME_FMT, t))
                {
                        snprintf(dest_buff, size, "[%s]", timebuff);
                        return;
                }
        }
        snprintf(dest_buff, size, "[TIME_ERROR]");
}

/* Инициализация конфигурации */
void
console_init(const struct console_cfg *cfg)
{
        if (config)
        {
                free(config);
        }
        config = malloc(sizeof(struct console_cfg));
        if (!config)
        {
                perror("console_init: malloc failed");
                return;
        }
        if (cfg)
        {
                memcpy(config, cfg, sizeof(struct console_cfg));
        }
        else
        {
                config->out_file  = NULL;
                config->duplicate = 1;
                config->buf_size  = 1024;
        }
}

/* Универсальная функция логирования */
void
console_log(enum CONSOLE_LEVEL level, const char *fmt, ...)
{
        char message[1024];
        char timebuff[128];
        // Форматирование строки
        va_list args;
        va_start(args, fmt);
        vsnprintf(message, sizeof(message), fmt, args);
        va_end(args);
        // Время
        get_time_source(timebuff, sizeof(timebuff));
        // Уровень и цвет
        const char *color    = GET_COLOR_BY_LEVEL(level);
        const char *levelstr = GET_LEVEL_STR(level);
        // Печать в консоль
        if (!config || config->duplicate)
        {
                printf("%s%s %s%s %s\n", color, timebuff, levelstr, COLOR_RESET,
                       message);
        }
        // Запись в файл, если включена
        if (config && config->out_file)
        {
                FILE *f = fopen(config->out_file, "a");
                if (f)
                {
                        fprintf(f, "%s %s %s\n", timebuff, levelstr, message);
                        fclose(f);
                }
                else
                {
                        perror("console_log: failed to open log file");
                }
        }
}

/* Упрощённые обёртки */
#define CONSOLE_DEBUG(fmt, ...) console_log(DEBUG, fmt, ##__VA_ARGS__)
#define CONSOLE_INFO(fmt, ...)  console_log(INFO, fmt, ##__VA_ARGS__)
#define CONSOLE_WARN(fmt, ...)  console_log(WARNING, fmt, ##__VA_ARGS__)
#define CONSOLE_ERROR(fmt, ...) console_log(ERROR, fmt, ##__VA_ARGS__)

/* Пример использования */
int
main(void)
{
        const struct console_cfg config = {
            .out_file = "log.txt", .duplicate = 1, .buf_size = 1024};

        console_init(&config);

        CONSOLE_DEBUG("Статичный вывод");
        CONSOLE_INFO("Строка: %s, Число: %d", "Apple", 42);
        CONSOLE_WARN("Что-то пошло не так: %s", "warning details");
        CONSOLE_ERROR("Фатальная ошибка: %s", "segmentation fault");

        return 0;
}