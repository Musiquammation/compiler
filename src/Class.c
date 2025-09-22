#include "Class.h"

#include "Scope.h"
#include "Variable.h"

void Class_create(Class* cl) {
	Array_create(&cl->variables, sizeof(Variable*));
	cl->definitionState = DEFINITIONSTATE_NOT;
	cl->size = -1; // size is undefined
}


void Class_delete(Class* cl) {
	// Delete variables
	Array_loopPtr(Variable, cl->variables, vPtr) {
		Variable* v = *vPtr;
		Variable_delete(v);
		free(v);
	}

	Array_free(cl->variables);
}




void ScopeClass_delete(ScopeClass* scope) {

}



Variable* ScopeClass_searchVariable(ScopeClass* scope, label_t name, ScopeSearchArgs* args) {
	Array_loopPtr(Variable, scope->cl->variables, ptr) {
		Variable* v = *ptr;
		if (v->name == name)
			return v;
	}
	
	return NULL;
}

Class* ScopeClass_searchClass(ScopeClass* scope, label_t name, ScopeSearchArgs* args) {
	return NULL;
}

Function* ScopeClass_searchFunction(ScopeClass* scope, label_t name, ScopeSearchArgs* args) {
	return NULL;
}


void ScopeClass_addVariable(ScopeClass* scope, Variable* v) {
	raiseError("[TODO]");

}

void ScopeClass_addClass(ScopeClass* scope, Class* cl) {
	raiseError("[TODO]");

}

void ScopeClass_addFunction(ScopeClass* scope, Function* fn) {
	raiseError("[TODO]");
}

