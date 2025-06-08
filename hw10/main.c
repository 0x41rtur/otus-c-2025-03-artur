#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_LINE 4096
#define MAX_ENTRIES 10000
#define TOP_N 10

typedef struct {
    char key[1024];
    long long value;
} map_entry;

typedef struct {
    char* filepath;
    long long total_bytes;
    map_entry url_map[MAX_ENTRIES];
    int url_count;
    map_entry ref_map[MAX_ENTRIES];
    int ref_count;
} thread_data;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int active_thread_count = 0;

void add_to_map(map_entry *map, int *count, const char *key, long long value) {
    for (int i = 0; i < *count; ++i) {
        if (strcmp(map[i].key, key) == 0) {
            map[i].value += value;
            return;
        }
    }
    if (*count < MAX_ENTRIES) {
        strncpy(map[*count].key, key, sizeof(map[*count].key) - 1);
        map[*count].key[sizeof(map[*count].key) - 1] = '\0';
        map[*count].value = value;
        (*count)++;
    }
}

int compare_desc(const void *a, const void *b) {
    return ((map_entry *)b)->value - ((map_entry *)a)->value;
}

void print_top(map_entry *map, int count, const char *title) {
    printf("\n== %s ==\n", title);
    qsort(map, count, sizeof(map_entry), compare_desc);
    for (int i = 0; i < count && i < TOP_N; ++i) {
        printf("%-10lld\t%s\n", map[i].value, map[i].key);
    }
}

bool parse_log_line(char *line, long long *total_bytes,
                  map_entry *url_map, int *url_count,
                  map_entry *ref_map, int *ref_count) {
    char *req_start = strchr(line, '"');
    if (!req_start) return false;

    char *req_end = strchr(req_start + 1, '"');
    if (!req_end) return false;

    char *url = NULL;
    char *saveptr;
    char *method = strtok_r(req_start + 1, " ", &saveptr);
    if (method) url = strtok_r(NULL, " ", &saveptr);

    char *size_str = NULL;
    char *p = req_end + 2;
    for (int i = 0; i < 2 && p; ++i) {
        p = strchr(p, ' ');
        if (p) p++;
    }
    if (p) size_str = p;

    char *ref_start = strchr(p ? p : req_end, '"');
    char *referer = NULL;
    if (ref_start) {
        char *ref_end = strchr(ref_start + 1, '"');
        if (ref_end) {
            *ref_end = '\0';
            referer = ref_start + 1;
        }
    }

    long long bytes = 0;
    if (size_str && strcmp(size_str, "-") != 0) {
        bytes = atoll(size_str);
        *total_bytes += bytes;
    }

    if (url) add_to_map(url_map, url_count, url, bytes);
    if (referer && strcmp(referer, "-") != 0)
        add_to_map(ref_map, ref_count, referer, 1);

    return true;
}

void* calculate(void *arg) {
    thread_data *data = (thread_data *)arg;
    printf("Поток начал обработку: %s\n", data->filepath);

    FILE *log_file = fopen(data->filepath, "r");
    if (!log_file) {
        perror(data->filepath);
        goto cleanup;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), log_file)) {
        parse_log_line(line, &data->total_bytes,
                      data->url_map, &data->url_count,
                      data->ref_map, &data->ref_count);
    }
    fclose(log_file);

    printf("\n== Файл: %s ==\n", data->filepath);
    printf("Общее количество отданных байт: %lld\n", data->total_bytes);
    print_top(data->url_map, data->url_count, "Топ 10 URL по трафику");
    print_top(data->ref_map, data->ref_count, "Топ 10 Referer'ов по числу");

cleanup:
    free(data->filepath);
    free(data);

    pthread_mutex_lock(&mutex);
    active_thread_count--;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, const char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <log_dir> <thread_count>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int max_threads = atoi(argv[2]);
    if (max_threads <= 0) {
        fprintf(stderr, "Thread count must be >= 1\n");
        return EXIT_FAILURE;
    }

    DIR *dir = opendir(argv[1]);
    if (!dir) {
        perror("opendir");
        return EXIT_FAILURE;
    }

    pthread_t *threads = calloc(max_threads, sizeof(pthread_t));
    if (!threads) {
        perror("malloc threads");
        closedir(dir);
        return EXIT_FAILURE;
    }

    struct dirent *entry;
    int running_threads = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", argv[1], entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0 || !S_ISREG(st.st_mode)) {
            fprintf(stderr, "Пропускаем: %s (не файл)\n", full_path);
            continue;
        }

        while (1) {
            pthread_mutex_lock(&mutex);
            if (active_thread_count < max_threads) {
                active_thread_count++;
                pthread_mutex_unlock(&mutex);
                break;
            }
            pthread_mutex_unlock(&mutex);
        }

        thread_data *data = calloc(1, sizeof(thread_data));
        if (!data) continue;

        data->filepath = strdup(full_path);
        if (!data->filepath) {
            free(data);
            continue;
        }

        if (pthread_create(&threads[running_threads % max_threads], NULL,
                          calculate, data) != 0) {
            perror("pthread_create");
            free(data->filepath);
            free(data);
            pthread_mutex_lock(&mutex);
            active_thread_count--;
            pthread_mutex_unlock(&mutex);
            continue;
        }
        running_threads++;
    }

    for (int i = 0; i < running_threads && i < max_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    closedir(dir);
    pthread_mutex_destroy(&mutex);
    return EXIT_SUCCESS;
}
