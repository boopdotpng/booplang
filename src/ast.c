#include "ast.h"
#include "lexer.h"
#include "token.h"
#include "vector.h"
#include <stdio.h>
#include <string.h>

typedef struct {
  vector *tokens;
  int current;
  int has_main;
  vector /* scope_table */ scopes;
  int in_func;
  int line;
  int col;
  int error_count;
} parser_state;

static int is_unary_op(token *t);
static int is_binary_op(token *t);
static void throw_error(parser_state *state, const char *msg);
static void print_indent(int depth);
static token *next(parser_state *state);
static token *peek(parser_state *state, int ahead);
static token *expect(parser_state *state, token_type type);
static ast_node *create_node(node_type type);
static ast_node *parse_function(parser_state *state);
static ast_node *parse_if(parser_state *state);
static ast_node *parse_while(parser_state *state);
static ast_node *parse_for(parser_state *state);
static ast_node *parse_assignment(parser_state *state);
static ast_node *parse_print(parser_state *state);
static ast_node *parse_expression(parser_state *state);
static ast_node *parse_binary_expression(parser_state *state, int min_precedence);
static ast_node *parse_statement(parser_state *state);
static void parse_block(parser_state *state, vector *children);
static int precedence(token_type op);

static int is_unary_op(token *t) {
  switch (t->type) {
  case SUB_ONE:
  case ADD_ONE:
  case NOT:
  case SUB:
  case BITW_NOT:
    return 1;
  default:
    return 0;
  }
}

static int is_binary_op(token *t) {
  switch (t->type) {
  case ADD:
  case SUB:
  case MUL:
  case DIV:
  case MODULO:
  case AND:
  case OR:
  case COMP_EQ:
  case NOT_EQ:
  case GT:
  case GTE:
  case LT:
  case LTE:
  case CARROT:
  case BITW_AND:
  case BITW_OR:
    return 1;
  default:
    return 0;
  }
}

static void throw_error(parser_state *state, const char *msg) {
  fprintf(stderr, "%s at line %d:%d (%s) \n", msg, state->line, state->col,
          token_type_str(peek(state, 0)->type));
  if (++state->error_count > 10) {
    fprintf(stderr, "too many errors, aborting. \n");
    exit(1);
  }
}

static void print_indent(int depth) {
  for (int i = 0; i < depth; i++) {
    printf("  ");
  }
}

