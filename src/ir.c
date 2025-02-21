#include "ir.h"
#include "utils.h"
#include "vector.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

/* opcode and arg types */
typedef enum {
  OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
  OP_AND, OP_OR, OP_XOR, OP_NOT, OP_SHL, OP_SHR,
  OP_CMP, OP_JMP, OP_BR, OP_RET, OP_CALL,
  OP_LOAD, OP_STORE, OP_PHI, OP_PRINT
} opcode;

typedef enum {
  ARG_NONE, ARG_REG, ARG_IMM, ARG_LABEL, ARG_MEM, ARG_FUNC,
} arg_type;

typedef struct {
  arg_type type;
  union {
    int reg;
    int imm;
    int label;
    void *mem;
    char *func;
  };
} ir_arg;

/* IR structures */
typedef struct {
  char *name;
  vector *blocks;  // vector<block*>
} func;

typedef struct {
  int label;
  vector *instructions; // vector<instruction*>
} block;

typedef struct {
  opcode op;
  vector *args; // vector<ir_arg>
} instruction;

int next_ssa_id = 0;
int next_block_label = 1;

instruction *emit_instruction(opcode op) {
  instruction *inst = malloc(sizeof(instruction));
  inst->op = op;
  inst->args = create_vector(sizeof(ir_arg), 4);
  return inst;
}
instruction *emit_store() { return emit_instruction(OP_STORE); }
instruction *emit_load()  { return emit_instruction(OP_LOAD); }
instruction *emit_phi()   { return emit_instruction(OP_PHI); }
instruction *emit_print() { return emit_instruction(OP_PRINT); }
instruction *emit_call()  { return emit_instruction(OP_CALL); }

void add_phi_arg(instruction *phi, int reg, int blk_label) {
  ir_arg arg_reg = { .type = ARG_REG, .reg = reg };
  ir_arg arg_label = { .type = ARG_LABEL, .label = blk_label };
  add_element(phi->args, &arg_reg);
  add_element(phi->args, &arg_label);
}

block *gen_block() {
  block *b = malloc(sizeof(block));
  b->label = next_block_label++;
  b->instructions = create_vector(sizeof(instruction *), 10);
  return b;
}

/* minimal AST node definition */
typedef struct ast_node {
  int type;
  char *identifier;      // for functions/variables
  double number;         // for numeric literals
  char *string;          // for string literals
  char op;               // for binary/unary op; in a real impl, use proper enums
  struct ast_node *left; // for binary op, return value, etc.
  struct ast_node *right;// for binary op
  struct ast_node *operand; // for unary op
  vector *body;          // function body, if-body, loop body, etc.
  vector *then_body;     // for if
  vector *else_body;     // for if
  struct ast_node *cond; // for if, while
  vector *parameters;    // for function parameters
  vector *arguments;     // for function call arguments
  struct ast_node *initializer; // for for-loop init
  struct ast_node *update;        // for for-loop update
  struct ast_node *end_condition; // for for-loop end condition
  struct ast_node *step;          // for for-loop step
} ast_node;

enum {
  NODE_PROGRAM, NODE_FUNCTION, NODE_ASSIGNMENT, NODE_RETURN,
  NODE_IF, NODE_WHILE, NODE_FOR, NODE_PRINT, NODE_FUNCTION_CALL,
  NODE_BINARY_OP, NODE_UNARY_OP, NODE_IDENTIFIER, NODE_NUMBER,
  NODE_STRING
};

