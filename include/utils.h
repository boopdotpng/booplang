#pragma once
#include <stdio.h>
#include <stdbool.h>

// essentially the max line size.
#define BUFFER_SIZE 512 

typedef struct {
    FILE *file;
    int line;
    char buffer[BUFFER_SIZE];
} FileStreamer;

FileStreamer *create_streamer(const char *filename);
char *stream_line(FileStreamer *streamer);
void destroy_streamer(FileStreamer *streamer);