#ifndef ANG_ENV_H
#define ANG_ENV_H

#include "hashtable.h"
#include "ang_type.h"

typedef struct {
    const Ang_Type *type;
    int loc;
    int mut;
    int assigned;
    int global;
} Symbol;

typedef struct {
    Hashtable symbols;
    Hashtable types;
} Ang_Env;

void ctor_ang_env(Ang_Env *env);
void dtor_ang_env(Ang_Env *env);

int symbol_exists(const Ang_Env *env, const char *sym);
Symbol *create_symbol(Ang_Env *env,
        const char *sym,
        const Ang_Type *type,
        int loc,
        int mut,
        int assigned,
        int global);
void destroy_symbol(Ang_Env *env, const char *sym);

void add_type(Ang_Env *env, const Ang_Type *type);

#endif // ANG_ENV_H