/* expression parsing; returns SSA register holding value */
int parse_expression(ast_node *node, block *current, func *fn) {
  int result_reg;
  switch (node->type) {
    case NODE_NUMBER: {
      result_reg = next_ssa_id++;
      instruction *inst = emit_load();
      ir_arg dest = { .type = ARG_REG, .reg = result_reg };
      ir_arg imm  = { .type = ARG_IMM, .imm = (int)node->number };
      add_element(inst->args, &dest);
      add_element(inst->args, &imm);
      add_element(current->instructions, &inst);
      return result_reg;
    }
    case NODE_STRING: {
      result_reg = next_ssa_id++;
      instruction *inst = emit_load();
      ir_arg dest = { .type = ARG_REG, .reg = result_reg };
      ir_arg mem = { .type = ARG_MEM, .mem = node->string };
      add_element(inst->args, &dest);
      add_element(inst->args, &mem);
      add_element(current->instructions, &inst);
      return result_reg;
    }
    case NODE_IDENTIFIER: {
      // in a full implementation, look up the variable in a symbol table
      result_reg = next_ssa_id++;
      instruction *inst = emit_load();
      ir_arg dest = { .type = ARG_REG, .reg = result_reg };
      ir_arg mem = { .type = ARG_MEM, .mem = node->identifier };
      add_element(inst->args, &dest);
      add_element(inst->args, &mem);
      add_element(current->instructions, &inst);
      return result_reg;
    }
    case NODE_BINARY_OP: {
      int left_reg = parse_expression(node->left, current, fn);
      int right_reg = parse_expression(node->right, current, fn);
      result_reg = next_ssa_id++;
      opcode op;
      switch (node->op) {
        case '+': op = OP_ADD; break;
        case '-': op = OP_SUB; break;
        case '*': op = OP_MUL; break;
        case '/': op = OP_DIV; break;
        // for comparisons (e.g., comp_eq, lt) we use OP_CMP
        case 'e': op = OP_CMP; break;  // assume 'e' means equality
        case 'l': op = OP_CMP; break;  // assume 'l' means less-than
        default:  op = OP_ADD; break;
      }
      instruction *bin = emit_instruction(op);
      ir_arg dest = { .type = ARG_REG, .reg = result_reg };
      ir_arg left  = { .type = ARG_REG, .reg = left_reg };
      ir_arg right = { .type = ARG_REG, .reg = right_reg };
      add_element(bin->args, &dest);
      add_element(bin->args, &left);
      add_element(bin->args, &right);
      add_element(current->instructions, &bin);
      return result_reg;
    }
    case NODE_UNARY_OP: {
      int operand_reg = parse_expression(node->operand, current, fn);
      result_reg = next_ssa_id++;
      opcode op = (node->op == '-') ? OP_SUB : OP_NOT;
      instruction *un = emit_instruction(op);
      ir_arg dest = { .type = ARG_REG, .reg = result_reg };
      ir_arg src  = { .type = ARG_REG, .reg = operand_reg };
      add_element(un->args, &dest);
      add_element(un->args, &src);
      add_element(current->instructions, &un);
      return result_reg;
    }
    case NODE_FUNCTION_CALL: {
      vector *arg_regs = create_vector(sizeof(int), 4);
      for (size_t i = 0; i < node->arguments->size; i++) {
        ast_node *arg = *(ast_node **)get_element(node->arguments, i);
        int reg = parse_expression(arg, current, fn);
        add_element(arg_regs, &reg);
      }
      result_reg = next_ssa_id++;
      instruction *call = emit_call();
      ir_arg dest = { .type = ARG_REG, .reg = result_reg };
      add_element(call->args, &dest);
      ir_arg func_arg = { .type = ARG_FUNC, .func = node->identifier };
      add_element(call->args, &func_arg);
      for (size_t i = 0; i < arg_regs->size; i++) {
        int reg = *(int *)get_element(arg_regs, i);
        ir_arg arg = { .type = ARG_REG, .reg = reg };
        add_element(call->args, &arg);
      }
      add_element(current->instructions, &call);
      free_vector(arg_regs);
      return result_reg;
    }
    default:
      fprintf(stderr, "unsupported expression node type: %d\n", node->type);
      exit(1);
  }
}

