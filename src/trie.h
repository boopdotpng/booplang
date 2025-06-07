#pragma once
#include "token.h"

typedef struct trie_node trie_node;

typedef struct {
  token_type type;
  int length;
} match_result;

trie_node *create_trie_node(void);
void insert_symbol(trie_node *root, const char *symbol, token_type type);
match_result search_trie(trie_node *root, const char *sym);
void free_trie(trie_node *root);
