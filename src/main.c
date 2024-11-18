#include <stdio.h> 
#include "utils.h"

int main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(stderr, "Usage: boop <filename>\n");
    return 1;
  }

  char *buffer = read_file_to_buffer(argv[1]);


  printf("%s\n", buffer);

  return 0;
}
