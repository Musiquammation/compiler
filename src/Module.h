#ifndef COMPILER_MODULE_H_
#define COMPILER_MODULE_H_

#include "declarations.h"

#include "LabelPool.h"
#include "Scope.h"

#include <stdint.h>

struct Module {
	Scope scope;
	label_t name;
	ScopeFile* files;
	Function* mainFunction;
	char* binFolder;
	int fileLength;
	bool compile;

	Array classes; // type: Class*
	Array functions; // type: Function*
};



void Module_create(Module* module);
void Module_delete(Module* module);

void Module_generateFilesScopes(Module* module);
void Module_readDeclarations(Module* module);
void Module_generateDefinitions(Module* module);
ScopeFile* Module_findModuleFile(Module* module, label_t target);

Variable* Module_searchVariable(Module* module, label_t name, ScopeSearchArgs* args);
Class* Module_searchClass(Module* module, label_t name, ScopeSearchArgs* args);
Function* Module_searchFunction(Module* module, label_t name, ScopeSearchArgs* args);

void Module_addVariable(Module* module, Variable* v);
void Module_addClass(Module* module, Class* cl);
void Module_addFunction(Module* module, Function* fn, int addFlag);



#endif



