#include "ast.h"
#include "token.h"
#include "vector.h"
#include "lexer.h"
#include <stdio.h>
#include <string.h>

typedef enum {
    NODE_PROGRAM,     // root node
    NODE_FUNCTION,    // function definition
    NODE_IF,          // if statement
    NODE_WHILE,       // while loop
    NODE_FOR,         // for loop
    NODE_ASSIGNMENT,  // variable assignment
    NODE_BINARY_OP,   // binary ops (+, -, *, /, etc.)
    NODE_UNARY_OP,    // unary ops (++, --)
    NODE_CALL,        // function call
    NODE_RETURN,      // return statement
    NODE_IDENTIFIER,  // variable/function names
    NODE_NUMBER,      // numeric literals
    NODE_STRING,      // a string
    NODE_PRINT,       // language-reserved print
} node_type;

typedef struct {
    char *name;
    int scope_level; // 0 for global
    struct symbol *next; // linked list chain to lookup symbols
} symbol;

// scope table
typedef struct {
    vector /* symbol */ *symbols;
    size_t parent_scope; // index of parent scope
} scope_table;

typedef struct {
    vector *tokens;
    int current; // current token index
    int has_main; // tracks if the main function was found
    vector /* scope_table */ scopes; // stack of scopes
    vector *errors; // accumulate error messages for the end of parsing
    ast_node *current_function; // current function (for nested function detection)
    // error reporting tokens
    int line;
    int col;
} parser_state;

struct ast_node {
    node_type type;
    // one of the following
    union {
        // need to keep track of ints and floats separately.
        // will default to 64-bit floats
        union {
            double fl; // float
            long number; // int
        } number;

        char *string; // "strings" are usually standalone

        // for binary operations
        // 3 + 5
        struct {
            ast_node *left;
            ast_node *right;
            token_type op;
        } binary;

        // left = right
        struct {
            char *var_name;
            ast_node *value;
        } assignment;

        // a function
        struct {
            char *name;
            vector /* ast_node */ *params;
            // pointer to return children for faster access
            vector /* *ast_node */ *returns; // list of return nodes
            token_type return_type; // ir needs a return type
        } function;

        // control structures
        // elifs are represented as if statements in the else block.
        struct {
            ast_node *condition;
            ast_node *else_body;

            // for loops are just while loops with a variable and step
            ast_node *initializer; // i = 0 for example
            ast_node *step; // i += 3
        } control;

        // general expressions; print statements for example
        ast_node *expression;

    } data;

    // all bodies would go here instead of being declared in the structs above.
    vector /* ast_node */ *children;

    // TODO: maybe add line and col for error tracking
    int line;
    int col;
};

// Utility functions
static int is_op(token *t);
static int is_unary_op(token *t);
static int is_binary_op(token *t);
static void throw_error(parser_state *state, const char *msg);
static void print_indent(int depth);
static token *next(parser_state *state);
static token *peek(parser_state *state, int ahead);
static int accept(parser_state *state, token_type type);
static int expect(parser_state *state, token_type type);

// AST helper functions
static ast_node *create_node(node_type type);

// Parsing functions
static ast_node *parse_function(parser_state *state);
static ast_node *parse_if(parser_state *state);
static ast_node *parse_while(parser_state *state);
static ast_node *parse_for(parser_state *state);
static ast_node *parse_assignment(parser_state *state);
static ast_node *parse_print(parser_state *state);
static ast_node *parse_function_call_or_definition(parser_state *state);
static ast_node *parse_expression(parser_state *state);
static ast_node *parse_binary_expression(parser_state *state, int min_precedence);

static ast_node *parse_statement(parser_state *state);

static void parse_block(parser_state *state, vector *children);

static int precedence(token_type op);

// scope management


// static void enter_scope(parser_state *state) {
//     scope_table *new_scope = malloc(sizeof(scope_table));
//     new_scope->size = 0;
//     new_scope->capacity = 8;
//     new_scope->symbols = create_vector(sizeof(symbol), 8);
//     add_element(state->scopes, &new_scope);
// }

