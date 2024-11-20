#include "utils.h"
#include <stdlib.h>
#include <stdio.h>

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

size_t stream_chunk(FileStreamer *streamer, char *buffer)
{
    if (!streamer || !streamer->file || !buffer) 
        return 0;

    size_t bytes_read = fread(buffer, 1, CHUNK_SIZE, streamer->file);
    if (bytes_read == 0 && ferror(streamer->file))
        perror("error reading file");
    return bytes_read; // returns number of bytes read
}


void destroy_streamer(FileStreamer *streamer)
{
    if (!streamer) return;
    if (streamer->file) fclose(streamer->file);
    free(streamer);
}