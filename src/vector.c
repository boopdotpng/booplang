#include "vector.h"
#include <stdio.h>
#include <string.h>



vector *create_vector(size_t elem_size, int initial_size) {
    vector *arr = malloc(sizeof(vector));
    arr->data = malloc(elem_size * initial_size);
    arr->size = 0;
    arr->capacity = elem_size * initial_size;
    arr->elem_size = elem_size;

    return arr;
}

static void resize_array(vector *arr) {
    arr->capacity *= 2;
    arr->data = realloc(arr->data, arr->capacity * arr->elem_size);
    if (!arr->data) {
        fprintf(stderr, "failed to resize array\n");
        exit(EXIT_FAILURE);
    }
}

void add_element(vector *arr, void *element) {
    if (arr->size >= arr->capacity) {
        resize_array(arr);
    }

    memcpy((char *)arr->data + (arr->size * arr->elem_size), element, arr->elem_size);
    arr->size++;
}

void *get_element(vector *arr, size_t index) {
    if (index >= arr->size) {
        return NULL;
    }
    return (char *)arr->data + (index * arr->elem_size);
}

void free_vector(vector *arr) {
    free(arr->data);
    free(arr);
}