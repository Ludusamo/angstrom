#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "hashtable.h"
#include "ang_env.h"

typedef struct {
    List instr;
    int enc_err;

    Ang_Env *env;
} Compiler;

void ctor_compiler(Compiler *compiler);
void dtor_compiler(Compiler *compiler);

void compile(Compiler *c, const Ast *code);
void compile_unary_op(Compiler *c, const Ast *code);
void compile_binary_op(Compiler *c, const Ast *code);
void compile_grouping(Compiler *c, const Ast *code);
void compile_literal(Compiler *c, const Ast *code);

#endif // COMPILER_H
