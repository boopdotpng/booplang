#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "intern.h"

#define EMPTY_SLOT 0
#define TOMBSTONE  ((char*)-1)

struct intern_table {
    int capacity;
    double load_factor;
    int size;
    int tombstone_count;
    char **keys;
    token_type *values;
};

// djb2
static unsigned long hash1(const char *str) {
    unsigned long h = 5381;
    int c;
    while ((c = *str++)) {
        h = ((h << 5) + h) + c;
    }
    return h;
}

// a second hash for double hashing
static unsigned long hash2(const char *str, int capacity) {
    unsigned long h = 0;
    int c;
    while ((c = *str++)) {
        h = (h * 31 + c) % (capacity - 1);
    }
    // ensure it's never zero, so we actually move
    // and we want the value to be odd
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
            // if we found a tomb earlier, use that for insertion
            if (for_insert && first_tomb != -1) return first_tomb;
            return slot;
        } else if (k == TOMBSTONE) {
            if (for_insert && first_tomb == -1) first_tomb = slot;
        } else if (strcmp(k, str) == 0) {
            return slot;
        }
        slot = (slot + h2v) % t->capacity;
    }
    return -1;
}

static void resize(intern_table *t);

intern_table *create_intern_table(int capacity, double load_factor) {
    intern_table *tbl = calloc(1, sizeof(*tbl));
    tbl->capacity = capacity;
    tbl->load_factor = load_factor;
    tbl->size = 0;
    tbl->tombstone_count = 0;
    tbl->keys = calloc(capacity, sizeof(char*));
    tbl->values = calloc(capacity, sizeof(token_type));
    return tbl;
}

intern_result intern_string(intern_table *t, const char *start, size_t len, token_type value) {
    // create a null-terminated string from the given start and length
    char temp[len + 1];
    memcpy(temp, start, len);
    temp[len] = '\0';

    // first check if it exists
    int slot = find_slot(t, temp, 0);
    if (slot != -1 && t->keys[slot] != EMPTY_SLOT && t->keys[slot] != TOMBSTONE) {
        return (intern_result) {
            t->keys[slot], t->values[slot]
        };
    }

    // not found; maybe we need to resize
    double ratio = (double)t->size / t->capacity;
    double tomb_ratio = (double)t->tombstone_count / t->capacity;
    if (ratio >= t->load_factor || tomb_ratio >= 0.4) {
        resize(t);
    }

    // find an insertion slot after potential resize
    slot = find_slot(t, temp, 1);
    if (slot == -1) {
        fprintf(stderr, "couldn't find a slot after resize, unexpected\n");
        return (intern_result) {
            NULL, 0
        };
    }

    if (t->keys[slot] == EMPTY_SLOT) {
        t->size++;
    } else if (t->keys[slot] == TOMBSTONE) {
        t->tombstone_count--;
    }

    // allocate memory and store the interned string
    char *dup = strndup(temp, len); // safer version of strdup for substrings
    t->keys[slot] = dup;
    t->values[slot] = value;
    return (intern_result) {
        dup, value
    };
}

// destroys all interned strings
void destroy_intern_table(intern_table *t) {
    if (!t) return;
    for (int i = 0; i < t->capacity; i++) {
        char *k = t->keys[i];
        if (k && k != EMPTY_SLOT && k != TOMBSTONE) {
            free(k);
        }
    }
    free(t->keys);
    free(t->values);
    free(t);
}

// optional: if you want to remove an interned string
// probably will never be used, can't hurt to have
void remove_interned_string(intern_table *t, const char *str) {
    int slot = find_slot(t, str, 0);
    if (slot == -1) return;
    if (t->keys[slot] == EMPTY_SLOT || t->keys[slot] == TOMBSTONE) return;

    // free it, mark TOMBSTONE
    free(t->keys[slot]);
    t->keys[slot] = TOMBSTONE;
    t->values[slot] = -1;
    t->size--;
    t->tombstone_count++;
}

static void resize(intern_table *t) {
    int oldcap = t->capacity;
    char **oldkeys = t->keys;
    token_type *oldvalues = t->values;

    t->capacity *= 2;
    t->size = 0;
    t->tombstone_count = 0;
    t->keys = calloc(t->capacity, sizeof(char*));
    t->values = calloc(t->capacity, sizeof(token_type));

    if (!t->keys || !t->values) {
        // revert
        t->keys = oldkeys;
        t->values = oldvalues;
        t->capacity = oldcap;
        fprintf(stderr, "failed to resize intern table\n");
        return;
    }

    // re-insert
    for (int i = 0; i < oldcap; i++) {
        char *k = oldkeys[i];
        if (k && k != EMPTY_SLOT && k != TOMBSTONE) {
            // re-intern the old pointer; no new strdup
            // bc the pointer is already ours
            int slot = find_slot(t, k, 1);
            if (slot == -1) {
                fprintf(stderr, "map is full during resize reinsert?\n");
                continue;
            }
            if (t->keys[slot] == EMPTY_SLOT) {
                t->size++;
            }
            t->keys[slot] = k;
            t->values[slot] = oldvalues[i];
        }
    }
    free(oldkeys);
    free(oldvalues);
}