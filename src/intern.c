#include "intern.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EMPTY_SLOT 0
#define TOMBSTONE ((char *)-1)

struct intern_table {
  int capacity;
  double load_factor;
  int size;
  int tombstone_count;
  char **keys;
  token_type *values;
};

static unsigned long hash1(const char *str) {
  unsigned long h = 5381;
  int c;
  while ((c = *str++))
    h = ((h << 5) + h) + c;
  return h;
}

static unsigned long hash2(const char *str, int capacity) {
  unsigned long h = 0;
  int c;
  while ((c = *str++))
    h = (h * 31 + c) % (capacity - 1);
  return (h | 1);
}

static int find_slot(intern_table *t, const char *str, int for_insert) {
  unsigned long h1v = hash1(str) % t->capacity;
  unsigned long h2v = hash2(str, t->capacity);
  int slot = (int)h1v;
  int first_tomb = -1;

  for (int i = 0; i < t->capacity; i++) {
    char *k = t->keys[slot];
    if (k == EMPTY_SLOT) {
      if (for_insert && first_tomb != -1) return first_tomb;
      return slot;
    } else if (k == TOMBSTONE) {
      if (for_insert && first_tomb == -1) first_tomb = slot;
    } else if (strcmp(k, str) == 0) {
      return slot;
    }
    slot = (slot + h2v) % t->capacity;
  }
  return -1;  // table is full
}

static void resize(intern_table *t) {
  int oldcap = t->capacity;
  char **oldkeys = t->keys;
  token_type *oldvalues = t->values;

  int newcap = t->capacity * 2;
  char **newkeys = calloc(newcap, sizeof(char *));
  token_type *newvalues = calloc(newcap, sizeof(token_type));
  if (!newkeys || !newvalues) {
    fprintf(stderr, "failed to allocate new intern table arrays\n");
    free(newkeys);
    free(newvalues);
    exit(EXIT_FAILURE);
  }

  t->capacity = newcap;
  t->size = 0;
  t->tombstone_count = 0;
  t->keys = newkeys;
  t->values = newvalues;

  // reinsert old entries into new table
  for (int i = 0; i < oldcap; i++) {
    char *k = oldkeys[i];
    if (k && k != EMPTY_SLOT && k != TOMBSTONE) {
      int slot = find_slot(t, k, 1);
      if (slot == -1) {
        fprintf(stderr, "unexpected error during rehashing\n");
        continue;
      }
      t->keys[slot] = k;
      t->values[slot] = oldvalues[i];
      t->size++;
    }
  }
  free(oldkeys);
  free(oldvalues);
}

intern_result intern_string(intern_table *t, const char *start, size_t len, token_type value) {
  // use strndup directly to allocate and null-terminate
  char *temp = strndup(start, len);
  if (!temp) {
    fprintf(stderr, "failed to allocate string in intern_string\n");
    exit(EXIT_FAILURE);
  }

  int slot = find_slot(t, temp, 0);
  if (slot != -1 && t->keys[slot] != EMPTY_SLOT && t->keys[slot] != TOMBSTONE) {
    // already interned; free our duplicate and return existing entry
    free(temp);
    return (intern_result){t->keys[slot], t->values[slot]};
  }

  double ratio = (double)t->size / t->capacity;
  double tomb_ratio = (double)t->tombstone_count / t->capacity;
  if (ratio >= t->load_factor || tomb_ratio >= 0.4) {
    resize(t);
    // recompute slot after resize
    slot = find_slot(t, temp, 1);
    if (slot == -1) {
      fprintf(stderr, "couldn't find a slot after resizing\n");
      free(temp);
      exit(EXIT_FAILURE);
    }
  } else if (slot == -1) {
    // fallback in case find_slot didn't return a valid slot
    slot = find_slot(t, temp, 1);
    if (slot == -1) {
      fprintf(stderr, "couldn't find a slot for insertion\n");
      free(temp);
      exit(EXIT_FAILURE);
    }
  }

  if (t->keys[slot] == EMPTY_SLOT) {
    t->size++;
  } else if (t->keys[slot] == TOMBSTONE) {
    t->tombstone_count--;
  }
  t->keys[slot] = temp;
  t->values[slot] = value;
  return (intern_result){temp, value};
}

intern_table *create_intern_table(int capacity, double load_factor) {
  intern_table *tbl = calloc(1, sizeof(*tbl));
  if (!tbl) {
    fprintf(stderr, "failed to allocate intern_table\n");
    exit(EXIT_FAILURE);
  }
  tbl->capacity = capacity;
  tbl->load_factor = load_factor;
  tbl->size = 0;
  tbl->tombstone_count = 0;
  tbl->keys = calloc(capacity, sizeof(char *));
  tbl->values = calloc(capacity, sizeof(token_type));
  if (!tbl->keys || !tbl->values) {
    fprintf(stderr, "failed to allocate intern_table arrays\n");
    free(tbl->keys);
    free(tbl->values);
    free(tbl);
    exit(EXIT_FAILURE);
  }
  return tbl;
}

void destroy_intern_table(intern_table *t) {
  if (!t) return;
  for (int i = 0; i < t->capacity; i++) {
    char *k = t->keys[i];
    if (k && k != EMPTY_SLOT && k != TOMBSTONE) free(k);
  }
  free(t->keys);
  free(t->values);
  free(t);
}
