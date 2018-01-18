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

void compile(Compiler *c, Ast *code);
void compile_unary_op(Compiler *c, Ast *code);
void compile_binary_op(Compiler *c, Ast *code);
void compile_grouping(Compiler *c, Ast *code);
void compile_literal(Compiler *c, Ast *code);
void compile_variable(Compiler *c, Ast *code);
void compile_decl(Compiler *c, Ast *code);
void compile_block(Compiler *c, Ast *code);

const Symbol *find_symbol(const Compiler *c, const char *sym);
Ang_Type *find_type(const Compiler *c, const char *sym);
size_t num_local(const Compiler *c);

#endif // COMPILER_H
