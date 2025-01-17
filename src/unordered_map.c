#include "unordered_map.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// sentinel values for empty and deleted slots
#define EMPTY_SLOT 0
#define TOMBSTONE ((char*)-1)

// djb2 hash: http://www.cse.yorku.ca/~oz/hash.html
unsigned long hash1(const char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;    /* hash * 33 + c */

    return hash;
}

unsigned long hash2(const char *str, int capacity) {
    unsigned long hash = 0;
    int c;
    while ((c = *str++))
        hash = (hash * 31 + c) % (capacity - 1);
    return (hash | 1);
}

unordered_map *create_map(int capacity, double load_factor) {
    unordered_map *map = malloc(sizeof(unordered_map));
    if (!map) {
        fprintf(stderr, "failed to allocate memory for map");
        exit(EXIT_FAILURE);
    }

    map->capacity = capacity;
    map->load_factor = load_factor;
    map->size = 0;

    map->keys = malloc(sizeof(char*) * capacity);
    if (!map->keys) {
        perror("failed to allocate memory for keys");
        free(map);
        exit(EXIT_FAILURE);
    }
    memset(map->keys, EMPTY_SLOT, sizeof(char*) * capacity);
    map->values = malloc(sizeof(int) * capacity);
    if (!map->values) {
        perror("failed to allocate memory for values");
        free(map->keys);
        free(map);
        exit(EXIT_FAILURE);
    }

    return map;
}

int find_slot(unordered_map *map, const char *key, int for_insert) {
    unsigned long h1 = hash1(key) % map->capacity;
    unsigned long h2 = hash2(key, map->capacity);
    int slot = h1;
    int first_tombstone = -1;

    for(int i = 0; i < map->capacity; i++) {
        if (map->keys[slot] == EMPTY_SLOT) {
            if (for_insert && first_tombstone != -1)
                return first_tombstone;
            return slot;
        } else if (map->keys[slot] == TOMBSTONE) {
            if (for_insert && first_tombstone == -1)
                first_tombstone = slot;
        } else {
            if (strcmp(map->keys[slot], key) == 0)
                return slot;
        }
        slot = (slot + h2) % map->capacity;
    }

    // map full or key not found
    return -1;
}

void resize(unordered_map *map) {
    int old_capacity = map->capacity;
    char **old_keys = map->keys;
    int *old_values = map->values;

    map->capacity *= 2;
    map->size = 0;
    map->tombstone_count = 0;

    map->keys = malloc(sizeof(char*) * map->capacity);
    memset(map->keys, EMPTY_SLOT, sizeof(char*) * map->capacity);
    map->values = malloc(sizeof(int) * map->capacity);

    for (int i = 0; i < old_capacity; i++) {
        if (old_keys[i] != EMPTY_SLOT && old_keys[i] != TOMBSTONE) {
            insert_map(map, old_keys[i], old_values[i]);
            free(old_keys[i]);
        }
    }

    free(old_keys);
    free(old_values);
}

void insert_map(unordered_map *map, const char *str, int value) {
    if ((double) map->size / map->capacity >= map->load_factor ||
            (double) map->tombstone_count / map->capacity >= 0.4)
        resize(map);

    int slot = find_slot(map, str, 1);

    if (slot == -1) {
        resize(map);
        slot = find_slot(map, str, 1);
    }

    // if we replace a tombstone, the occupied size doesn't go up
    if (map->keys[slot] == EMPTY_SLOT) {
        map->size++;
    }

    map->keys[slot] = strdup(str);
    map->values[slot] = value;
}

int get_map(unordered_map *map, const char *str) {
    int slot = find_slot(map, str, 0);
    if (slot == -1 || map->keys[slot] == EMPTY_SLOT || map->keys[slot] == TOMBSTONE)
        return -1;
    return map->values[slot];
}

void remove_map(unordered_map *map, const char *str) {
    int slot = find_slot(map, str, 0);
    if (slot == -1 || map->keys[slot] == EMPTY_SLOT || map->keys[slot] == TOMBSTONE)
        return;

    free(map->keys[slot]);
    map->keys[slot] = TOMBSTONE;
    map->tombstone_count++;
    map->values[slot] = 0;
    map->size--;
}


void destroy_map(unordered_map *map) {
    if (!map) return;

    for(int i = 0; i < map->capacity; i++) {
        if (map->keys[i] != EMPTY_SLOT && map->keys[i] != TOMBSTONE) {
            free(map->keys[i]);
        }
    }

    free(map->keys);
    free(map->values);
    free(map);
}