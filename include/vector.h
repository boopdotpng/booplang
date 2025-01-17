#pragma once
#include <stdlib.h>

#define INITIAL_SIZE 128

typedef struct {
    void *data;
    size_t size;
    size_t capacity;
    size_t elem_size;
} vector;

vector *create_vector(size_t elem_size);
void add_element(vector *arr, void *element);
void *get_element(vector *arr, size_t index);
void free_vector(vector *arr);