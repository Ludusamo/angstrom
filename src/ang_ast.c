#include "ang_ast.h"

#include "error.h"
#include "string.h"
#include "utility.h"
#include <stdio.h>

void print_ast(const Ast *ast, int depth) {
    if (!ast) return;
    printf("%*s%s\n", depth * 4, "", ast_type_to_str(ast->type));
    for (size_t i = 0; i < ast->num_children; i++) {
        print_ast(get_child(ast, i), depth + 1);
    }
}

#define DEFINE_CODE_STRING(type) case AST_##type: return #type;
const char *ast_type_to_str(Ast_Type t) {
    switch(t) {
        AST_TYPE_LIST(DEFINE_CODE_STRING)
    }
}
#undef DEFINE_CODE_STRING

Ast *create_ast(Ast_Type t, Token assoc_token) {
    Ast *ast = calloc(1, sizeof(Ast));
    ast->children = calloc(1, sizeof(Ast *));
    ast->capacity = 1;
    ast->type = t;
    ast->assoc_token = assoc_token;
    return ast;
}

Ast *copy_ast(Ast *ast) {
    Ast *copy = create_ast(ast->type, ast->assoc_token);
    for (size_t i = 0; i < ast->num_children; i++) {
        add_child(copy, copy_ast(get_child(ast, i)));
    }
    copy->eval_type = ast->eval_type;
    return copy;
}

void destroy_ast(Ast *ast) {
    if (!ast) return;
    for (size_t i = 0; i < ast->num_children; i++) {
        Ast *node = ast->children[i];
        destroy_ast(node);
        free(node);
    }
    free(ast->children);
}

Ast *get_child(const Ast *ast, int child_i) {
    return ast->children[child_i];
}

Ast *add_child(Ast *parent, Ast *child) {
    if (parent->num_children == parent->capacity) {
        parent->capacity *= 2;
        parent->children =
            realloc(parent->children, parent->capacity * sizeof(Ast*));
    }
    parent->children[parent->num_children++] = child;
    return parent;
}
