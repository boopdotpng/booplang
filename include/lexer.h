#pragma once
#include "vector.h"
#include "token.h"

typedef struct token token;
typedef struct lexer lexer;
typedef struct intern_table intern_table;


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