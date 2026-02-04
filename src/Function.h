#pragma once

#include "declarations.h"
#include "label_t.h"
#include "definitionState_t.h"
#include "Scope.h"

#include <stdio.h>
#include "tools/Array.h"

extern long functionNextId;

typedef struct {
	label_t object;
	label_t fn;
} labelRequireCouple_t;

typedef struct {
	int position;
	Function* fn;
} realRequireCouple_t;


enum {
	FUNCTIONFLAGS_SEPARATED = 1,
	FUNCTIONFLAGS_THIS = 2,
	FUNCTIONFLAGS_INTERPRET = 4,
	FUNCTIONFLAGS_ARGUMENT_CONSTRUCTOR = 8,
	FUNCTIONFLAGS_TEST = 16,
	FUNCTIONFLAGS_CONDITION = 32,
	FUNCTIONFLAGS_REAL_REQUIRES = 64,
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
	Prototype* returnPrototype;
	Interpreter* interpreter;
	
	struct {
		label_t pass;
		label_t miss;
	} testOrCond;

	union {
		labelRequireCouple_t* labelRequires;
		realRequireCouple_t* realRequires;
	};

	int requires_len;

	int stdBehavior;
	int flags;
	long traceId;
};

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


void Function_makeRequiresReal(Function* fn, Class* thisclass);

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


