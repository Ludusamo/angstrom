#include "ang_env.h"

#include "ang_primitives.h"
#include <stdio.h>

void ctor_ang_env(Ang_Env *env) {
    ctor_hashtable(&env->symbols);
    ctor_hashtable(&env->types);
}

void dtor_ang_env(Ang_Env *env) {
    Iter sym_iter;
    iter_hashtable(&sym_iter, &env->symbols);
    foreach (sym_iter) {
        destroy_symbol(env, ((Keyval*) get_ptr(val_iter_hashtable(&sym_iter)))->key);
    }
    destroy_iter_hashtable(&sym_iter);
    dtor_hashtable(&env->symbols);
    Iter type_iter;
    iter_hashtable(&type_iter, &env->types);
    foreach (type_iter) {
        Ang_Type *t =
            get_ptr(((Keyval*) (get_ptr(val_iter_hashtable(&type_iter))))->val);
        if (t->id == TUPLE_TYPE) {
            Iter tuple_iter;
            iter_hashtable(&tuple_iter, &t->slots);
            foreach(tuple_iter) {
                Ang_Type *tup_type =
                    get_ptr(((Keyval*) (get_ptr(val_iter_hashtable(&tuple_iter))))->val);
                free((void*) tup_type->name);
                List *default_tuple = get_ptr(tup_type->default_value);
                dtor_list(default_tuple);
                free(default_tuple);
                default_tuple = 0;
                dtor_ang_type(tup_type);
                free(tup_type);
                tup_type = 0;
            }
            destroy_iter_hashtable(&tuple_iter);
            t->slots.size = 0;
        }
    }
    destroy_iter_hashtable(&type_iter);
    dtor_hashtable(&env->types);
}

int symbol_exists(const Ang_Env *env, const char *sym) {
    return access_hashtable(&env->symbols, sym).bits != nil_val.bits;
}

void create_symbol(Ang_Env *env, const char *sym, const Ang_Type *type, int loc, int global) {
    Symbol *symbol = calloc(sizeof(Symbol), 1);
    symbol->type = type;
    symbol->loc = loc;
    symbol->global = global;
    set_hashtable(&env->symbols, sym, from_ptr(symbol));
}

void destroy_symbol(Ang_Env *env, const char *sym) {
    if (!symbol_exists(env, sym)) return;
    Symbol *symbol = get_ptr(access_hashtable(&env->symbols, sym));
    free(symbol);
    symbol = 0;
    delete_hashtable(&env->symbols, sym);
}
