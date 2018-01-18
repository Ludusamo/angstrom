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

void compile(Compiler *c, Ast *code) {
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
    case VARIABLE:
        compile_variable(c, code);
        break;
    case VAR_DECL:
        compile_decl(c, code);
        break;
    case BLOCK:
        compile_block(c, code);
        break;
    default:
        break;
    }
}

void compile_unary_op(Compiler *c, Ast *code) {
    switch (code->assoc_token->type) {
    case MINUS:
        if (get_child(code, 0)->eval_type->id != NUM) {
            error(code->assoc_token->line,
                    TYPE_ERROR,
                    "Cannot do unary '-' operations on non-numbers.\n");
        }
        code->eval_type = find_type(c, "num");
        append_list(&c->instr, from_double(PUSH_0));
        compile(c, get_child(code, 0));
        append_list(&c->instr, from_double(SUBF));
        break;
    default:
        break;
    }
}

void compile_binary_op(Compiler *c, Ast *code) {
    compile(c, get_child(code, 0));
    compile(c, get_child(code, 1));

    code->eval_type = find_type(c, "num");
    if (get_child(code, 0)->eval_type->id != NUM_TYPE ||
            get_child(code, 1)->eval_type->id != NUM_TYPE) {
        error(code->assoc_token->line,
                TYPE_ERROR,
                "Cannot do binary operations on non-numbers.\n");
        c->enc_err = 1;
        return;
    }

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

void compile_grouping(Compiler *c, Ast *code) {
    compile(c, code);
}

void compile_literal(Compiler *c, Ast *code) {
    Value literal = code->assoc_token->literal;
    if (code->assoc_token->type == NUM) {
        code->eval_type = find_type(c, "num");
    } else if (code->assoc_token->type == STR) {
        code->eval_type = find_type(c, "string");
    }
    //TODO: Handle Strings
    append_list(&c->instr, from_double(PUSH));
    append_list(&c->instr, literal);
}

void compile_variable(Compiler *c, Ast *code) {
    const Symbol *sym = find_symbol(c, code->assoc_token->lexeme);
    if (!sym) {
        error(code->assoc_token->line,
            UNDECLARED_VARIABLE,
            code->assoc_token->lexeme);
        fprintf(stderr, "\n");
        c->enc_err = 1;
        return;
    }
    append_list(&c->instr, from_double(sym->global ? GLOAD : LOAD));
    append_list(&c->instr, from_double(sym->loc));
    code->eval_type = sym->type;
}

void compile_decl(Compiler *c, Ast *code) {
    const char *sym = code->assoc_token->lexeme;
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
                fprintf(stderr, "\n");
                c->enc_err = 1;
                return;
            }
        } else {
            has_assignment = 1;
            compile(c, child);
        }
    }
    if (!has_assignment) {
        if (type->id == NUM_TYPE) {
            append_list(&c->instr, from_double(PUSH));
        } else {
            append_list(&c->instr, from_double(PUSOBJ));
            Value type_val = from_ptr(type);
            printf("%lx\n", type_val.bits);
            append_list(&c->instr, type_val);
            printf("%lx\n", c->instr.length - 1);
            printf("%lx\n", access_list(&c->instr, c->instr.length - 1).bits);
            printf("Type %s\n", type->name);
        }
        append_list(&c->instr, type->default_value);
    } else {
        if (type->id == UNDECLARED)
            type = get_child(code, 0)->eval_type;
        if (type != get_child(code, 0)->eval_type) {
            error(code->assoc_token->line, TYPE_ERROR, "RHS type mismatch.\n");
        }
    }
    code->eval_type = type;
    int local = c->parent != 0; // If the variable is global or local
    int loc = local ? num_local(c) : c->env.symbols.size;
    create_symbol(&c->env, sym, type, loc, !local);

    if (!local) {
        append_list(&c->instr, from_double(GSTORE));
        append_list(&c->instr, from_double(loc));
    }
    append_list(&c->instr, from_double(local ? LOAD : GLOAD));
    append_list(&c->instr, from_double(loc));
}

void compile_block(Compiler *c, Ast *code) {
    Compiler block;
    ctor_compiler(&block);
    block.parent = c;
    for (size_t i = 0; i < code->nodes.length; i++) {
        compile(&block, get_child(code, i));
        if (i + 1 != code->nodes.length)
            append_list(&block.instr, from_double(POP));
    }
    for (size_t i = 0; i < block.instr.length; i++) {
        append_list(&c->instr, access_list(&block.instr, i));
    }
    append_list(&c->instr, from_double(STORET));
    if (block.env.symbols.size > 0) {
        append_list(&c->instr, from_double(POPN));
        append_list(&c->instr, from_double(block.env.symbols.size));
    }
    append_list(&c->instr, from_double(PUSRET));

    code->eval_type = get_child(code, code->nodes.length - 1)->eval_type;
    c->enc_err = block.enc_err;
    dtor_compiler(&block);
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

size_t num_local(const Compiler *c) {
    size_t num = 0;
    while (c) {
        if (c->parent) num += c->env.symbols.size;
        c = c->parent;
    }
    return num;
}
