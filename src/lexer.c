#include "lexer.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vector.h"
#include "intern.h"
#include "trie.h"

// a file can be indented using spaces or tabs, but it must be consistent
// spaces are just set using a number (2 for two spaces, etc)
#define TABS 1
#define SPACES 2
#define UNSET 0

// max number of nested indents
#define MAX_INDENT_LEVEL 32
// max variable size (might not enforce: TODO)
#define IDENTIFIER_SIZE 128

#define PEEK(count) peek((lexer), (buffer), (count), (bytes_read))

// represents an actual token
struct token {
    token_type type;
    char *ident; // if its not a token, its probably a variable name or function name
    int col;
    int line;
};

struct lexer {
    int indent_style; // space or tab
    int spaces_per_level; // indents are relative; the first one
    int col;
    int line;
    int current_indent; // current indent level
    int indent_stack[MAX_INDENT_LEVEL]; // track changes in indentation
    int indent_sp; // stack pointer for indent stack
    bool multiline_str; // """ starts a multiline string, just line in python. needs to be preserved across lines
    vector *tokens;
    intern_table *interns;
};

static lexer *init_lexer() {
    lexer *l = malloc(sizeof(lexer));
    l->indent_style = UNSET;
    l->col = 0;
    l->line = 1;
    l->tokens = create_vector(sizeof(token), 128);
    l->indent_stack[0] = 0;
    l->current_indent = 0;
    l->indent_sp = 1;
    l->multiline_str = FALSE;
    l->interns = create_intern_table(128, 0.7);
    return l;
}

typedef struct {
    const char *symbol;
    token_type type;
} symbol_entry;

static void parse_indent(lexer *lexer, char *buffer) {
    int spaces = 0, tabs = 0;
    while (isspace(buffer[lexer->col])) {
        if (buffer[lexer->col] == ' ') {
            spaces++;
        }
        if (buffer[lexer->col] == '\t') {
            tabs++;
        }
        lexer->col++;
    }

    // comment detection
    if (buffer[lexer->col] == ';') {
        while (buffer[lexer->col] != '\n' && buffer[lexer->col] != '\0') {
            lexer->col++;
        }
        return;
    }

    // skip empty lines
    if (buffer[lexer->col] == '\n' || buffer[lexer->col] == '\0') {
        return;
    }

    // first indent?
    if (lexer->indent_style == UNSET && ( (spaces > 0) != (tabs > 0) )) {
        if (spaces > 0) {
            lexer->indent_style = SPACES;
            lexer->spaces_per_level = spaces;
        } else {
            lexer->indent_style = TABS;
        }
    }

    // check for mixed tab and space usage
    if (spaces > 0 && tabs > 0) {
        fprintf(stderr, "Use of tabs and spaces at %d:%d, which is forbidden.\n", lexer->line, lexer->col);
        exit(1);
    }

    // make sure user indents by the same amount each time
    if (lexer->indent_style == SPACES) {
        if (spaces % lexer->spaces_per_level != 0) {
            fprintf(stderr, "Inconsistent space indentation near %d:%d. Expected a multiple of %d.\n", lexer->line, lexer->col, lexer->spaces_per_level);
            exit(1);
        }
        lexer->current_indent = spaces / lexer->spaces_per_level;
    } else {
        lexer->current_indent = tabs;
    }

    // handle an indent
    if (lexer->current_indent > lexer->indent_stack[lexer->indent_sp-1]) {
        if (lexer->current_indent != lexer->indent_stack[lexer->indent_sp-1] + 1) {
            fprintf(stderr, "Invalid indentation increase at line %d\n", lexer->line);
            exit(1);
        }

        if(lexer->indent_sp >= MAX_INDENT_LEVEL) {
            fprintf(stderr, "Maximum indentation depth exceeded at line %d\n", lexer->line);
            exit(1);
        }
        lexer->indent_stack[lexer->indent_sp] = lexer->current_indent;
        lexer->indent_sp++;

        add_element(lexer->tokens, &(token) {
            .type = INDENT, .ident = NULL, .col = lexer->col, .line = lexer->line
        });
    }

    // handle an dedent
    else {
        // pop indents off stack until we get to the current level
        while (lexer->indent_sp > 1 && lexer->indent_stack[lexer->indent_sp - 1] > lexer->current_indent) {
            lexer->indent_sp--;

            add_element(lexer->tokens, &(token) {
                .type = DEDENT, .ident = NULL, .col = lexer->col, .line = lexer->line
            });
        }

        if (lexer->indent_stack[lexer->indent_sp- 1] != lexer->current_indent) {
            fprintf(stderr, "Error: Invalid dedentation level at line %d\n",
                    lexer->line);
            exit(1);
        }
    }
}

