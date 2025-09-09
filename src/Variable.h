#ifndef COMPILER_VARIABLE_H_
#define COMPILER_VARIABLE_H_

#include "label_t.h"
#include "declarations.h"



struct Variable {
    label_t name;
};

void Variable_create(Variable* variable);
void Variable_delete(Variable* variable);


#endif