#pragma once
#include <stdio.h>

#define BUFFER_SIZE 2048 

typedef struct {
    FILE *file;
    unsigned char buffer[BUFFER_SIZE];
} FileStreamer;

FileStreamer *create_streamer(const char *filename);
size_t stream_ahead(FileStreamer *streamer, unsigned char **output);
void destroy_streamer(FileStreamer *streamer);