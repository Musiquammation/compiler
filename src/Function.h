#ifndef COMPILER_FUNCTION_H_
#define COMPILER_FUNCTION_H_

#include "declarations.h"
#include "label_t.h"
#include "definitionState_t.h"
#include "Scope.h"

#include <stdio.h>
#include "tools/Array.h"

extern long functionNextId;


enum {
	FUNCTIONFLAGS_SEPARATED = 1,
	FUNCTIONFLAGS_THIS = 2,
	FUNCTIONFLAGS_INTERPRET = 4
};


struct Function {
	label_t name;
	definitionState_t definitionState;
	definitionState_t metaDefinitionState;
	
	Function* meta;
	
	Variable** arguments;
	Variable** settings;
	int args_len;
	int settings_len;
	Prototype* returnType;
	Interpreter* interpreter;
	
	
	long traceId;
	int stdBehavior;
	int flags;
};

/// TODO: init defnodes
struct ScopeFunction {
	Scope scope;
	Function* fn;

	Variable* thisvar;
	Class* thisclass;
	Array types; // type: TypeDefinition
};



struct FunctionAssembly {
	Function* fn;
	FILE* output;
};

typedef struct {
	Variable* variable;
	Type* type;
} TypeDefinition;

void Function_create(Function* fn);
void Function_delete(Function* fn);
label_t Function_generateMetaName(label_t name, char addChar);



void FunctionAssembly_create(FunctionAssembly* fa, ScopeFunction* sf);
void FunctionAssembly_delete(FunctionAssembly* fa);


void ScopeFunction_create(ScopeFunction* scope);
void ScopeFunction_delete(ScopeFunction* scope);

Variable* ScopeFunction_searchVariable(ScopeFunction* scope, label_t name, ScopeSearchArgs* args);
Class* ScopeFunction_searchClass(ScopeFunction* scope, label_t name, ScopeSearchArgs* args);
Function* ScopeFunction_searchFunction(ScopeFunction* scope, label_t name, ScopeSearchArgs* args);

void ScopeFunction_addVariable(ScopeFunction* scope, Variable* v);
void ScopeFunction_addClass(ScopeFunction* scope, Class* cl);
void ScopeFunction_addFunction(ScopeFunction* scope, Function* fn);

void ScopeFunction_pushVariable(ScopeFunction* scope, Variable* v, Prototype* proto, Type* type);

Type* ScopeFunction_quickSearchMetaBlock(ScopeFunction* scope, Variable* variable);



Type* ScopeFunction_searchType(ScopeFunction* scope, Variable* variable);

#endif

