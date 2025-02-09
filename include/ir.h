#pragma once
#include "ast.h"

typedef struct ir_node ir_node;

void pretty_print_ir();
ir_node *gen_ir(ast_node *node);