static const char *token_type_str(token_type t) {
    switch (t) {
    // keywords
    case FN:
        return "FN";
    case FOR:
        return "FOR";
    case WHILE:
        return "WHILE";
    case IF:
        return "IF";
    case ELSE:
        return "ELSE";
    case ELSE_IF:
        return "ELSE_IF";
    case IS:
        return "IS";
    case RETURN:
        return "RETURN";
    case BY:
        return "BY";
    case FROM:
        return "FROM";
    case IMPORT:
        return "IMPORT";
    case TO:
        return "TO";
    case PRINT:
        return "PRINT";
    case MATCH:
        return "MATCH";

    // operators
    case NOT:
        return "NOT";
    case AND:
        return "AND";
    case OR:
        return "OR";
    case FALSE:
        return "FALSE";
    case TRUE:
        return "TRUE";
    case MODULU:
        return "MODULU";
    case MUL:
        return "MUL";
    case DIV:
        return "DIV";
    case INT_DIV:
        return "INT_DIV";
    case ADD:
        return "ADD";
    case SUB:
        return "SUB";
    case ADD_ONE:
        return "ADD_ONE";
    case SUB_ONE:
        return "SUB_ONE";
    case EQ:
        return "EQ";
    case COMP_EQ:
        return "COMP_EQ";
    case ADD_EQ:
        return "ADD_EQ";
    case SUB_EQ:
        return "SUB_EQ";
    case MUL_EQ:
        return "MUL_EQ";
    case DIV_EQ:
        return "DIV_EQ";
    case INTDIV_EQ:
        return "INDIV_EQ";
    case GT:
        return "GT";
    case LT:
        return "LT";
    case GTE:
        return "GTE";
    case LTE:
        return "LTE";

    // literals
    case IDENTIFIER:
        return "IDENTIFIER";
    case STRING:
        return "STRING";
    case INTEGER:
        return "NUMBER";
    case FLOAT:
        return "FLOAT";
    case MULTILINE_STR:
        return "MULTILINE_STR";
    // single characters
    case COMMA:
        return "COMMA";
    case LPAREN:
        return "LPAREN";
    case RPAREN:
        return "RPAREN";
    case LSQPAREN:
        return "LSQPAREN";
    case RSQPAREN:
        return "RSQPAREN";

    // scope
    case INDENT:
        return "INDENT";
    case DEDENT:
        return "DEDENT";
    case NEWLINE:
        return "NEWLINE";

    // misc.
    case END:
        return "END";

    // default case for unknown token types
    default:
        return "UNKNOWN_TOKEN";
    }
}

void print_token(const token *token) {
    printf("type=%s, ident=%s, col=%u, line=%u\n", token_type_str(token->type), token->ident, token->col, token->line);
}

// n-character lookahead
static char peek(lexer *l, char *buffer, int count, size_t buffer_size) {
    if (l->col + count < buffer_size)
        return buffer[l->col+count];
    return '\0';
}

// trie intialization
static trie_node *intialize_trie() {
    const symbol_entry symbols[] = {
        {"+", ADD}, {"++", ADD_ONE}, {"+=", ADD_EQ},
        {"-", SUB}, {"--", SUB_ONE}, {"-=", SUB_EQ},
        {"*", MUL}, {"*=", MUL_EQ},
        {"/", DIV}, {"/=", DIV_EQ}, {"//", INT_DIV}, {"//=", INTDIV_EQ},
        {"%", MODULU},
        {">", GT}, {">=", GTE}, {"<", LT}, {"<=", LTE},
        {"==", COMP_EQ}, {"=", EQ},
        {"&&", AND}, {"||", OR}, {"!", NOT},
        {"(", LPAREN}, {")", RPAREN},
        {"[", LSQPAREN}, {"]", RSQPAREN},
        {",", COMMA} 
    };

    trie_node *node = create_trie_node();
    for (int i = 0; i < sizeof(symbols)/sizeof(symbol_entry); i++)
        insert_symbol(node, symbols[i].symbol, symbols[i].type);

    return node;
}

static bool issymbol(char c) {
    return c == '%' || c == '+' || c == '-' || c == '*' || c == '/' || 
           c == '=' || c == '!' || c == '<' || c == '>' || 
           c == '&' || c == '|' || c == '^' || c == '~' || 
           c == '(' || c == ')' || c == '[' || c == ']' || 
           c == '{' || c == '}' || c == ',' || c == '.' || 
           c == ';' || c == ':';
}

static void parse_string(lexer *lexer, char *buffer){

}

static void parse_number(lexer *lexer, char *buffer){

}

static void parse_symbol(lexer *lexer, char *buffer){

}

static parse_identifier(lexer *lexer, char *buffer){

}

lexer_result *lex(const char *filename) {
    file_streamer *streamer = create_streamer(filename);
    lexer *lexer = init_lexer();
    trie_node *root = intialize_trie();

    char buffer[MAX_LINE];
    size_t bytes_read;

    while ((bytes_read = stream_line(streamer, buffer)) > 0) {
        lexer->col=0;
        // actual line parsing
        parse_indent(lexer, buffer);

        while (lexer->col < bytes_read) {
            char c = buffer[lexer->col];
            if (c == ';') // comment character
                break; // skip to the next line
            else if (isspace(c)) {
                lexer->col++;
                continue; 
            }
            else if(isalpha(c) || c == '_') {
                
            }
            else if(isdigit(c)) {

            }
            else if(c == '"') {

            }
            else if(issymbol(c)) {

            }
        }

        add_element(lexer->tokens, &(token) {
            .type = NEWLINE, .ident = NULL, .col = lexer->col, .line = lexer->line
        });
        lexer->line++;
    }

    // TODO: not sure if this needed
    // handle remaining dedents
    // while (lexer->indent_sp > 1)
    // {
    //     lexer->indent_sp--;
    //     Token dedent_token =
    //     {
    //         .type = DEDENT,
    //         .line = lexer->line,
    //         .col = lexer->col,
    //     };
    //     add_token(lexer, DEDENT, NULL, lexer->col, lexer->line);
    // }

    destroy_streamer(streamer);

    return &(lexer_result) {
        .interns=lexer->interns,
        .tokens=lexer->tokens
    };
}