#pragma once
#include <stdio.h>

// max bytes that we will keep in memory at once when reading a file
#define BUFFER_SIZE 2048 

typedef struct {
    FILE *file;
    unsigned char *buffer;
} FileStreamer;

FileStreamer *create_streamer(const char *filename);
size_t getline(FileStreamer *streamer, unsigned char **output);
void destroy_streamer(FileStreamer *streamer);