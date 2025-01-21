#pragma once

#include "token.h"
#include <stddef.h>

typedef struct intern_table intern_table;

typedef struct {
    char *key;
    token_type value;
} intern_result;

/**
 * Creates a new string intern table.
 *
 * @param capacity Initial capacity of the table (will resize as needed).
 * @param load_factor Load factor threshold for resizing (e.g., 0.75 for 75%).
 * @return A pointer to the newly created intern table.
 */
intern_table *create_intern_table(int capacity, double load_factor);

/**
 * Interns a string, ensuring all instances of the same string share the same memory.
 *
 * @param t Pointer to the intern table.
 * @param start Pointer to the start of the string to intern.
 * @param len Size of the string to intern.
 * @return A struct intern_result that contains {char *key, token_type value}
 */
intern_result intern_string(intern_table *t, const char *start, size_t len, token_type value);

/**
 * Removes an interned string from the table (optional use).
 *
 * @param t Pointer to the intern table.
 * @param str The string to remove from the table.
 */
void remove_interned_string(intern_table *t, const char *str);

/**
 * Destroys the intern table and frees all interned strings.
 *
 * @param t Pointer to the intern table.
 */
void destroy_intern_table(intern_table *t);

/**
 * @brief Gets the value associated with an interned string.
 *
 * @param t Pointer to the intern table.
 * @param str The key to search for.
 * @return token_type
 */
token_type get_interned_value(intern_table *t, const char *str);