void pretty_print_ast(ast_node *node, int depth) {
  if (!node)
    return;

  print_indent(depth);

  switch (node->type) {
  case NODE_PROGRAM:
    printf("program\n");
    break;

  case NODE_FUNCTION:
    printf("function: %s\n", node->data.function.name);
    print_indent(depth + 1);
    printf("parameters (%zu):\n", node->data.function.params->size);
    for (size_t i = 0; i < node->data.function.params->size; i++) {
      ast_node *param = *(ast_node **)get_element(node->data.function.params, i);
      print_indent(depth + 2);
      if (param && param->data.string) {
        printf("%s\n", param->data.string);
      }
    }
    break;

  case NODE_IF:
    printf("if\n");
    print_indent(depth + 1);
    printf("condition:\n");
    pretty_print_ast(node->data.control.condition, depth + 2);
    if (node->data.control.else_body) {
      print_indent(depth + 1);
      printf("else:\n");
      pretty_print_ast(node->data.control.else_body, depth + 2);
    }
    break;

  case NODE_WHILE:
    printf("while\n");
    print_indent(depth + 1);
    printf("condition:\n");
    pretty_print_ast(node->data.control.condition, depth + 2);
    break;

  case NODE_FOR:
    printf("for\n");
    if (node->data.control.initializer) {
      print_indent(depth + 1);
      printf("initializer:\n");
      pretty_print_ast(node->data.control.initializer, depth + 2);
    }
    if (node->data.control.condition) {
      print_indent(depth + 1);
      printf("end condition:\n");
      pretty_print_ast(node->data.control.condition, depth + 2);
    }
    if (node->data.control.step) {
      print_indent(depth + 1);
      printf("step:\n");
      pretty_print_ast(node->data.control.step, depth + 2);
    }
    break;

  case NODE_ASSIGNMENT:
    printf("assignment: %s =\n", node->data.assignment.var_name);
    print_indent(depth + 1);
    printf("value:\n");
    pretty_print_ast(node->data.assignment.value, depth + 2);
    break;

  case NODE_BINARY_OP:
    printf("binary operation: %s\n", token_type_str(node->data.binary.op));
    print_indent(depth + 1);
    printf("left:\n");
    pretty_print_ast(node->data.binary.left, depth + 2);
    print_indent(depth + 1);
    printf("right:\n");
    pretty_print_ast(node->data.binary.right, depth + 2);
    break;

  case NODE_UNARY_OP:
    printf("unary operation: %s\n", token_type_str(node->data.binary.op));
    print_indent(depth + 1);
    printf("operand:\n");
    pretty_print_ast(node->data.binary.left, depth + 2);
    break;

  case NODE_CALL:
    printf("function call: %s\n", node->data.string);
    if (node->children->size > 0) {
      print_indent(depth + 1);
      printf("arguments:\n");
      for (size_t i = 0; i < node->children->size; i++) {
        ast_node *arg = *(ast_node **)get_element(node->children, i);
        pretty_print_ast(arg, depth + 2);
      }
    }
    break;

  case NODE_RETURN:
    printf("return\n");
    if (node->data.expression) {
      print_indent(depth + 1);
      printf("value:\n");
      pretty_print_ast(node->data.expression, depth + 2);
    }
    break;

  case NODE_IDENTIFIER:
    printf("identifier: %s\n", node->data.string ? node->data.string : "(null)");
    break;

  case NODE_NUMBER:
    if (node->data.number.num_type == TYPE_INT) {
      printf("number: %ld\n", (long)node->data.number.value);
    } else {
      printf("number: %f\n", node->data.number.value);
    }
    break;

  case NODE_STRING:
    printf("string: \"%s\"\n", node->data.string ? node->data.string : "(null)");
    break;

  case NODE_PRINT:
    printf("print\n");
    print_indent(depth + 1);
    printf("expression:\n");
    pretty_print_ast(node->data.expression, depth + 2);
    break;

  default:
    printf("unknown node type: %d\n", node->type);
    break;
  }

  if (node->children && node->children->size > 0) {
    print_indent(depth + 1);
    printf("body:\n");
    for (size_t i = 0; i < node->children->size; i++) {
      ast_node *child = *(ast_node **)get_element(node->children, i);
      pretty_print_ast(child, depth + 2);
    }
  }
}

static token *next(parser_state *state) {
  if ((size_t)state->current + 1 >= state->tokens->size)
    return NULL;
  token *n = get_element(state->tokens, ++state->current);
  state->col = n->col;
  state->line = n->line;
  return n;
}

static token *peek(parser_state *state, int ahead) {
  if ((size_t)state->current + ahead >= state->tokens->size) {
    return NULL;
  }

  return get_element(state->tokens, state->current + ahead);
}

static token *expect(parser_state *state, token_type type) {
  token *t = peek(state, 0);
  if (!t || t->type != type) {
    throw_error(state, "unexpected token type");
    return NULL;
  }

  state->current++;
  state->line = t->line;
  state->col = t->col;
  return t;
}

static ast_node *create_node(node_type type) {
  ast_node *node = calloc(1, sizeof(ast_node));
  node->type = type;
  node->children = create_vector(sizeof(ast_node *), 8);
  return node;
}

static ast_node *parse_return(parser_state *state) {
  ast_node *node = create_node(NODE_RETURN);
  node->data.expression = parse_expression(state);
  return node;
}

