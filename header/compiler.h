#ifndef COMPILER_H
#define COMPILER_H

#include "ang_ast.h"
#include "hashtable.h"
#include "ang_env.h"
#include "parser.h"

typedef struct Compiler Compiler;
struct Compiler{
    List instr;
    int *enc_err;

    List compiled_ast;
    Ang_Env env;

    Compiler *parent;
    List jmp_locs;
};

void ctor_compiler(Compiler *compiler);
void dtor_compiler(Compiler *compiler);

void compile_code(Compiler *c, const char *code, const char *src_name);

void compile(Compiler *c, Ast *code);
void compile_unary_op(Compiler *c, Ast *code);
void compile_binary_op(Compiler *c, Ast *code);
void compile_grouping(Compiler *c, Ast *code);
void compile_literal(Compiler *c, Ast *code);
void compile_variable(Compiler *c, Ast *code);
void compile_keyval(Compiler *c, Ast *code);
void compile_accessor(Compiler *c, Ast *code);
void compile_type_decl(Compiler *c, Ast *code);
void compile_bind_local(Compiler *c, Ast *code);
void compile_decl(Compiler *c, Ast *code);
void compile_assign(Compiler *c, Ast *code);
void compile_return(Compiler *c, Ast *code);
void compile_destr_decl(Compiler *c, Ast *code);
void compile_destr_decl_helper(Compiler *c, int has_assignment, Ast *lhs, const Ang_Type *ttype);

Ang_Type *compile_type(Compiler *c, Ast *code);
char *construct_tuple_name(const List *slots, const List *types);
Ang_Type *construct_tuple(const List *slots, const List *types, int id, char *tuple_name);
Ang_Type *get_tuple_type(Compiler *c, const List *slots, const List *types);

void compile_block(Compiler *c, Ast *code);

void compile_lambda(Compiler *c, Ast *code);
void compile_lambda_call(Compiler *c, Ast *code);
void compile_placeholder(Compiler *c, Ast *code);

void compile_pattern(Compiler *c, Ast *code);
void compile_match(Compiler *c, Ast *code);

void compile_array(Compiler *c, Ast *code);
void compile_array_accessor(Compiler *c, Ast *code);

void push_default_value(Compiler *c, const Ang_Type *t, Value default_value);

Symbol *find_symbol(const Compiler *c, const char *sym);
Ang_Type *find_type(const Compiler *c, const char *sym);
size_t num_local(const Compiler *c);
size_t num_types(const Compiler *c);
size_t instr_count(const Compiler *c);
Compiler *get_root_compiler(Compiler *c);

#endif // COMPILER_H
