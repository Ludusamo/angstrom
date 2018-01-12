#include "ang_env.h"

void ctor_ang_env(Ang_Env *env) {
    ctor_hashtable(&env->symbols);
}

void dtor_ang_env(Ang_Env *env) {
    dtor_hashtable(&env->symbols);
}

int symbol_exists(const Ang_Env *env, const char *sym) {
    return access_hashtable(&env->symbols, sym).bits != nil_val.bits;
}

void create_symbol(Ang_Env *env, const char *sym, int type) {
    Symbol *symbol = calloc(sizeof(Symbol), 1);
    symbol->type = type;
    set_hashtable(&env->symbols, sym, from_ptr(symbol));
}

void destroy_symbol(Ang_Env *env, const char *sym) {
    if (!symbol_exists(env, sym)) return;
    Symbol *symbol = get_ptr(access_hashtable(&env->symbols, sym));
    free(symbol);
    symbol = 0;
    delete_hashtable(&env->symbols, sym);
}
