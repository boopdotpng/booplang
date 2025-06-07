#pragma once
#include "token.h"
#include "vector.h"

typedef struct token token;
typedef struct lexer lexer;
typedef struct intern_table intern_table;

struct token {
  token_type type;
  char *ident;
  int col;
  int line;
};

typedef struct {
  vector *tokens;
  intern_table *interns;
} lexer_result;

void print_token(const token *token);
lexer_result *lex(const char *filename);
const char *token_type_str(token_type t);
