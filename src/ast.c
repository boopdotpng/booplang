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
            ast_node *return_expr; // optional.
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
    vector *children;

    // TODO: maybe add line and col for error tracking
};

static void print_indent(int depth) {
    for (int i = 0; i < depth; i++)
        printf("  ");
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
    node->children = NULL; // create_vector if it has children later on
    return node;
}

// helper parse functions
ast_node *parse_function(parser_state *state) {

}

ast_node *parse_while(parser_state *state) {

}

ast_node *parse_for(parser_state *state) {

}

ast_node *parse_assignment(parser_state *state) {

}

ast_node *parse_print(parser_state *state) {

}

ast_node *parse_function_call(parser_state *state) {

}

ast_node *parse_expression(parser_state *state) {

}

ast_node *gen_ast(vector *tokens) {
    parser_state state = {.current = 0, .tokens = tokens, .has_main = 0};
    ast_node *program = create_node(NODE_PROGRAM);
    ast_node *current_function = NULL;

    while (state.current < tokens->size) {
        token *t = get_element(state.tokens, state.current);

        if (t->type == FN) {
            ast_node *fn = parse_function(&state);
        }
    }




    return program;
}
