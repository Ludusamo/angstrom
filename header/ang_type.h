#ifndef ANG_TYPE_H
#define ANG_TYPE_H

#include "value.h"

typedef struct {
    int id;
    const char *name;
    Value default_value;
} Ang_Type;

#endif // ANG_TYPE_H
