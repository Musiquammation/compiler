#ifndef COMPILER_MODULE_H_
#define COMPILER_MODULE_H_

#include "declarations.h"

#include "label_t.h"
#include "Scope.h"

struct Module {
    label_t name;
    Scope scope;
};

void Module_create(Module* module, label_t name);
void Module_delete(Module* module);


#endif


