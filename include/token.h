// had to be separated to avoid circular dependencies
#pragma once

typedef enum {
  // keywords
  FN,
  FOR,
  WHILE,
  IF,
  ELSE,
  ELSE_IF,
  RETURN,
  BY,
  FROM,
  IMPORT,
  TO,
  PRINT,
  MATCH,
  FALSE,
  TRUE,  // since print doesn't require parenthesis

  // operators
  NOT,
  AND,
  OR,
  MODULO,
  MUL,
  DIV,
  INT_DIV,
  ADD,
  SUB,
  ADD_ONE,
  SUB_ONE,  // ++, --
  EQ,
  COMP_EQ,
  ADD_EQ,
  SUB_EQ,
  MUL_EQ,
  DIV_EQ,
  INTDIV_EQ,
  NOT_EQ,
  GT,
  LT,
  GTE,
  LTE,
  CARROT,
  CARROT_EQ,
  BITW_AND,
  BITW_OR,
  BITW_NOT,
  LBITSHIFT,
  RBITSHIFT,

  // literals
  IDENTIFIER,
  STRING,
  INTEGER,
  FLOAT,
  MULTILINE_STR,

  // single characters
  COMMA,
  LPAREN,
  RPAREN,
  LSQPAREN,
  RSQPAREN,

  // scope
  INDENT,
  DEDENT,
  NEWLINE,

  // misc.
  END
} token_type;
