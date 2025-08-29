#include "Scope.h"

#include "Property.h"
#include "Expression.h"
#include "Variable.h"
#include "Class.h"
#include "Function.h"

#include <stdio.h>


void Scope_create(Scope* scope) {
	Array_create(&scope->variables, sizeof(Variable*));
	Array_create(&scope->classes, sizeof(Class*));
	Array_create(&scope->functions, sizeof(Function*));
	Array_create(&scope->properties, sizeof(Property*));
	
}

void Scope_delete(Scope* scope) {
	Array_loopPtr(Property, scope->properties, p_ptr) {
		Property* p = *p_ptr;
		Property_delete(p);
		free(p);
	}

	Array_loopPtr(void, scope->variables, i) {free(*i);}
	Array_loopPtr(void, scope->classes, i) {free(*i);}
	Array_loopPtr(void, scope->functions, i) {free(*i);}

	Array_free(scope->variables);
	Array_free(scope->classes);
	Array_free(scope->functions);
	Array_free(scope->properties);
}


void Scope_pushVariable(Scope* scope, label_t name, const TypeCall* typeCall, Expression* value) {
	Variable* variable = malloc(sizeof(Variable));
	variable->name = name;
	variable->typeCall = *typeCall;
	*Array_push(Variable*, &scope->variables) = variable;
	
	Property* property = malloc(sizeof(Property));
	Property_fill(property, NULL, variable, typeCall);
	*Array_push(Property*, &scope->properties) = property;

	if (value) {
		ExpressionProperty* expression = property->expression;
		expression->value = value;
		expression->value = value;
		Expression_follow(value, Expression_cast(expression));
	} else {
		
	}
}

