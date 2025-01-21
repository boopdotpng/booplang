#pragma once
#include "vector.h"
#include "intern.h"

typedef struct token token;
typedef struct lexer lexer;
typedef struct intern_table intern_table;

// all the token types
typedef enum {
    // keywords
    FN, FOR, WHILE, IF, ELSE, ELSE_IF,
    IS, RETURN, BY, FROM, IMPORT, TO,
    PRINT, MATCH, // since print doesn't require parenthesis

    // operators
    NOT, AND, OR, FALSE, TRUE, MODULU,
    MUL, DIV, INT_DIV, ADD, SUB, ADD_ONE, SUB_ONE, // ++, --
    EQ, COMP_EQ, ADD_EQ, SUB_EQ, MUL_EQ, DIV_EQ, INTDIV_EQ, // ==
    GT, LT, GTE, LTE, CARROT, CARROT_EQ,

    // literals
    IDENTIFIER, STRING, INTEGER, FLOAT, MULTILINE_STR,

    // single characters
    COMMA, LPAREN, RPAREN, LSQPAREN, RSQPAREN,

    // scope
    INDENT, DEDENT, NEWLINE,

    // misc.
    END
} token_type;

typedef struct {
    vector *tokens;
    intern_table *interns;
} lexer_result;

/**
 * @brief Prints a token for debugging purposes.
 *
 * @param token Pointer to the token to be printed.
 */
void print_token(const token *token);

/**
 * @brief Lexes a file into tokens.
 *
 * @param filename Pointer to the filename to lex.
 * @return vector* Vector of tokens.
 */
lexer_result *lex(const char *filename);