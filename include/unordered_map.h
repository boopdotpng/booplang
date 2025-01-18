#pragma once

// function pointers for memory mgmt
typedef void (*free_fn)(void*);

// open addressing with double hashing
typedef struct unordered_map {
    int capacity;
    double load_factor;

    // these flags indicate if the map "owns" keys/values
    // if it does, it calls the associated free fns
    int owns_keys;
    int owns_vals;

    // callbacks that are used if the map has ownership
    free_fn key_destructor;
    free_fn val_destructor;

    int size;
    int tombstone_count;
    char **keys;
    void **values;
} unordered_map;

unordered_map *create_map(int capacity, double load_factor, int owns_keys, int owns_vals, free_fn key_destructor, free_fn val_destructor);
void insert_map(unordered_map *map, const char *str, void *value);
void *get_map(unordered_map *map, const char *str);
void remove_map(unordered_map *map, const char *str);
static void resize(unordered_map *map);
void destroy_map(unordered_map *map);
char *intern_string(unordered_map *intern_table, const char *str);