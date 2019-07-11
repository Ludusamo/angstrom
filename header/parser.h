#ifndef PARSER_H
#define PARSER_H

#include "ang_ast.h"

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY
} Precedence;

typedef struct {
    Token *cur;
    Token *prev;
    Token **tokens;
    int num_tokens;
    size_t cap;
    Scanner scanner;
    int *enc_err;
} Parser;

typedef Ast *(*ParseFn)(Parser *parser);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;


/** Retrieves a parse rule from the global parse table
 */
ParseRule *get_rule(TokenType type);

/** Advances parser to the next token
 */
void advance_parser(Parser *p);

int match_token(Parser *p, TokenType t);

int peek_token(Parser *p, TokenType t);

/** Advances and consumes an expected token
 * If the next token does not match type t, this is an error and the error_msg
 * is shown
 */
void consume_token(Parser *p, TokenType t, const char *error_msg);

void ctor_parser(Parser *parser, const char *code, const char *src_name);
void dtor_parser(Parser *parser);

Ast *parse(Parser *parser, const char *code, const char *src_name);

Ast *parse_precedence(Parser *parser, Precedence precedence);

Ast *parse_expression(Parser *parser);
Ast *parse_grouping(Parser *parser);
Ast *parse_number(Parser *parser);
Ast *parse_unary(Parser *parser);
Ast *parse_binary(Parser *parser);
Ast *parse_type(Parser *parser);
Ast *parse_var_decl(Parser *parser);
Ast *parse_var(Parser *parser);
Ast *parse_block(Parser *parser);
Ast *parse_pattern(Parser *parser);
Ast *parse_pattern_matching(Parser *parser);

#endif /* ifndef PARSER_H */
