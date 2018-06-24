#include "ast.h"

#include "error.h"
#include "string.h"
#include "utility.h"
#include <stdio.h>

void ctor_parser(Parser *p) {
    p->current = 0;
    ctor_list(&p->tokens);
}

void dtor_parser(Parser *p) {
    destroy_tokens(&p->tokens);
    dtor_list(&p->tokens);
}

#define DEFINE_CODE_STRING(type) case type: return #type;
const char *ast_type_to_str(Ast_Type t) {
    switch (t) {
        AST_TYPE_LIST(DEFINE_CODE_STRING)
    }
}
#undef DEFINE_CODE_STRING

void print_ast(const Ast *ast, int depth) {
    if (!ast) return;
    printf("%*s%s\n", depth * 4, "", ast_type_to_str(ast->type));
    for (size_t i = 0; i < ast->nodes.length; i++) {
        print_ast((Ast *) get_ptr(access_list(&ast->nodes, i)), depth + 1);
    }
}

Ast *create_ast(Ast_Type t, const Token *assoc_token) {
    Ast *ast = calloc(1, sizeof(Ast));
    ast->type = t;
    ast->assoc_token = assoc_token;
    ctor_list(&ast->nodes);
    return ast;
}

Ast *copy_ast(Ast *ast) {
    Ast *copy = create_ast(ast->type, ast->assoc_token);
    for (size_t i = 0; i < ast->nodes.length; i++) {
        append_list(&copy->nodes, from_ptr(copy_ast(get_child(ast, i))));
    }
    copy->eval_type = ast->eval_type;
    return copy;
}

const Token *advance_token(Parser *parser) {
    return (Token *) get_ptr(access_list(&parser->tokens, parser->current++));
}

const Token *peek_token(const Parser *parser, int peek) {
    if (parser->current + peek - 1 >= parser->tokens.length) return 0;
    return (Token *) get_ptr(
        access_list(&parser->tokens, parser->current + --peek));
}

const Token *previous_token(const Parser *parser) {
    return (Token *) get_ptr(access_list(&parser->tokens, parser->current - 1));
}

int check(const Parser *parser, Token_Type type) {
    const Token *t = peek_token(parser, 1);
    return t && t->type == type;
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
    error(peek_token(parser, 1)->line, UNEXPECTED_TOKEN, err);
    *parser->enc_err = 1;
    synchronize(parser);
    return 0;
}

int at_end(const Parser *parser) {
    return parser->current >= parser->tokens.length;
}

