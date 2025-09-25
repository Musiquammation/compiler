#ifndef COMPILER_FUNCTION_H_
#define COMPILER_FUNCTION_H_

#include "declarations.h"
#include "TypeNode.h"
#include "label_t.h"

#include <tools/Array.h>

struct Function {
    label_t name;
    definitionState_t definitionState;

    Array arguments;
    Prototype returnType;
};


/// TODO: init defnodes
struct ScopeFunction {
	Scope scope;
	Function* fn;
    TypeNode rootNode;

    Array variables; // type: Variable*
    
};

void Function_create(Function* fn);
void Function_delete(Function* fn);





void ScopeFunction_create(ScopeFunction* scope);
void ScopeFunction_delete(ScopeFunction* scope);

Variable* ScopeFunction_searchVariable(ScopeFunction* scope, label_t name, ScopeSearchArgs* args);
Class* ScopeFunction_searchClass(ScopeFunction* scope, label_t name, ScopeSearchArgs* args);
Function* ScopeFunction_searchFunction(ScopeFunction* scope, label_t name, ScopeSearchArgs* args);

void ScopeFunction_addVariable(ScopeFunction* scope, Variable* v);
void ScopeFunction_addClass(ScopeFunction* scope, Class* cl);
void ScopeFunction_addFunction(ScopeFunction* scope, Function* fn);

TypeNode* ScopeFunction_pushVariable(ScopeFunction* scope, Variable* v);


#endif