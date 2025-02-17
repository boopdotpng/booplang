#pragma once
#include "vector.h"
#include <stdbool.h>
#include <stdio.h>

#define MAX_LINE 256
#define BOOPLANG_VERSION "0.0.1"

typedef struct {
  FILE *file;
} file_streamer;

file_streamer *create_streamer(const char *filename);
size_t stream_line(file_streamer *streamer, char *buffer);
void destroy_streamer(file_streamer *streamer);
int write_file(const char *filename, vector *buffer);
int check_architecture();
