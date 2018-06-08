#include "compiler.h"

#include "ang_opcodes.h"
#include <stdio.h>
#include "error.h"
#include "ang_primitives.h"
#include "utility.h"
#include "ang_mem.h"

void ctor_compiler(Compiler *compiler) {
    ctor_list(&compiler->instr);
    ctor_ang_env(&compiler->env);
    compiler->parent = 0;
    ctor_list(&compiler->compiled_ast);
    ctor_list(&compiler->jmp_locs);
    ctor_parser(&compiler->parser);
    compiler->parser.enc_err = compiler->enc_err;
}

void dtor_compiler(Compiler *compiler) {
    dtor_list(&compiler->instr);
    dtor_list(&compiler->jmp_locs);
    dtor_ang_env(&compiler->env);
    for (size_t i = 0; i < compiler->compiled_ast.length; i++) {
        Ast *ast = get_ptr(access_list(&compiler->compiled_ast, i));
        destroy_ast(ast);
        free(ast);
    }
    dtor_list(&compiler->compiled_ast);
    dtor_parser(&compiler->parser);
}

void compile_code(Compiler *c, const char *code, const char *src_name) {
    Ast *ast = parse(&c->parser, code, src_name);
    append_list(&c->compiled_ast, from_ptr(ast));
    // Don't know if this check should be tied to error checking
    if (!ast || ast->nodes.length == 0) *c->enc_err = 1;
    if (*c->enc_err) return;
    #ifdef DEBUG
    print_ast(ast, 0);
    #endif
    compile(c, ast);
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
    case TYPE_DECL:
        compile_type_decl(c, code);
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
    case LAMBDA_LIT:
        compile_lambda(c, code);
        break;
    case LAMBDA_CALL:
        compile_lambda_call(c, code);
        break;
    case PLACEHOLD:
        compile_placeholder(c, code);
        break;
    case PATTERN_MATCH:
        compile_match(c, code);
        break;
    case PATTERN:
        compile_pattern(c, code);
        break;
    case RET_EXPR:
        compile_return(c, code);
        break;
    default:
        error(code->assoc_token->line, UNKNOWN_AST, ast_type_to_str(code->type));
        fprintf(stderr, "\n");
        *c->enc_err = 1;
        return;
    }
}

