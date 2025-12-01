#include "Function.h"

#include "Variable.h"
#include "Scope.h"
#include "Class.h"

#include "helper.h"

#include <string.h>



void Function_create(Function* fn) {

}

void Function_delete(Function* fn) {
	Array_loopPtr(Variable, fn->arguments, vptr) {
		Variable* v = *vptr;
		Variable_destroy(v);
		free(v);
	}

	Array_free(fn->arguments);
}

void FunctionAssembly_create(FunctionAssembly* fa, ScopeFunction* sf) {
	ScopeFile* file = Scope_reachFile(&sf->scope);

	fa->output = ScopeFile_requireAssembly(file, file->generationState);
	fa->fn = sf->fn;
}

void FunctionAssembly_delete(FunctionAssembly* fa) {

}



void ScopeFunction_create(ScopeFunction* scope) {
	// Add defintions
	int argLength = scope->fn->arguments.length;
	Array_create(&scope->types, sizeof(TypeDefinition));

	Variable** arguments = scope->fn->arguments.data;
	for (int i = 0; i < argLength; i++) {
		Variable* v = arguments[i];
		TypeDefinition* def = Array_push(TypeDefinition, &scope->types);
		Prototype_reachMetaSizes(v->proto, &scope->scope, true);

		Type* type = Prototype_generateType(v->proto);
		def->variable = v;
		def->type = type;
	}
}

void ScopeFunction_delete(ScopeFunction* scope) {
	int arglen = scope->fn->arguments.length;
	int typelen = scope->types.length;
	TypeDefinition* types = scope->types.data;
	
	for (int i = 0; i < typelen; i++) {
		TypeDefinition* td = &types[i];
		Type_free(td->type);
		
		if (i >= arglen) {
			Variable* v = td->variable;
			Variable_destroy(v);
			free(v);
		}
	}
	
	Array_free(scope->types);
	
}



Variable* ScopeFunction_searchVariable(ScopeFunction* scope, label_t name, ScopeSearchArgs* args) {
	if (name == _commonLabels._this && scope->thisvar)
		return scope->thisvar;

	Array_loop(TypeDefinition, scope->types, td) {
		if (td->variable->name == name)
			return td->variable;
	}

	return NULL;
}

Class* ScopeFunction_searchClass(ScopeFunction* scope, label_t name, ScopeSearchArgs* args) {
	return NULL;
}

Function* ScopeFunction_searchFunction(ScopeFunction* scope, label_t name, ScopeSearchArgs* args) {
	return NULL;
}


void ScopeFunction_addVariable(ScopeFunction* scope, Variable* v) {
	/// TODO: let this code ?
	ScopeFunction_pushVariable(scope, v, NULL);
}

void ScopeFunction_addClass(ScopeFunction* scope, Class* cl) {
	raiseError("[TODO]");

}

void ScopeFunction_addFunction(ScopeFunction* scope, Function* fn) {
	raiseError("[TODO]");
}


Type* ScopeFunction_pushVariable(ScopeFunction* scope, Variable* v, Expression* value) {
	TypeDefinition* td = Array_push(TypeDefinition, &scope->types);
	Type* type = Prototype_generateType(v->proto);
	td->type = type;
	td->variable = v;

	return type;
}


Type* ScopeFunction_quickSearchMetaBlock(ScopeFunction* scope, Variable* variable) {
	Array_loop(TypeDefinition, scope->types, m)
		if (m->variable == variable)
			return m->type;

	return NULL;
}