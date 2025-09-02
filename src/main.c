#include "ast.h"
#include "ir.h"
#include "lexer.h"
#include "utils.h"
#include "vector.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int emit_ast;
  int emit_tokens;
  int save_ir;
  char *filename;
} compiler_options;

void print_usage(const char *prog_name) {
  fprintf(stderr,
          "usage: %s version %s [options] <filename>\n\n"
          "options:\n"
          "  -a, --emit-ast     output the abstract syntax tree\n"
          "  -t, --emit-tokens  output the token stream\n"
          "  -s, --save-ir      save the intermediate representation\n\n"
          "example:\n"
          "  %s -a source.boop  emit the AST of source.boop\n",
          prog_name, BOOPLANG_VERSION, prog_name);
  exit(EXIT_FAILURE);
}

void print_token_stream(lexer_result *l) {
  if (!l || !l->tokens) return;

  printf("\n=== token stream ===\n");
  for (size_t i = 0; i < l->tokens->size; ++i)
    print_token((token *)get_element(l->tokens, i));
}

void parse_arguments(int argc, char *argv[], compiler_options *options) {
  struct option long_options[] = {{"emit-ast", no_argument, NULL, 'a'},
                                  {"emit-tokens", no_argument, NULL, 't'},
                                  {"save-ir", no_argument, NULL, 's'},
                                  {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "ats", long_options, NULL)) != -1) {
    switch (opt) {
    case 'a': options->emit_ast = 1; break;
    case 't': options->emit_tokens = 1; break;
    case 's': options->save_ir = 1; break;
    default: print_usage(argv[0]);
    }
  }

  if (optind >= argc) print_usage(argv[0]);

  options->filename = argv[optind];
}

int main(int argc, char *argv[]) {
  compiler_options options = {0};
  parse_arguments(argc, argv, &options);

  lexer_result *l = lex(options.filename);
  if (!l) {
    fprintf(stderr, "error: lexing failed.\n");
    return EXIT_FAILURE;
  }

  if (options.emit_tokens) print_token_stream(l);

  ast_node *program = gen_ast(l->tokens);
  if (options.emit_ast) pretty_print_ast(program, 0);

  // use LLVM-IR temporarily
  // this will save an executable directly unless save ir is enabled
  gen_ir(options.filename, options.save_ir, program);

  // // check architecture before lowering
  // if (check_architecture() == -1) {
  //   fprintf(stderr, "aarch64-darwin is currently the only supported architecture.");
  //   exit(1);
  // }

  return EXIT_SUCCESS;
}
