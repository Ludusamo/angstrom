#ifndef AST_H
#define AST_H

#include <list.h>
#include "tokenizer.h"
#include "ang_type.h"

#define AST_TYPE_LIST(code) \
    code(PROG) \
    code(LITERAL) \
    code(BLOCK) \
    code(EQ_OP) \
    code(COMP_OP) \
    code(ADD_OP) \
    code(MUL_OP) \
    code(UNARY_OP) \
    code(VAR_DECL) \
    code(TYPE_DECL) \
    code(DESTR_DECL) \
    code(TYPE) \
    code(PRODUCT_TYPE) \
    code(SUM_TYPE) \
    code(ACCESSOR) \
    code(VARIABLE) \
    code(WILDCARD) \
    code(RET_EXPR) \
    code(KEYVAL) \
    code(PLACEHOLD) \
    code(LAMBDA_LIT) \
    code(LAMBDA_CALL) \
    code(PATTERN_MATCH) \
    code(PATTERN)

#define DEFINE_ENUM_TYPE(type) type,
typedef enum {
    AST_TYPE_LIST(DEFINE_ENUM_TYPE)
} Ast_Type;
#undef DEFINE_ENUM_TYPE

typedef struct {
    Ast_Type type;
    List nodes;
    const Token *assoc_token;
    const Ang_Type *eval_type;
} Ast;

typedef struct {
    int current;
    List tokens;
    int *enc_err;
} Parser;

void ctor_parser(Parser *p);
void dtor_parser(Parser *p);

const char *ast_type_to_str(Ast_Type t);
void print_ast(const Ast *ast, int depth);

Ast *create_ast(Ast_Type t, const Token *assoc_token);

const Token *advance_token(Parser *parser);
const Token *peek_token(const Parser *parser, int peek);
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
Ast *parse_type_decl(Parser *parser);
Ast *parse_lambda_call(Parser *parser);
Ast *parse_match(Parser *parser);
Ast *parse_decl(Parser *parser);
Ast *parse_destr_decl(Parser *parser);
Ast *parse_type(Parser *parser);
Ast *parse_lambda(Parser *parser);
Ast *parse_block(Parser *parser);
Ast *parse_return(Parser *parser);
Ast *parse_primary(Parser *parser);
Ast *parse_accessor(Parser *parser, Ast *prev);
void destroy_ast(Ast *ast);

Ast *parse(Parser *p, const char *code, const char *src_name);

Ast *get_child(const Ast *ast, int child_i);

#endif // AST_H