static ast_node *parse_function(parser_state *state) {
  if (state->in_func) {
    throw_error(state, "nested functions are not allowed.");
    return NULL;
  }
  state->in_func = 1;

  token *t = peek(state, 0);
  if (!t || t->type != IDENTIFIER) {
    throw_error(state, "function name must be identifier");
    return NULL;
  }
  ast_node *func = create_node(NODE_FUNCTION);
  func->data.function.name = t->ident;
  if (strcmp(t->ident, "main") == 0)
    state->has_main = 1;
  t = next(state);

  if (!(t = expect(state, LPAREN)))
    return NULL;

  func->data.function.params = create_vector(sizeof(ast_node *), 1);
  int expect_comma = 0;

  t = peek(state, 0);
  while (t && t->type != RPAREN) {
    if (t->type == IDENTIFIER) {
      ast_node *param = create_node(NODE_IDENTIFIER);
      param->data.string = t->ident;
      add_element(func->data.function.params, &param);
      expect_comma = 1;
      next(state);
    } else if (t->type == COMMA && expect_comma) {
      next(state);
      expect_comma = 0;
    } else {
      throw_error(state, "unexpected token in function parameter list.");
      return NULL;
    }
    t = peek(state, 0);
  }

  if (!expect(state, RPAREN))
    return NULL;

  parse_block(state, func->children);

  state->in_func = 0;
  return func;
}

static ast_node *parse_if(parser_state *state) {
  ast_node *node = create_node(NODE_IF);

  node->data.control.condition = parse_expression(state);
  if (!node->data.control.condition) {
    throw_error(state, "invalid condition in if statement");
    return NULL;
  }

  parse_block(state, node->children);

  ast_node *last_branch = node;
  while (peek(state, 0) && (peek(state, 0)->type == ELSE_IF || peek(state, 0)->type == ELSE)) {
    token *t = next(state);
    ast_node *branch = create_node(NODE_IF);

    if (t->type == ELSE_IF) {
      branch->data.control.condition = parse_expression(state);
      if (!branch->data.control.condition) {
        throw_error(state, "invalid condition in elif statement");
        return NULL;
      }
    }
    parse_block(state, branch->children);

    last_branch->data.control.else_body = branch;
    last_branch = branch;
  }
  return node;
}

static ast_node *parse_while(parser_state *state) {
  ast_node *w = create_node(NODE_WHILE);
  w->data.control.condition = parse_expression(state);
  parse_block(state, w->children);
  return w;
}

static ast_node *parse_for(parser_state *state) {
  ast_node *for_node = create_node(NODE_FOR);
  token *t = peek(state, 0);

  if (!t || t->type != IDENTIFIER) {
    throw_error(state, "expected iterator variable in for loop");
    return NULL;
  }

  for_node->data.control.initializer = create_node(NODE_ASSIGNMENT);
  for_node->data.control.initializer->data.assignment.var_name = t->ident;
  next(state);

  if (!expect(state, FROM))
    return NULL;

  ast_node *start_expr = parse_expression(state);
  if (!start_expr) {
    throw_error(state, "invalid start value in for loop");
    return NULL;
  }
  for_node->data.control.initializer->data.assignment.value = start_expr;

  if (!expect(state, TO))
    return NULL;

  ast_node *end_expr = parse_expression(state);
  if (!end_expr) {
    throw_error(state, "invalid end value in for loop");
    return NULL;
  }

  ast_node *step_expr = NULL;
  t = peek(state, 0);
  if (t && t->type == BY) {
    next(state);
    step_expr = parse_expression(state);
    if (!step_expr) {
      throw_error(state, "invalid step expression in for loop");
      return NULL;
    }
  } else {
    if (!(start_expr->type == NODE_NUMBER && end_expr->type == NODE_NUMBER)) {
      throw_error(state, "missing 'by' clause in for loop with non-numeric boundaries");
      return NULL;
    }
    step_expr = create_node(NODE_NUMBER);
    step_expr->data.number.num_type = t->type == INTEGER ? TYPE_INT : TYPE_FLOAT;
    step_expr->data.number.value = 1.0;
  }

  for_node->data.control.condition = end_expr;
  for_node->data.control.step = step_expr;

  parse_block(state, for_node->children);
  return for_node;
}

