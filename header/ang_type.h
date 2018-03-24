#ifndef ANG_TYPE_H
#define ANG_TYPE_H

#include "hashtable.h"

typedef struct {
    int id;
    const char *name;
    Value default_value;
    Hashtable slots;
    List slot_types;
} Ang_Type;

void ctor_ang_type(Ang_Type *type, int id, const char *name, Value default_val);
void dtor_ang_type(Ang_Type *type);

void add_slot(Ang_Type *type, const char *sym, const Ang_Type *slot_type);

#endif // ANG_TYPE_H
