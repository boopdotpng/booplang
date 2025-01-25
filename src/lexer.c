#include "intern.h"
#include "lexer.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vector.h"
#include "trie.h"

// a file can be indented using spaces or tabs, but it must be consistent
// spaces are just set using a number (2 for two spaces, etc)
#define TABS 1
#define SPACES 2
#define UNSET 0

// max number of nested indents
#define MAX_INDENT_LEVEL 32
// max length for strings
#define MAX_STRING_LEN 256

typedef struct {
    const char *symbol;
    token_type type;
} symbol_entry;

struct lexer {
    int indent_style;       // space or tab
    int spaces_per_level;   // for space-based indentation
    int line;
    int col;
    int current_indent;     // current indent level
    int indent_stack[MAX_INDENT_LEVEL];
    int indent_sp;          // stack pointer for indent_stack
    vector *tokens;
    intern_table *interns; // string intern table
};

// tokens that don't require an identifier
static void add_token_null(lexer *lexer, token_type type) {
    token new_token = {
        .type = type,
        .ident = NULL,
        .col = lexer->col,
        .line = lexer->line
    };

    add_element(lexer->tokens, &new_token);
}

// add tokens with an identifier
static void add_token_len(lexer *lexer, token_type type, const char *ptr, size_t length) {
    intern_result result = intern_string(lexer->interns, ptr, length, type);

    token new_token = {
        .type = result.value,  // token_type
        .ident = result.key,   // the interned string
        .col = lexer->col,
        .line = lexer->line
    };

    add_element(lexer->tokens, &new_token);
}

// add all language keywords to the intern table
static void add_language_keywords(intern_table *interns) {
    // list of reserved language keywords
    const char *keywords[] = {
        "fn", "for", "while", "if", "else", "elif",
        "return", "by", "from", "import", "to",
        "print", "match", "false", "true"
    };

    // corresponding token types
    const token_type types[] = {
        FN, FOR, WHILE, IF, ELSE, ELSE_IF,
        RETURN, BY, FROM, IMPORT, TO,
        PRINT, MATCH, FALSE, TRUE
    };

    size_t keyword_count = sizeof(keywords) / sizeof(keywords[0]);

    for (size_t i = 0; i < keyword_count; i++) {
        intern_string(interns, keywords[i], strlen(keywords[i]), types[i]);
    }
}

static lexer *init_lexer() {
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

    // comment detection
    if (buffer[lexer->col] == ';') {
        while (buffer[lexer->col] != '\n' && buffer[lexer->col] != '\0')
            lexer->col++;
        return;
    }

    // skip empty lines
    if (buffer[lexer->col] == '\n' || buffer[lexer->col] == '\0')
        return;

    // first indent?
    if (lexer->indent_style == UNSET && ((spaces > 0) != (tabs > 0))) {
        if (spaces > 0) {
            lexer->indent_style = SPACES;
            lexer->spaces_per_level = spaces;
        } else {
            lexer->indent_style = TABS;
        }
    }

    // check for mixed tab and space usage
    if (spaces > 0 && tabs > 0) {
        fprintf(stderr, "use of tabs and spaces at %d:%d, which is forbidden.\n",
                lexer->line, lexer->col);
        exit(1);
    }

    // ensure consistent spacing
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

    // handle indent
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
        // handle dedent
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
    // keywords
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
        return "indiv_eq";
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
    printf("type=%s, ident=%s, line=%d, col=%d\n",
           token_type_str(token->type),
           token->ident ? token->ident : "(null)",
           token->line,
           token->col);
}

