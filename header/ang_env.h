#ifndef ANG_ENV_H
#define ANG_ENV_H

#include "hashtable.h"

typedef struct {
    int type;
} Symbol;

typedef struct {
    Hashtable symbols;
} Ang_Env;

void ctor_ang_env(Ang_Env *env);
void dtor_ang_env(Ang_Env *env);

int symbol_exists(const Ang_Env *env, const char *sym);
void create_symbol(Ang_Env *env, const char *sym, int type);

#endif // ANG_ENV_H
