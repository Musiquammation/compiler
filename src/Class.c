#include "Class.h"

#include "Scope.h"
#include "Function.h"
#include "Variable.h"

#include "helper.h"

void Class_create(Class* cl) {
	Array_create(&cl->variables, sizeof(Variable*));
	Array_create(&cl->functions, sizeof(Function*));
	cl->definitionState = DEFINITIONSTATE_NOT;
	cl->size = CLASSSIZE_UNDEFINED; // size is undefined
}


void Class_delete(Class* cl) {
	if (cl->meta) {
		Class_delete(cl->meta);
		free(cl->meta);
	}


	// Delete variables
	Array_loopPtr(Variable, cl->variables, ptr) {
		Variable* i = *ptr;
		Variable_delete(i);
		free(i);
	}

	Array_free(cl->variables);

	// Delete functions
	Array_loopPtr(Function, cl->functions, ptr) {
		Function* i = *ptr;
		Function_delete(i);
		free(i);
	}

	Array_free(cl->functions);

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
	Array_loopPtr(Function, scope->cl->functions, ptr) {
		Function* f = *ptr;
		if (f->name == name)
			return f;
	}
	
	return NULL;
}


void ScopeClass_addVariable(ScopeClass* scope, Variable* v) {
	raiseError("[TODO]");

}

void ScopeClass_addClass(ScopeClass* scope, Class* cl) {
	raiseError("[TODO]");

}

void ScopeClass_addFunction(ScopeClass* scope, Function* fn) {
	*Array_push(Function*, &scope->cl->functions) = fn;
}

