#pragma once
#include "vector.h"

typedef struct ast_node ast_node;

/**
 * @brief Prints a pretty representation of the program's AST.
 *
 * @param node The root node to print.
 * @param depth How many levels to show.
 */
void pretty_print_ast(ast_node *node, int depth);

/**
 * @brief Generate an AST for a program.
 *
 * @param tokens Pointer to the start of an array of tokens.
 * @return ast_node*
 */
ast_node *gen_ast(vector *tokens);