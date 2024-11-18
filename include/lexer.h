#pragma once

// all the token types
typedef enum {
    DEFINE,
    FOR,
    WHILE,
    IF,
    ELSE,
    ELSE_IF,
    LET,
    QUES,
    COLON,
    FSLASH,
    PERIOD,
    COMMA,
    INDENT,
    BE,
    IS,
    GT,
    LT,
    LTE,
    GTE,
    NE,
    EQ,
    RETURN,
    OVER,
    LPAREN,
    RPAREN,
} TokenType; 

// represents an actual token
typedef struct {
    TokenType type; 
    char *ident; 
} Token;