#ifndef COMPILER_CLASS_H_
#define COMPILER_CLASS_H_

#include "declarations.h"
#include "definitionState_t.h"

#include "Scope.h"

#include <tools/Array.h>

struct Class {
	Array variables; // type: Variable*
	definitionState_t definitionState;
	label_t name;
	char isPrimitive;
	int size;
	int maxMinimalSize;
};

struct ScopeClass {
	Scope scope;
	Class* cl;
	bool allowThis;
};

void Class_create(Class* cl);
void Class_delete(Class* cl);



void ScopeClass_delete(ScopeClass* scope);

Variable* ScopeClass_searchVariable(ScopeClass* scope, label_t name, ScopeSearchArgs* args);
Class* ScopeClass_searchClass(ScopeClass* scope, label_t name, ScopeSearchArgs* args);
Function* ScopeClass_searchFunction(ScopeClass* scope, label_t name, ScopeSearchArgs* args);

void ScopeClass_addVariable(ScopeClass* scope, Variable* v);
void ScopeClass_addClass(ScopeClass* scope, Class* cl);
void ScopeClass_addFunction(ScopeClass* scope, Function* fn);


#endif