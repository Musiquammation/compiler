#ifndef COMPILER_VARIABLE_H_
#define COMPILER_VARIABLE_H_

#include "label_t.h"
#include "declarations.h"

#include "TypeCall.h"


struct Variable {
    label_t name;
    TypeCall typeCall;
};

#endif