#include "ast.h"

#include "error.h"
#include "string.h"
#include <stdio.h>

#define DEFINE_CODE_STRING(type) case type: return #type;
const char *ast_type_to_str(Ast_Type t) {
    switch (t) {
        AST_TYPE_LIST(DEFINE_CODE_STRING)
    }
}
#undef DEFINE_CODE_STRING

void print_ast(const Ast *ast, int depth) {
    printf("%*s%s\n", depth * 4, "", ast_type_to_str(ast->type));
    for (size_t i = 0; i < ast->nodes.length; i++) {
        print_ast((Ast *) get_ptr(access_list(&ast->nodes, i)), depth + 1);
    }
}

const Token *advance_token(Parser *parser) {
    return (Token *) get_ptr(access_list(parser->tokens, parser->current++));
}

const Token *peek_token(const Parser *parser) {
    return (Token *) get_ptr(access_list(parser->tokens, parser->current));
}

const Token *previous_token(const Parser *parser) {
    return (Token *) get_ptr(access_list(parser->tokens, parser->current - 1));
}

int check(const Parser *parser, Token_Type type) {
    return peek_token(parser)->type == type;
}

int match_token(Parser *parser, Token_Type type) {
    if (check(parser, type)) {
        advance_token(parser);
        return 1;
    }
    return 0;
}

const Token *consume_token(Parser *parser, Token_Type type, const char *err) {
    if (check(parser, type)) {
        return advance_token(parser);
    }
    error(peek_token(parser)->line, UNEXPECTED_TOKEN, err);
    parser->enc_err = 1;
    return 0;
}

int at_end(const Parser *parser) {
    return check(parser, TEOF);
}

void synchronize(Parser *parser) {
    advance_token(parser);
    while (!at_end(parser)) {
        switch(peek_token(parser)->type) {
        case VAR:
        case FUNC:
            return;
        default:
            break;
        }
        advance_token(parser);
    }
}

void destroy_ast(Ast *ast) {
    if (!ast) return;
    for (size_t i = 0; i < ast->nodes.length; i++) {
        Ast *node = (Ast *) get_ptr(access_list(&ast->nodes, i));
        destroy_ast(node);
        free(node);
    }
    dtor_list(&ast->nodes);
}

Ast *parse_expression(Parser *parser) {
    return parse_equality(parser);
}

Ast *parse_equality(Parser *parser) {
    Ast *expr = parse_comparison(parser);
    while (match_token(parser, NEQ) || match_token(parser, EQ_EQ)) {
        Ast *new_expr = calloc(1, sizeof(Ast));
        new_expr->type = EQ_OP;
        new_expr->assoc_token = previous_token(parser);
        ctor_list(&new_expr->nodes);
        append_list(&new_expr->nodes, from_ptr(expr));
        append_list(&new_expr->nodes, from_ptr(parse_comparison(parser)));
        expr = new_expr;
    }

    return expr;
}

Ast *parse_comparison(Parser *parser) {
    Ast *expr = parse_addition(parser);

    while (match_token(parser, GT) || match_token(parser, LT)
            || match_token(parser, GTE) || match_token(parser, LTE)) {
        Ast *new_expr = calloc(1, sizeof(Ast));
        new_expr->type = COMP_OP;
        new_expr->assoc_token = previous_token(parser);
        ctor_list(&new_expr->nodes);
        append_list(&new_expr->nodes, from_ptr(expr));
        append_list(&new_expr->nodes, from_ptr(parse_addition(parser)));
        expr = new_expr;
    }

    return expr;
}

Ast *parse_addition(Parser *parser) {
    Ast *expr = parse_multiplication(parser);

    while (match_token(parser, PLUS) || match_token(parser, MINUS)) {
        Ast *new_expr = calloc(1, sizeof(Ast));
        new_expr->type = ADD_OP;
        new_expr->assoc_token = previous_token(parser);
        ctor_list(&new_expr->nodes);
        append_list(&new_expr->nodes, from_ptr(expr));
        append_list(&new_expr->nodes, from_ptr(parse_multiplication(parser)));
        expr = new_expr;
    }

    return expr;
}

