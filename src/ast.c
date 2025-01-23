#include "ast.h"
#include "token.h"
#include "vector.h"
#include "lexer.h"
#include <stdio.h>

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
    NODE_PRINT        // language-reserved print
} node_type;

typedef struct {
    vector *tokens;
    int current; // current token index
    int has_main; // tracks if the main function was found
    vector *scopes; // stack of scopes 
    vector *errors; // accumulate error messages for the end of parsing
    ast_node *current_function; // currently parsed function
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
            ast_node *params;
            int param_count;
            ast_node *return_expr; // optional
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

    } data;

    // all bodies would go here instead of being declared in the structs above.
    vector /* ast_node */ *children;

    // TODO: maybe add line and col for error tracking
};

static void throw_error(parser_state *state, const char *msg) {
    add_element(state->errors, &msg);
    fprintf(stderr, "Error: %s\n", msg);
}

static void print_indent(int depth) {
    for (int i = 0; i < depth; i++)
        printf("  ");
}

static token *next(parser_state *state) {
    // no END handling here. we should never call next after seeing END
    return get_element(state->tokens, ++state->current);
}

void pretty_print_ast(ast_node *node, int depth) {
    if (!node) return;
    print_indent(depth);  // print indentation based on the depth

    switch (node->type) {
    case NODE_PROGRAM:
        printf("start of program\n");
        break;
    case NODE_FUNCTION:
        printf("node_function: %s\n", node->data.function.name);
        if (node->children) {
            for (int i = 0; i < node->children->size; i++) {
                pretty_print_ast(node->children[i], depth + 1);
            }
        }
        break;
    case NODE_IF:
        printf("node_if\n");
        pretty_print_ast(node->data.control.condition, depth + 1);
        if (node->children) {
            for (int i = 0; i < node->children->size; i++) {
                pretty_print_ast(node->children[i], depth + 1);
            }
        }
        if (node->data.control.else_body) {
            printf("else:\n");
            pretty_print_ast(node->data.control.else_body, depth + 1);
        }
        break;
    case NODE_WHILE:
        printf("node_while\n");
        pretty_print_ast(node->data.control.condition, depth + 1);
        pretty_print_ast(node->data.control.body, depth + 1);
        break;
    case NODE_FOR:
        printf("node_for\n");
        pretty_print_ast(node->data.control.initializer, depth + 1);
        pretty_print_ast(node->data.control.condition, depth + 1);
        pretty_print_ast(node->data.control.step, depth + 1);
        pretty_print_ast(node->data.control.body, depth + 1);
        break;
    case NODE_ASSIGNMENT:
        printf("node_assignment: %s = \n", node->data.assignment.var_name);
        pretty_print_ast(node->data.assignment.value, depth + 1);
        break;
    case NODE_BINARY_OP:
        printf("node_binary_op: %s\n", token_type_str(node->data.binary.op));
        pretty_print_ast(node->data.binary.left, depth + 1);
        pretty_print_ast(node->data.binary.right, depth + 1);
        break;
    case NODE_UNARY_OP:
        printf("node_unary_op\n");
        pretty_print_ast(node->data.binary.left, depth + 1);
        break;
    case NODE_CALL:
        printf("node_call\n");
        break;
    case NODE_RETURN:
        printf("node_return\n");
        pretty_print_ast(node->data.function.return_expr, depth + 1);
        break;
    case NODE_IDENTIFIER:
        printf("node_identifier: %s\n", node->data.string);
        break;
    case NODE_NUMBER:
        printf("node_number: %f\n", node->data.number.n);
        break;
    case NODE_STRING:
        printf("node_string: \"%s\"\n", node->data.string);
        break;
    case NODE_PRINT:
        printf("node_print\n");
        break;
    default:
        printf("unknown node type\n");
        break;
    }

    if (node->children) {
        for (int i = 0; i < node->child_count; i++) {
            pretty_print_ast(node->children[i], depth + 1);
        }
    }

    print_indent(depth);  // close indentation for this level
}

