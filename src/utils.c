#include "utils.h"
#include <stdlib.h>
#include <stdio.h>

FileStreamer *create_streamer(const char *filename) {
    FileStreamer *streamer = malloc(sizeof(FileStreamer));
    if (!streamer) {
        perror("could not allocate file streamer:");
        return NULL;
    }

    streamer->file = fopen(filename, "rb");
    if (!streamer->file) {
        perror("failed to open file"); 
        free(streamer);
        return NULL;
    }

    return streamer;
}

size_t getline(FileStreamer *streamer, unsigned char **output){
    if (!streamer || !streamer->file) return 0;
    size_t bytes_read = fread(streamer->buffer, 1, BUFFER_SIZE, streamer->file);
    *output = streamer->buffer;
    return bytes_read;
}

void destroy_streamer(FileStreamer *streamer) {
    if (!streamer) return;
    if (streamer->file) fclose(streamer->file);
    free(streamer);
}