#ifndef AST_H
#define AST_H

#include <list.h>
#include "tokenizer.h"

#define AST_TYPE_LIST(code) \
    code(PROG) \
    code(LITERAL) \
    code(EQ_OP) \
    code(COMP_OP) \
    code(ADD_OP) \
    code(MUL_OP) \
    code(UNARY_OP) \
    code(VAR_DECL)

#define DEFINE_ENUM_TYPE(type) type,
typedef enum {
    AST_TYPE_LIST(DEFINE_ENUM_TYPE)
} Ast_Type;
#undef DEFINE_ENUM_TYPE

typedef struct {
    Ast_Type type;
    List nodes;
    const Token *assoc_token;
} Ast;

typedef struct {
    int current;
    const List *tokens;
    int enc_err;
} Parser;

const char *ast_type_to_str(Ast_Type t);
void print_ast(const Ast *ast, int depth);

const Token *advance_token(Parser *parser);
const Token *peek_token(const Parser *parser);
const Token *previous_token(const Parser *parser);
int check(const Parser *parser, Token_Type type);
int match_token(Parser *parser, Token_Type type);
const Token *consume_token(Parser *parser, Token_Type type, const char *err);
int at_end(const Parser *parser);

void synchronize(Parser *parser);

Ast *parse_expression(Parser *parser);
Ast *parse_equality(Parser *parser);
Ast *parse_comparison(Parser *parser);
Ast *parse_addition(Parser *parser);
Ast *parse_multiplication(Parser *parser);
Ast *parse_unary(Parser *parser);
Ast *parse_decl(Parser *parser);
Ast *parse_primary(Parser *parser);
Ast *parse_literal(Parser *parser);
void destroy_ast(Ast *ast);

Ast *parse(const List *tokens);

Ast *get_child(const Ast *ast, int child_i);

#endif // AST_H
