#ifndef COMPILER_FUNCTION_H_
#define COMPILER_FUNCTION_H_

#include "declarations.h"

struct Function {
    label_t name;
};

void Function_delete(Function* fn);

#endif