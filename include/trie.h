#pragma once
#include "lexer.h"

typedef struct trie_node trie_node;

// struct to keep track of the longest possible match
typedef struct {
    token_type type;
    int length;
} match_result;

/**
 * @brief Create a trie node object
 *
 * @return trie_node*
 */
trie_node *create_trie_node();

/**
 * @brief
 *
 * @param root
 * @param symbol
 * @param type
 */
void insert_symbol(trie_node *root, const char *symbol, token_type type);

/**
 * @brief
 *
 * @param root
 * @param sym
 * @return token_type
 */
match_result search_trie(trie_node *root, const char *sym);

/**
 * @brief
 *
 * @param root
 */
void free_trie(trie_node *root);