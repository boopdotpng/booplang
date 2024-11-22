#include "lexer.h"
#include "utils.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

static LexerState *init_lexer_state()
{
    LexerState *state = malloc(sizeof(LexerState));
    state->partial_capacity = 64;
    state->partial_token = malloc(state->partial_capacity);
    state->partial_len = 0;
    state->partial_state= P_UNKNOWN;
    state->start_line = false;
    state->in_comment = false;
    state->has_dot = false;
    return state;
}

static Lexer *init_lexer()
{
    Lexer *l = malloc(sizeof(Lexer));
    l->indent_style = UNSET;
    l->token_capacity = 256;
    l->col = 0;
    l->line = 1;
    l->tokens = malloc(sizeof(Token) * l->token_capacity);
    l->indent_stack[0] = 0;
    l->current_indent = 0;
    l->indent_sp = 1;
    l->token_count = 0;
    return l;
}

static void add_token(Lexer *lexer, Token token)
{
    if (lexer->token_count+1 >= lexer->token_capacity)
    {
        lexer->token_capacity *= 2;
        lexer->tokens = realloc(lexer->tokens, sizeof(Token) * lexer->token_capacity);
    }
    lexer->tokens[lexer->token_count++] = token;
}

static void append_to_partial(Lexer* lexer, LexerState *state, char c)
{
    if (state->partial_len + 1 >= state->partial_capacity)
    {
        fprintf(stderr, "Exceeded maximum identifier length at line %d", lexer->line);
        exit(1);
    }
    state->partial_token[state->partial_len++] = c;
    state->partial_token[state->partial_len] = '\0';
}

static void reset_partial(LexerState *state)
{
    state->partial_len = 0;
    state->partial_token[0] = '\0';
    state->partial_state= P_UNKNOWN;
    state->has_dot = 0;
    state->in_comment = false;
    state->has_dot = false;
    state->start_line = false;
}

Lexer *lex(const char *filename)
{
    FileStreamer *streamer = create_streamer(filename);
    Lexer *lexer = init_lexer();
    LexerState *state = init_lexer_state();

    char buffer[CHUNK_SIZE];
    size_t bytes_read;

    while ((bytes_read = stream_chunk(streamer, buffer)) > 0)
    {
        for(size_t i = 0; i < bytes_read; i++)
        {
            putc(buffer[i], stdout);
        }
    }

    // Clean up
    free(state->partial_token);
    free(state);
    destroy_streamer(streamer);

    return lexer;
}

static const char *token_type_str(TokenType t)
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
    case FLOAT:
        return "float";
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