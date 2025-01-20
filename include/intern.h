#pragma once

#include <stddef.h>

typedef struct intern_table intern_table;

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
 * @param str The string to intern.
 * @return A pointer to the interned string (owned by the table).
 */
char *intern_string(intern_table *t, const char *str);

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