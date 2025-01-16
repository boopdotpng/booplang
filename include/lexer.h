#pragma once
#include <stdbool.h>
#include <stdlib.h>

// a file can be indented using spaces or tabs, but it must be consistent
// spaces are just set using a number (2 for two spaces, etc)
#define TABS 1
#define SPACES 2
#define UNSET 0

// max number of nested indents
#define MAX_INDENT_LEVEL 32
#define IDENTIFIER_SIZE 128

// all the token types
typedef enum
{
    // keywords
    DEFINE, FOR, WHILE, IF, ELSE, ELSE_IF, LET,
    BE, IS, GT, LT, LTE, GTE, NE, EQ, RETURN,
    NOT, AND, OR, FALSE, TRUE,

    // literals
    IDENTIFIER, STRING, NUMBER, FLOAT,

    // single characters
    COLON, FSLASH, COMMA, LPAREN, RPAREN,

    // scope
    INDENT, DEDENT, NEWLINE,

    // misc.
    END
} TokenType;

// represents an actual token
typedef struct
{
    TokenType type;
    char *ident; // if its not a token, its probably a variable name or function name
    int col;
    int line;
} Token;

typedef struct
{
    int indent_style; // space or tab
    int spaces_per_level; // indents are relative; the first one
    int token_count;
    int token_capacity;
    int col;
    int line;
    int current_indent; // current indent level
    int indent_stack[MAX_INDENT_LEVEL]; // track changes in indentation
    int indent_sp; // stack pointer for indent stack
    bool multiline_str; // """ starts a multiline string, just line in python. needs to be preserved across lines
    Token *tokens;
} Lexer;

void print_token(Token *token);
Lexer *lex(const char *filename);