#include "parser.h"

#include <stdlib.h>
#include "error.h"
#include "tokenizer.h"
#include <stdio.h>
#include "utility.h"

ParseRule rules[] = {
    { parse_grouping, parse_lambda_call, PREC_CALL }, // TOKEN_LPAREN
    { NULL, NULL, PREC_NONE }, // TOKEN_RPAREN
    { parse_block, NULL, PREC_NONE }, // TOKEN_LBRACE
    { NULL, NULL, PREC_NONE }, // TOKEN_RBRACE
    { NULL, NULL, PREC_NONE }, // TOKEN_LBRACK
    { NULL, NULL, PREC_NONE }, // TOKEN_RBRACK
    { NULL, NULL, PREC_NONE }, // TOKEN_COMMA
    { NULL, NULL, PREC_NONE }, // TOKEN_COLON
    { NULL, NULL, PREC_NONE }, // TOKEN_COLON_COLON
    { NULL, NULL, PREC_NONE }, // TOKEN_UNDERSCORE
    { NULL, parse_accessor, PREC_CALL }, // TOKEN_DOT
    { parse_unary, parse_binary, PREC_TERM }, // TOKEN_MINUS
    { NULL, parse_binary, PREC_TERM }, // TOKEN_PLUS
    { NULL, parse_binary, PREC_FACTOR }, // TOKEN_SLASH
    { NULL, parse_binary, PREC_FACTOR }, // TOKEN_STAR
    { parse_unary, NULL, PREC_UNARY }, // TOKEN_NOT
    { NULL, parse_binary, PREC_COMPARISON }, // TOKEN_NEQ
    { NULL, parse_binary, PREC_COMPARISON }, // TOKEN_EQ_EQ
    { NULL, parse_assign, PREC_ASSIGNMENT }, // TOKEN_EQ
    { NULL, parse_binary, PREC_COMPARISON }, // TOKEN_GT
    { NULL, parse_binary, PREC_COMPARISON }, // TOKEN_GTE
    { NULL, parse_binary, PREC_COMPARISON }, // TOKEN_LT
    { NULL, parse_binary, PREC_COMPARISON }, // TOKEN_LTE
    { NULL, NULL, PREC_NONE }, // TOKEN_PIPE
    { NULL, NULL, PREC_NONE }, // TOKEN_THIN_ARROW
    { NULL, NULL, PREC_NONE }, // TOKEN_ARROW
    { parse_var, NULL, PREC_NONE }, // TOKEN_IDENT
    { NULL, NULL, PREC_NONE }, // TOKEN_STR
    { parse_number, NULL, PREC_NONE }, // TOKEN_NUM
    { parse_var_decl, NULL, PREC_ASSIGNMENT }, // TOKEN_VAR
    { parse_lambda, NULL, PREC_NONE }, // TOKEN_FN
    { parse_pattern_matching, NULL, PREC_NONE }, // TOKEN_MATCH
    { parse_type_decl, NULL, PREC_NONE }, // TOKEN_TYPE_KEYWORD
    { parse_return, NULL, PREC_NONE }, // TOKEN_RETURN
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
    int match = peek_token(p, t);
    if (match) advance_parser(p);
    return match;
}

