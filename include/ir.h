#pragma once
#include "ast.h"

typedef struct file_metadata file_metadata;

typedef struct {
  const char *target;
  enum endianness { LITTLE_END, BIG_END } endian;
  const char *source_file;
  vector *functions;
} ir_module;

ir_module *gen_ir(const char *filename, int write_file, ast_node *node);
