#pragma once

// all the token types
typedef enum {
    // keywords 
    DEFINE, FOR, WHILE, IF, ELSE, ELSE_IF, LET,
    BE, IS, GT, LT, LTE, GTE, NE, EQ, RETURN,
    NOT, AND, OR,

    // literals
    IDENTIFIER, STRING, NUMBER, 

    // single characters
    COLON, FSLASH, COMMA, LPAREN, RPAREN,

    // scope
    INDENT, DEDENT, NEWLINE,

    END, UNKNOWN
} TokenType; 

// represents an actual token
typedef struct {
    TokenType type; 
    char *ident; // if its not a token, its probably a variable name or function name
    int col;
    int line;
    int indent_level;
} Token;

typedef struct {
    char *line; // the current line being processed
    int indent_style;
    int spaces_per_indent;
    int token_count;
    int token_capacity;
    int col;
    int line_number;
    int indent_level; 
    Token *tokens;
} Lexer;

void print_token(Token *token);
const char *token_type_str(TokenType t);
void add_token(Lexer *lexer, Token token);
char *next(Lexer *lexer);
char *parse_number(Lexer *lexer);
void parse_indent(Lexer *lexer);
char *peek(Lexer *lexer, int ahead);
Lexer *lex(const char *filename);
void lex_line(Lexer *lexer);
Lexer *init_lexer();