#pragma once
#include "vector.h"
#include "token.h"

typedef struct token token;
typedef struct lexer lexer;
typedef struct intern_table intern_table;

// represents an actual token
struct token {
    token_type type;
    char *ident; // if not a keyword/operator, probably a variable or function name
    int col;
    int line;
};

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

/**
 * @brief Converts a token_type enum to a string for debugging.
 *
 * @param t The token_type enum to print.
 * @return const char*
 */
const char *token_type_str(token_type t);