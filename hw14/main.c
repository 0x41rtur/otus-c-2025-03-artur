#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <float.h>
#include <math.h>

typedef struct {
    double sum;
    double mean;
    double variance;
    double min;
    double max;
    int count;
} Stats;

int compute_stats(sqlite3 *db, const char *table, const char *column, Stats *stats) {
    char query[256];
    snprintf(query, sizeof(query), "SELECT %s FROM %s;", column, table);

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Ошибка SQL: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // Инициализация
    stats->sum = 0.0;
    stats->count = 0;
    stats->min = DBL_MAX;
    stats->max = -DBL_MAX;

    // Алгоритм Вельфорда для среднего и дисперсии
    double M = 0.0, S = 0.0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) == SQLITE_NULL) {
            continue; // пропускаем NULL
        }

        double val;
        if (sqlite3_column_type(stmt, 0) == SQLITE_INTEGER) {
            val = sqlite3_column_int64(stmt, 0);
        } else if (sqlite3_column_type(stmt, 0) == SQLITE_FLOAT) {
            val = sqlite3_column_double(stmt, 0);
        } else {
            // не число — пропускаем
            continue;
        }

        stats->sum += val;
        if (val < stats->min) stats->min = val;
        if (val > stats->max) stats->max = val;

        stats->count++;

        // Обновление для дисперсии (онлайн)
        double oldM = M;
        M += (val - M) / stats->count;
        S += (val - oldM) * (val - M);
    }

    sqlite3_finalize(stmt);

    if (stats->count > 0) {
        stats->mean = stats->sum / stats->count;
        stats->variance = S / stats->count; // дисперсия
    } else {
        stats->mean = 0;
        stats->variance = 0;
        stats->min = 0;
        stats->max = 0;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Использование: %s <база.sqlite> <таблица> <колонка>\n", argv[0]);
        return 1;
    }

    const char *db_name = argv[1];
    const char *table = argv[2];
    const char *column = argv[3];

    sqlite3 *db;
    if (sqlite3_open(db_name, &db) != SQLITE_OK) {
        fprintf(stderr, "Не удалось открыть БД '%s': %s\n", db_name, sqlite3_errmsg(db));
        return 1;
    }

    Stats stats;
    if (compute_stats(db, table, column, &stats) == 0) {
        if (stats.count == 0) {
            printf("Нет числовых данных в колонке '%s'.\n", column);
        } else {
            printf("Колонка: %s\n", column);
            printf("Количество чисел: %d\n", stats.count);
            printf("Сумма: %.6f\n", stats.sum);
            printf("Среднее: %.6f\n", stats.mean);
            printf("Минимум: %.6f\n", stats.min);
            printf("Максимум: %.6f\n", stats.max);
            printf("Дисперсия: %.6f\n", stats.variance);
        }
    }

    sqlite3_close(db);
    return 0;
}
