#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "hashtable.h"
#include "ang_env.h"

typedef struct Compiler Compiler;
struct Compiler{
    List instr;
    int enc_err;

    Ang_Env env;

    Compiler *parent;
};

void ctor_compiler(Compiler *compiler);
void dtor_compiler(Compiler *compiler);

void compile(Compiler *c, const Ast *code);
void compile_unary_op(Compiler *c, const Ast *code);
void compile_binary_op(Compiler *c, const Ast *code);
void compile_grouping(Compiler *c, const Ast *code);
void compile_literal(Compiler *c, const Ast *code);
void compile_decl(Compiler *c, const Ast *code);

const Symbol *find_symbol(const Compiler *c, const char *sym);
int find_type(const Compiler *c, const char *sym);

#endif // COMPILER_H
