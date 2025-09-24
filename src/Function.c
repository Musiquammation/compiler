#include "Function.h"

#include "Variable.h"
#include "Scope.h"


void Function_create(Function* fn) {

}

void Function_delete(Function* fn) {
    Array_loopPtr(Variable, fn->arguments, v) {
        free(*v);
    }

    Array_free(fn->arguments);
}






void ScopeFunction_create(ScopeFunction* scope) {
	Array_create(&scope->variables, sizeof(Variable*));
	scope->rootNode.length = 0;
}

void ScopeFunction_delete(ScopeFunction* scope) {
	TypeNode_unfollow(&scope->rootNode, 1);

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
	TypeNode* node = malloc(sizeof(TypeNode));
	*Array_push(Variable*, &scope->variables) = v;
	
	node->length = 0;
	node->usage = 0;
	/// TODO: define type and remove v
	node->value.v = v;
	
	TypeNode_set(&scope->rootNode, &v, node, 1, false);
}

void ScopeFunction_addClass(ScopeFunction* scope, Class* cl) {
	raiseError("[TODO]");

}

void ScopeFunction_addFunction(ScopeFunction* scope, Function* fn) {
	raiseError("[TODO]");
}