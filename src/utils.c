#include "utils.h"
#include <stdlib.h>
#include <stdio.h>

FileStreamer *create_streamer(const char *filename)
{
    FileStreamer *streamer = malloc(sizeof(FileStreamer));
    if (!streamer)
    {
        perror("could not allocate file streamer:");
        return NULL;
    }

    streamer->file = fopen(filename, "rb");
    if (!streamer->file)
    {
        perror("failed to open file");
        free(streamer);
        return NULL;
    }

    return streamer;
}

char *stream_line(FileStreamer *streamer)
{
    if (!streamer || !streamer->file) return 0;
    if (fgets(streamer->buffer, BUFFER_SIZE, streamer->file)) return streamer->buffer;

    if (feof(streamer->file)) return NULL;
    if (ferror(streamer->file)) perror("error reading file");

    return NULL;

}

void destroy_streamer(FileStreamer *streamer)
{
    if (!streamer) return;
    if (streamer->file) fclose(streamer->file);
    free(streamer);
}