/* statement parsing */
void parse_statement(ast_node *node, block **current, func *fn) {
  switch (node->type) {
    case NODE_ASSIGNMENT: {
      int rhs = parse_expression(node->right, *current, fn);
      instruction *store = emit_store();
      ir_arg dest = { .type = ARG_MEM, .mem = node->identifier };
      ir_arg src  = { .type = ARG_REG, .reg = rhs };
      add_element(store->args, &dest);
      add_element(store->args, &src);
      add_element((*current)->instructions, &store);
      break;
    }
    case NODE_RETURN: {
      int ret_reg = -1;
      if (node->left)
        ret_reg = parse_expression(node->left, *current, fn);
      instruction *ret = emit_instruction(OP_RET);
      if (ret_reg != -1) {
        ir_arg arg = { .type = ARG_REG, .reg = ret_reg };
        add_element(ret->args, &arg);
      }
      add_element((*current)->instructions, &ret);
      *current = gen_block();
      add_element(fn->blocks, current);
      break;
    }
    case NODE_IF: {
      int cond_reg = parse_expression(node->cond, *current, fn);
      block *then_blk  = gen_block();
      block *merge_blk = gen_block();
      block *else_blk  = node->else_body ? gen_block() : merge_blk;

      instruction *br = emit_instruction(OP_BR);
      ir_arg cond_arg = { .type = ARG_REG, .reg = cond_reg };
      ir_arg then_arg = { .type = ARG_LABEL, .label = then_blk->label };
      ir_arg else_arg = { .type = ARG_LABEL, .label = else_blk->label };
      add_element(br->args, &cond_arg);
      add_element(br->args, &then_arg);
      add_element(br->args, &else_arg);
      add_element((*current)->instructions, &br);

      // then block
      block *saved = *current;
      *current = then_blk;
      for (size_t i = 0; i < node->body->size; i++) {
        ast_node *child = *(ast_node **)get_element(node->body, i);
        parse_statement(child, current, fn);
      }
      instruction *jmp_then = emit_instruction(OP_JMP);
      ir_arg merge_arg = { .type = ARG_LABEL, .label = merge_blk->label };
      add_element(jmp_then->args, &merge_arg);
      add_element((*current)->instructions, &jmp_then);

      // else block if present
      if (node->else_body) {
        *current = else_blk;
        for (size_t i = 0; i < node->else_body->size; i++) {
          ast_node *child = *(ast_node **)get_element(node->else_body, i);
          parse_statement(child, current, fn);
        }
        instruction *jmp_else = emit_instruction(OP_JMP);
        add_element(jmp_else->args, &merge_arg);
        add_element((*current)->instructions, &jmp_else);
      }

      add_element(fn->blocks, &then_blk);
      if (node->else_body)
        add_element(fn->blocks, &else_blk);
      add_element(fn->blocks, &merge_blk);
      *current = merge_blk;
      break;
    }
    case NODE_WHILE: {
      block *header = gen_block();
      block *body   = gen_block();
      block *merge  = gen_block();

      instruction *jmp_hdr = emit_instruction(OP_JMP);
      ir_arg hdr_arg = { .type = ARG_LABEL, .label = header->label };
      add_element(jmp_hdr->args, &hdr_arg);
      add_element((*current)->instructions, &jmp_hdr);

      *current = header;
      int cond_reg = parse_expression(node->cond, *current, fn);
      instruction *br = emit_instruction(OP_BR);
      ir_arg cond_arg = { .type = ARG_REG, .reg = cond_reg };
      ir_arg body_arg = { .type = ARG_LABEL, .label = body->label };
      ir_arg merge_arg= { .type = ARG_LABEL, .label = merge->label };
      add_element(br->args, &cond_arg);
      add_element(br->args, &body_arg);
      add_element(br->args, &merge_arg);
      add_element((*current)->instructions, &br);

      *current = body;
      for (size_t i = 0; i < node->body->size; i++) {
        ast_node *child = *(ast_node **)get_element(node->body, i);
        parse_statement(child, current, fn);
      }
      instruction *jmp_back = emit_instruction(OP_JMP);
      add_element(jmp_back->args, &hdr_arg);
      add_element((*current)->instructions, &jmp_back);

      add_element(fn->blocks, &header);
      add_element(fn->blocks, &body);
      add_element(fn->blocks, &merge);
      *current = merge;
      break;
    }
    case NODE_FOR: {
      parse_statement(node->initializer, current, fn);
      block *header = gen_block();
      block *body   = gen_block();
      block *merge  = gen_block();

      instruction *jmp_hdr = emit_instruction(OP_JMP);
      ir_arg hdr_arg = { .type = ARG_LABEL, .label = header->label };
      add_element(jmp_hdr->args, &hdr_arg);
      add_element((*current)->instructions, &jmp_hdr);

      *current = header;
      int cond_reg = parse_expression(node->end_condition, *current, fn);
      instruction *br = emit_instruction(OP_BR);
      ir_arg cond_arg = { .type = ARG_REG, .reg = cond_reg };
      ir_arg body_arg = { .type = ARG_LABEL, .label = body->label };
      ir_arg merge_arg= { .type = ARG_LABEL, .label = merge->label };
      add_element(br->args, &cond_arg);
      add_element(br->args, &body_arg);
      add_element(br->args, &merge_arg);
      add_element((*current)->instructions, &br);

      *current = body;
      for (size_t i = 0; i < node->body->size; i++) {
        ast_node *child = *(ast_node **)get_element(node->body, i);
        parse_statement(child, current, fn);
      }
      if (node->step)
        parse_statement(node->step, current, fn);
      instruction *jmp_back = emit_instruction(OP_JMP);
      add_element(jmp_back->args, &hdr_arg);
      add_element((*current)->instructions, &jmp_back);

      add_element(fn->blocks, &header);
      add_element(fn->blocks, &body);
      add_element(fn->blocks, &merge);
      *current = merge;
      break;
    }
    case NODE_PRINT: {
      int expr_reg = parse_expression(node->left, *current, fn);
      instruction *pr = emit_print();
      ir_arg arg = { .type = ARG_REG, .reg = expr_reg };
      add_element(pr->args, &arg);
      add_element((*current)->instructions, &pr);
      break;
    }
    default:
      fprintf(stderr, "unsupported statement node type: %d\n", node->type);
      exit(1);
  }
}