static ast_node *create_node(node_type type) {
    ast_node *node = malloc(sizeof(ast_node));
    node->type = type;
    node->children = create_vector(sizeof(ast_node), 8); // everything will have children
    return node;
}

// helper parse functions
static ast_node *parse_function(parser_state *state) {
    token *t = next(state);
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

    func->data.name = t->ident;

    t = next(state);
    if (t->type != LPAREN) {
        throw_error(state, "Expected '(' after function name.");
        return NULL;
    }

    // parse parameters as list of identifiers
    t = next(state);
    while (t->type == IDENTIFIER) {
        ast_node *param = create_node(NODE_IDENTIFIER);
        param->data.string = t->ident;
        add_element(func->data.function.params, &param);
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
    t = next(state);



    // parse return stmt
    if (t->type == RETURN) {
        t = next(state);
        ast_node *return_stmt = parse_expression(state);
        func->data.function.return_expr = return_stmt;
    }

    // TODO: tokens after this at the same indent level are invalid. write a check for this 


    // no longer inside a function
    state->current_function == NULL;

    return func;
}

static ast_node *parse_if(state) {
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
    ast_node *i = create_node(NODE_FOR);
    token *t = next(state);

    // TODO: there has to be a better way to write this
    if (t->type == IDENTIFIER) {
        t = next(state);
        if (t->type == FROM) {
            t = next(state);
            if (t->type == INTEGER || t->type == FLOAT) {
                t = next(state);

            }
        }
    }

    if (t->type != INDENT) {
        throw_error(state, "Missing body for for loop.");
    }

    // parse body until dedent

    return i;

}

static ast_node *parse_assignment(parser_state *state) {
    ast_node *i = create_node(NODE_ASSIGNMENT);
    token *t = next(state);

    return i;
}

static ast_node *parse_print(parser_state *state) {
    ast_node *i = create_node(NODE_PRINT);
    // read until a newline token. everything goes into print 
    parse_expression(state);

    // TODO: make an ast_node for print
    return i;
}

static ast_node *parse_function_call_or_definition(parser_state *state) {
    ast_node *i = create_node(NODE_IF);

    return i;
}

static ast_node *parse_expression(parser_state *state) {
    token *t = next(state);
    if (t->type == IDENTIFIER) {
        // could be a function call or an assignment, or other random crap idk
    }

    return NULL;
}

// bodies of things
static ast_node *parse_block(parser_state *state) {
    ast_node *block = create_node(NODE_PROGRAM); 
    token *t = next(state);

    while(t->type != DEDENT && t->type != END) {
        ast_node *stmt = parse_statement(state);
        if (stmt) add_element(block->children, &stmt);
        t = next(state);
    }

    return block;
}

ast_node *gen_ast(vector *tokens) {
    parser_state state = {
        .current = -1, 
        .tokens = tokens,
        .has_main = false,
        .errors=create_vector(sizeof(char*), 4)
    };
    token *t = next(state);
    ast_node *program = create_node(NODE_PROGRAM);

    while (state.current < tokens->size) {
        switch(current_token->type) {
            case FN:
                ast_node *node = parse_function(state);
                add_element(program->children, node);
            break;
            case IF:
                ast_node *node = parse_if(state);
                add_element(program->children, node);
            break;
            case FOR:
                ast_node *node = parse_for(state);
                add_element(program->children, node);
            break;
            case WHILE:
                ast_node *node = parse_while(state);
                add_element(program->children, node);
            break;
            case PRINT:
                ast_node *node = parse_print(state);
                add_element(program->children, node);
            break;
            case IDENTIFIER:
                ast_node *node = parse_ident(state);
                add_element(program->children, node);
            break;
            case END:
                // gather errors or safely return
            break;
            default:
                fprintf(stderr, "hmm, why is a %s here?", token_type_str(t->type));
                // TODO: add to errors later on
            break;
        }
        state.current++;
    }

    // make sure main exists
    // TODO: add this to the errors array
    if (!state.has_main) {
       throw_error(&state, "Error: your program has no entry point. Please define a main function.");
        return NULL;
    }

    // check for errors
    if (state.errors) {
       // TODO: handle this later 
       return NULL;
    }

    return program;
}