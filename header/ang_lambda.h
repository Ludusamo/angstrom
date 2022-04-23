#ifndef LAMBDA_H
#define LAMBDA_H

#include "ang_obj.h"
#include "ang_mem.h"

typedef struct {
    int ip;
    int nenv;
    Value *env;
} Lambda;

void save_lambda_env(Lambda *l, const Memory *mem);
void load_lambda_env(const Lambda *l, Memory *mem);
void destroy_lambda(Lambda *l);

#endif // LAMBDA_H
