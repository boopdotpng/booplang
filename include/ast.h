#pragma once
#include "vector.h"
typedef struct ast_node ast_node;
void pretty_print_ast(ast_node *node, int depth);
ast_node *gen_ast(vector *tokens);