void synchronize(Parser *parser) {
    advance_token(parser);
    while (!at_end(parser)) {
        switch(peek_token(parser, 1)->type) {
        case VAR:
        case FN:
        case MATCH:
        case TEOF:
        case RBRACE:
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

static Ast *record_type_to_destr(Ast *type) {
    Ast *lit = create_ast(LITERAL, type->assoc_token);
    for (size_t i = 0; i < type->nodes.length; i++) {
        Ast *child = get_child(type, i);
        if (child->type == TYPE) {
            char *slot_num = calloc(num_digits(i) + 1, sizeof(char));
            sprintf(slot_num, "%lu", i);
            append_list(&lit->nodes,
                from_ptr(create_ast(VARIABLE, child->assoc_token)));
        } else if (child->type == WILDCARD) {
            append_list(&lit->nodes,
                from_ptr(create_ast(WILDCARD, child->assoc_token)));
        } else if (get_child(child, 0)->type == PRODUCT_TYPE) {
            append_list(&lit->nodes,
                from_ptr(record_type_to_destr(get_child(child, 0))));
        } else {
            append_list(&lit->nodes,
                from_ptr(create_ast(VARIABLE, child->assoc_token)));
        }
    }
    return lit;
}

static Ast *type_to_destr(Ast *type) {
    Ast *destr = 0;
    if (type->nodes.length != 0) {
        if (type->nodes.length == 1) {
            // Variable declaration
            const Token *var_name = get_child(type, 0)->assoc_token;
            destr = create_ast(VAR_DECL, var_name);

            // only one type
            Ast *ptype = get_child(get_child(type, 0), 0);
            Ast *copy = create_ast(ptype->type, ptype->assoc_token);

            destroy_ast(type);
            free(type);
            type = 0;

            type = copy;
        } else {
            destr = create_ast(DESTR_DECL, 0);
            append_list(&destr->nodes,
                from_ptr(record_type_to_destr(type)));
        }
        Ast *placehold = create_ast(PLACEHOLD, 0);
        append_list(&placehold->nodes, from_ptr(type));
        append_list(&destr->nodes, from_ptr(placehold));
    }
    return destr;
}

Ast *parse_expression(Parser *parser) {
    Ast *expr = parse_equality(parser);
    return expr;
}

Ast *parse_equality(Parser *parser) {
    Ast *expr = parse_comparison(parser);
    while (match_token(parser, NEQ) || match_token(parser, EQ_EQ)) {
        Ast *new_expr = create_ast(EQ_OP, previous_token(parser));
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
        Ast *new_expr = create_ast(COMP_OP, previous_token(parser));
        append_list(&new_expr->nodes, from_ptr(expr));
        append_list(&new_expr->nodes, from_ptr(parse_addition(parser)));
        expr = new_expr;
    }

    return expr;
}

Ast *parse_addition(Parser *parser) {
    Ast *expr = parse_multiplication(parser);

    while (match_token(parser, PLUS) || match_token(parser, MINUS)) {
        Ast *new_expr = create_ast(ADD_OP, previous_token(parser));
        append_list(&new_expr->nodes, from_ptr(expr));
        Value v = from_ptr(parse_multiplication(parser));
        append_list(&new_expr->nodes, v);
        expr = new_expr;
    }

    return expr;
}

Ast *parse_multiplication(Parser *parser) {
    Ast *expr = parse_unary(parser);

    while (match_token(parser, STAR) || match_token(parser, SLASH)) {
        Ast *new_expr = create_ast(MUL_OP, previous_token(parser));
        append_list(&new_expr->nodes, from_ptr(expr));
        append_list(&new_expr->nodes, from_ptr(parse_unary(parser)));
        expr = new_expr;
    }

    return expr;
}

Ast *parse_unary(Parser *parser) {
    if (match_token(parser, NOT) || match_token(parser, MINUS)) {
        Ast *expr = create_ast(UNARY_OP, previous_token(parser));
        append_list(&expr->nodes, from_ptr(parse_decl(parser)));
        return expr;
    }
    return parse_type_decl(parser);
}

Ast *parse_type_decl(Parser *parser) {
    if (match_token(parser, TYPE_KEYWORD)) {
        if (consume_token(parser, IDENT, "Expected identifier.\n")) {
            Ast *expr = create_ast(TYPE_DECL, previous_token(parser));
            if (consume_token(parser, COLON_COLON, "Expected double colon.\n")) {
                append_list(&expr->nodes, from_ptr(parse_type(parser)));
                return expr;
            } else free(expr);
        }
        return 0;
    }
    return parse_lambda_call(parser);
}

Ast *parse_lambda_call(Parser *parser) {
    Ast *expr = parse_match(parser);
    while (!at_end(parser) && peek_token(parser, 1)->type == LPAREN) {
        Ast *lambda_call = create_ast(LAMBDA_CALL, previous_token(parser));
        append_list(&lambda_call->nodes, from_ptr(expr));
        append_list(&lambda_call->nodes, from_ptr(parse_match(parser)));
        expr = lambda_call;
    }
    return expr;
}

Ast *parse_match(Parser *parser) {
    if (match_token(parser, MATCH)) {
        Ast *match = create_ast(PATTERN_MATCH, previous_token(parser));
        Ast *block = create_ast(BLOCK, 0);
        append_list(&match->nodes, from_ptr(parse_expression(parser)));
        append_list(&match->nodes, from_ptr(block));
        while (match_token(parser, PIPE)) {

            Ast *pattern = create_ast(PATTERN, previous_token(parser));
            append_list(&block->nodes, from_ptr(pattern));

            // Wrap the match code in a return
            // so it can use block construct to jump out
            Ast *pseudo_return = create_ast(RET_EXPR, 0);
            Ast *ret_block = create_ast(BLOCK, 0);

            if (match_token(parser, NUM) || match_token(parser, STR)) {
                Ast *num_lit = create_ast(LITERAL, previous_token(parser));
                append_list(&pattern->nodes, from_ptr(num_lit));
            } else {
                Ast *type = parse_type(parser);
                if (!type) {
                    return match;
                } else if (type->type == WILDCARD) {
                    append_list(&pattern->nodes,
                        from_ptr(create_ast(WILDCARD, 0)));
                } else if (type->type == PRODUCT_TYPE) {
                    append_list(&pattern->nodes, from_ptr(copy_ast(type)));
                    append_list(&ret_block->nodes, from_ptr(type_to_destr(type)));
                } else {
                    append_list(&pattern->nodes, from_ptr(type));
                }
            }
            consume_token(parser,
                THIN_ARROW,
                "Expected '->' to denote pattern behaviour.\n");

            append_list(&pattern->nodes, from_ptr(pseudo_return));
            append_list(&pseudo_return->nodes, from_ptr(ret_block));
            append_list(&ret_block->nodes, from_ptr(parse_expression(parser)));
        }
        return match;
    }
    return parse_decl(parser);
}

Ast *parse_decl(Parser *parser) {
    if (match_token(parser, VAR)) {
        Ast *expr = calloc(1, sizeof(Ast));
        ctor_list(&expr->nodes);
        if (peek_token(parser, 1)->type == LPAREN) {
            expr->type = DESTR_DECL;
            expr->assoc_token = previous_token(parser);
            append_list(&expr->nodes, from_ptr(parse_destr_decl(parser)));
        } else if (match_token(parser, IDENT)) {
            expr->type = VAR_DECL;
            expr->assoc_token = previous_token(parser);
        } else {
            int lineno = peek_token(parser, 1)->line;
            error(lineno, UNEXPECTED_TOKEN, "Expected identifier.\n");
            *parser->enc_err = 1;
            synchronize(parser);
            dtor_list(&expr->nodes);
            free(expr);
            return 0;
        }
        if (match_token(parser, EQ)) {
            append_list(&expr->nodes, from_ptr(parse_expression(parser)));
        }
        if (match_token(parser, COLON_COLON)) {
            append_list(&expr->nodes, from_ptr(parse_type(parser)));
        }
        return expr;
    }
    return parse_block(parser);
}

Ast *parse_destr_decl(Parser *parser) {
    Ast *literal_node = create_ast(LITERAL, previous_token(parser));
    consume_token(parser, LPAREN, "Expected to initial \"(\".\n");
    while (peek_token(parser, 1)->type != RPAREN) {
        if (match_token(parser, IDENT)) {
            Ast *ident = create_ast(VARIABLE, previous_token(parser));
            append_list(&literal_node->nodes, from_ptr(ident));
        } else if (match_token(parser, UNDERSCORE)) {
            Ast *wildcard = create_ast(WILDCARD, previous_token(parser));
            append_list(&literal_node->nodes, from_ptr(wildcard));
        } else if (peek_token(parser, 1)->type == LPAREN) {
            append_list(&literal_node->nodes, from_ptr(parse_destr_decl(parser)));
        } else {
            int lineno = peek_token(parser, 1)->line;
            error(lineno, UNEXPECTED_TOKEN, "Encountered unexpected token.\n");
            *parser->enc_err = 1;
            synchronize(parser);
        }

        if (!match_token(parser, COMMA)) {
            break;
        }
    }
    consume_token(parser, RPAREN, "Expected \")\" after expression.\n");
    return literal_node;
}

static Ast *parse_product_type(Parser *parser) {
    if (!consume_token(parser,
            LPAREN,
            "Cannot parse product type not beginning with paren\n")) {
        return 0;
    }
    Ast *expr = create_ast(PRODUCT_TYPE, previous_token(parser));
    while (peek_token(parser, 1)->type != RPAREN) {
        append_list(&expr->nodes, from_ptr(parse_type(parser)));
        if (peek_token(parser, 1)->type == COMMA)
            consume_token(parser,
                COMMA,
                "Expected ',' in between tuple elements.\n");
        else if (peek_token(parser, 1)->type == TEOF) {
            error(peek_token(parser, 1)->line,
                UNCLOSED_TUPLE,
                "Tuple type missing closing ')'\n");
            *parser->enc_err = 1;
            synchronize(parser);
            destroy_ast(expr);
            return 0;
        }
    }
    consume_token(parser, RPAREN, "Panic...");
    return expr;
}

Ast *parse_type(Parser *parser) {
    Ast *expr = 0;
    if (match_token(parser, IDENT)) {
        expr = create_ast(TYPE, previous_token(parser));
        if (match_token(parser, COLON)) {
            expr->type = KEYVAL;
            append_list(&expr->nodes, from_ptr(parse_type(parser)));
        }
    } else if (match_token(parser, UNDERSCORE)) {
        expr = create_ast(WILDCARD, previous_token(parser));
    } else if (peek_token(parser, 1)->type == LPAREN) {
        expr = parse_product_type(parser);
    } else {
        int lineno = peek_token(parser, 1)->line;
        error(lineno, UNEXPECTED_TOKEN, "Expected type identifier.\n");
        *parser->enc_err = 1;
        synchronize(parser);
        destroy_ast(expr);
        return 0;
    }

    // Sum Type
    if (match_token(parser, PIPE)) {
        Ast *sum_type = create_ast(SUM_TYPE, previous_token(parser));
        append_list(&sum_type->nodes, from_ptr(expr));
        append_list(&sum_type->nodes, from_ptr(parse_type(parser)));
        expr = sum_type;
    }

    // Lambda Type
    if (match_token(parser, ARROW)) {
        Ast *lambda_type = create_ast(LAMBDA_TYPE, previous_token(parser));
        append_list(&lambda_type->nodes, from_ptr(expr));
        append_list(&lambda_type->nodes, from_ptr(parse_type(parser)));
        expr = lambda_type;
    }
    return expr;
}

Ast *parse_block(Parser *parser) {
    if (match_token(parser, LBRACE)) {
        Ast *expr = create_ast(BLOCK, previous_token(parser));
        while (peek_token(parser, 1)->type != RBRACE) {
            if (peek_token(parser, 1)->type == TEOF) {
                error(peek_token(parser, 1)->line,
                    UNCLOSED_BLOCK,
                    "Block missing closing '}'\n");
                *parser->enc_err = 1;
                synchronize(parser);
                destroy_ast(expr);
                return 0;
            } else {
                append_list(&expr->nodes, from_ptr(parse_expression(parser)));
            }
        }
        consume_token(parser, RBRACE, "Panic...");
        expr = parse_accessor(parser, expr);
        return expr;
    }
    return parse_return(parser);
}

Ast *parse_return(Parser *parser) {
    if (match_token(parser, RETURN)) {
        Ast *expr = create_ast(RET_EXPR, previous_token(parser));
        append_list(&expr->nodes, from_ptr(parse_expression(parser)));
        return expr;
    }
    return parse_lambda(parser);
}

Ast *parse_lambda(Parser *parser) {
    if (match_token(parser, FN)) {
        Ast *lambda = create_ast(LAMBDA_LIT, previous_token(parser));

        // Place everything into its own block
        Ast *block = create_ast(BLOCK, previous_token(parser));
        append_list(&lambda->nodes, from_ptr(block));

        Ast *type = parse_product_type(parser);
        if (type->nodes.length == 0 || type->type == PRODUCT_TYPE) {
            append_list(&block->nodes, from_ptr(type_to_destr(type)));
        } else {
            error(peek_token(parser, 1)->line,
                TYPE_ERROR,
                "Must supply product type to lambda.\n");
            *parser->enc_err = 1;
            synchronize(parser);
            destroy_ast(lambda);
            return 0;
        }

        if (match_token(parser, ARROW)) {
            append_list(&block->nodes, from_ptr(parse_expression(parser)));
        } else {
            error(peek_token(parser, 1)->line,
                UNEXPECTED_TOKEN,
                "Lambda expected '=>' token.\n");
            *parser->enc_err = 1;
            synchronize(parser);
            destroy_ast(lambda);
            return 0;
        }
        return lambda;
    }
    return parse_primary(parser);
}

Ast *parse_primary(Parser *parser) {
    if (match_token(parser, LPAREN)) {
        const Token *paren_token = previous_token(parser);
        Ast *expr = parse_expression(parser);
        if (peek_token(parser, 1)->type == COMMA) {
            Ast *literal_node = create_ast(LITERAL, paren_token);
            append_list(&literal_node->nodes, from_ptr(expr));
            do {
                consume_token(parser,
                    COMMA,
                    "Expected ',' in between tuple elements.\n");
                append_list(&literal_node->nodes, from_ptr(parse_expression(parser)));
            } while (peek_token(parser, 1)->type == COMMA);
            expr = literal_node;
        }
        consume_token(parser, RPAREN, "Expected \")\" after expression.");
        expr = parse_accessor(parser, expr);
        return expr;
    }

    if (match_token(parser, NUM) || match_token(parser, STR)) {
        return create_ast(LITERAL, previous_token(parser));
    }

    if (match_token(parser, IDENT)) {
        Ast *expr = create_ast(VARIABLE, previous_token(parser));
        if (match_token(parser, COLON)) {
            expr->type = KEYVAL;
            append_list(&expr->nodes, from_ptr(parse_expression(parser)));

            // TODO: Single element record
            //if (peek_token(parser, -3)->type == LPAREN) {
            //    Ast *lit = create_ast(LITERAL, peek_token(parser, -2));
            //    append_list(&lit->nodes, from_ptr(expr));
            //    return lit;
            //}
            return expr;
        }
        expr = parse_accessor(parser, expr);
        return expr;
    }

    const Token *t = peek_token(parser, 1);
    int lineno = t->line;
    char error_msg[32 + strlen(t->lexeme)];
    sprintf(error_msg, "Encountered unexpected token %s.\n", t->lexeme);
    error(lineno, UNEXPECTED_TOKEN, error_msg);
    *parser->enc_err = 1;
    synchronize(parser);
    return 0;
}

Ast *parse_accessor(Parser *parser, Ast *prev) {
    while (match_token(parser, DOT)) {
        Ast *acc_node = create_ast(ACCESSOR, previous_token(parser));
        Ast *slot = create_ast(LITERAL, peek_token(parser, 1));
        if (match_token(parser, IDENT)) slot->type = VARIABLE;
        else if (!match_token(parser, NUM)) {
            int lineno = peek_token(parser, 1)->line;
            error(lineno,
                UNEXPECTED_TOKEN,
                "Expected a number or identifier for accessor.\n");
            *parser->enc_err = 1;
        }
        append_list(&acc_node->nodes, from_ptr(prev));
        append_list(&acc_node->nodes, from_ptr(slot));
        prev = acc_node;
    }
    return prev;
}

Ast *parse(Parser *p, const char *code, const char *src_name) {
    if(!tokenize(&p->tokens, code, src_name)) {
        *p->enc_err = 1;
        return 0;
    }
    Ast *prog = calloc(1, sizeof(Ast));
    prog->type = PROG;
    ctor_list(&prog->nodes);
    while (p->current < p->tokens.length && !match_token(p, TEOF)) {
        append_list(&prog->nodes, from_ptr(parse_expression(p)));
    }
    return prog;
}

Ast *get_child(const Ast *ast, int child_i) {
    return get_ptr(access_list(&ast->nodes, child_i));
}
