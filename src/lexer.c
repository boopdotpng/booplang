#include "lexer.h"
#include "intern.h"
#include "trie.h"
#include "utils.h"
#include "vector.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define TABS 1
#define SPACES 2
#define UNSET 0

#define MAX_INDENT_LEVEL 32
#define MAX_STRING_LEN 256

typedef struct {
  const char *symbol;
  token_type type;
} symbol_entry;

struct lexer {
  int indent_style;
  int spaces_per_level;
  int line;
  int col;
  int current_indent;
  int indent_stack[MAX_INDENT_LEVEL];
  int indent_sp;
  vector *tokens;
  intern_table *interns;
};

static void add_token_null(lexer *lexer, token_type type) {
  token new_token = {.type = type, .ident = NULL, .col = lexer->col, .line = lexer->line};

  add_element(lexer->tokens, &new_token);
}

static void add_token_len(lexer *lexer, token_type type, const char *ptr, size_t length) {
  intern_result result = intern_string(lexer->interns, ptr, length, type);

  token new_token = {
      .type = result.value, .ident = result.key, .col = lexer->col, .line = lexer->line};

  add_element(lexer->tokens, &new_token);
}

static void add_language_keywords(intern_table *interns) {
  const char *keywords[] = {"fn",   "for",    "while", "if",    "else",  "elif",  "return", "by",
                            "from", "import", "to",    "print", "match", "false", "true"};

  const token_type types[] = {FN,   FOR,    WHILE, IF,    ELSE,  ELSE_IF, RETURN, BY,
                              FROM, IMPORT, TO,    PRINT, MATCH, FALSE,   TRUE};

  for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
    intern_string(interns, keywords[i], strlen(keywords[i]), types[i]);
  }
}

static lexer *init_lexer(void) {
  lexer *l = malloc(sizeof(lexer));
  l->indent_style = UNSET;
  l->col = 0;
  l->line = 1;
  l->tokens = create_vector(sizeof(token), 128);
  l->indent_stack[0] = 0;
  l->current_indent = 0;
  l->indent_sp = 1;
  l->interns = create_intern_table(128, 0.7);

  add_language_keywords(l->interns);
  return l;
}

static void parse_indent(lexer *lexer, char *buffer) {
  int spaces = 0, tabs = 0;
  while (isspace(buffer[lexer->col])) {
    if (buffer[lexer->col] == ' ') {
      spaces++;
    } else if (buffer[lexer->col] == '\t') {
      tabs++;
    }
    lexer->col++;
  }

  if (buffer[lexer->col] == ';') {
    while (buffer[lexer->col] != '\n' && buffer[lexer->col] != '\0')
      lexer->col++;
    return;
  }

  if (buffer[lexer->col] == '\n' || buffer[lexer->col] == '\0')
    return;

  if (lexer->indent_style == UNSET && ((spaces > 0) != (tabs > 0))) {
    if (spaces > 0) {
      lexer->indent_style = SPACES;
      lexer->spaces_per_level = spaces;
    } else {
      lexer->indent_style = TABS;
    }
  }

  if (spaces > 0 && tabs > 0) {
    fprintf(stderr, "use of tabs and spaces at %d:%d, which is forbidden.\n", lexer->line,
            lexer->col);
    exit(1);
  }

  if (lexer->indent_style == SPACES) {
    if (spaces % lexer->spaces_per_level != 0) {
      fprintf(stderr, "inconsistent space indentation near %d:%d. expected multiple of %d.\n",
              lexer->line, lexer->col, lexer->spaces_per_level);
      exit(1);
    }
    lexer->current_indent = spaces / lexer->spaces_per_level;
  } else {
    lexer->current_indent = tabs;
  }

  if (lexer->current_indent > lexer->indent_stack[lexer->indent_sp - 1]) {
    if (lexer->current_indent != lexer->indent_stack[lexer->indent_sp - 1] + 1) {
      fprintf(stderr, "invalid indentation increase at line %d\n", lexer->line);
      exit(1);
    }
    if (lexer->indent_sp >= MAX_INDENT_LEVEL) {
      fprintf(stderr, "max indentation depth exceeded at line %d\n", lexer->line);
      exit(1);
    }
    lexer->indent_stack[lexer->indent_sp] = lexer->current_indent;
    lexer->indent_sp++;

    add_token_null(lexer, INDENT);
  } else {
    while (lexer->indent_sp > 1 &&
           lexer->indent_stack[lexer->indent_sp - 1] > lexer->current_indent) {
      lexer->indent_sp--;
      add_token_null(lexer, DEDENT);
    }

    if (lexer->indent_stack[lexer->indent_sp - 1] != lexer->current_indent) {
      fprintf(stderr, "error: invalid dedent level at line %d\n", lexer->line);
      exit(1);
    }
  }
}

