#include <stdio.h> 
#include "utils.h"
#include "lexer.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s <filename>\n", argv[0]);
    return 1;
  }

  Lexer *lexer = lex(argv[1]);
  for (int i = 0; i < lexer->token_count;++i) {
    print_token(&lexer->tokens[i]);
  }
  return 0;
}