// trie initialization
static trie_node *intialize_trie() {
    const symbol_entry symbols[] = {
        {"+", ADD},   {"++", ADD_ONE},    {"+=", ADD_EQ},
        {"-", SUB},   {"--", SUB_ONE},    {"-=", SUB_EQ},
        {"*", MUL},   {"*=", MUL_EQ},
        {"/", DIV},   {"/=", DIV_EQ},     {"//", INT_DIV},   {"//=", INTDIV_EQ},
        {"^", CARROT},{"^=", CARROT_EQ},
        {"%", MODULO},
        {">", GT},    {">=", GTE},       {"<", LT},        {"<=", LTE}, {">>", RBITSHIFT}, {"<<", LBITSHIFT},
        {"~", BITW_NOT},{"&", BITW_AND},{"|", BITW_OR},
        {"==", COMP_EQ}, {"=", EQ}, {"!=", NOT_EQ},
        {"&&", AND},  {"||", OR},        {"!", NOT},
        {"(", LPAREN},{")", RPAREN},
        {"[", LSQPAREN},{"]", RSQPAREN},
        {",", COMMA}
    };

    trie_node *node = create_trie_node();
    for (int i = 0; i < (int)(sizeof(symbols) / sizeof(symbol_entry)); i++)
        insert_symbol(node, symbols[i].symbol, symbols[i].type);

    return node;
}

static bool issymbol(char c) {
    return c == '%' || c == '+' || c == '-' || c == '*' || c == '/' ||
           c == '=' || c == '!' || c == '<' || c == '>' ||
           c == '&' || c == '|' || c == '^' ||
           c == '(' || c == ')' || c == '[' || c == ']' ||
           c == ',';
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
    // skip initial "
    lexer->col++;

    // TODO: dynamically allocate this or print a useful error
    // when the buffer overflows
    char string_buffer[MAX_STRING_LEN];
    int sb_index = 0;

    while (lexer->col < (int)bytes_read) {
        char c = buffer[lexer->col];
        if (c == '\\') {
            // escape
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
            // end of string
            lexer->col++;
            break;
        } else {
            // normal char
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
    // add token with full string
    add_token_len(lexer, STRING, string_buffer, sb_index);
}

static void parse_number(lexer *lexer, char *buffer, size_t bytes_read) {
    int start = lexer->col;
    bool is_float = false;

    while (lexer->col < (int)bytes_read &&
            buffer[lexer->col] != '\n' &&
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

// ++, --, etc
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
    if (res.length == 0 && res.type == -1) {
        fprintf(stderr, "invalid symbol '%s' at line %d col %d\n", tmp, lexer->line, lexer->col);
        exit(1);
    } else {
        add_token_null(lexer, res.type);
    }
}

// variable names, function names, and reserved language keywords
static void parse_identifier(lexer *lexer, char *buffer, size_t bytes_read) {
    int start = lexer->col;
    while (lexer->col < (int)bytes_read &&
            (isalpha(buffer[lexer->col]) || isdigit(buffer[lexer->col]) || buffer[lexer->col] == '_')) {
        lexer->col++;
    }
    int length = lexer->col - start;

    // search intern table to check if ident is reserved
    // the value is only updated if the key does not exist. anything that doesn't exist in the table has to be a user created ident
    intern_result res = intern_string(lexer->interns, buffer+start, length, IDENTIFIER);

    if (res.value == IDENTIFIER) {
        add_token_len(lexer, res.value, buffer+start, length);
    } else {
        add_token_null(lexer, res.value);
    }
}

lexer_result *lex(const char *filename) {
    file_streamer *streamer = create_streamer(filename);
    lexer *lexer = init_lexer();
    trie_node *root = intialize_trie();

    char buffer[MAX_LINE];
    size_t bytes_read;

    while ((bytes_read = stream_line(streamer, buffer)) > 0) {
        lexer->col = 0;
        // handle indentation per line
        parse_indent(lexer, buffer);

        // ignore empty lines after indentation
        if (buffer[lexer->col] == '\n' || buffer[lexer->col] == '\0') {
            lexer->line++;
            continue;
        }

        while (lexer->col < (int)bytes_read) {
            char c = buffer[lexer->col];
            if (c == ';') {
                // comment => skip rest of line
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

        // end of line => add newline token
        add_token_null(lexer, NEWLINE);
        lexer->line++;
    }

    // add an END token
    add_token_null(lexer, END);

    destroy_streamer(streamer);
    free_trie(root);

    static lexer_result lr;
    lr.interns = lexer->interns;
    lr.tokens = lexer->tokens;
    return &lr;
}