#pragma once
#include <stdlib.h>

typedef struct {
    void *data;
    size_t size;
    size_t capacity;
    size_t elem_size;
} vector;

/**
 * @brief Creates a new dynamically sized vector.
 *
 * @param elem_size Size of each element in the vector (in bytes).
 * @param initial_size Initial number of elements to allocate space for.
 * @return vector* Pointer to the newly created vector.
 */
vector *create_vector(size_t elem_size, int initial_size);

/**
 * @brief Appends an element to the end of the vector.
 *
 * @param arr Pointer to the vector.
 * @param element Pointer to the element to be added (copied into the vector).
 */
void add_element(vector *arr, void *element);

/**
 * @brief Retrieves an element from the vector at a given index.
 *
 * @param arr Pointer to the vector.
 * @param index Index of the element to retrieve (0-based).
 * @return void* Pointer to the element at the given index.
 */
void *get_element(vector *arr, size_t index);

/**
 * @brief Frees the memory used by the vector, including its internal buffer.
 *
 * @param arr Pointer to the vector to be freed.
 */
void free_vector(vector *arr);