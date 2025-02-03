#pragma once
#include "token.h"
#include <stddef.h>

typedef struct intern_table intern_table;

typedef struct {
  char *key;
  token_type value;
} intern_result;

intern_table *create_intern_table(int capacity, double load_factor);
intern_result intern_string(intern_table *t, const char *start, size_t len, token_type value);
void destroy_intern_table(intern_table *t);
token_type get_interned_value(intern_table *t, const char *str);
