#include "parser.h"

#include <stdlib.h>
#include "error.h"
#include "tokenizer.h"
#include <stdio.h>

ParseRule rules[] = {
    { parse_grouping, NULL, PREC_NONE }, // TOKEN_LPAREN
    { NULL, NULL, PREC_NONE }, // TOKEN_RPAREN
    { parse_block, NULL, PREC_NONE }, // TOKEN_LBRACE
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
    { parse_var, NULL, PREC_NONE }, // TOKEN_IDENT
    { NULL, NULL, PREC_NONE }, // TOKEN_STR
    { parse_number, NULL, PREC_NONE }, // TOKEN_NUM
    { parse_var_decl, NULL, PREC_ASSIGNMENT }, // TOKEN_VAR
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
    if (p->num_tokens == p->cap) {
        p->cap *= 2;
        p->tokens = realloc(p->tokens, p->cap * sizeof(Token));
    }
    p->prev = p->cur;
    p->tokens[p->num_tokens] = scan_token(&p->scanner);
    p->cur = p->tokens[p->num_tokens++];
    #if DEBUG
    print_token(p->cur);
    #endif
}

int match_token(Parser *p, TokenType t) {
    if (p->cur->type == t) {
        advance_parser(p);
        return 1;
    }
    return 0;
}

void consume_token(Parser *p, TokenType t, const char *error_msg) {
    if (!match_token(p, t)) {
        error(p->cur->line, UNEXPECTED_TOKEN, error_msg);
        *p->enc_err = 1;
    }
}

void ctor_parser(Parser *parser, const char *code, const char *src_name) {
    ctor_scanner(&parser->scanner, code, src_name);
    parser->num_tokens = 0;
    parser->cap = 2;
    parser->tokens = calloc(parser->cap, sizeof(Token*));
    parser->cur = NULL;
    parser->prev = NULL;
}

void dtor_parser(Parser *parser) {
    dtor_scanner(&parser->scanner);
    for (int i = 0; i < parser->num_tokens; i++) {
        free(parser->tokens[i]->lexeme);
        free(parser->tokens[i]);
    }
    if (parser->tokens) {
        free(parser->tokens);
    }
}

Ast *parse(Parser *parser, const char *code, const char *src_name) {
    ctor_parser(parser, code, src_name);
    advance_parser(parser);
    return parse_expression(parser);
}

Ast *parse_precedence(Parser *parser, Precedence precedence) {
    advance_parser(parser);
    ParseFn prefix_rule = get_rule(parser->prev->type)->prefix;
    if (parser->prev->type == TOKEN_END) return NULL;
    if (prefix_rule == NULL) {
        fprintf(stderr, "received unexpected token %s\n", parser->prev->lexeme);
        return NULL;
    }

    Ast *prefix = prefix_rule(parser);

    while (precedence <= get_rule(parser->cur->type)->precedence) {
        advance_parser(parser);
        ParseFn infix_rule = get_rule(parser->prev->type)->infix;
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
    const Token *op = parser->prev;
    advance_parser(parser);

    ParseFn prefix = get_rule(parser->prev->type)->prefix;
    Ast *unary = create_ast(AST_UNARY_OP, op);
    add_child(unary, prefix(parser));

    return unary;
}

Ast *parse_binary(Parser *parser) {
    const Token *op = parser->prev;
    Ast *op_ast = create_ast(AST_ADD_OP, parser->prev);

    ParseRule *rule = get_rule(op->type);
    Ast *rhs = parse_precedence(parser, (Precedence) (rule->precedence + 1));
    add_child(op_ast, rhs);

    if (op->type == TOKEN_PLUS || op->type == TOKEN_MINUS) {
        return op_ast;
    } else if (op->type == TOKEN_STAR || op->type == TOKEN_SLASH) {
        op_ast->type = AST_MUL_OP;
        return op_ast;
    }

    error(op->line, UNEXPECTED_TOKEN, "Invalid operator.\n");
    destroy_ast(op_ast);
    return NULL;
}

Ast *parse_type(Parser *parser) {
    if (match_token(parser, TOKEN_IDENT)) {
        return create_ast(AST_TYPE, parser->prev);
    }
    if (match_token(parser, TOKEN_PIPE)) {
        return add_child(
            create_ast(AST_SUM_TYPE, parser->prev),
            parse_type(parser));
    }

    // TODO: Error log
    return NULL;
}

static Ast *parse_destr_tuple(Parser *parser) {
    Ast *ast = create_ast(AST_LITERAL, NULL);
    do {
        if (match_token(parser, TOKEN_UNDERSCORE)) {
            add_child(ast, create_ast(AST_WILDCARD, NULL));
        } else if (match_token(parser, TOKEN_IDENT)) {
            add_child(ast, create_ast(AST_VARIABLE, parser->prev));
        } else if (match_token(parser, TOKEN_LPAREN)) {
            add_child(ast, parse_destr_tuple(parser));
        } else {
            error(parser->cur->line,
                UNEXPECTED_TOKEN,
                "Destructure tuple can only contain wildcard '_' or identifiers to bind.\n");
            *parser->enc_err = 1;
        }
    } while (match_token(parser, TOKEN_COMMA));
    consume_token(parser, TOKEN_RPAREN, "Expected token ')'.");
    return ast;
}

Ast *parse_var_decl(Parser *parser) {
    Ast *decl = NULL;
    if (match_token(parser, TOKEN_IDENT)) {
        decl = create_ast(AST_VAR_DECL, parser->prev);
    } else if (match_token(parser, TOKEN_LPAREN)) {
        decl = create_ast(AST_DESTR_DECL, NULL);
        add_child(decl, parse_destr_tuple(parser));
    } else {
        error(parser->cur->line, UNEXPECTED_TOKEN, "Unexpected token.\n");
        return NULL;
    }
    if (match_token(parser, TOKEN_EQ)) {
        add_child(decl, parse_expression(parser));
    }
    if (match_token(parser, TOKEN_COLON_COLON)) {
        add_child(decl, parse_type(parser));
    }
    return decl;
}

Ast *parse_var(Parser *parser) {
    return create_ast(AST_VARIABLE, parser->prev);
}

Ast *parse_block(Parser *parser) {
    Ast *block = create_ast(AST_BLOCK, NULL);
    for (;;) {
        if (match_token(parser, TOKEN_RBRACE)) {
            break;
        } else if (match_token(parser, TOKEN_END)) {
            error(parser->cur->line,
                UNCLOSED_BLOCK,
                "Block did not have closing '}'.\n");
            *parser->enc_err = 1;
            break;
        } else {
            add_child(block, parse_expression(parser));
        }
    }
    return block;
}
