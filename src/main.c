#include <stdio.h>
#include "utils.h"
#include "lexer.h"
#include "vector.h"
#include "intern.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: boop <filename>\n");
        return EXIT_FAILURE;
    }

    vector *tokens = lex(argv[1]);
    for (int i = 0; i < tokens->size; ++i) {
        print_token(get_element(tokens, i));
    }
    return 0;
}