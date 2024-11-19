#include "lexer.h"
#include "utils.h"
#include <stdlib.h>
#include <ctype.h>

Lexer *lex(const char *filename)
{
    FileStreamer *streamer = create_streamer(filename);
    Lexer *lexer = init_lexer();

    while ((lexer->line = stream_line(streamer)))
    {
        lex_line(lexer);
    }

    destroy_streamer(streamer);

    return lexer;
}

void lex_line(Lexer *lexer)
{
    parse_indent(lexer);

    // Token token =
    // {
    //     .type=ELSE,
    //     .col=10,
    //     .ident="hello",
    //     .line=12,
    // };
    // add_token(lexer, token);

    lexer->line_number++;
    lexer->col = 0;
}

Lexer *init_lexer()
{
    Lexer *l = malloc(sizeof(Lexer));
    l->indent_style = UNSET;
    l->token_capacity = 256;
    l->col = 0;
    l->line_number = 1;
    l->tokens = malloc(sizeof(Token) * l->token_capacity);
    l->indent_stack[0] = 0;
    l->current_indent = 0;
    l->indent_sp = 1;
    return l;
}

void add_token(Lexer *lexer, Token token)
{
    if (lexer->token_count+1 >= lexer->token_capacity)
    {
        lexer->token_capacity *= 2;
        lexer->tokens = realloc(lexer->tokens, sizeof(Token) * lexer->token_capacity);
    }
    lexer->tokens[lexer->token_count++] = token;
}

char *next(Lexer *lexer)
{
    // while not whitespace, create buffer
    char *buffer = "hi";




    return buffer;
}

char *parse_number(Lexer *lexer)
{
    return "3.1415";
}

void parse_indent(Lexer *lexer)
{
    int spaces = 0, tabs = 0;
    while (isspace(lexer->line[lexer->col]))
    {
        if (lexer->line[lexer->col] == ' ') spaces++;
        if (lexer->line[lexer->col] == '\t') tabs++;
        lexer->col++;
    }

    // skip empty lines
    if (lexer->line[lexer->col] == '\n' || lexer->line[lexer->col] == '\0') return;

    // first indent?
    if (lexer->indent_style == UNSET && ( (spaces > 0) != (tabs > 0) ))
    {
        if (spaces > 0)
        {
            lexer->indent_style = SPACES;
            lexer->spaces_per_level = spaces;
        }
        else
        {
            lexer->indent_style = TABS;
        }
    }

    // check for mixed tab and space usage
    if (spaces > 0 && tabs > 0)
    {
        fprintf(stderr, "Use of tabs and spaces at %d:%d, which is forbidden.\n", lexer->line_number, lexer->col);
        exit(1);
    }

    // make sure user indents by the same amount each time
    if (lexer->indent_style == SPACES)
    {
        if (spaces % lexer->spaces_per_level != 0)
        {
            fprintf(stderr, "Inconsistent space indentation near %d:%d. Expected a multiple of %d.\n", lexer->line_number, lexer->col, lexer->spaces_per_level);
            exit(1);
        }
        lexer->current_indent = spaces / lexer->spaces_per_level;
    }
    else
    {
        lexer->current_indent = tabs;
    }

    // handle an indent
    if (lexer->current_indent > lexer->indent_stack[lexer->indent_sp-1])
    {
        if (lexer->current_indent != lexer->indent_stack[lexer->indent_sp-1] + 1)
        {
            fprintf(stderr, "Invalid indentation increase at line %d\n", lexer->line_number);
            exit(1);
        }

        if(lexer->indent_sp >= MAX_INDENT_LEVEL)
        {
            fprintf(stderr, "Maximum indentation depth exceeded at line %d\n", lexer->line_number);
            exit(1);
        }
        lexer->indent_stack[lexer->indent_sp] = lexer->current_indent;
        lexer->indent_sp++;

        // add indent token
        Token indent =
        {
            .col=lexer->col,
            .line=lexer->line_number,
            .type=INDENT,
        };
        add_token(lexer, indent);
    }


    // handle an dedent
    else
    {
        // pop indents off stack until we get to the current level
        while (lexer->indent_sp > 1 && lexer->indent_stack[lexer->indent_sp - 1] > lexer->current_indent)
        {
            lexer->indent_sp--;

            Token token =
            {
                .type=DEDENT,
                .line=lexer->line_number,
                .col=lexer->col,
            };
            add_token(lexer, token);
        }

        if (lexer->indent_stack[lexer->indent_sp- 1] != lexer->current_indent)
        {
            fprintf(stderr, "Error: Invalid dedentation level at line %d\n",
                    lexer->line_number);
            exit(1);
        }

    }

}

char *peek(Lexer *lexer, int ahead)
{
    return "hello";
}

const char *token_type_str(TokenType t)
{
    switch (t)
    {
    case DEFINE:
        return "define";
    case FOR:
        return "for";
    case WHILE:
        return "while";
    case IF:
        return "if";
    case ELSE:
        return "else";
    case ELSE_IF:
        return "else if";
    case LET:
        return "let";
    case BE:
        return "be";
    case IS:
        return "is";
    case GT:
        return ">";
    case LT:
        return "<";
    case LTE:
        return "<=";
    case GTE:
        return ">=";
    case NE:
        return "!=";
    case EQ:
        return "==";
    case RETURN:
        return "return";
    case NOT:
        return "not";
    case AND:
        return "and";
    case OR:
        return "or";
    case IDENTIFIER:
        return "identifier";
    case STRING:
        return "string";
    case NUMBER:
        return "number";
    case COLON:
        return ":";
    case FSLASH:
        return "/";
    case COMMA:
        return ",";
    case LPAREN:
        return "(";
    case RPAREN:
        return ")";
    case INDENT:
        return "indent";
    case DEDENT:
        return "dedent";
    case NEWLINE:
        return "newline";
    case UNKNOWN:
        return "unknown";
    case END:
        return "eof";
    default:
        return "unknown";
    }
}

void print_token(Token *token)
{
    printf("type=%s, ident=%s, col=%u, line=%u\n", token_type_str(token->type), token->ident, token->col, token->line);
}
