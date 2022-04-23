#include "ang_lambda.h"

void save_lambda_env(Lambda *l, const Memory *mem) {
    l->nenv = mem->sp - mem->fp;
    l->env = calloc(l->nenv, sizeof(Value));
    for (int i = 0; i < l->nenv; i++) {
        l->env[i] = mem->stack[mem->fp + i];
    }
}

void load_lambda_env(const Lambda *l, Memory *mem) {
    for (int i = 0; i < l->nenv; i++) {
        push_stack(mem, l->env[i]);
    }
}

void destroy_lambda(Lambda *l) {
    free(l->env);
}
