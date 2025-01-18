#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unordered_map.h"

// intentionally all-lowercase except macros we need to keep
#define empty_slot 0
#define tombstone  ((char*)-1)

// djb2
static unsigned long hash1(const char *str) {
    unsigned long h = 5381;
    int c;
    while ((c = *str++)) {
        h = ((h << 5) + h) + c;
    }
    return h;
}

// a second hash to reduce collisions
static unsigned long hash2(const char *str, int capacity) {
    unsigned long h = 0;
    int c;
    while ((c = *str++)) {
        h = (h * 31 + c) % (capacity - 1);
    }
    return (h | 1);
}

static int find_slot(unordered_map *map, const char *key, int for_insert) {
    unsigned long h1v = hash1(key) % map->capacity;
    unsigned long h2v = hash2(key, map->capacity);
    int slot = h1v;
    int first_tombstone = -1;

    for (int i = 0; i < map->capacity; i++) {
        if (map->keys[slot] == empty_slot) {
            if (for_insert && first_tombstone != -1) return first_tombstone;
            return slot;
        } else if (map->keys[slot] == tombstone) {
            if (for_insert && first_tombstone == -1) {
                first_tombstone = slot;
            }
        } else {
            if (strcmp(map->keys[slot], key) == 0) {
                return slot;
            }
        }
        slot = (slot + h2v) % map->capacity;
    }
    return -1;
}

unordered_map *create_map(
    int capacity,
    double load_factor,
    int owns_keys,
    int owns_vals,
    free_fn key_destructor,
    free_fn val_destructor
) {
    unordered_map *m = calloc(1, sizeof(*m));
    m->capacity = capacity;
    m->load_factor = load_factor;
    m->owns_keys = owns_keys;
    m->owns_vals = owns_vals;
    m->key_destructor = key_destructor;
    m->val_destructor = val_destructor;
    m->keys = calloc(capacity, sizeof(char*));
    m->values = calloc(capacity, sizeof(void*));
    return m;
}

// inserts or updates a key->value
void insert_map(unordered_map *map, const char *key, void *value) {
    double ratio = (double) map->size / map->capacity;
    double tomb_ratio = (double) map->tombstone_count / map->capacity;

    if (ratio >= map->load_factor || tomb_ratio >= 0.4)
        resize(map);

    int slot = find_slot(map, key, 1);
    if (slot == -1) {
        resize(map);
        slot = find_slot(map, key, 1);
        if (slot == -1) {
            fprintf(stderr, "map is still full somehow?");
            return;
        }
    }
    if (map->keys[slot] == empty_slot) {
        map->size++;
    } else if (map->keys[slot] == tombstone) {
        map->tombstone_count--;
    } else {
        // existing key, free the old key if owned
        if (map->owns_keys && map->key_destructor) {
            map->key_destructor(map->keys[slot]);
        }
        // free old value if owned
        if (map->owns_vals && map->val_destructor) {
            map->val_destructor(map->values[slot]);
        }
    }

    // if we own the key, we copy it
    if (map->owns_keys) {
        char *new_key = strdup(key);
        map->keys[slot] = new_key;
    } else {
        map->keys[slot] = (char*) key;
    }

    map->values[slot] = value;
}

void *get_map(unordered_map *map, const char *key) {
    int slot = find_slot(map, key, 0);
    if (slot == -1) return NULL;
    if (map->keys[slot] == empty_slot || map->keys[slot] == tombstone) {
        return NULL;
    }
    return map->values[slot];
}

void remove_map(unordered_map *map, const char *key) {
    int slot = find_slot(map, key, 0);
    if (slot == -1) return;
    if (map->keys[slot] == empty_slot || map->keys[slot] == tombstone) return;

    // free key if we own it
    if (map->owns_keys && map->key_destructor) {
        map->key_destructor(map->keys[slot]);
    }
    // free value if we own it
    if (map->owns_vals && map->val_destructor) {
        map->val_destructor(map->values[slot]);
    }
    map->keys[slot] = tombstone;
    map->values[slot] = NULL;
    map->size--;
    map->tombstone_count++;
}

static void resize(unordered_map *map) {
    int old_capacity = map->capacity;
    char **old_keys = map->keys;
    void **old_vals = map->values;

    map->capacity *= 2;
    map->size = 0;
    map->tombstone_count = 0;
    map->keys = calloc(map->capacity, sizeof(char*));
    map->values = calloc(map->capacity, sizeof(void*));

    for (int i = 0; i < old_capacity; i++) {
        if (old_keys[i] && old_keys[i] != tombstone) {
            // we do not free them here bc we are re-inserting them
            insert_map(map, old_keys[i], old_vals[i]);
            // but if the map owns keys, we had duplicated them in insert_map
            // so we need to free the old one
            if (map->owns_keys && map->key_destructor) {
                map->key_destructor(old_keys[i]);
            }
        }
    }
    free(old_keys);
    free(old_vals);
}

// frees the entire map, optionally freeing all keys and values
void destroy_map(unordered_map *map) {
    if (!map) return;
    for (int i = 0; i < map->capacity; i++) {
        if (map->keys[i] != empty_slot && map->keys[i] != tombstone) {
            if (map->owns_keys && map->key_destructor) {
                map->key_destructor(map->keys[i]);
            }
            if (map->owns_vals && map->val_destructor) {
                map->val_destructor(map->values[i]);
            }
        }
    }
    free(map->keys);
    free(map->values);
    free(map);
}

// string interning:
// we want both key and value to be the same pointer (the deduplicated string)
// so the map owns everything
char *intern_string(unordered_map *map, const char *str) {
    // get existing
    char *found = get_map(map, str);
    if (found) return found;

    // if not found, we create
    char *new_copy = strdup(str);
    insert_map(map, new_copy, new_copy);
    return new_copy;
}