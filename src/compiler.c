#include "compiler.h"

#include "ang_opcodes.h"
#include <stdio.h>
#include "error.h"
#include "ang_primitives.h"

void ctor_compiler(Compiler *compiler) {
    ctor_list(&compiler->instr);
    ctor_ang_env(&compiler->env);
    compiler->enc_err = 0;
    compiler->parent = 0;
}

void dtor_compiler(Compiler *compiler) {
    dtor_ang_env(&compiler->env);
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
    case VAR_DECL:
        compile_decl(c, code);
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

void compile_decl(Compiler *c, const Ast *code) {
    const char *sym = code->assoc_token->lexeme;
    printf("Declaring %s\n", sym);
    if (symbol_exists(&c->env, sym)) {
        error(code->assoc_token->line, NAME_COLLISION, sym);
        c->enc_err = 1;
        return;
    }
    Ang_Type *type = find_type(c, "undeclared");
    int has_assignment = 0;
    for (size_t i = 0; i < code->nodes.length; i++) {
        Ast *child = get_child(code, i);
        if (child->type == TYPE_DECL) {
            const char *type_sym = child->assoc_token->lexeme;
            type = find_type(c, type_sym);
            if (!type) {
                error(child->assoc_token->line, UNKNOWN_TYPE, type_sym);
                c->enc_err = 1;
            }
        } else {
            has_assignment = 1;
            compile(c, child);
        }
    }
    if (!has_assignment) {
        append_list(&c->instr, from_double(PUSH));
        append_list(&c->instr, type->default_value);
    }
    int loc = c->env.symbols.size;
    create_symbol(&c->env, sym, type, loc);

    int local = c->parent != 0; // If the variable is global or local
    append_list(&c->instr, from_double(local ? STORE : GSTORE));
    append_list(&c->instr, from_double(loc));
    append_list(&c->instr, from_double(local ? LOAD : GLOAD));
    append_list(&c->instr, from_double(loc));
}

const Symbol *find_symbol(const Compiler *c, const char *sym) {
    if (!c) return 0;
    Value symbol = access_hashtable(&c->env.symbols, sym);
    if (symbol.bits != nil_val.bits) {
        return get_ptr(symbol);
    }
    return find_symbol(c->parent, sym);
}

Ang_Type *find_type(const Compiler *c, const char *sym) {
    if (!c) return 0;
    Value type = access_hashtable(&c->env.types, sym);
    if (type.bits != nil_val.bits) {
        return get_ptr(type);
    }
    return find_type(c->parent, sym);
}
