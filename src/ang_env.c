#include "ang_env.h"

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
    dtor_hashtable(&env->types);
}

int symbol_exists(const Ang_Env *env, const char *sym) {
    return access_hashtable(&env->symbols, sym).bits != nil_val.bits;
}

void create_symbol(Ang_Env *env, const char *sym, Ang_Type *type, int loc, int global) {
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
