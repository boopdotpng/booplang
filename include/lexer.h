#pragma once
#include <stdbool.h>
#include <stdlib.h>
#include "vector.h"

// a file can be indented using spaces or tabs, but it must be consistent
// spaces are just set using a number (2 for two spaces, etc)
#define TABS 1
#define SPACES 2
#define UNSET 0

// max number of nested indents
#define MAX_INDENT_LEVEL 32
#define IDENTIFIER_SIZE 128

#define MAX_IDENTIFIERS 1024
#define MAX_IDENTIFIER_LEN 128

// all the token types
typedef enum {
    // keywords
    FN, FOR, WHILE, IF, ELSE, ELSE_IF, 
    IS, RETURN, BY, FROM, IMPORT, TO,
    PRINT, MATCH, // since print doesn't require parenthesis

    // operators
    NOT, AND, OR, FALSE, TRUE, MODULU,
    MUL, DIV, INT_DIV, ADD, SUB, ADD_ONE, SUB_ONE, // ++, --
    EQ, COMP_EQ, // ==

    // literals
    IDENTIFIER, STRING, NUMBER, FLOAT,

    // single characters
    COMMA, LPAREN, RPAREN, LSQPAREN, RSQPAREN,

    // scope
    INDENT, DEDENT, NEWLINE,

    // misc.
    END
} token_type;

// represents an actual token
typedef struct {
    token_type type;
    char *ident; // if its not a token, its probably a variable name or function name
    int col;
    int line;
} token;

typedef struct {
    int indent_style; // space or tab
    int spaces_per_level; // indents are relative; the first one
    int col;
    int line;
    int current_indent; // current indent level
    int indent_stack[MAX_INDENT_LEVEL]; // track changes in indentation
    int indent_sp; // stack pointer for indent stack
    bool multiline_str; // """ starts a multiline string, just line in python. needs to be preserved across lines
    vector *tokens;
} lexer;

void print_token(token *token);
lexer *lex(const char *filename);