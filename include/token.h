// had to be separated to avoid circular dependencies
#pragma once

typedef enum {
    // keywords
    FN, FOR, WHILE, IF, ELSE, ELSE_IF,
    IS, RETURN, BY, FROM, IMPORT, TO,
    PRINT, MATCH, // since print doesn't require parenthesis

    // operators
    NOT, AND, OR, FALSE, TRUE, MODULU,
    MUL, DIV, INT_DIV, ADD, SUB, ADD_ONE, SUB_ONE, // ++, --
    EQ, COMP_EQ, ADD_EQ, SUB_EQ, MUL_EQ, DIV_EQ, INTDIV_EQ, // ==
    GT, LT, GTE, LTE, CARROT, CARROT_EQ,

    // literals
    IDENTIFIER, STRING, INTEGER, FLOAT, MULTILINE_STR,

    // single characters
    COMMA, LPAREN, RPAREN, LSQPAREN, RSQPAREN,

    // scope
    INDENT, DEDENT, NEWLINE,

    // misc.
    END
} token_type;