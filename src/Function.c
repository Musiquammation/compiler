#include "Function.h"

#include "Variable.h"
#include "Scope.h"
#include "TypeNode.h"

#include "helper.h"


void Function_create(Function* fn) {

}

void Function_delete(Function* fn) {
	Array_loopPtr(Variable, fn->arguments, v) {
		free(*v);
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
	Array_create(&scope->variables, sizeof(Variable*));
}

void ScopeFunction_delete(ScopeFunction* scope) {
	Array_loopPtr(Variable, scope->variables, v_ptr) {
		Variable* v = *v_ptr;
		Variable_delete(v);
		free(v);
	} 

	Array_free(scope->variables);
}



Variable* ScopeFunction_searchVariable(ScopeFunction* scope, label_t name, ScopeSearchArgs* args) {
	Array_loopPtr(Variable, scope->variables, ptr) {
		Variable* v = *ptr;
		if (v->name == name)
			return v;
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
	ScopeFunction_pushVariable(scope, v, NULL)->value.type = NULL;
}

void ScopeFunction_addClass(ScopeFunction* scope, Class* cl) {
	raiseError("[TODO]");

}

void ScopeFunction_addFunction(ScopeFunction* scope, Function* fn) {
	raiseError("[TODO]");
}


TypeNode* ScopeFunction_pushVariable(ScopeFunction* scope, Variable* v, Expression* value) {
	*Array_push(Variable*, &scope->variables) = v;
	return TypeNode_push(scope->rootNode, v, value);
}