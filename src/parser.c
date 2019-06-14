#include "parser.h"

#include <stdlib.h>
#include "error.h"
#include "tokenizer.h"
#include <stdio.h>

ParseRule rules[] = {
    { parse_grouping, NULL, PREC_NONE }, // TOKEN_LPAREN
    { NULL, NULL, PREC_NONE }, // TOKEN_RPAREN
    { NULL, NULL, PREC_NONE }, // TOKEN_LBRACE
    { NULL, NULL, PREC_NONE }, // TOKEN_RBRACE
    { NULL, NULL, PREC_NONE }, // TOKEN_LBRACK
    { NULL, NULL, PREC_NONE }, // TOKEN_RBRACK
    { NULL, NULL, PREC_NONE }, // TOKEN_COMMA
    { NULL, NULL, PREC_NONE }, // TOKEN_COLON
    { NULL, NULL, PREC_NONE }, // TOKEN_COLON_COLON
    { NULL, NULL, PREC_NONE }, // TOKEN_UNDERSCORE
    { NULL, NULL, PREC_CALL }, // TOKEN_DOT
    { parse_unary, parse_binary, PREC_TERM }, // TOKEN_MINUS
    { NULL, parse_binary, PREC_TERM }, // TOKEN_PLUS
    { NULL, parse_binary, PREC_FACTOR }, // TOKEN_SLASH
    { NULL, parse_binary, PREC_FACTOR }, // TOKEN_STAR
    { parse_unary, parse_binary, PREC_NONE }, // TOKEN_NOT
    { NULL, parse_binary, PREC_NONE }, // TOKEN_NEQ
    { NULL, parse_binary, PREC_NONE }, // TOKEN_EQ_EQ
    { NULL, NULL, PREC_NONE }, // TOKEN_EQ
    { NULL, parse_binary, PREC_NONE }, // TOKEN_GT
    { NULL, parse_binary, PREC_NONE }, // TOKEN_GTE
    { NULL, parse_binary, PREC_NONE }, // TOKEN_LT
    { NULL, parse_binary, PREC_NONE }, // TOKEN_LTE
    { NULL, NULL, PREC_NONE }, // TOKEN_PIPE
    { NULL, NULL, PREC_NONE }, // TOKEN_THIN_ARROW
    { NULL, NULL, PREC_NONE }, // TOKEN_ARROW
    { NULL, NULL, PREC_NONE }, // TOKEN_IDENT
    { NULL, NULL, PREC_NONE }, // TOKEN_STR
    { parse_number, NULL, PREC_NONE }, // TOKEN_NUM
    { NULL, NULL, PREC_NONE }, // TOKEN_VAR
    { NULL, NULL, PREC_NONE }, // TOKEN_FN
    { NULL, NULL, PREC_NONE }, // TOKEN_MATCH
    { NULL, NULL, PREC_NONE }, // TOKEN_TYPE_KEYWORD
    { NULL, NULL, PREC_NONE }, // TOKEN_RETURN
    { NULL, NULL, PREC_NONE }, // TOKEN_END
};

ParseRule *get_rule(TokenType type) {
    return &rules[type];
}

void advance_parser(Parser *p) {
    free(p->prev.lexeme);
    p->prev = p->cur;
    p->cur = scan_token(&p->scanner);
    #if DEBUG
    print_token(&p->cur);
    #endif
}

void consume_token(Parser *p, TokenType t, const char *error_msg) {
    advance_parser(p);
    if (p->cur.type != t) {
        error(p->cur.line, UNEXPECTED_TOKEN, error_msg);
        *p->enc_err = 1;
    }
}

void ctor_parser(Parser *parser, const char *code, const char *src_name) {
    ctor_scanner(&parser->scanner, code, src_name);
    parser->cur.lexeme = 0;
    parser->prev.lexeme = 0;
}

void dtor_parser(Parser *parser) {
    dtor_scanner(&parser->scanner);
    free(parser->cur.lexeme);
    free(parser->prev.lexeme);
}

Ast *parse(Parser *parser, const char *code, const char *src_name) {
    ctor_parser(parser, code, src_name);
    advance_parser(parser);
    return parse_precedence(parser, PREC_ASSIGNMENT);
}

Ast *parse_precedence(Parser *parser, Precedence precedence) {
    advance_parser(parser);
    ParseFn prefix_rule = get_rule(parser->prev.type)->prefix;
    if (parser->prev.type == TOKEN_END) return NULL;
    if (prefix_rule == NULL) {
        fprintf(stderr, "received unexpected token %s\n", parser->prev.lexeme);
        return NULL;
    }

    Ast *prefix = prefix_rule(parser);

    while (precedence <= get_rule(parser->cur.type)->precedence) {
        advance_parser(parser);
        ParseFn infix_rule = get_rule(parser->prev.type)->infix;
        prefix = add_child(infix_rule(parser), prefix);
    }
    return prefix;
}

Ast *parse_expression(Parser *parser) {
    return parse_precedence(parser, PREC_ASSIGNMENT);
}

Ast *parse_grouping(Parser *parser) {
    Ast *expr = parse_expression(parser);
    consume_token(parser, TOKEN_RPAREN, "expected token ')' after grouping");
    return expr;
}

Ast *parse_number(Parser *parser) {
    return create_ast(AST_LITERAL, parser->prev);
}

Ast *parse_unary(Parser *parser) {
    const Token op = parser->prev;
    advance_parser(parser);

    ParseFn prefix = get_rule(parser->prev.type)->prefix;
    Ast *unary = create_ast(AST_UNARY_OP, op);
    add_child(unary, prefix(parser));

    return unary;
}

Ast *parse_binary(Parser *parser) {
    const Token op = parser->prev;
    Ast *op_ast = create_ast(AST_ADD_OP, parser->prev);

    ParseRule *rule = get_rule(op.type);
    Ast *rhs = parse_precedence(parser, (Precedence) (rule->precedence + 1));
    add_child(op_ast, rhs);

    if (op.type == TOKEN_PLUS || op.type == TOKEN_MINUS) {
        return op_ast;
    } else if (op.type == TOKEN_STAR || op.type == TOKEN_SLASH) {
        op_ast->type = AST_MUL_OP;
        return op_ast;
    }

    error(op.line, UNEXPECTED_TOKEN, "Invalid operator.\n");
    destroy_ast(op_ast);
    return NULL;
}
