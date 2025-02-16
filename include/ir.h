#pragma once
#include "ast.h"

typedef struct ir_node ir_node;

ir_node *gen_ir(const char *filename, int write_file, ast_node *node);
