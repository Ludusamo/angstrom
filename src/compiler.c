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
    case KEYVAL:
        compile_keyval(c, code);
        break;
    case ACCESSOR:
        compile_accessor(c, code);
        break;
    case VAR_DECL:
        compile_decl(c, code);
        break;
    case DESTR_DECL:
        compile_destr_decl(c, code);
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
        append_list(&c->instr, from_double(PUSH));
        append_list(&c->instr, literal);
    } else if (code->assoc_token->type == STR) {
        code->eval_type = find_type(c, "String");
    } else if (code->assoc_token->type == LPAREN) {
        List types;
        ctor_list(&types);
        for (int i = code->nodes.length - 1; i >= 0; i--) {
            compile(c, get_child(code, i));
        }
        for (int i = 0; i < code->nodes.length; i++) {
            Ang_Type *child_type = get_child(code,i)->eval_type;
            append_list(&types, from_ptr(child_type));
        }
        code->eval_type = get_tuple_type(c, &types);
        dtor_list(&types);
        append_list(&c->instr, from_double(CONS_TUPLE));
        append_list(&c->instr, from_ptr(code->eval_type));
        append_list(&c->instr, from_double(code->nodes.length));
    }
    //TODO: Handle Strings
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

void compile_keyval(Compiler *c, Ast *code) {
    compile(c, get_child(code, 0));
}

void compile_accessor(Compiler *c, Ast *code) {
    compile(c, get_child(code, 0));
    compile(c, get_child(code, 1));

    append_list(&c->instr, from_double(LOAD_TUPLE));
    const Ang_Type *type = get_child(code, 1)->eval_type;
    int slot_num = get_child(code, 0)->assoc_token->literal.as_int32;
    code->eval_type = get_slot_type(c, type, slot_num);
}

void compile_decl(Compiler *c, Ast *code) {
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
            append_list(&c->instr, type->default_value);
        } else {
            append_list(&c->instr, from_double(PUSOBJ));
            Value type_val = from_ptr(type);
            append_list(&c->instr, type_val);
            size_t def_size = sizeof(*get_ptr(type->default_value));
            void *cpy = malloc(def_size);
            memcpy(cpy, get_ptr(type->default_value), def_size);
            append_list(&c->instr, from_ptr(cpy));
        }
    } else {
        if (type->id == UNDECLARED)
            type = get_child(code, 0)->eval_type;
        if (type != get_child(code, 0)->eval_type) {
            error(code->assoc_token->line, TYPE_ERROR, "RHS type mismatch.\n");
            c->enc_err = 1;
        }
    }
    code->eval_type = type;
    int local = c->parent != 0; // If the variable is global or local
    int loc = local ? num_local(c) : c->env.symbols.size;
    const char *sym = code->assoc_token->lexeme;
    if (symbol_exists(&c->env, sym)) {
        error(code->assoc_token->line, NAME_COLLISION, sym);
        c->enc_err = 1;
        return;
    }
    create_symbol(&c->env, sym, type, loc, !local);

    if (!local) {
        append_list(&c->instr, from_double(GSTORE));
        append_list(&c->instr, from_double(loc));
    }
    append_list(&c->instr, from_double(local ? LOAD : GLOAD));
    append_list(&c->instr, from_double(loc));
}

void compile_destr_decl(Compiler *c, Ast *code) {
    Ang_Type *tuple_type = find_type(c, "und");
    int has_assignment = 0;
    for (size_t i = 1; i < code->nodes.length; i++) {
        Ast *child = get_child(code, i);
        if (child->type == TYPE_DECL) {
            tuple_type = compile_type(c, child);
        } else {
            has_assignment = 1;
            compile(c, child);
        }
    }
    if (has_assignment) {
        if (tuple_type->id == UNDECLARED)
            tuple_type = get_child(code, 1)->eval_type;
        if (tuple_type != get_child(code, 1)->eval_type) {
            error(code->assoc_token->line, TYPE_ERROR, "RHS type mismatch.\n");
            c->enc_err = 1;
        }
    }
    code->eval_type = tuple_type;
    for (size_t i = 0; i < get_child(code, 0)->nodes.length; i++) {
        Ang_Type *type = get_slot_type(c, tuple_type, i);
        if (!has_assignment) {
            if (type->id == NUM_TYPE) {
                append_list(&c->instr, from_double(PUSH));
                append_list(&c->instr, type->default_value);
            } else {
                append_list(&c->instr, from_double(PUSOBJ));
                Value type_val = from_ptr(type);
                append_list(&c->instr, type_val);
                size_t def_size = sizeof(*get_ptr(type->default_value));
                void *cpy = malloc(def_size);
                memcpy(cpy, get_ptr(type->default_value), def_size);
                append_list(&c->instr, from_ptr(cpy));
            }
        }
        int local = c->parent != 0; // If the variable is global or local
        int loc = local ? num_local(c) : c->env.symbols.size;
        const char *sym = code->assoc_token->lexeme;
        if (symbol_exists(&c->env, sym)) {
            error(code->assoc_token->line, NAME_COLLISION, sym);
            c->enc_err = 1;
            return;
        }
        create_symbol(&c->env, sym, type, loc, !local);

        if (!local) {
            append_list(&c->instr, from_double(GSTORE));
            append_list(&c->instr, from_double(loc));
        }
        append_list(&c->instr, from_double(local ? LOAD : GLOAD));
        append_list(&c->instr, from_double(loc));
    }
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

        Ang_Type *tuple_type = get_tuple_type(c, &types);
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

Ang_Type *get_tuple_type(const Compiler *c, const List *types) {
    Ang_Type *tuple = find_type(c, "(");
    char *type_name = construct_tuple_name(types);
    Ang_Type *tuple_type = get_ptr(access_hashtable(&tuple->slots, type_name));
    if (!tuple_type) {
        Ang_Type *t = construct_tuple_product(types, num_types(c) + 1, type_name);
        tuple_type = t;
        set_hashtable(&tuple->slots, type_name, from_ptr(t));
    } else {
        free(type_name);
    }
    return tuple_type;
}

Ang_Type *get_slot_type(const Compiler *c, const Ang_Type *tuple_type, int slot_num) {
    const char *type = tuple_type->name;

    int skipped = 0;
    int paren_count = 0;
    int start = 1;
    int end = 0;
    for (int i = 1; i < strlen(type); i++) {
        switch (type[i]) {
        case ',':
            if (!paren_count) {
                skipped++;
                if (skipped == slot_num) start = i + 1;
            }
            break;
        case '(':
            paren_count++;
            break;
        case ')':
            paren_count--;
            break;
        default:
            break;
        }
        if (skipped == slot_num + 1 || paren_count < 0) {
            end = i;
            break;
        }
    }
    char type_name[end - start + 1];
    strncpy(type_name, type + start, end - start);
    type_name[end - start] = 0;

    return find_type(c, type_name);
}

void compile_block(Compiler *c, Ast *code) {
    Compiler block;
    ctor_compiler(&block);
    block.parent = c;
    append_list(&c->instr, from_double(SET_FP));
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
    append_list(&c->instr, from_double(RESET_FP));

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
    if (!c->parent) {
        Ang_Type *tuple = get_ptr(access_hashtable(&c->env.types, "("));
        type = access_hashtable(&tuple->slots, sym);
        if (type.bits != nil_val.bits) {
            return get_ptr(type);
        }
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
