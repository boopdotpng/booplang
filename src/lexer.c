#include "lexer.h"
#include "utils.h"
#include <stdlib.h>
#include <ctype.h>

// a file can be indented using spaces or tabs, but it must be consistent 
// spaces are just set using a number (2 for two spaces, etc)
#define TAB 1
#define SPACE 2
#define UNSET 0 

// max number of nested indents
#define INDENT_STACK_CAPACITY 32 

Lexer *lex(const char *filename) {
    FileStreamer *streamer = create_streamer(filename);
    Lexer *lexer = init_lexer();
    
    while ((lexer->line = stream_line(streamer))) {
        lex_line(lexer);
    }

    destroy_streamer(streamer);

    return lexer;
}

void lex_line(Lexer *lexer){
    parse_indent(lexer);

    Token token = {
        .type=ELSE,
        .col=10,
        .ident="hello",
        .line=12
    };
    add_token(lexer, token);
    lexer->line_number++;
}

Lexer *init_lexer(){ 
    Lexer *l = malloc(sizeof(Lexer));
    l->indent_style = UNSET;
    l->token_capacity = 256;
    l->col = 0;
    l->line_number = 1;
    l->tokens = malloc(sizeof(Token) * l->token_capacity);
    l->indent_level = 0;
    return l;
}

void add_token(Lexer *lexer, Token token) {
    if (lexer->token_count+1 >= lexer->token_capacity) {
        lexer->token_capacity *= 2;
        lexer->tokens = realloc(lexer->tokens, sizeof(Token) * lexer->token_capacity);
    }
    lexer->tokens[lexer->token_count++] = token;
}

char *next(Lexer *lexer) {
    // while not whitespace, create buffer
    char *buffer = "hi"; 




    return buffer;
}

char *parse_number(Lexer *lexer){
    return "3.1415";
}

void parse_indent(Lexer *lexer) {
    int spaces = 0, tabs = 0;
    while (isspace(lexer->line[lexer->col])) {
        if (lexer->line[lexer->col] == ' ') spaces++;
        if (lexer->line[lexer->col] == '\t') tabs++;
        lexer->col++;
    }
    printf("%d %d\n", spaces, tabs);
}

char *peek(Lexer *lexer, int ahead) {
    return "hello";
}

const char *token_type_str(TokenType t) {
    switch (t) {
        case DEFINE: return "define";
        case FOR: return "for";
        case WHILE: return "while";
        case IF: return "if";
        case ELSE: return "else";
        case ELSE_IF: return "else if";
        case LET: return "let";
        case BE: return "be";
        case IS: return "is";
        case GT: return ">";
        case LT: return "<";
        case LTE: return "<=";
        case GTE: return ">=";
        case NE: return "!=";
        case EQ: return "==";
        case RETURN: return "return";
        case NOT: return "not";
        case AND: return "and";
        case OR: return "or";
        case IDENTIFIER: return "identifier";
        case STRING: return "string";
        case NUMBER: return "number";
        case COLON: return ":";
        case FSLASH: return "/";
        case COMMA: return ",";
        case LPAREN: return "(";
        case RPAREN: return ")";
        case INDENT: return "indent";
        case DEDENT: return "dedent";
        case NEWLINE: return "newline";
        case UNKNOWN: return "unknown";
        case END: return "eof";
        default: return "unknown";
    }
}

void print_token(Token *token) {
    printf("type=%s, ident=%s, col=%u, line=%u\n", token_type_str(token->type), token->ident, token->col, token->line);
}

