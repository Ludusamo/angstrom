#ifndef ANG_TYPE_H
#define ANG_TYPE_H

#include "hashtable.h"

typedef struct {
    int id;
    const char *name;
    Value default_value;
    Hashtable slots;
} Ang_Type;

void ctor_ang_type(Ang_Type *type, int id, const char *name, Value default_val);
void dtor_ang_type(Ang_Type *type);

#endif // ANG_TYPE_H