func *parse_function(ast_node *node) {
  func *fn = malloc(sizeof(func));
  fn->name = strdup(node->identifier);
  fn->blocks = create_vector(sizeof(block *), 4);
  next_ssa_id = 0;

  block *entry = gen_block();
  add_element(fn->blocks, &entry);
  for (size_t i = 0; i < node->body->size; i++) {
    ast_node *stmt = *(ast_node **)get_element(node->body, i);
    parse_statement(stmt, &entry, fn);
  }
  return fn;
}

ir_module *create_module(const char *filename) {
  ir_module *m = malloc(sizeof(ir_module));
  m->target = "aarch64-darwin";
  m->endian = LITTLE_END;
  m->source_file = strdup(filename);
  m->functions = create_vector(sizeof(func *), 10);
  return m;
}

static void ir_to_file(const char *filename, ir_module *root) {
  vector *buf = create_vector(1, 1024);
  // serialize metadata and instructions into buf
  write_file("ir.boopir", buf);
  free_vector(buf);
}

ir_module *gen_ir(const char *filename, int write_file, ast_node *program) {
  ir_module *m = create_module(filename);
  // program->body is a vector<ast_node*> containing functions
  for (size_t i = 0; i < program->body->size; i++) {
    ast_node *func_node = *(ast_node **)get_element(program->body, i);
    if (func_node->type != NODE_FUNCTION) {
      fprintf(stderr, "only functions allowed at top level.\n");
      exit(1);
    }
    func *f = parse_function(func_node);
    add_element(m->functions, &f);
  }
  if (write_file)
    ir_to_file("ir.boopir", m);
  return m;
}

/*
  this implementation parses an AST representing functions (like factorial and main),
  handling if, while, for, assignments, returns, prints, and function calls.
  it emits a simple SSA-based IR using basic instructions and control-flow blocks.
  note: variable resolution and full expression/operator handling are oversimplified.
*/
