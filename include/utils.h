#pragma once
#include <stdio.h>
#include <stdbool.h>

// how much of the file to read in at once
#define CHUNK_SIZE 2048 

typedef struct
{
    FILE *file;
} FileStreamer;

FileStreamer *create_streamer(const char *filename);
char *stream_chunk(FileStreamer *streamer);
void destroy_streamer(FileStreamer *streamer);