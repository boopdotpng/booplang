#include "ir.h"

struct ir_node {};

static ir_node *create_node() {}

void pretty_print_ir(ir_node *node) {}

ir_node *gen_ir(ast_node *node) {
  ir_node *n = create_node();

  return n;
}