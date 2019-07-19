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
    code(ASSIGN) \
    code(TYPE) \
    code(PARAMETRIC_TYPE) \
    code(PRODUCT_TYPE) \
    code(SUM_TYPE) \
    code(LAMBDA_TYPE) \
    code(ARRAY_TYPE) \
    code(ACCESSOR) \
    code(ACCESS_ARR) \
    code(VARIABLE) \
    code(WILDCARD) \
    code(RET_EXPR) \
    code(KEYVAL) \
    code(PLACEHOLD) \
    code(ARRAY) \
    code(LAMBDA_LIT) \
    code(LAMBDA_CALL) \
    code(PATTERN_MATCH) \
    code(PATTERN)

#define DEFINE_ENUM_TYPE(type) AST_##type,
typedef enum {
    AST_TYPE_LIST(DEFINE_ENUM_TYPE)
} Ast_Type;
#undef DEFINE_ENUM_TYPE

typedef struct Ast_t {
    Ast_Type type;
    struct Ast_t **children;
    int num_children;
    int capacity;
    Token *assoc_token;
    const Ang_Type *eval_type;
} Ast;

const char *ast_type_to_str(Ast_Type t);
void print_ast(const Ast *ast, int depth);

Ast *create_ast(Ast_Type t, const Token *assoc_token);
Ast *copy_ast(Ast *ast);

void destroy_ast(Ast *ast);

Ast *add_child(Ast *parent, Ast *child);
Ast *get_child(const Ast *ast, int child_i);

#endif // AST_H
