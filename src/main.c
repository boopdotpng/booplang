#include <stdio.h> 
#include "utils.h"

int main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(stderr, "usage: %s <filename>\n", argv[0]);
    return 1;
  }
  FileStreamer *streamer = create_streamer(argv[1]);
  char *line;
  while ((line = stream_line(streamer))) {
    printf("%s", line);
  }

  destroy_streamer(streamer);

  return 0;
}
