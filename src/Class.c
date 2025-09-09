#include "Class.h"

#include "Scope.h"

void Class_create(Class* cl) {
	Array_create(&cl->variables, sizeof(Variable*));
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



Variable* ScopeClass_searchVariable(ScopeClass* scl, label_t name, ScopeSearchArgs* args) {
	return NULL;
}

Class* ScopeClass_searchClass(ScopeClass* scl, label_t name, ScopeSearchArgs* args) {
	return NULL;
}

Function* ScopeClass_searchFunction(ScopeClass* scl, label_t name, ScopeSearchArgs* args) {
	return NULL;
}


void ScopeClass_addVariable(ScopeClass* scl, Variable* v) {

}

void ScopeClass_addClass(ScopeClass* scl, Class* cl) {

}

void ScopeClass_addFunction(ScopeClass* scl, Function* fn) {

}

