#pragma once
#include "token.h"
#include "vector.h"

typedef enum {
  NODE_PROGRAM,
  NODE_FUNCTION,
  NODE_IF,
  NODE_WHILE,
  NODE_FOR,
  NODE_ASSIGNMENT,
  NODE_BINARY_OP,
  NODE_UNARY_OP,
  NODE_CALL,
  NODE_RETURN,
  NODE_IDENTIFIER,
  NODE_NUMBER,
  NODE_STRING,
  NODE_PRINT,
} node_type;

typedef struct {
  enum { TYPE_INT, TYPE_FLOAT } num_type;
  double value;
} number_value;

typedef struct ast_node {
  node_type type;
  union {
    number_value number;
    char *string;

    struct {
      struct ast_node *left;
      struct ast_node *right;
      token_type op;
    } binary;

    struct {
      char *var_name;
      struct ast_node *value;
    } assignment;

    struct {
      char *name;
      vector /* ast_node */ *params;
      token_type return_type;
    } function;

    struct {
      struct ast_node *condition;
      struct ast_node *else_body;
      struct ast_node *initializer;
      struct ast_node *step;
    } control;

    struct ast_node *expression;

  } data;

  vector /* ast_node */ *children;
} ast_node;

void pretty_print_ast(ast_node *node, int depth);
ast_node *gen_ast(vector *tokens);
