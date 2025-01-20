#pragma once
#include "vector.h"

typedef struct token token;

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
vector *lex(const char *filename);