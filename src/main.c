#include <stdio.h>
#include "lexer.h"
#include "vector.h"
#include "ast.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: boop <filename>\n");
        return EXIT_FAILURE;
    }

    lexer_result *l = lex(argv[1]);
    for (int i = 0; i < l->tokens->size; ++i) {
        print_token(get_element(l->tokens, i));
    }

    printf("\n\n\n\n");

    ast_node *program = gen_ast(l->tokens);
}