#include <curl/curl.h>
#include <stdio.h>
#include <string.h>

#include "./external/mjson.h"

typedef size_t (*fetch_callback)(void *data, size_t size, size_t nmemb,
                                 void *userp);

typedef struct {
  char *memory;
  size_t size;
} response_buffer;

CURL *init_client(void) {
  static int inited = 0;
  static CURL *instance;
  if (!inited) {
    curl_global_init(CURL_GLOBAL_ALL);
    instance = curl_easy_init();
    inited = 1;
  }
  return instance;
}

void destroy_client(CURL *instance) {
  static int cleaned = 0;
  if (!cleaned) {
    curl_easy_cleanup(instance);
    curl_global_cleanup();
    cleaned = 1;
  }
}

void print_response(const response_buffer *buffer) {
  char *current_condition;
  int len = 0;
  mjson_find(buffer->memory, buffer->size, "$.current_condition[0]",
             &current_condition, &len);

  if (len > 0) {
    char *data;
    int data_len;
    mjson_find(current_condition, len, "$.temp_C", &data, &data_len);
    printf("Температура %.*s градусов цельсия\n", data_len, data);

    mjson_find(current_condition, len, "$.windspeedKmph", &data, &data_len);
    printf("Ветер %.*s метров в секунду\n", data_len, data);

    mjson_find(current_condition, len, "$.weatherDesc[0].value", &data,
               &data_len);
    printf("%.*s\n", data_len, data);
  } else {
    printf("No data\n");
  }
}

void fetch(CURL *instance, const char *url, const fetch_callback handler) {
  curl_easy_setopt(instance, CURLOPT_URL, url);
  curl_easy_setopt(instance, CURLOPT_WRITEFUNCTION, handler);

  response_buffer buffer = {0};

  curl_easy_setopt(instance, CURLOPT_WRITEDATA, (void *)&buffer);

  const CURLcode code = curl_easy_perform(instance);
  if (code != CURLE_OK) {
    printf("Fetch failed: %s\n", curl_easy_strerror(code));
    return;
  }

  print_response(&buffer);
}

char *create_path(const char *city) {
  char *url = malloc(sizeof(char) * 128);
  strcpy(url, "https://wttr.in/");
  strcat(url, city);
  strcat(url, "?format=j1");
  return url;
}

size_t handler(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t total_size = size * nmemb;
  response_buffer *buf = userp;
  char *new_mem =
      realloc(buf->memory, buf->size + total_size + 1); // +1 для '\0'
  if (new_mem == NULL) {
    fprintf(stderr, "realloc failed\n");
    return 0;
  }
  buf->memory = new_mem;
  memcpy(&(buf->memory[buf->size]), contents, total_size);
  buf->size += total_size;
  buf->memory[buf->size] = 0;
  return total_size;
}

int main(const int argc, char **argv) {
  if (argc != 2) {
    printf("USAGE: %s <city>\n", argv[0]);
    return EXIT_FAILURE;
  }

  CURL *client = init_client();
  if (client == NULL) {
    printf("Curl init failed\n");
    return EXIT_FAILURE;
  }

  char *path = create_path(argv[1]);
  fetch(client, path, handler);

  free(path);
  destroy_client(client);

  return EXIT_SUCCESS;
}
