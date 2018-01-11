#ifndef ANG_DEBUG_H
#define ANG_DEBUG_H

#include <stdio.h>
#include <string.h>
#include "ang_opcodes.h"
#include "list.h"
#include "ang_vm.h"

int num_digits(int num);
void print_stack_trace(const Ang_VM *vm);

#endif // ANG_DEBUG_H
