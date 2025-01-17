#pragma once

// open addressing with double hashing
typedef struct {
    int capacity;
    double load_factor;
    int size;
    char **keys; // array of string pointers
    int *values;
    int tombstone_count;
} unordered_map;

unordered_map *create_map(int capacity, double load_factor);
void insert_map(unordered_map *map, const char *str, int value);
int get_map(unordered_map *map, const char *str);
void remove_map(unordered_map *map, const char *str);
void resize(unordered_map *map);
void destroy_map(unordered_map *map);