int peek_token(Parser *p, TokenType t) {
    if (p->cur->type == t) {
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

static Ast *record_type_to_destr(Ast *type) {
    Ast * lit = create_ast(AST_LITERAL, type->assoc_token);
    for (size_t i = 0; i < type->num_children; i++) {
        Ast *child = get_child(type, i);
        if (child->type == AST_TYPE) {
            char *slot_num = calloc(num_digits(i) + 1, sizeof(char));
            sprintf(slot_num, "%lu", i);
            add_child(lit, create_ast(AST_VARIABLE, child->assoc_token));
        } else if (child->type == AST_WILDCARD) {
            add_child(lit, create_ast(AST_WILDCARD, child->assoc_token));
        } else if (get_child(child, 0)->type == AST_PRODUCT_TYPE) {
            add_child(lit, record_type_to_destr(get_child(child, 0)));
        } else {
            add_child(lit, create_ast(AST_VARIABLE, child->assoc_token));
        }
    }
    return lit;
}

static Ast *type_to_destr(Ast *type) {
    Ast *destr = 0;
    if (type->num_children != 0) {
        if (type->num_children == 1) {
            // Variable declaration
            const Token * var_name = get_child(type, 0)->assoc_token;
            destr = create_ast(AST_VAR_DECL, var_name);

            // only one type
            Ast *ptype = get_child(get_child(type, 0), 0);
            Ast *copy = create_ast(ptype->type, ptype->assoc_token);
            destroy_ast(type);
            free(type);
            type = 0;

            type = copy;
        } else {
            destr = create_ast(AST_DESTR_DECL, type->assoc_token);
            add_child(destr, record_type_to_destr(type));
        }
        Ast *placehold = create_ast(AST_PLACEHOLD, type->assoc_token);
        add_child(placehold, type);
        add_child(destr, placehold);
    }
    return destr;
}

Ast *parse(Parser *parser, const char *code, const char *src_name) {
    ctor_parser(parser, code, src_name);
    advance_parser(parser);
    Ast *prog = create_ast(AST_PROG, parser->cur);
    while (parser->cur->type != TOKEN_END) {
        add_child(prog, parse_expression(parser));
    }

    return prog;
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
    Token *paren = parser->prev;
    Ast *expr = parse_expression(parser);

     if (match_token(parser, TOKEN_COMMA)) {
        Ast *tuple = create_ast(AST_LITERAL, paren);
        expr = add_child(tuple, expr);
        do {
            Ast *ele = parse_expression(parser);
            if (!ele) {
                error(parser->prev->line,
                    INSUFFICIENT_TUPLE,
                    "expected element after comma for tuple\n");
                *parser->enc_err = 1;
                return expr;
            }
            add_child(tuple, ele);
        } while (match_token(parser, TOKEN_COMMA));
    }
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

static Ast *parse_product_type(Parser *parser) {
    Ast *type = create_ast(AST_PRODUCT_TYPE, parser->prev);
    if (!match_token(parser, TOKEN_RPAREN)) {
        do {
            add_child(type, parse_type(parser));
        } while (match_token(parser, TOKEN_COMMA));
        consume_token(parser, TOKEN_RPAREN, "Expected token ')'.\n");
    }
    return type;
}

Ast *parse_type(Parser *parser) {
    Ast *type = NULL;
    if (match_token(parser, TOKEN_IDENT)) {
        Token *ident = parser->prev;
        if (match_token(parser, TOKEN_COLON)) {
            Ast *keyval = create_ast(AST_KEYVAL, ident);
            type = add_child(keyval, parse_type(parser));
        } else {
            type = create_ast(AST_TYPE, ident);
        }
    } else if (match_token(parser, TOKEN_UNDERSCORE)) {
        type = create_ast(AST_WILDCARD, parser->prev);
    } else if (match_token(parser, TOKEN_LPAREN)) {
        type = parse_product_type(parser);
    }
    if (match_token(parser, TOKEN_PIPE)) {
        type = add_child( create_ast(AST_SUM_TYPE, parser->prev), type);
        add_child(type, parse_type(parser));
    }
    if (match_token(parser, TOKEN_ARROW)) {
        type = add_child( create_ast(AST_LAMBDA_TYPE, parser->prev), type);
        add_child(type, parse_type(parser));
    }

    if (type == NULL) {
        error(parser->prev->line, UNEXPECTED_TOKEN, "Expected type identifier.\n");
        *parser->enc_err = 1;
    }
    return type;
}

static Ast *parse_destr_tuple(Parser *parser) {
    Ast *ast = create_ast(AST_LITERAL, parser->prev);
    do {
        if (match_token(parser, TOKEN_UNDERSCORE)) {
            add_child(ast, create_ast(AST_WILDCARD, parser->prev));
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
        decl = create_ast(AST_DESTR_DECL, parser->prev);
        add_child(decl, parse_destr_tuple(parser));
    } else {
        error(parser->cur->line, UNEXPECTED_TOKEN, "Unexpected token.\n");
        *parser->enc_err = 1;
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
    Token *literal = parser->prev;
    if (match_token(parser, TOKEN_COLON)) {
        Ast *keyval = create_ast(AST_KEYVAL, literal);
        return add_child(keyval, parse_expression(parser));
    }
    return create_ast(AST_VARIABLE, literal);
}

Ast *parse_block(Parser *parser) {
    Ast *block = create_ast(AST_BLOCK, parser->prev);
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

Ast *parse_pattern(Parser *parser) {
    Ast *pattern = create_ast(AST_PATTERN, parser->prev);
    Ast *pseudo_return = create_ast(AST_RET_EXPR, parser->prev);
    Ast *ret_block = create_ast(AST_BLOCK, parser->prev);
    if (match_token(parser, TOKEN_NUM) || match_token(parser, TOKEN_STR)) {
        add_child(pattern, create_ast(AST_LITERAL, parser->prev));
    } else {
        Ast *type = parse_type(parser);
        if (!type) {
            return NULL;
        } else if (type->type == AST_WILDCARD) {
            add_child(pattern, type);
        } else if (type->type == AST_PRODUCT_TYPE) {
            add_child(pattern, copy_ast(type));
            add_child(ret_block, type_to_destr(type));
        } else {
            add_child(pattern, type);
        }
    }
    consume_token(parser,
        TOKEN_THIN_ARROW,
        "Expected '->' to denote pattern behaviour.\n");
    add_child(pattern, pseudo_return);
    add_child(pseudo_return, ret_block);
    add_child(ret_block, parse_expression(parser));
    return pattern;
}

Ast *parse_pattern_matching(Parser *parser) {
    Ast *match_ast = create_ast(AST_PATTERN_MATCH, parser->prev);
    add_child(match_ast, parse_expression(parser));
    Ast *block = create_ast(AST_BLOCK, parser->prev);
    add_child(match_ast, block);
    while (match_token(parser, TOKEN_PIPE)) {
        Ast *pattern = parse_pattern(parser);
        if (!pattern) break;
        add_child(block, pattern);
    }

    return match_ast;
}

Ast *parse_accessor(Parser *parser) {
    Ast *accessor = create_ast(AST_ACCESSOR, parser->prev);
    if (match_token(parser, TOKEN_IDENT))
        add_child(accessor, create_ast(AST_VARIABLE, parser->prev));
    else if (match_token(parser, TOKEN_NUM)) {
        add_child(accessor, create_ast(AST_LITERAL, parser->prev));
    } else {
        error(parser->cur->line,
            UNEXPECTED_TOKEN,
            "Expected a number or identifier for accessor.\n");
        *parser->enc_err = 1;
    }
    return accessor;
}

Ast *parse_lambda(Parser *parser) {
    Ast *lambda = create_ast(AST_LAMBDA_LIT, parser->prev);
    Ast *block  = create_ast(AST_BLOCK, parser->prev);
    add_child(lambda, block);

    if (match_token(parser, TOKEN_LPAREN)) {
        Ast *type = parse_product_type(parser);
        if (type->num_children != 0) add_child(block, type_to_destr(type));
    } else {
        error(parser->cur->line,
            TYPE_ERROR,
            "Must supply product type to lambda.\n");
        *parser->enc_err = 1;
        destroy_ast(lambda);
        return NULL;
    }
    consume_token(parser,
        TOKEN_ARROW,
        "Lambda expected '=>' to denote body of lambda.\n");
    add_child(block, parse_expression(parser));

    return lambda;
}

Ast *parse_lambda_call(Parser *parser) {
    Ast *lambda_call = create_ast(AST_LAMBDA_CALL, parser->prev);
    return add_child(lambda_call, parse_grouping(parser));
}

Ast *parse_assign(Parser *parser) {
    Ast *assign = create_ast(AST_ASSIGN, parser->prev);
    return add_child(assign, parse_expression(parser));
}

Ast *parse_return(Parser *parser) {
    Ast *ret = create_ast(AST_RET_EXPR, parser->prev);
    return add_child(ret, parse_expression(parser));
}

Ast *parse_type_decl(Parser *parser) {
    consume_token(parser, TOKEN_IDENT, "Expect identifier of type.\n");
    Ast *type = create_ast(AST_TYPE_DECL, parser->prev);
    if (match_token(parser, TOKEN_EQ)) {
        add_child(type, parse_expression(parser));
    }
    if (match_token(parser, TOKEN_COLON_COLON)) {
        add_child(type, parse_type(parser));
    }
    return type;
}