void compile_unary_op(Compiler *c, Ast *code) {
    switch (code->assoc_token->type) {
    case MINUS:
        if (get_child(code, 0)->eval_type->id != NUM) {
            error(code->assoc_token->line,
                    TYPE_ERROR,
                    "Cannot do unary '-' operations on non-numbers.\n");
            *c->enc_err = 1;
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
    Ast *lhs = get_child(code, 0);
    Ast *rhs = get_child(code, 1);

    compile(c, lhs);
    compile(c, rhs);

    code->eval_type = find_type(c, "Num");
    if (lhs->eval_type->id != NUM_TYPE || rhs->eval_type->id != NUM_TYPE) {
        error(code->assoc_token->line,
                TYPE_ERROR,
                "Cannot do binary operations on non-numbers.\n");
        *c->enc_err = 1;
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
    } else if ((code->nodes.length > 0 && get_child(code, 0)->type == KEYVAL)
            || code->assoc_token->type == LPAREN) {
        int is_record = get_child(code, 0)->type == KEYVAL;
        for (int i = code->nodes.length - 1; i >= 0; i--) {
            if (is_record && (get_child(code, i)->type != KEYVAL
                    && get_child(code, i)->type != WILDCARD)) {
                error(code->assoc_token->line,
                    INCOMPLETE_RECORD,
                    "Record type needs all members to be key value pairs.\n");
                *c->enc_err = 1;
                return;
            }
            compile(c, get_child(code, i));
        }
        List types;
        ctor_list(&types);
        List slots;
        ctor_list(&slots);
        for (size_t i = 0; i < code->nodes.length; i++) {
            if (code->type == WILDCARD) {
                append_list(&types, from_ptr((void *) find_type(c, "Any")));
                append_list(&slots, from_ptr((void *) code->assoc_token->lexeme));
                continue;
            }
            const Ang_Type *child_type = get_child(code, i)->eval_type;
            append_list(&types, from_ptr((void *) child_type));
            if (is_record) {
                const char *slot_name = get_child(code, i)->assoc_token->lexeme;
                append_list(&slots, from_ptr((char *) slot_name));
            } else {
                // Populating slots with slot number
                char *slot_num = calloc(num_digits(i) + 1, sizeof(char));
                sprintf(slot_num, "%lx", i);
                append_list(&slots, from_ptr(slot_num));
            }
        }
        code->eval_type = get_tuple_type(c, &slots, &types);
        dtor_list(&types);
        if (!is_record) {
            for (size_t i = 0; i < code->nodes.length; i++) {
                free(get_ptr(access_list(&slots, i)));
            }
        }
        dtor_list(&slots);
        append_list(&c->instr, from_double(CONS_TUPLE));
        append_list(&c->instr, from_ptr((void *) code->eval_type));
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
        *c->enc_err = 1;
        return;
    }
    append_list(&c->instr, from_double(sym->global ? GLOAD : LOAD));
    append_list(&c->instr, from_double(sym->loc));
    code->eval_type = sym->type;
}

void compile_keyval(Compiler *c, Ast *code) {
    compile(c, get_child(code, 0));
    code->eval_type = get_child(code, 0)->eval_type;
}

void compile_accessor(Compiler *c, Ast *code) {
    compile(c, get_child(code, 0));
    const Ang_Type *type = get_child(code, 0)->eval_type;
    const char *slot_name = get_child(code, 1)->assoc_token->lexeme;
    Value slot_num_val = nil_val;
    if (type->slots) slot_num_val = access_hashtable(type->slots, slot_name);
    if (slot_num_val.bits == nil_val.bits) {
        char error_msg[32 + strlen(type->name) + strlen(slot_name)];
        sprintf(error_msg, "Type <%s> does not have the slot %s.\n", type->name, slot_name);
        error(code->assoc_token->line, INVALID_SLOT, error_msg);
        *c->enc_err = 1;
        return;
    }
    int slot_num = slot_num_val.as_int32;
    code->eval_type = get_ptr(access_list(type->slot_types, slot_num));

    append_list(&c->instr, from_double(PUSH));
    append_list(&c->instr, from_double(slot_num));
    append_list(&c->instr, from_double(LOAD_TUPLE));
}

void compile_type_decl(Compiler *c, Ast *code) {
    int has_default = code->nodes.length == 2;
    const Ang_Type *type = compile_type(c, get_child(code, has_default ? 1 : 0));
    if (!type) return;
    const char *type_name = code->assoc_token->lexeme;

    // Set default value
    if (has_default) {
        compile(c, get_child(code, 0));
        append_list(&c->instr, from_double(DUP));
        append_list(&c->instr, from_double(SET_DEFAULT_VAL));
        append_list(&c->instr, from_ptr((void *) type_name));
    } else {
        push_default_value(c, type, type->default_value);
    }

    code->eval_type = type;

    // Register the new type
    Ang_Type *new_type = calloc(1, sizeof(Ang_Type));
    ctor_ang_type(new_type, type->id, type_name, type->cat, type->default_value);
    new_type->user_defined = 1;
    new_type->slots = type->slots;
    new_type->slot_types = type->slot_types;
    add_type(&c->env, new_type);
}

void compile_decl(Compiler *c, Ast *code) {
    const Ang_Type *type = find_type(c, "Und");
    int has_assignment = 0;
    for (size_t i = 0; i < code->nodes.length; i++) {
        Ast *child = get_child(code, i);
        if (child->type == TYPE ||
                child->type == SUM_TYPE ||
                child->type == PRODUCT_TYPE) {
            type = compile_type(c, child);
            if (!type) return;
        } else {
            has_assignment = 1;
            compile(c, child);
        }
    }
    if (!has_assignment) {
        push_default_value(c, type, type->default_value);
    } else {
        if (type->id == UNDECLARED)
            type = get_child(code, 0)->eval_type;
        if (!type_equality(type, get_child(code, 0)->eval_type)) {
            error(code->assoc_token->line, TYPE_ERROR, "RHS type mismatch.\n");
            *c->enc_err = 1;
            return;
        }
    }
    code->eval_type = type;
    int local = c->parent != 0; // If the variable is global or local
    int loc = local ? num_local(c) : c->env.symbols.size;
    const char *sym = code->assoc_token->lexeme;
    if (symbol_exists(&c->env, sym)) {
        error(code->assoc_token->line, NAME_COLLISION, sym);
        fprintf(stderr, "\n");
        *c->enc_err = 1;
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

void compile_return(Compiler *c, Ast *code) {
    if (!c->parent) {
        error(code->assoc_token->line,
            NON_BLOCK_RETURN,
            "Cannot return outside of a block.\n");
        *c->enc_err = 1;
        return;
    }
    compile(c, get_child(code, 0));
    code->eval_type = get_child(code, 0)->eval_type;
    append_list(&c->instr, from_double(JMP));
    append_list(&c->jmp_locs, from_double(c->instr.length));
    append_list(&c->instr, nil_val);
}

void compile_destr_decl(Compiler *c, Ast *code) {
    const Ang_Type *tuple_type = find_type(c, "Und");
    int has_assignment = 0;
    for (size_t i = 1; i < code->nodes.length; i++) {
        Ast *child = get_child(code, i);
        if (child->type == TYPE ||
                child->type == SUM_TYPE ||
                child->type == PRODUCT_TYPE) {
            tuple_type = compile_type(c, child);
            if (!tuple_type) return;
        } else {
            has_assignment = 1;
            compile(c, child);
        }
    }
    if (has_assignment) {
        if (tuple_type->id == UNDECLARED)
            tuple_type = get_child(code, 1)->eval_type;
        if (!type_equality(tuple_type, get_child(code, 1)->eval_type)) {
            error(code->assoc_token->line, TYPE_ERROR, "RHS type mismatch.\n");
            *c->enc_err = 1;
        }
        append_list(&c->instr, from_double(STO_REG));
        append_list(&c->instr, from_double(A));
    }
    code->eval_type = tuple_type;

    if (tuple_type->cat == PRIMITIVE) {
        error(code->assoc_token->line,
            INVALID_DESTR,
            "Cannot destructure a primitive type.\n");
        *c->enc_err = 1;
        return;
    } else if (tuple_type->cat == SUM) {
        error(code->assoc_token->line,
            INVALID_DESTR,
            "Cannot destructure a sum type.\n");
        *c->enc_err = 1;
        return;
    }

    compile_destr_decl_helper(c, has_assignment, get_child(code, 0), tuple_type);

    if (has_assignment) {
        append_list(&c->instr, from_double(LOAD_REG));
        append_list(&c->instr, from_double(A));
    } else {
        append_list(&c->instr, from_double(PUSOBJ));
        Value type_val = from_ptr((void *) tuple_type);
        append_list(&c->instr, type_val);
        List *obj = get_ptr(tuple_type->default_value);
        List *cpy = malloc(sizeof(List));
        ctor_list(cpy);
        copy_list(obj, cpy);
        append_list(&c->instr, from_ptr(cpy));
    }
}

void compile_destr_decl_helper(Compiler *c, int has_assignment, Ast *lhs, const Ang_Type *ttype) {
    if (lhs->nodes.length > ttype->slot_types->length) {
        error(lhs->assoc_token->line,
            INSUFFICIENT_TUPLE,
            "Trying to destructure more slots than the tuple holds.\n");
        *c->enc_err = 1;
        return;
    }
    int local = c->parent != 0; // If the variables are global or local
    for (size_t i = 0; i < lhs->nodes.length; i++) {
        if (get_child(lhs, i)->type == WILDCARD) continue;

        // If trying to destructure inner tuple, recurse
        if (get_child(lhs, i)->type == LITERAL) {
            // Move register A to B and pull out inner tuple
            if (has_assignment) {
                append_list(&c->instr, from_double(MOV_REG));
                append_list(&c->instr, from_double(A));
                append_list(&c->instr, from_double(B));
                append_list(&c->instr, from_double(LOAD_REG));
                append_list(&c->instr, from_double(B));
                append_list(&c->instr, from_double(PUSH));
                append_list(&c->instr, from_double(i));
                append_list(&c->instr, from_double(LOAD_TUPLE));
                append_list(&c->instr, from_double(STO_REG));
                append_list(&c->instr, from_double(A));
            }
            const Ang_Type *inner_ttype = get_ptr(access_list(ttype->slot_types, i));
            compile_destr_decl_helper(c, has_assignment, get_child(lhs, i), inner_ttype);

            // Restore outer tuple
            if(has_assignment) {
                append_list(&c->instr, from_double(MOV_REG));
                append_list(&c->instr, from_double(B));
                append_list(&c->instr, from_double(A));
            }
            continue;
        }

        const char *sym = get_child(lhs, i)->assoc_token->lexeme;
        const Ang_Type *slot_type =
            get_ptr(access_list(ttype->slot_types, i));
        if (!has_assignment) push_default_value(c, slot_type, slot_type->default_value);
        else {
            append_list(&c->instr, from_double(LOAD_REG));
            append_list(&c->instr, from_double(A));
            append_list(&c->instr, from_double(PUSH));
            append_list(&c->instr, from_double(i));
            append_list(&c->instr, from_double(LOAD_TUPLE));
        }
        int loc = local ? num_local(c) : c->env.symbols.size;

        if (symbol_exists(&c->env, sym)) {
            error(lhs->assoc_token->line, NAME_COLLISION, sym);
            *c->enc_err = 1;
            return;
        }
        create_symbol(&c->env, sym, slot_type, loc, !local);

        if (!local) {
            append_list(&c->instr, from_double(GSTORE));
            append_list(&c->instr, from_double(loc));
        }
    }
}

static char *construct_sum_type_name(const List *types) {
    // TYPE|TYPE...|TYPE
    size_t name_size = types->length;
    for (size_t i = 0; i < types->length; i++) {
        name_size += strlen(((Ang_Type *) get_ptr(access_list(types, i)))->name);
    }
    char *type_name = calloc(name_size, sizeof(char));
    for (size_t i = 0; i < types->length - 1; i++) {
        strcat(type_name, ((Ang_Type *) get_ptr(access_list(types, i)))->name);
        strcat(type_name, "|");
    }
    int last = types->length - 1;
    strcat(type_name, ((Ang_Type *) get_ptr(access_list(types, last)))->name);
    type_name[name_size - 1] = '\0';
    return type_name;
}

static Ang_Type *get_sum_type(Compiler *c, const List *types) {
    char *sum_type_name = construct_sum_type_name(types);
    Ang_Type *sum_type = find_type(c, sum_type_name);
    if (!sum_type) {
        sum_type = calloc(1, sizeof(Ang_Type));
        ctor_ang_type(sum_type,
                num_types(c),
                sum_type_name,
                SUM,
                ((Ang_Type *) get_ptr(access_list(types, 0)))->default_value);
        sum_type->slots = calloc(1, sizeof(Hashtable));
        sum_type->slot_types = calloc(1, sizeof(List));
        ctor_hashtable(sum_type->slots);
        ctor_list(sum_type->slot_types);
        for (size_t i = 0; i < types->length; i++) {
            Ang_Type *type = get_ptr(access_list(types, i));
            set_hashtable(sum_type->slots, type->name, from_double(i));
            append_list(sum_type->slot_types, from_ptr(type));
        }
        add_type(&get_root_compiler(c)->env, sum_type);
    } else {
        free(sum_type_name);
    }
    return sum_type;
}

static char *construct_lambda_type_name(
        Compiler *c,
        const Ang_Type *lhs,
        const Ang_Type *rhs) {
    // TYPE=>TYPE
    const char *lhs_name = lhs->name;
    const char *rhs_name = rhs->name;
    size_t name_size = 3 + strlen(lhs_name) + strlen(rhs_name);
    char *type_name = calloc(name_size, sizeof(char));
    strcat(type_name, lhs_name);
    strcat(type_name, "=>");
    strcat(type_name, rhs_name);
    type_name[name_size - 1] = '\0';
    return type_name;
}

static Ang_Type *get_lambda_type(
        Compiler *c,
        const Ang_Type *lhs,
        const Ang_Type *rhs) {
    char *lambda_name = construct_lambda_type_name(c, lhs, rhs);
    Ang_Type *lambda_type = find_type(c, lambda_name);
    if (!lambda_type) {
        lambda_type = calloc(1, sizeof(Ang_Type));
        ctor_ang_type(lambda_type, num_types(c), lambda_name, LAMBDA, nil_val);
        lambda_type->slots = calloc(1, sizeof(Hashtable));
        lambda_type->slot_types = calloc(1, sizeof(List));
        ctor_hashtable(lambda_type->slots);
        ctor_list(lambda_type->slot_types);

        set_hashtable(lambda_type->slots, lhs->name, from_double(0));
        append_list(lambda_type->slot_types, from_ptr((void *) lhs));
        set_hashtable(lambda_type->slots, rhs->name, from_double(1));
        append_list(lambda_type->slot_types, from_ptr((void *) rhs));

        add_type(&get_root_compiler(c)->env, lambda_type);
    } else {
        free(lambda_name);
    }
    return lambda_type;
}

Ang_Type *compile_type(Compiler *c, Ast *code) {
    if (code->type == KEYVAL) return compile_type(c, get_child(code, 0));
    const char *type_sym = code->assoc_token->lexeme;
    if (code->type == PRODUCT_TYPE) {
        int is_record = get_child(code, 0)->type == KEYVAL;
        List types;
        ctor_list(&types);
        List slots;
        ctor_list(&slots);
        for (size_t i = 0; i < code->nodes.length; i++) {
            Ang_Type *child_type = compile_type(c, get_child(code, i));
            append_list(&types, from_ptr(child_type));

            // Populating slots with slot number
            if (is_record) {
                if (get_child(code, i)->type != KEYVAL
                        && get_child(code, i)->type != WILDCARD) {
                    error(code->assoc_token->line,
                        INCOMPLETE_RECORD,
                        "Record type needs all members to be key value pairs.\n");
                    *c->enc_err = 1;
                    return 0;
                }
                const char *slot_name = get_child(code, i)->assoc_token->lexeme;
                append_list(&slots, from_ptr((char *) slot_name));
            } else {
                char *slot_num = calloc(num_digits(i) + 1, sizeof(char));
                sprintf(slot_num, "%lx", i);
                append_list(&slots, from_ptr(slot_num));
            }
        }

        Ang_Type *tuple_type = get_tuple_type(c, &slots, &types);
        dtor_list(&types);
        if (!is_record) {
            for (size_t i = 0; i < code->nodes.length; i++) {
                free(get_ptr(access_list(&slots, i)));
            }
        }
        dtor_list(&slots);
        code->eval_type = tuple_type;
        return tuple_type;
    } else if (code->type == SUM_TYPE) {
        List types;
        ctor_list(&types);
        Ast *buf = code;
        do {
            append_list(&types, from_ptr(compile_type(c, get_child(code, 0))));
            buf = get_child(buf, 1);
        } while (buf->type == SUM_TYPE);
        append_list(&types, from_ptr(compile_type(c, get_child(code, 1))));
        Ang_Type *sum_type = get_sum_type(c, &types);
        dtor_list(&types);
        code->eval_type = sum_type;
        return sum_type;
    } else if (code->type == WILDCARD) {
        type_sym = "Any";
    }
    Ang_Type *type = find_type(c, type_sym);
    if (!type) {
        error(code->assoc_token->line, UNKNOWN_TYPE, type_sym);
        fprintf(stderr, "\n");
        *c->enc_err = 1;
        return find_type(c, "Und");
    }
    code->eval_type = type;
    return type;
}

char *construct_tuple_name(const List *slots, const List *types) {
    // (slot:type,...,slot:type)
    size_t name_size = 2 + types->length + slots->length;
    for (size_t i = 0; i < slots->length; i++) {
        name_size += strlen(((char *) get_ptr(access_list(slots, i))));
    }
    for (size_t i = 0; i < types->length; i++) {
        name_size +=
            strlen(((Ang_Type *) get_ptr(access_list(types, i)))->name);
    }
    char *type_name = calloc(name_size, sizeof(char));
    strcat(type_name, "(");
    for (size_t i = 0; i < types->length; i++) {
        if (slots->length) {
            strcat(type_name, get_ptr(access_list(slots, i)));
            strcat(type_name, ":");
        }
        strcat(type_name,
            ((Ang_Type *) get_ptr(access_list(types, i)))->name);
        if (i + 1 < types->length) strcat(type_name, ",");
    }
    strcat(type_name, ")");
    return type_name;
}

Ang_Type *construct_tuple(const List *slots, const List *types, int id, char *tuple_name) {
    List *default_tuple = malloc(sizeof(List));
    ctor_list(default_tuple);
    for (size_t i = 0; i < types->length; i++) {
        Ang_Type *child_type = get_ptr(access_list(types, i));
        append_list(default_tuple, child_type->default_value);
    }
    Ang_Type *t = calloc(1, sizeof(Ang_Type));
    ctor_ang_type(t, id, tuple_name, PRODUCT, from_ptr(default_tuple));
    t->slots = calloc(1, sizeof(Hashtable));
    ctor_hashtable(t->slots);
    t->slot_types = calloc(1, sizeof(List));
    ctor_list(t->slot_types);
    for (size_t i = 0; i < slots->length; i++) {
        const char *slot_name = get_ptr(access_list(slots, i));
        char *sym = calloc(strlen(slot_name) + 1, sizeof(char));
        strcpy(sym, slot_name);

        const Ang_Type *slot_type = get_ptr(access_list(types, i));
        add_slot(t, sym, slot_type);
    }
    return t;
}

Ang_Type *get_tuple_type(Compiler *c, const List *slots, const List *types) {
    char *type_name = construct_tuple_name(slots, types);
    Ang_Type *tuple_type = find_type(c, type_name);
    if (!tuple_type) {
        tuple_type = construct_tuple(slots, types, num_types(c) + 1, type_name);
        add_type(&get_root_compiler(c)->env, tuple_type);
    } else {
        free(type_name);
    }
    return tuple_type;
}

void compile_block(Compiler *c, Ast *code) {
    Compiler block;
    ctor_compiler(&block);
    block.enc_err = c->enc_err;
    block.parent = c;

    List return_types;
    ctor_list(&return_types);

    // Compile all block expressions
    for (size_t i = 0; i < code->nodes.length; i++) {
        Ast *child = get_child(code, i);
        compile(&block, child);
        if (i + 1 != code->nodes.length)
            append_list(&block.instr, from_double(POP));

        // Is return or last statement
        if (child->type == RET_EXPR || i + 1 == code->nodes.length) {
            append_list(&return_types, from_ptr((void *) child->eval_type));
        } else if (child->type == PATTERN) {
            // For pattern matching the second child of a pattern is a return
            Ast *ret = get_child(child, 1);
            append_list(&return_types, from_ptr((void *) ret->eval_type));
        }
    }

    // Set all return jump locations to here
    Value jmp_loc = from_double(instr_count(&block));
    for (size_t i = 0; i < block.jmp_locs.length; i++) {
        int jmp_instr = access_list(&block.jmp_locs, i).as_int32;
        set_list(&block.instr, jmp_instr, jmp_loc);
    }

    for (size_t i = 0; i < block.instr.length; i++) {
        append_list(&c->instr, access_list(&block.instr, i));
    }

    append_list(&c->instr, from_double(STORET));

    // Pop all local variables
    if (block.env.symbols.size > 0) {
        append_list(&c->instr, from_double(POPN));
        append_list(&c->instr, from_double(block.env.symbols.size));
    }
    append_list(&c->instr, from_double(PUSRET));

    // Return type of block
    if (return_types.length > 1) {
        code->eval_type = get_sum_type(c, &return_types);
    } else {
        code->eval_type = get_child(code, code->nodes.length - 1)->eval_type;
    }
    dtor_list(&return_types);

    *c->enc_err = *block.enc_err;
    dtor_compiler(&block);
}

void compile_lambda(Compiler *c, Ast *code) {
    append_list(&c->instr, from_double(JMP));
    append_list(&c->instr, nil_val);
    int jmp_loc = c->instr.length - 1;
    int ip = instr_count(c); // Instruction pointer for call

    Ast *block = get_child(code, 0);
    compile_block(c, block);
    const Ang_Type *lhs_type = get_child(block, 0)->eval_type;
    const Ang_Type *rhs_type = block->eval_type;
    code->eval_type = get_lambda_type(c, lhs_type, rhs_type);
    append_list(&c->instr, from_double(RET));

    set_list(&c->instr, jmp_loc, from_double(instr_count(c)));

    append_list(&c->instr, from_double(CONS_LAMBDA));
    append_list(&c->instr, from_ptr((void *) code->eval_type));
    append_list(&c->instr, from_double(ip));
}

void compile_lambda_call(Compiler *c, Ast *code) {
    Ast *lambda = get_child(code, 0);
    Ast *param = get_child(code, 1);

    compile(c, lambda);
    compile(c, param);

    const Ang_Type *lambda_type = lambda->eval_type;
    if (!lambda_type || lambda_type->cat != LAMBDA) {
        error(code->assoc_token->line,
            NON_LAMBDA_CALL,
            "Cannot call a non-lambda.\n");
        *c->enc_err = 1;
        return;
    }
    const Ang_Type *lhs_type = get_ptr(access_list(lambda_type->slot_types, 0));
    if (!type_structure_equality(lhs_type, param->eval_type)) {
        char error_msg[256 + strlen(lhs_type->name) + strlen(param->eval_type->name)];
        sprintf(error_msg,
            "Lambda requires parameter of type %s. Received parameter of type %s.\n",
            lhs_type->name,
            param->eval_type->name);
        error(code->assoc_token->line,
            INVALID_LAMBDA_PARAM,
            error_msg);
        *c->enc_err = 1;
        return;
    }
    code->eval_type = get_ptr(access_list(lambda_type->slot_types, 1));
    append_list(&c->instr, from_double(CALL));
}

void compile_placeholder(Compiler *c, Ast *code) {
    code->eval_type = compile_type(c, get_child(code, 0));
    append_list(&c->instr, from_double(LOAD_REG));
    append_list(&c->instr, from_double(A));
}

void compile_pattern(Compiler *c, Ast *code) {
    Compiler block;
    ctor_compiler(&block);
    block.enc_err = c->enc_err;
    block.parent = c;

    Ast *lhs = get_child(code, 0);
    Ast *rhs = get_child(code, 1);

    append_list(&block.instr, from_double(LOAD_REG));
    append_list(&block.instr, from_double(A));
    if (lhs->type == LITERAL) {
        compile(&block, lhs);

        if (lhs->eval_type->id == NUM_TYPE) {
            append_list(&block.instr, from_double(NUM_EQ));
        }
    } else if (lhs->type == TYPE) {
        append_list(&block.instr, from_double(CMP_TYPE));
        append_list(&block.instr, from_ptr(compile_type(&block, lhs)));
    } else if (lhs->type != WILDCARD) {
        append_list(&block.instr, from_double(POP));
        compile(&block, lhs);
    }

    if (lhs->type != WILDCARD) {
        append_list(&block.instr, from_double(JNE));
        append_list(&block.instr, nil_val);
    }
    int jmp_loc = block.instr.length - 1;

    compile(&block, rhs);

    if (lhs->type != WILDCARD) {
        set_list(&block.instr, jmp_loc, from_double(instr_count(&block) + 1));
    }

    code->eval_type = rhs->eval_type;

    // Set all return jump locations to here
    Value end_jmp = from_double(instr_count(&block));
    for (size_t i = 0; i < block.jmp_locs.length; i++) {
        int jmp_instr = access_list(&block.jmp_locs, i).as_int32;
        set_list(&block.instr, jmp_instr, end_jmp);
    }

    for (size_t i = 0; i < block.instr.length; i++) {
        append_list(&c->instr, access_list(&block.instr, i));
    }

    *c->enc_err = *block.enc_err;
    dtor_compiler(&block);
}

void compile_match(Compiler *c, Ast *code) {
    Ast *expr = get_child(code, 0);
    Ast *block = get_child(code, 1);

    compile(c, expr);
    append_list(&c->instr, from_double(STO_REG));
    append_list(&c->instr, from_double(A));

    compile_block(c, block);

    code->eval_type = block->eval_type;
}

void push_default_value(Compiler *c, const Ang_Type *t, Value default_value) {
    if (t->cat == PRIMITIVE) {
        // Base case for Num type
        append_list(&c->instr, from_double(PUSH));
        append_list(&c->instr, default_value);
    } else if (t->cat == PRODUCT) {
        // Construct default value
        const List *def_val = get_ptr(default_value);
        for (int i = t->slot_types->length - 1; i >= 0; i--) {
            const Ang_Type *slot_type = get_ptr(access_list(t->slot_types, i));
            push_default_value(c, slot_type, access_list(def_val, i));
        }
        append_list(&c->instr, from_double(CONS_TUPLE));
        // Push on the type of the object
        Value type_val = from_ptr((void *) t);
        append_list(&c->instr, type_val);
        // Push the number of slots
        append_list(&c->instr, from_double(t->slot_types->length));
    } else if (t->cat == SUM) {
        push_default_value(c, get_ptr(access_list(t->slot_types, 0)), default_value);
    }
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

size_t instr_count(const Compiler *c) {
    if (!c->parent) return c->instr.length;
    return c->instr.length + instr_count(c->parent);
}

Compiler *get_root_compiler(Compiler *c) {
    while (c->parent) c = c->parent;
    return c;
}
