#pragma once
#include <stdio.h>
#include <stdbool.h>

// how much of the file to read in at once
// TODO: change this when the tokenizer is working
#define CHUNK_SIZE 8

typedef struct
{
    FILE *file;
} FileStreamer;

FileStreamer *create_streamer(const char *filename);
size_t stream_chunk(FileStreamer *streamer, char *buffer);
void destroy_streamer(FileStreamer *streamer);