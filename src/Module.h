#ifndef COMPILER_MODULE_H_
#define COMPILER_MODULE_H_

#include "declarations.h"

#include "LabelPool.h"
#include "Scope.h"

struct Module {
	Scope scope;
	label_t name;
	ScopeFile* files;
	int fileLength;
};

void Module_create(Module* module);
void Module_delete(Module* module);

void Module_generateFilesScopes(Module* module);
void Module_readDeclarations(Module* module);
ScopeFile* Module_findModuleFile(Module* module, label_t target);

#endif



