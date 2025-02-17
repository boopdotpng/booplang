#include "ir.h"
#include "utils.h"
#include "vector.h"

typedef struct {
  char *target;
  enum endianness { LITTLE_END, BIG_END } endian;
  char *source_file;
  // #define version
} file_metadata;

typedef enum {
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_MOD,
  OP_AND,
  OP_OR,
  OP_XOR,
  OP_NOT,
  OP_SHL,
  OP_SHR,
  OP_CMP,
  OP_JMP,
  OP_BR,
  OP_RET,
  OP_CALL,
  OP_LOAD,
  OP_STORE,
  OP_PHI
} opcode;

// a file
typedef struct {
  file_metadata meta;
  vector *functions;
} module;

typedef struct {
  char *name;
  vector *blocks;
} func;

// L1, L2, L3, etc.
typedef struct {
  int label;
  vector *instructions;
} block;

typedef struct {
  opcode op;
  int args[3];
} instruction;

int next_ssa_id = 0;

// emits create instructions
void emit_store() {}

void emit_load() {}

void emit_phi() {}

void emit_loop_phi() {}

// parsing functions
func *parse_function() {
  func *func = malloc(sizeof(func));

  next_ssa_id = 0;
}

block *gen_block() {}

// this creates the root IR node
static module *create_module(const char *filename) {
  module *m = malloc(sizeof(module));
  m->meta.target = "aarch64-darwin";
  m->meta.endian = LITTLE_END;
  m->meta.source_file = filename;
  m->functions = create_vector(sizeof(func *), 10);
  return m;
}

static void ir_to_file(const char *filename, ir_node *root) {
  vector *buf = create_vector(1, 1024);
  // generate metadata

  // generate rest of file

  // write & return
  // TODO: finish string concat
  write_file("ir.boopir", buf);
  free_vector(buf);
}

ir_node *gen_ir(const char *filename, int write_file, ast_node *node) {
  ir_node *n = create_module(filename);

  // traverse AST
  // all top level program children should be functions. no global statements allowed for now
  for (size_t i = 0; i < node->children->size; i++) {
    ast_node *child = *(ast_node **)get_element(node->children, i);
    if (child->type != NODE_FUNCTION) {
      fprintf(stderr, "only functions are allowed at the top level.");
      exit(1);
    }
  }

  // TODO: string concat for different name (filename.boopir)
  if (write_file)
    ir_to_file("ir.boopir", n);
  return n;
}
