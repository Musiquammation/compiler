#include "Function.h"

#include "Type.h"
#include "Variable.h"
#include "Scope.h"
#include "Class.h"
#include "Interpreter.h"

#include "helper.h"

#include <string.h>

long functionNextId = 0;

void Function_create(Function* fn) {
	fn->stdBehavior = -1;
	fn->traceId = functionNextId;
	fn->interpreter = NULL;
	functionNextId++;
}

void Function_delete(Function* fn) {
	typedef Variable* vptr_t;
	if (fn->metaDefinitionState == DEFINITIONSTATE_DONE) {
		Function_delete(fn->meta);
		free(fn->meta);
	}

	if (fn->arguments) {
		Array_for(vptr_t, fn->arguments, fn->args_len, vptr) {
			Variable* v = *vptr;
			Variable_destroy(v);
			free(v);
		}
		free(fn->arguments);
	}

	if (fn->returnType) {
		Prototype_free(fn->returnType, true);
	}

	if (fn->settings) {
		Array_for(vptr_t, fn->settings, fn->settings_len, vptr) {
			Variable* v = *vptr;
			Variable_destroy(v);
			free(v);
		}
		free(fn->settings);
	}

	if (fn->interpreter) {
		Interpreter_delete(fn->interpreter);
		free(fn->interpreter);
	}
}


label_t Function_generateMetaName(label_t name, char addChar) {
	if (name == NULL)
		return NULL;

	size_t length = strlen(name);
	char* dest = malloc(length+2);
	memcpy(dest, name, length);
	dest[length] = addChar;
	dest[length+1] = 0;
	label_t result = LabelPool_push(&_labelPool, dest);
	free(dest);

	return result;
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
	int argLength = scope->fn->args_len;
	Array_create(&scope->types, sizeof(TypeDefinition));

	Variable** arguments = scope->fn->arguments;
	for (int i = 0; i < argLength; i++) {
		Variable* v = arguments[i];
		TypeDefinition* def = Array_push(TypeDefinition, &scope->types);
		Prototype_reachMetaSizes(v->proto, &scope->scope, true);

		Type* type = Prototype_generateType(v->proto, &scope->scope);
		def->variable = v;
		def->type = type;
	}
}

void ScopeFunction_delete(ScopeFunction* scope) {
	int arglen = scope->fn->args_len;
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
	raiseError("[TODO]: addVariable");
	/// TODO: let this code ?
	// ScopeFunction_pushVariable(scope, v, v->proto, NULL);
}

void ScopeFunction_addClass(ScopeFunction* scope, Class* cl) {
	raiseError("[TODO]");

}

void ScopeFunction_addFunction(ScopeFunction* scope, Function* fn) {
	raiseError("[TODO]");
}


void ScopeFunction_pushVariable(ScopeFunction* scope, Variable* v, Prototype* proto, Type* type) {
	if (Prototype_mode(*proto) == PROTO_MODE_REFERENCE) {
		raiseError("[Syntax] Cannot create a variable whose type is a reference of an other type");
	}

	TypeDefinition* td = Array_push(TypeDefinition, &scope->types);
	td->type = type;
	td->variable = v;
}


Type* ScopeFunction_quickSearchMetaBlock(ScopeFunction* scope, Variable* variable) {
	Array_loop(TypeDefinition, scope->types, m)
		if (m->variable == variable)
			return m->type;

	return NULL;
}



Type* ScopeFunction_searchType(ScopeFunction* scope, Variable* variable) {
	Array_loop(TypeDefinition, scope->types, t)
		if (t->variable == variable)
			return t->type;
	
	return NULL;
}