const char *token_type_str(token_type t) {
  switch (t) {
  case FN:
    return "fn";
  case FOR:
    return "for";
  case WHILE:
    return "while";
  case IF:
    return "if";
  case ELSE:
    return "else";
  case ELSE_IF:
    return "else_if";
  case RETURN:
    return "return";
  case BY:
    return "by";
  case FROM:
    return "from";
  case IMPORT:
    return "import";
  case TO:
    return "to";
  case PRINT:
    return "print";
  case MATCH:
    return "match";
  case NOT:
    return "not";
  case AND:
    return "and";
  case OR:
    return "or";
  case FALSE:
    return "false";
  case TRUE:
    return "true";

  case MUL:
    return "mul";
  case DIV:
    return "div";
  case INT_DIV:
    return "int_div";
  case ADD:
    return "add";
  case SUB:
    return "sub";
  case ADD_ONE:
    return "add_one";
  case SUB_ONE:
    return "sub_one";
  case EQ:
    return "eq";
  case COMP_EQ:
    return "comp_eq";
  case ADD_EQ:
    return "add_eq";
  case SUB_EQ:
    return "sub_eq";
  case MUL_EQ:
    return "mul_eq";
  case DIV_EQ:
    return "div_eq";
  case INTDIV_EQ:
    return "intdiv_eq";
  case GT:
    return "gt";
  case LT:
    return "lt";
  case GTE:
    return "gte";
  case LTE:
    return "lte";
  case CARROT:
    return "carrot";
  case CARROT_EQ:
    return "carrot_eq";

  case IDENTIFIER:
    return "identifier";
  case STRING:
    return "string";
  case INTEGER:
    return "number";
  case FLOAT:
    return "float";

  case COMMA:
    return "comma";
  case LPAREN:
    return "lparen";
  case RPAREN:
    return "rparen";
  case LSQPAREN:
    return "lsqparen";
  case RSQPAREN:
    return "rsqparen";

  case INDENT:
    return "indent";
  case DEDENT:
    return "dedent";
  case NEWLINE:
    return "newline";
  case END:
    return "end";

  default:
    return "unknown_token";
  }
}

void print_token(const token *token) {
  printf("%s %s @ %d:%d\n", token_type_str(token->type), token->ident ? token->ident : "",
         token->line, token->col);
}

static trie_node *initialize_trie(void) {
  const symbol_entry symbols[] = {
      {"+", ADD},        {"++", ADD_ONE},    {"+=", ADD_EQ},  {"-", SUB},        {"--", SUB_ONE},
      {"-=", SUB_EQ},    {"*", MUL},         {"*=", MUL_EQ},  {"/", DIV},        {"/=", DIV_EQ},
      {"//", INT_DIV},   {"//=", INTDIV_EQ}, {"^", CARROT},   {"^=", CARROT_EQ}, {"%", MODULO},
      {">", GT},         {">=", GTE},        {"<", LT},       {"<=", LTE},       {">>", RBITSHIFT},
      {"<<", LBITSHIFT}, {"~", BITW_NOT},    {"&", BITW_AND}, {"|", BITW_OR},    {"==", COMP_EQ},
      {"=", EQ},         {"!=", NOT_EQ},     {"&&", AND},     {"||", OR},        {"!", NOT},
      {"(", LPAREN},     {")", RPAREN},      {"[", LSQPAREN}, {"]", RSQPAREN},   {",", COMMA}};

  trie_node *node = create_trie_node();
  for (int i = 0; i < (int)(sizeof(symbols) / sizeof(symbol_entry)); i++)
    insert_symbol(node, symbols[i].symbol, symbols[i].type);

  return node;
}

static int issymbol(char c) {
  return c == '%' || c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '!' ||
         c == '<' || c == '>' || c == '&' || c == '|' || c == '^' || c == '(' || c == ')' ||
         c == '[' || c == ']' || c == ',';
}

static char handle_escape_sequence(char c) {
  switch (c) {
  case 'n':
    return '\n';
  case 't':
    return '\t';
  case '\\':
    return '"';
  case '\'':
    return '\'';
  default:
    fprintf(stderr, "unknown escape sequence '\\%c'\n", c);
    return '\0';
  }
}

