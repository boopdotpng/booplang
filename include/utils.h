#pragma once
#include <stdbool.h>
#include <stdio.h>

// how much of the file to read in at once
// TODO: change this when the tokenizer is working
#define MAX_LINE 256

typedef struct {
  FILE *file;
} file_streamer;

file_streamer *create_streamer(const char *filename);
size_t stream_line(file_streamer *streamer, char *buffer);
void destroy_streamer(file_streamer *streamer);