static ast_node *parse_assignment(parser_state *state) {
  token *t = peek(state, 0);
  if (!t || t->type != IDENTIFIER) {
    throw_error(state, "expected variable name in assignment");
    return NULL;
  }
  ast_node *node = create_node(NODE_ASSIGNMENT);
  node->data.assignment.var_name = t->ident;
  next(state);

  if (!expect(state, EQ))
    return NULL;

  node->data.assignment.value = parse_expression(state);
  if (!node->data.assignment.value) {
    throw_error(state, "invalid expression on right side of assignment");
    return NULL;
  }

  return node;
}

static ast_node *parse_print(parser_state *state) {
  ast_node *node = create_node(NODE_PRINT);
  node->data.expression = parse_expression(state);
  if (!node->data.expression) {
    throw_error(state, "Expected an expression to print.");
    return NULL;
  }
  return node;
}

static ast_node *parse_function_call(parser_state *state) {
  ast_node *call = create_node(NODE_CALL);
  token *t = peek(state, 0);
  if (!t || t->type != IDENTIFIER) {
    throw_error(state, "expected function name in function call");
    return NULL;
  }
  call->data.string = t->ident;
  next(state);

  if (!expect(state, LPAREN))
    return NULL;

  if (peek(state, 0) && peek(state, 0)->type != RPAREN) {
    while (1) {
      ast_node *arg = parse_expression(state);
      if (!arg) {
        throw_error(state, "invalid function argument");
        return NULL;
      }
      add_element(call->children, &arg);

      t = peek(state, 0);
      if (t && t->type == COMMA) {
        next(state);
      } else {
        break;
      }
    }
  }

  if (!expect(state, RPAREN))
    return NULL;
  return call;
}

static ast_node *parse_expression(parser_state *state) {
  return parse_binary_expression(state, 0);
}

static ast_node *parse_binary_expression(parser_state *state, int min_prec) {
  ast_node *lhs = NULL;
  token *t = peek(state, 0);
  if (!t) {
    throw_error(state, "unexpected end of tokens in expression");
    return NULL;
  }

  if (t->type == NEWLINE) {
    throw_error(state, "unexpected newline in expression");
    return NULL;
  }

  if (is_unary_op(t)) {
    token *op = t;
    next(state);
    ast_node *operand = parse_binary_expression(state, precedence(op->type) + 1);    
    if (!operand) {
      throw_error(state, "invalid operand for unary op");
      return NULL;
    }
    lhs = create_node(NODE_UNARY_OP);
    lhs->data.binary.op = op->type;
    lhs->data.binary.left = operand;
  } else if (t->type == LPAREN) {
    next(state);
    lhs = parse_binary_expression(state, 0);
    if (!lhs) {
      throw_error(state, "invalid expression in parentheses");
      return NULL;
    }
    t = peek(state, 0);
    if (!t || t->type != RPAREN) {
      throw_error(state, "missing closing parenthesis");
      return lhs;
    }
    next(state);
  } else if (t->type == IDENTIFIER) {
    if (peek(state, 1) && peek(state, 1)->type == LPAREN) {
      lhs = parse_function_call(state);
    } else {
      lhs = create_node(NODE_IDENTIFIER);
      lhs->data.string = t->ident;
      next(state);
    }
  } else if (t->type == INTEGER) {
    lhs = create_node(NODE_NUMBER);
    lhs->data.number.num_type = TYPE_INT;
    lhs->data.number.value = strtod(t->ident, NULL);  // store as float, but mark as int
    next(state);
  } else if (t->type == FLOAT) {
    lhs = create_node(NODE_NUMBER);
    lhs->data.number.num_type = TYPE_FLOAT;
    lhs->data.number.value = strtod(t->ident, NULL);
    next(state);
  } else if (t->type == STRING) {
    lhs = create_node(NODE_STRING);
    lhs->data.string = t->ident;
    next(state);
  } else {
    throw_error(state, "unexpected token in expression");
    return NULL;
  }

  while (1) {
    token *op = peek(state, 0);
    if (!op)
      break;
    if (op->type == COMMA || op->type == RPAREN || op->type == NEWLINE)
      break;
    if (!is_binary_op(op))
      break;
    int op_prec = precedence(op->type);
    if (op_prec < min_prec)
      break;

    next(state);
    int next_min_prec = op_prec + 1;
    ast_node *rhs = parse_binary_expression(state, next_min_prec);
    if (!rhs) {
      throw_error(state, "invalid rhs in binary expression");
      return lhs;
    }

    if ((lhs->type == NODE_STRING || rhs->type == NODE_STRING) &&
        (op->type != ADD && op->type != COMP_EQ && op->type != NOT_EQ)) {
      throw_error(state, "operator not permitted for string operands");
      return lhs;
    }

    ast_node *bin = create_node(NODE_BINARY_OP);
    bin->data.binary.left = lhs;
    bin->data.binary.right = rhs;
    bin->data.binary.op = op->type;
    lhs = bin;
  }

  return lhs;
}

