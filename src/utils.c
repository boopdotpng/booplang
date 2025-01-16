#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

FileStreamer *create_streamer(const char *filename)
{
    FileStreamer *streamer = malloc(sizeof(FileStreamer));

    streamer->file = fopen(filename, "rb");
    if (!streamer->file)
    {
        perror("failed to open file");
        free(streamer);
        return NULL;
    }

    return streamer;
}

size_t stream_line(FileStreamer *streamer, char *buffer)
{
    if (!streamer || !streamer->file || !buffer) return 0;

    if (fgets(buffer, MAX_LINE, streamer->file)) return strlen(buffer);

    if (feof(streamer->file)) return 0;

    else if (ferror(streamer->file)) perror("error reading file");

    return 0;
}


void destroy_streamer(FileStreamer *streamer)
{
    if (!streamer) return;
    if (streamer->file) fclose(streamer->file);
    free(streamer);
}