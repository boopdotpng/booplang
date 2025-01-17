#include <stdio.h>
#include "utils.h"
#include "lexer.h"
#include "vector.h"
#include <assert.h>
#include "unordered_map.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: boop <filename>\n");
        return EXIT_FAILURE;
    }

    lexer *lexer = lex(argv[1]);
    for (int i = 0; i < lexer->tokens->size; ++i) {
        print_token(get_element(lexer->tokens, i));
    }
    return 0;
}