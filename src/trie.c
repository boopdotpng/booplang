#include "trie.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// +, -, *, /, |, =, &, >, <, %, !, ^, (, ), [, ], , "
#define SYMBOL_COUNT 19

// used to determine the index of each char
static const char SYMBOL_LIST[SYMBOL_COUNT] = {'+', '-', '*', '/', '|', '=', '&', '>', '<', '%',
                                               '!', '"', '^', '(', ')', '[', ']', ',', '~'};

struct trie_node {
  struct trie_node *children[SYMBOL_COUNT];
  int is_end;  // is this the end of a word
  token_type token_type;
};

static int symbol_to_index(const char *sym) {
  for (int i = 0; i < SYMBOL_COUNT; i++) {
    if (*sym == SYMBOL_LIST[i]) return i;
  }
  return -1;
}

trie_node *create_trie_node(void) {
  trie_node *node = malloc(sizeof(trie_node));
  if (node) {
    node->is_end = 0;
    node->token_type = -1;
    memset(node->children, 0, sizeof(node->children));
  }
  return node;
}

void insert_symbol(trie_node *root, const char *sym, token_type type) {
  trie_node *cur = root;
  while (*sym) {
    int index = symbol_to_index(sym);
    if (index == -1) {
      fprintf(stderr, "error: invalid character %c in pattern\n", *sym);
      return;
    }
    if (!cur->children[index]) {
      cur->children[index] = create_trie_node();
    }
    cur = cur->children[index];
    sym++;
  }

  cur->is_end = 1;
  cur->token_type = type;
}

match_result search_trie(trie_node *root, const char *sym) {
  trie_node *cur = root;
  match_result longest = {-1, 0};
  int pos = 0;

  // walk through the trie following the input symbol sequence
  while (sym[pos]) {
    int index = symbol_to_index(&sym[pos]);
    // if there's no path for this character, symbol isn't in trie
    if (index == -1 || !cur->children[index]) break;  // end of match

    cur = cur->children[index];
    pos++;

    if (cur->is_end) {
      longest.type = cur->token_type;
      longest.length = pos;
    }
  }

  // return the longest possible match
  return longest;
}

void free_trie(trie_node *root) {
  if (!root) return;
  for (int i = 0; i < SYMBOL_COUNT; i++) {
    if (root->children[i]) free_trie(root->children[i]);
  }
  free(root);
}