static int precedence(token_type op) {
  switch (op) {
  case OR:
    return 1;
  case AND:
    return 2;
  case EQ:
  case NOT_EQ:
    return 3;
  case LT:
  case LTE:
  case GT:
  case GTE:
    return 4;
  case ADD:
  case SUB:
    return 5;
  case MUL:
  case DIV:
  case MODULO:
    return 6;
  case CARROT:
    return 7;
  default:
    return 0;
  }
}

static ast_node *parse_statement(parser_state *state) {
  token *t = peek(state, 0);

  while (t && t->type == NEWLINE) {
    next(state);
    t = peek(state, 0);
  }

  if (!t || t->type == END) {
    return NULL;
  }

  switch (t->type) {
  case FN:
    next(state);
    return parse_function(state);
  case IF:
    next(state);
    return parse_if(state);
  case FOR:
    next(state);
    return parse_for(state);
  case WHILE:
    next(state);
    return parse_while(state);
  case PRINT:
    next(state);
    return parse_print(state);
  case IDENTIFIER:
    if (peek(state, 1) && peek(state, 1)->type == EQ) {
      return parse_assignment(state);
    } else if (peek(state, 1) && peek(state, 1)->type == LPAREN) {
      return parse_function_call(state);
    } else {
      return parse_expression(state);
    }
  case MATCH:
    return NULL;
  case RETURN:
    next(state);
    return parse_return(state);
  case DEDENT:
  case INDENT:
    next(state);
    return NULL;
  default:
    throw_error(state, "unexpected token in statement");
    return NULL;
  }
}

static void parse_block(parser_state *state, vector *children) {
  if (!expect(state, NEWLINE)) {
    throw_error(state, "expected newline before block");
    return;
  }
  if (!expect(state, INDENT)) {
    throw_error(state, "expected indent before block");
    return;
  }

  while (1) {
    token *t = peek(state, 0);
    while (t && t->type == NEWLINE) {
      next(state);
      t = peek(state, 0);
    }

    if (!t || t->type == END)
      break;

    if (t->type == DEDENT) {
      next(state);
      return;
    }

    ast_node *stmt = parse_statement(state);
    if (stmt)
      add_element(children, &stmt);
  }

  token *t = peek(state, 0);
  if (t && t->type != END) {
    if (!expect(state, DEDENT))
      throw_error(state, "expected dedent at end of block");
  }
}

ast_node *gen_ast(vector *tokens) {
  parser_state state = {
      .current = 0,
      .tokens = tokens,
      .has_main = 0,
      .in_func = 0,
      .error_count = 0,
      .line = 0,
      .col = 0,
  };

  ast_node *program = create_node(NODE_PROGRAM);
  program->children = create_vector(sizeof(ast_node *), 8);

  while (1) {
    token *t = peek(&state, 0);

    if (!t || t->type == END || state.current >= (int)tokens->size) {
      break;
    }

    ast_node *stmt = parse_statement(&state);
    if (stmt) {
      add_element(program->children, &stmt);
    }
  }

  // ensure "main" function exists
  if (!state.has_main) {
    fprintf(stderr, "your program has no entry point. please define a main function.");
    return NULL;
  }

  if (state.error_count) {
    fprintf(stderr, "unable to compile due to above errors.\n");
    return NULL;
  }

  return program;
}
