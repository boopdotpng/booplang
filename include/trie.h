#pragma once
#include "token.h"

typedef struct trie_node trie_node;

typedef struct {
  token_type type;
  int length;
} match_result;

/**
 * @brief Allocates and initializes a new trie node.
 *
 * @return Pointer to the created node, or NULL on failure.
 */
trie_node *create_trie_node();

/**
 * @brief Inserts a symbol and its token type into the trie.
 *
 * @param root Trie root node.
 * @param symbol Null-terminated string to insert.
 * @param type Token type to associate with the symbol.
 */
void insert_symbol(trie_node *root, const char *symbol, token_type type);

/**
 * @brief Searches for the longest matching symbol in the trie.
 *
 * @param root Trie root node.
 * @param sym Input string to search.
 * @return Longest match (token type and length). If no match, returns `{-1, 0}`.
 */
match_result search_trie(trie_node *root, const char *sym);

/**
 * @brief Frees all nodes in the trie.
 *
 * @param root Trie root node.
 */
void free_trie(trie_node *root);