#include "compiler.h"

#include "ang_opcodes.h"
#include <stdio.h>

void ctor_compiler(Compiler *compiler) {
    ctor_list(&compiler->instr);
    compiler->enc_err = 0;
}

void dtor_compiler(Compiler *compiler) {
    dtor_list(&compiler->instr);
}

void compile(Compiler *c, const Ast *code) {
    switch (code->type) {
    case PROG:
        for (int i = 0; i < code->nodes.length; i++)
            compile(c, get_child(code, i));
        break;
    case ADD_OP:
    case MUL_OP:
        compile_binary_op(c, code);
        break;
    case UNARY_OP:
        compile_unary_op(c, code);
        break;
    case LITERAL:
        compile_literal(c, code);
        break;
    default:
        break;
    }
}

void compile_unary_op(Compiler *c, const Ast *code) {
    switch (code->assoc_token->type) {
    case MINUS:
        append_list(&c->instr, from_double(PUSH_0));
        compile(c, get_child(code, 0));
        append_list(&c->instr, from_double(SUBF));
        break;
    default:
        break;
    }
}

void compile_binary_op(Compiler *c, const Ast *code) {
    compile(c, get_child(code, 0));
    compile(c, get_child(code, 1));

    switch (code->assoc_token->type) {
    case PLUS:
        append_list(&c->instr, from_double(ADDF));
        break;
    case MINUS:
        append_list(&c->instr, from_double(SUBF));
        break;
    case STAR:
        append_list(&c->instr, from_double(MULF));
        break;
    case SLASH:
        append_list(&c->instr, from_double(DIVF));
        break;
    default:
        break;
    }
}

void compile_grouping(Compiler *c, const Ast *code) {
    compile(c, code);
}

void compile_literal(Compiler *c, const Ast *code) {
    Value literal = code->assoc_token->literal;
    //TODO: Handle Strings
    append_list(&c->instr, from_double(PUSH));
    append_list(&c->instr, literal);
}