// static void exit_scope(parser_state *state) {

// }

// static void define_variable(parser_state *state, const char *name, token_type type) {

// }

// static symbol *resolve_variable(parser_state *state, const char *name) {
//     // search from innermost scope outward
// }

// check if its an operator
static int is_op(token *t) {
    return is_binary_op(t) || is_unary_op(t);
}

// check if an operator is unary
static int is_unary_op(token *t) {
    switch(t->type) {
    case SUB_ONE:
    case ADD_ONE:
    case NOT: // !
    case SUB: // -x
    case BITW_NOT: // ~
        return 1;
    default:
        return 0;
    }
}

static int is_binary_op(token *t) {
    switch(t->type) {
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
    add_element(state->errors, &msg);
    fprintf(stderr, "%s at line %d:%d (%s) \n", msg, state->line, state->col,token_type_str(peek(state,0)->type));
}

static void print_indent(int depth) {
    if (depth == 0) return;

    for (int i = 0; i < depth; i++)
        printf("  ");
}

// pretty print AST
void pretty_print_ast(ast_node *node, int depth) {
    if (!node) return;

    print_indent(depth);

    switch (node->type) {
    case NODE_PROGRAM:
        printf("program\n");
        break;

    case NODE_FUNCTION:
        printf("function: %s\n", node->data.function.name);
        // TODO: change logic for return expressions
        // if (node->data.function.return_expr) {
        //     print_indent(depth + 1);
        //     printf("return:\n");
        //     pretty_print_ast(node->data.function.return_expr, depth + 2);
        // }
        break;

    case NODE_IF:
        printf("if\n");
        if (node->data.control.condition) {
            print_indent(depth + 1);
            printf("condition:\n");
            pretty_print_ast(node->data.control.condition, depth + 2);
        }
        if (node->data.control.else_body) {
            print_indent(depth + 1);
            printf("else:\n");
            pretty_print_ast(node->data.control.else_body, depth + 2);
        }
        break;

    case NODE_WHILE:
        printf("while\n");
        if (node->data.control.condition) {
            print_indent(depth + 1);
            printf("condition:\n");
            pretty_print_ast(node->data.control.condition, depth + 2);
        }
        break;

    case NODE_FOR:
        printf("for\n");
        if (node->data.control.initializer) {
            print_indent(depth + 1);
            printf("init:\n");
            pretty_print_ast(node->data.control.initializer, depth + 2);
        }
        if (node->data.control.condition) {
            print_indent(depth + 1);
            printf("condition:\n");
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
        pretty_print_ast(node->data.assignment.value, depth + 1);
        break;

    case NODE_BINARY_OP:
        printf("binary op: %d\n", node->data.binary.op);
        print_indent(depth + 1);
        printf("left:\n");
        pretty_print_ast(node->data.binary.left, depth + 2);
        print_indent(depth + 1);
        printf("right:\n");
        pretty_print_ast(node->data.binary.right, depth + 2);
        break;

    case NODE_UNARY_OP:
        printf("unary op: %d\n", node->data.binary.op);
        print_indent(depth + 1);
        printf("expr:\n");
        pretty_print_ast(node->data.binary.left, depth + 2);
        break;

    case NODE_CALL:
        printf("function call (todo: fill details)\n");
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
        printf("identifier: %s\n", node->data.string);
        break;

    case NODE_NUMBER:
        printf("number: ");
        // pick which field was used; for simplicity let's assume .number is an integer if .fl == 0
        if (node->data.number.fl == 0.0) {
            printf("%ld\n", node->data.number.number);
        } else {
            printf("%f\n", node->data.number.fl);
        }
        break;

    case NODE_STRING:
        printf("string: \"%s\"\n", node->data.string);
        break;

    case NODE_PRINT:
        printf("print\n");
        if (node->data.expression) {
            print_indent(depth + 1);
            printf("expr:\n");
            pretty_print_ast(node->data.expression, depth + 2);
        }
        break;

    default:
        printf("unknown node type\n");
        break;
    }

    // print children
    for (size_t i = 0; i < node->children->size; i++) {
        ast_node *child = *(ast_node **)get_element(node->children, i);
        pretty_print_ast(child, depth + 1);
    }
}

// unconditional move forward
static token *next(parser_state *state) {
    // no END handling here. we should never call next after seeing END
    token *n = get_element(state->tokens, ++state->current); // move forward and get the next token
    state->col = n->col;
    state->line = n->line;
    return n;
}

// consume n-ahead without consuming
static token *peek(parser_state *state, int ahead) {
    if (state->current + ahead > state->tokens->size) {
        return NULL;
    } else {
        return get_element(state->tokens, state->current+ahead);
    }
}

// move forward only if we get the right token
static int accept(parser_state *state, token_type type) {
    token *current_token = peek(state, 0);
    if (current_token->type == type) {
        state->current++;
        state->line = current_token->line;
        state->col = current_token->col;
        return 1;
    }
    return 0;
}

// for mandatory tokens
static int expect(parser_state *state, token_type type) {
    token *t = peek(state, 1);
    if (t->type == type) {
        state->current++;
        return 1;
    } else {
        throw_error(state, "Unexpected token type.");
        return 0;
    }
}

static ast_node *create_node(node_type type) {
    ast_node *node = malloc(sizeof(ast_node));
    node->type = type;
    node->children = create_vector(sizeof(ast_node), 8); // everything will have children
    return node;
}

// modifies current function to add the return statement and type
static ast_node *parse_return(parser_state *state) {
    // skip return token
    next(state);

    ast_node *ret = parse_expression(state);
    // track returns for an easier during IR generation
    // add_element(state->current_function->data.function.returns, ret);
    // TODO: determine the return type by looking at the expression

    // state->current_function->data.function.return_type = FLOAT;

    return ret;
}

// helper parse functions
static ast_node *parse_function(parser_state *state) {
    token *t = next(state); // skip fn
    ast_node *func = create_node(NODE_FUNCTION);

    if(state->current_function) {
        throw_error(state, "Nested functions are not allowed.");
        return NULL;
    }

    state->current_function = func;

    if (t->type != IDENTIFIER) {
        throw_error(state, "Function name must be an identifier.");
        return NULL;
    }

    func->data.function.name = t->ident;

    printf("parsing function %s\n", t->ident);

    // set has_main flag
    if (strcmp(t->ident, "main") == 0)
        state->has_main = 1;

    if (!expect(state, LPAREN)) {
        throw_error(state, "Expected '(' after function name.");
        return NULL;
    }

    // parse parameters as comma separated list of identifiers
    t = next(state);
    int expect_comma = 0;

    func->data.function.params = create_vector(sizeof(ast_node), 4);

    while (t->type == IDENTIFIER || (t->type == COMMA && expect_comma)) {
        if (t->type == IDENTIFIER) {
            ast_node *param = create_node(NODE_IDENTIFIER);
            param->data.string = t->ident;
            add_element(func->data.function.params, &param);
            expect_comma = 1;
        } else if (t->type == COMMA) {
            expect_comma = 0;
        } else {
            throw_error(state, "Unexpected token in function parameter list.");
            return NULL;
        }
        t = next(state);
    }

    // expect closing )
    if (t->type != RPAREN) {
        throw_error(state, "Expected ')' after function parameters");
        return NULL;
    }

    // parse function body
    // TODO: pretty sure the vector is already created
    func->children = create_vector(sizeof(ast_node), 8);
    // create return vector
    func->data.function.returns = create_vector(sizeof(ast_node*), 1);
    parse_block(state, func->children);

    // no longer inside a function
    state->current_function = NULL;

    return func;
}

static ast_node *parse_if(parser_state *state) {
    ast_node *i = create_node(NODE_IF);
    token *t = next(state);

    // assume parse_expression parses a whole line
    i->data.control.condition = parse_expression(state);

    t = next(state);
    if (t->type == INDENT) {
        // parse body of if statement until DEDENT, somehow
    } else {
        throw_error(state, "If statement missing a body.");
    }

    return i;
}

static ast_node *parse_while(parser_state *state) {
    ast_node *w = create_node(NODE_WHILE);
    token *t = next(state);

    w->data.control.condition = parse_expression(state);

    if (t->type == INDENT) {
        // parse while body
    } else {
        throw_error(state, "Missing body for while loop.");
    }

    return w;
}

static ast_node *parse_for(parser_state *state) {
    ast_node *for_node = create_node(NODE_FOR);

    // parse iterator variable
    token *t = next(state);
    if (t->type != IDENTIFIER) {
        throw_error(state, "Expected an iterator variable in for loop.");
        return NULL;
    }
    // create initializer node for the iterator
    for_node->data.control.initializer = create_node(NODE_ASSIGNMENT);
    for_node->data.control.initializer->data.assignment.var_name = t->ident;

    // expect `from`
    if (!expect(state, FROM)) {
        throw_error(state, "Expected 'from' keyword in for loop.");
        return NULL;
    }

    // parse start value (x)
    t = next(state);
    ast_node *start_value = NULL;
    int start_is_var = 0;
    if (t->type == IDENTIFIER || t->type == INTEGER || t->type == FLOAT) {
        start_value = create_node(t->type == IDENTIFIER ? NODE_IDENTIFIER : NODE_NUMBER);
        if (t->type == IDENTIFIER) {
            start_value->data.string = t->ident;
        } else {
            if (t->type == FLOAT)
                start_value->data.number.number = strtof(t->ident, NULL);
            if (t->type == INTEGER)
                start_value->data.number.number = strtol(t->ident, NULL, 10);
        }
    } else {
        throw_error(state, "Expected a variable or numeric value after 'from' in for loop.");
        return NULL;
    }
    for_node->data.control.initializer->data.assignment.value = start_value;

    // expect `to`
    if (!expect(state, TO)) {
        throw_error(state, "Expected 'to' keyword in for loop.");
        return NULL;
    }

    // parse end value (y)
    t = next(state);
    ast_node *end_value = NULL;
    int end_is_variable = 0;
    if (t->type == IDENTIFIER || t->type == INTEGER || t->type == FLOAT) {
        end_value = create_node(t->type == IDENTIFIER ? NODE_IDENTIFIER : NODE_NUMBER);
        if (t->type == IDENTIFIER) {
            end_value->data.string = t->ident;
        } else {
            if (t->type == FLOAT)
                end_value->data.number.number = strtof(t->ident, NULL);
            if (t->type == INTEGER)
                end_value->data.number.number = strtol(t->ident, NULL, 10);
        }
    } else {
        throw_error(state, "Expected a variable or numeric value after 'to' in for loop.");
        return NULL;
    }
    for_node->data.control.condition = end_value;

    // optional `by`
    ast_node *step_value = NULL;
    /*
    We can't infer the step size if the start or end values are variables. There's no easy way to tell if x > y.
    */
    if(start_is_var || end_is_variable) {
        if (!expect(state, BY)) {
            throw_error(state, "Expected 'by' keyword in for loop when 'from' or 'two' is a variable");
            return NULL;
        }
    }
    if (accept(state, BY)) {
        t = next(state);
        if (t->type == IDENTIFIER || t->type == INTEGER || t->type == FLOAT) {
            step_value = create_node(t->type == IDENTIFIER ? NODE_IDENTIFIER : NODE_NUMBER);
            if (t->type == IDENTIFIER) {
                step_value->data.string = t->ident;
            } else {
                if (t->type == FLOAT)
                    step_value->data.number.number = strtof(t->ident, NULL);
                if (t->type == INTEGER)
                    step_value->data.number.number = strtol(t->ident, NULL, 10);
            }
        } else {
            throw_error(state, "Expected a variable or numeric value after 'by' in for loop.");
            return NULL;
        }
    } else {
        // default step: 1 or -1 based on start and end values
        step_value = create_node(NODE_NUMBER);
        step_value->data.number.number =
            (start_value->type == NODE_NUMBER && end_value->type == NODE_NUMBER &&
             start_value->data.number.number < end_value->data.number.number)
            ? 1
            : -1;
    }
    for_node->data.control.step = step_value;

    // expect an indented block for the loop body
    if (!expect(state, INDENT)) {
        throw_error(state, "Expected indented block after for loop.");
        return NULL;
    }

    // parse loop body

    parse_block(state, for_node->children);

    return for_node;
}

static ast_node *parse_assignment(parser_state *state) {
    token *t = peek(state, 0);
    if (t->type != IDENTIFIER) {
        throw_error(state, "Expected variable name in assignment.");
        return NULL;
    }
    ast_node *node = create_node(NODE_ASSIGNMENT);
    node->data.assignment.var_name = t->ident;

    if (!expect(state, EQ)) {
        throw_error(state, "Expected '=' in assignment.");
        return NULL;
    }

    // skip EQ token
    next(state);

    node->data.assignment.value = parse_expression(state);
    if (!node->data.assignment.value) {
        throw_error(state, "Invalid expression on right side of assignment");
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

static ast_node *parse_function_call_or_definition(parser_state *state) {
    ast_node *i = create_node(NODE_IF);

    return i;
}

static ast_node *parse_expression(parser_state *state) {
    return parse_binary_expression(state, 0);
}

static ast_node *parse_binary_expression(parser_state *state, int min_precedence) {
    // parse a "primary" expression or unary op
    token *t = peek(state, 0);
    if (!t) {
        throw_error(state, "unexpected end of tokens in expression.");
        return NULL;
    }

    ast_node *lhs = NULL;

    // handle unary first
    if (is_unary_op(t)) {
        // consume unary op
        next(state);
        // parse the operand, giving the unary op's precedence so it binds correctly
        ast_node *operand = parse_binary_expression(state, precedence(t->type));
        if (!operand) {
            throw_error(state, "invalid operand for unary op.");
            return NULL;
        }
        lhs = create_node(NODE_UNARY_OP);
        lhs->data.binary.op = t->type;
        lhs->data.binary.left = operand;
    } else if (t->type == LPAREN) {
        // handle parentheses
        next(state); // consume '('
        lhs = parse_binary_expression(state, 0);
        if (!lhs) {
            throw_error(state, "invalid expression in parentheses.");
            return NULL;
        }
        if (!peek(state, 0) || peek(state, 0)->type != RPAREN) {
            throw_error(state, "missing closing parenthesis.");
            return lhs; // returning partially so we don't leak memory
        }
        // consume ')'
        next(state);
    } else if (t->type == IDENTIFIER) {
        lhs = create_node(NODE_IDENTIFIER);
        lhs->data.string = t->ident;
        next(state);
    } else if (t->type == INTEGER || t->type == FLOAT) {
        lhs = create_node(NODE_NUMBER);
        if (t->type == INTEGER) {
            lhs->data.number.number = strtol(t->ident, NULL, 10);
        } else {
            lhs->data.number.fl = strtof(t->ident, NULL);
        }
        next(state);
    } else if (t->type == STRING) {
        lhs = create_node(NODE_STRING);
        lhs->data.string = t->ident;
        next(state);
    } else {
        throw_error(state, "unexpected token in expression.");
        return NULL;
    }

    // now parse trailing binary ops (left-associative with precedence)
    while (1) {
        token *op_token = peek(state, 0);
        if (!op_token || !is_binary_op(op_token)) break;

        int op_prec = precedence(op_token->type);
        if (op_prec < min_precedence) break;

        next(state); // consume the operator
        int next_min_prec = op_prec + 1;

        ast_node *rhs = parse_binary_expression(state, next_min_prec);
        if (!rhs) {
            throw_error(state, "invalid rhs in binary expression.");
            return lhs; // better than returning null bc we keep some part
        }

        // if either side is a string, only allow certain ops
        if (lhs->type == NODE_STRING || rhs->type == NODE_STRING) {
            // allow '+' or '==' or '!=' for strings, everything else is nonsense
            if (op_token->type != ADD &&
                    op_token->type != COMP_EQ &&
                    op_token->type != NOT_EQ) {
                throw_error(state, "operator not permitted for string operands.");
                return lhs;
            }
        }

        ast_node *bin_node = create_node(NODE_BINARY_OP);
        bin_node->data.binary.left = lhs;
        bin_node->data.binary.right = rhs;
        bin_node->data.binary.op = op_token->type;
        lhs = bin_node;
    }

    return lhs;
}

static int precedence(token_type op) {
    switch (op) {
    case OR:            // logical OR (lowest precedence)
        return 1;
    case AND:           // logical AND
        return 2;
    case EQ:            // equality (==) or inequality (!=)
    case NOT_EQ:
        return 3;
    case LT:            // relational operators (<, <=, >, >=)
    case LTE:
    case GT:
    case GTE:
        return 4;
    case ADD:           // addition and subtraction
    case SUB:
        return 5;
    case MUL:           // multiplication, division, and modulo
    case DIV:
    case MODULO:
        return 6;
    case CARROT:        // exponentiation (^)
        return 7;       // highest precedence
    default:
        return 0;       // unknown operators have no precedence
    }
}

static ast_node *parse_statement(parser_state *state) {
    while (peek(state, 0)->type == NEWLINE)
        next(state);

    token *t = peek(state, 0);

    switch (t->type) {
    case FN:
        return parse_function(state);
    case IF:
        return parse_if(state);
    case FOR:
        return parse_for(state);
    case WHILE:
        return parse_while(state);
    case PRINT:
        return parse_print(state);
    case IDENTIFIER:
        return parse_assignment(state); // assume it's an assignment if it starts with an identifier
    case END:
        return NULL; // signal the end of parsing
    case MATCH:
        return NULL;
    case RETURN:
        return parse_return(state);
    default:
        throw_error(state, "unexpected token in statement");
        return NULL;
    }
}

// bodies of things
static void parse_block(parser_state *state, vector *children) {
    if (!expect(state, NEWLINE)) {
        throw_error(state, "expected newline before block");
        return;
    }

    if (!expect(state, INDENT)) {
        throw_error(state, "expected indent before block");
        return;
    }

    while (state->current < state->tokens->size) {
        token *t = next(state);

        if(t->type == DEDENT || t->type == END)
            break;

        ast_node *stmt = parse_statement(state);
        if(stmt)
            add_element(children, stmt);
        else
            fprintf(stderr, "not a statement in parse_block");
    }

    if(peek(state, 0)->type == DEDENT)
        next(state);
}

ast_node *gen_ast(vector *tokens) {
    parser_state state = {
        .current = 0,
        .tokens = tokens,
        .has_main = 0,
        .errors = create_vector(sizeof(char *), 4),
        .current_function = NULL,
        .line = 0,
        .col = 0,
    };

    ast_node *program = create_node(NODE_PROGRAM);

    while (state.current < (int) tokens->size) {
        ast_node *stmt = parse_statement(&state);
        if (stmt) {
            add_element(program->children, &stmt);
        } else {
            // synchronize(&state); // skip invalid tokens
        }
    }

    // ensure "main" function exists
    if (!state.has_main) {
        fprintf(stderr, "your program has no entry point. please define a main function.");
        return NULL;
    }

    // check for accumulated errors
    if (state.errors->size > 0) {
        fprintf(stderr, "see above errors\n");
        return NULL;
    }

    return program;
}
