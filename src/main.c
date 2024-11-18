#include <stdio.h> 
#include "utils.h"

int main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(stderr, "usage: %s <filename>\n", argv[0]);
    return 1;
  }
  FileStreamer *streamer = create_streamer(argv[1]);
  unsigned char *data;
  size_t bytes_read;
  while ((bytes_read = stream_ahead(streamer, &data)) > 0) {
      fwrite(data, 1, bytes_read, stdout);
      putchar('\n');
      putchar('\n');
  }

  destroy_streamer(streamer);

  return 0;
}
