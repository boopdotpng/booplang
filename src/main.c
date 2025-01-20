#include <stdio.h>
#include "utils.h"
#include "lexer.h"
#include "vector.h"
#include "intern.h"
#include "trie.h"

int main(int argc, char *argv[]) {
    // if (argc < 2) {
    //     fprintf(stderr, "usage: boop <filename>\n");
    //     return EXIT_FAILURE;
    // }

    // lexer_result *l = lex(argv[1]);
    // for (int i = 0; i < l->tokens->size; ++i) {
    //     print_token(get_element(l->tokens, i));
    // }
    trie_node *root = create_trie_node();

    // Insert operators with their token types
    insert_symbol(root, "+", 0);
    insert_symbol(root, "++", 1);
    insert_symbol(root, "+=", 2);

    // Test some inputs
    const char *input = "+=foo";
    const char *input1 = "+";
    const char *input2 = "++";
    const char *input3 = "+h=foo";
    match_result match = search_trie(root, input);
    match_result match1 = search_trie(root, input1);
    match_result match2 = search_trie(root, input2);
    match_result match3 = search_trie(root, input3);
    printf("%d %d\n", match.type, match.length);
    printf("%d %d\n", match1.type, match1.length);
    printf("%d %d\n", match2.type, match2.length);
    printf("%d %d\n", match3.type, match3.length);
}