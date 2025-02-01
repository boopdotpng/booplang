#include "ast.h"
#include "lexer.h"
#include "vector.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#define TOKENS_PER_LINE 3

typedef struct {
  int emit_ast;
  int emit_tokens;
  char *filename;
} compiler_options;

void print_usage(const char *prog_name) {
  fprintf(stderr, "usage: %s [-a | --emit-ast] [-t | --emit-tokens] <filename>\n", prog_name);
  exit(EXIT_FAILURE);
}

void parse_arguments(int argc, char *argv[], compiler_options *options) {
  int opt;
  struct option long_options[] = {{"emit-ast", no_argument, NULL, 'a'},
                                  {"emit-tokens", no_argument, NULL, 't'},
                                  {NULL, 0, NULL, 0}};

  while ((opt = getopt_long(argc, argv, "at", long_options, NULL)) != -1) {
    switch (opt) {
    case 'a':
      options->emit_ast = 1;
      break;
    case 't':
      options->emit_tokens = 1;
      break;
    default:
      print_usage(argv[0]);
    }
  }

  if (optind >= argc)
    print_usage(argv[0]);
  options->filename = argv[optind];
}

void print_token_stream(lexer_result *l) {
  printf("\n=== token stream ===\n");
  for (size_t i = 0; i < l->tokens->size; ++i) {
    token *t = (token *)get_element(l->tokens, i);
    print_token(t);
  }
}

void print_ast(ast_node *program) {
  printf("=== abstract syntax tree ===\n");
  pretty_print_ast(program, 0);
  printf("\n");
}

int main(int argc, char *argv[]) {
  compiler_options options = {0, 0, NULL};
  parse_arguments(argc, argv, &options);

  lexer_result *l = lex(options.filename);
  if (!l) {
    fprintf(stderr, "error: lexing failed.\n");
    return EXIT_FAILURE;
  }

  if (options.emit_tokens)
    print_token_stream(l);

  ast_node *program = gen_ast(l->tokens);
  if (!program) {
    fprintf(stderr, "error: AST generation failed.\n");
    return EXIT_FAILURE;
  }

  if (options.emit_ast)
    print_ast(program);

  return EXIT_SUCCESS;
}