static void parse_string(lexer *lexer, char *buffer, size_t bytes_read) {
  lexer->col++;
  char string_buffer[MAX_STRING_LEN];
  int sb_index = 0;

  while (lexer->col < (int)bytes_read) {
    char c = buffer[lexer->col];
    if (c == '\\') {
      lexer->col++;
      if (lexer->col >= (int)bytes_read) {
        fprintf(stderr, "unterminated string at line %d\n", lexer->line);
        exit(1);
      }
      char escaped_char = handle_escape_sequence(buffer[lexer->col]);
      if (sb_index < MAX_STRING_LEN - 1)
        string_buffer[sb_index++] = escaped_char;
      else {
        fprintf(stderr, "string too long\n");
        exit(1);
      }
    } else if (c == '"') {
      lexer->col++;
      break;
    } else {
      if (sb_index < MAX_STRING_LEN - 1)
        string_buffer[sb_index++] = c;
      else {
        fprintf(stderr, "string too long\n");
        exit(1);
      }
    }
    lexer->col++;

    if (lexer->col >= (int)bytes_read && buffer[lexer->col - 1] != '"') {
      fprintf(stderr, "unterminated string at line %d\n", lexer->line);
      exit(1);
    }
  }

  string_buffer[sb_index] = '\0';
  add_token_len(lexer, STRING, string_buffer, sb_index);
}

static void parse_number(lexer *lexer, char *buffer, size_t bytes_read) {
  int start = lexer->col;
  bool is_float = false;

  while (lexer->col < (int)bytes_read && buffer[lexer->col] != '\n' &&
         (isdigit(buffer[lexer->col]) || buffer[lexer->col] == '.')) {
    if (buffer[lexer->col] == '.') {
      if (is_float) {
        fprintf(stderr, "malformed number at line %d:%d\n", lexer->line, lexer->col);
        exit(1);
      }
      is_float = true;
    }
    lexer->col++;
  }

  int length = lexer->col - start;

  add_token_len(lexer, is_float ? FLOAT : INTEGER, buffer + start, length);
}

static void parse_symbol(lexer *lexer, trie_node *root, char *buffer, size_t bytes_read) {
  int start = lexer->col;
  while (lexer->col < (int)bytes_read && issymbol(buffer[lexer->col])) {
    lexer->col++;
  }

  int length = lexer->col - start;
  char tmp[8];
  if (length >= 8) {
    fprintf(stderr, "symbol too long at line %d col %d\n", lexer->line, lexer->col);
    exit(1);
  }
  memcpy(tmp, buffer + start, length);
  tmp[length] = '\0';

  match_result res = search_trie(root, tmp);
  if (res.length == 0) {
    fprintf(stderr, "invalid symbol '%s' at line %d col %d\n", tmp, lexer->line, lexer->col);
    exit(1);
  } else {
    add_token_null(lexer, res.type);
  }
}

static void parse_identifier(lexer *lexer, char *buffer, size_t bytes_read) {
  int start = lexer->col;
  while (
      lexer->col < (int)bytes_read &&
      (isalpha(buffer[lexer->col]) || isdigit(buffer[lexer->col]) || buffer[lexer->col] == '_')) {
    lexer->col++;
  }
  int length = lexer->col - start;

  intern_result res = intern_string(lexer->interns, buffer + start, length, IDENTIFIER);

  if (res.value == IDENTIFIER) {
    add_token_len(lexer, res.value, buffer + start, length);
  } else {
    add_token_null(lexer, res.value);
  }
}

lexer_result *lex(const char *filename) {
  file_streamer *streamer = create_streamer(filename);
  lexer *lexer = init_lexer();
  trie_node *root = initialize_trie();

  char buffer[MAX_LINE];
  size_t bytes_read;

  while ((bytes_read = stream_line(streamer, buffer)) > 0) {
    lexer->col = 0;
    parse_indent(lexer, buffer);

    if (buffer[lexer->col] == '\n' || buffer[lexer->col] == '\0') {
      lexer->line++;
      continue;
    }

    while (lexer->col < (int)bytes_read) {
      char c = buffer[lexer->col];

      if (c == ';') {
        break;
      } else if (isspace(c)) {
        lexer->col++;
        continue;
      } else if (isalpha(c) || c == '_') {
        parse_identifier(lexer, buffer, bytes_read);
      } else if (isdigit(c)) {
        parse_number(lexer, buffer, bytes_read);
      } else if (c == '"') {
        parse_string(lexer, buffer, bytes_read);
      } else if (issymbol(c)) {
        parse_symbol(lexer, root, buffer, bytes_read);
      } else {
        lexer->col++;
      }
    }

    add_token_null(lexer, NEWLINE);
    lexer->line++;
  }

  add_token_null(lexer, END);

  destroy_streamer(streamer);
  free_trie(root);

  static lexer_result lr;
  lr.interns = lexer->interns;
  lr.tokens = lexer->tokens;
  return &lr;
}
