#pragma once
#include <stdio.h>
#include <stdbool.h>

// how much of the file to read in at once
// TODO: change this when the tokenizer is working
#define MAX_LINE 256

typedef struct
{
    FILE *file;
} FileStreamer;

FileStreamer *create_streamer(const char *filename);
size_t stream_line(FileStreamer *streamer, char *buffer);
void destroy_streamer(FileStreamer *streamer);