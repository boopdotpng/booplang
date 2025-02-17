#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

file_streamer *create_streamer(const char *filename) {
  file_streamer *streamer = malloc(sizeof(file_streamer));

  streamer->file = fopen(filename, "rb");
  if (!streamer->file) {
    perror("failed to open file");
    free(streamer);
    exit(1);
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
    exit(1);
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

int write_file(const char *filename, vector *buffer) {
  FILE *file = fopen(filename, "wb");
  if (!file)
    return -1;
  size_t size = buffer->size;
  size_t written = fwrite(buffer->data, 1, size, file);
  fclose(file);
  return (written == size) ? 0 : -1;
}

int check_architecture() {
#if defined(__aarch64__) || defined(_M_ARM64)
  return 1;
#elif defined(__x86_64__) || defined(_M_X64)
  return -1;
#elif defined(__i386__) || defined(_M_IX86)
  return -1;
  ;
#elif defined(__arm__) || defined(_M_ARM)
  return -1;
#else
  return -1;
#endif
}
