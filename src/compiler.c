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
        code->eval_type = find_type(c, "Num");
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

    code->eval_type = find_type(c, "Num");
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
        code->eval_type = find_type(c, "Num");
    } else if (code->assoc_token->type == STR) {
        code->eval_type = find_type(c, "String");
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
    Ang_Type *type = find_type(c, "und");
    int has_assignment = 0;
    for (size_t i = 0; i < code->nodes.length; i++) {
        Ast *child = get_child(code, i);
        if (child->type == TYPE_DECL) {
            type = compile_type(c, child);
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
            append_list(&c->instr, type_val);
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

Ang_Type *compile_type(Compiler *c, Ast *code) {
    const char *type_sym = code->assoc_token->lexeme;
    Ang_Type *type = find_type(c, type_sym);
    if (type->id == TUPLE_TYPE) {
        List types;
        ctor_list(&types);
        for (size_t i = 0; i < code->nodes.length; i++) {
            Ang_Type *child_type = compile_type(c, get_child(code, i));
            append_list(&types, from_ptr(child_type));
        }

        char *type_name = construct_tuple_name(&types);

        Ang_Type *tuple_type = get_ptr(access_hashtable(&type->slots, type_name));
        if (!tuple_type) {
            Ang_Type *t = construct_tuple_product(&types, num_types(c) + 1, type_name);
            tuple_type = t;
            set_hashtable(&type->slots, type_name, from_ptr(t));
        } else {
            free(type_name);
        }
        dtor_list(&types);
        return tuple_type;
    }
    if (!type) {
        error(code->assoc_token->line, UNKNOWN_TYPE, type_sym);
        fprintf(stderr, "\n");
        c->enc_err = 1;
        return find_type(c, "und");
    }
    return type;
}

char *construct_tuple_name(const List *types) {
    size_t name_size = 2 + types->length; // (type,...,type)
    for (size_t i = 0; i < types->length; i++) {
        name_size +=
            strlen(((Ang_Type *) get_ptr(access_list(types, i)))->name);
    }
    char *type_name = calloc(name_size, sizeof(char));
    strcat(type_name, "(");
    for (size_t i = 0; i < types->length; i++) {
        strcat(type_name,
            ((Ang_Type *) get_ptr(access_list(types, i)))->name);
        if (i + 1 < types->length) strcat(type_name, ",");
    }
    strcat(type_name, ")");
    return type_name;
}

Ang_Type *construct_tuple_product(const List *types, int id, char *tuple_name) {
    List *default_tuple = malloc(sizeof(List));
    ctor_list(default_tuple);
    for (size_t i = 0; i < types->length; i++) {
        Ang_Type *child_type = get_ptr(access_list(types, i));
        append_list(default_tuple, child_type->default_value);
    }
    Ang_Type *t = calloc(1, sizeof(Ang_Type));
    ctor_ang_type(t, id, tuple_name, from_ptr(default_tuple));
    return t;
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

size_t num_types(const Compiler *c) {
    size_t num = 0;
    while (c) {
        num += c->env.types.size;
        c = c->parent;
    }
    return num;
}
