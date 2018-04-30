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
        if (t->name[0] == '(') {
            free((void*) t->name);
            List *default_tuple = get_ptr(t->default_value);
            dtor_list(default_tuple);
            free(default_tuple);
            default_tuple = 0;

            /* You only need to free slots and slot_types for tuples
             * because any aliased ones will just hold references to these
             * base tuple objects.
             */
            Iter slot_iter;
            iter_hashtable(&slot_iter, t->slots);
            foreach(slot_iter) {
                const char *slot = ((Keyval *) get_ptr(val_iter_hashtable(&slot_iter)))->key;
                free((void *)slot);
            }
            destroy_iter_hashtable(&slot_iter);
            dtor_hashtable(t->slots);
            dtor_list(t->slot_types);
            free(t->slots);
            free(t->slot_types);

            // Destroy the tuple type
            dtor_ang_type(t);
            free(t);
            t = 0;
        } else if (t->id > TUPLE_TYPE) {
            free(t);
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
