#ifndef COMPILER_CLASS_H_
#define COMPILER_CLASS_H_

#include "declarations.h"
#include "definitionState_t.h"

#include <tools/Array.h>

struct Class {
	Array variables; // type: Variable*
	definitionState_t definitionState;
	label_t name;
};

struct ScopeClass {
	Scope scope;
	Class* cl;
};

void Class_create(Class* class);
void Class_delete(Class* class);

void ScopeClass_delete(ScopeClass* scope);

Variable* ScopeClass_searchVariable(ScopeClass* scl, label_t name, ScopeSearchArgs* args);
Class* ScopeClass_searchClass(ScopeClass* scl, label_t name, ScopeSearchArgs* args);
Function* ScopeClass_searchFunction(ScopeClass* scl, label_t name, ScopeSearchArgs* args);

void ScopeClass_addVariable(ScopeClass* scl, Variable* v);
void ScopeClass_addClass(ScopeClass* scl, Class* cl);
void ScopeClass_addFunction(ScopeClass* scl, Function* fn);


#endif