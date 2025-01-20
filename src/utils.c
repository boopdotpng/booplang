#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

file_streamer *create_streamer(const char *filename) {
    file_streamer *streamer = malloc(sizeof(file_streamer));

    streamer->file = fopen(filename, "rb");
    if (!streamer->file) {
        perror("failed to open file");
        free(streamer);
        return NULL;
    }

    return streamer;
}

size_t stream_line(file_streamer *streamer, char *buffer) {
    if (!streamer || !streamer->file || !buffer) {
        return 0;
    }

    if (fgets(buffer, MAX_LINE, streamer->file)) {
        return strlen(buffer);
    }

    if (feof(streamer->file)) {
        return 0;
    }

    else if (ferror(streamer->file)) {
        perror("error reading file");
    }

    return 0;
}


void destroy_streamer(file_streamer *streamer) {
    if (!streamer) {
        return;
    }
    if (streamer->file) {
        fclose(streamer->file);
    }
    free(streamer);
}