#include <stdio.h>
#include <stdlib.h>

// empty_str: db 0x0
char *empty_str = "";
// int_format: db "%ld ", 0x0
char *int_format = "%ld";
// data: dq 4, 8, 15, 16, 23, 42
long data[] = {4, 8, 15, 16, 23, 42};
// data_length: equ ($-data) / 8
int data_length = sizeof(data) / sizeof(data[0]);

// print_int:
void print_int(const long data) {
  printf(int_format, data);
  fflush(stdout);
}

typedef struct list {
  long data;
  struct list *next;
} list;

// add_element:
list *add_element(const long data, list *head) {
  list *l = malloc(sizeof(list));
  if (l == NULL) {
    // abort
    return NULL;
  }
  l->data = data;
  l->next = head;
  return l;
}

int main() {
  list *head = NULL;
  int i = data_length - 1;

  // adding_loop
  while (i != 0) {
    head = add_element(data[i], head);
    --i;
  }

  // печать
  const list *cur = head;
  while (cur) {
    print_int(cur->data);
    cur = cur->next;
  }
  puts(empty_str);

  // функция filter(список, акумелятор, предикат)
  // список
  cur = head;
  // акумулятор
  list *odd_list = NULL;
  while (cur) {
    // предикат в оригинале как функция высшего порядка в filter
    if (cur->data % 2 == 1) {
      odd_list = add_element(cur->data, odd_list);
    }
    cur = cur->next;
  }

  // печать
  while (odd_list) {
    print_int(odd_list->data);
    odd_list = odd_list->next;
  }
  puts(empty_str);

  // все же
  while (head) {
    const list *tmp = head;
    free(head);
    head = tmp->next;
  }
}
