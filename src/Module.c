#include "Module.h"

void Module_create(Module* module, label_t name) {
    module->name = name;
    Scope_create(&module->scope);
}

void Module_delete(Module* module) {
    Scope_delete(&module->scope);
}