Ast *parse_multiplication(Parser *parser) {
    Ast *expr = parse_unary(parser);

    while (match_token(parser, STAR) || match_token(parser, SLASH)) {
        Ast *new_expr = calloc(1, sizeof(Ast));
        new_expr->type = MUL_OP;
        new_expr->assoc_token = previous_token(parser);
        ctor_list(&new_expr->nodes);
        append_list(&new_expr->nodes, from_ptr(expr));
        append_list(&new_expr->nodes, from_ptr(parse_unary(parser)));
        expr = new_expr;
    }

    return expr;
}

Ast *parse_unary(Parser *parser) {
    if (match_token(parser, NOT) || match_token(parser, MINUS)) {
        Ast *expr = calloc(1, sizeof(Ast));
        expr->type = UNARY_OP;
        ctor_list(&expr->nodes);
        expr->assoc_token = previous_token(parser);
        append_list(&expr->nodes, from_ptr(parse_decl(parser)));
        return expr;
    }
    return parse_decl(parser);
}

Ast *parse_decl(Parser *parser) {
    if (match_token(parser, VAR)) {
        if (match_token(parser, IDENT)) {
            Ast *expr = calloc(1, sizeof(Ast));
            expr->type = VAR_DECL;
            expr->assoc_token = previous_token(parser);
            ctor_list(&expr->nodes);

            if (match_token(parser, EQ)) {
                append_list(&expr->nodes, from_ptr(parse_expression(parser)));
            }
            if (match_token(parser, COLON)) {
                append_list(&expr->nodes, from_ptr(parse_type(parser)));
            }
            return expr;
        } else {
            int lineno = peek_token(parser)->line;
            error(lineno, UNEXPECTED_TOKEN, "Expected identifier.");
            printf("\n");
            parser->enc_err = 1;
            synchronize(parser);
        }
    }
    return parse_block(parser);
}

Ast *parse_type(Parser *parser) {
    if (match_token(parser, IDENT)) {
        Ast *expr = calloc(1, sizeof(Ast));
        expr->type = TYPE_DECL;
        expr->assoc_token = previous_token(parser);
        ctor_list(&expr->nodes);
        return expr;
    } else {
        int lineno = peek_token(parser)->line;
        error(lineno, UNEXPECTED_TOKEN, "Expected type identifier.\n");
        parser->enc_err = 1;
        synchronize(parser);
    }
    return 0;
}

Ast *parse_block(Parser *parser) {
    if (match_token(parser, LBRACE)) {
        Ast *expr = calloc(1, sizeof(Ast));
        expr->type = BLOCK;
        expr->assoc_token = previous_token(parser);
        ctor_list(&expr->nodes);
        while (peek_token(parser)->type != RBRACE) {
            if (peek_token(parser)->type == TEOF) {
                error(peek_token(parser)->line,
                    UNCLOSED_BLOCK,
                    "Block missing closing '}'\n");
                parser->enc_err = 1;
                return expr;
            } else {
                append_list(&expr->nodes, from_ptr(parse_expression(parser)));
            }
        }
        consume_token(parser, RBRACE, "Panic...");
        return expr;
    }
    return parse_primary(parser);
}

Ast *parse_primary(Parser *parser) {
    if (match_token(parser, LPAREN)) {
        Ast *expr = parse_expression(parser);
        consume_token(parser, RPAREN, "Expected \")\" after expression.");
        return expr;
    }

    if (match_token(parser, NUM) || match_token(parser, STR)) {
        Ast *literal_node = calloc(1, sizeof(Ast));
        const Token *t = previous_token(parser);
        literal_node->type = LITERAL;
        ctor_list(&literal_node->nodes);
        literal_node->assoc_token = t;
        return literal_node;
    }

    int lineno = peek_token(parser)->line;
    error(lineno, UNEXPECTED_TOKEN, "Encountered unknown token.\n");
    parser->enc_err = 1;
    synchronize(parser);
    return 0;
}

Ast *parse(const List *tokens) {
    Parser parser = (Parser) { 0, tokens, 0};
    Ast *prog = calloc(1, sizeof(Ast));
    prog->type = PROG;
    ctor_list(&prog->nodes);

    append_list(&prog->nodes, from_ptr(parse_expression(&parser)));
    return prog;
}

Ast *get_child(const Ast *ast, int child_i) {
    return get_ptr(access_list(&ast->nodes, child_i));
}
