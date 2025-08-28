#ifndef COMPILER_MODULE_H_
#define COMPILER_MODULE_H_

#include "label_t.h"
#include "Scope.h"

typedef struct {
    label_t name;
    Scope scope;
} Module;


void Module_create(Module* module, label_t name);
void Module_delete(Module* module);


#endif


