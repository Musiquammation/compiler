#include "Scope.h"

void Scope_create(Scope* scope) {
	Array_create(&scope->variables, sizeof(expressionPtr_t));
	Array_create(&scope->classes, sizeof(expressionPtr_t));
	Array_create(&scope->functions, sizeof(expressionPtr_t));
}

void Scope_delete(Scope* scope) {
	Array_loop(expressionPtr_t, scope->variables, i) {
		Expression_unfollowAsNull(EXPRESSION_VARIABLE, *i);
	}

	Array_loop(expressionPtr_t, scope->classes, i) {
		Expression_unfollowAsNull(EXPRESSION_CLASS, *i);
	}

	Array_loop(expressionPtr_t, scope->functions, i) {
		Expression_unfollowAsNull(EXPRESSION_FUNCTION, *i);
	}

	Array_free(scope->variables);
	Array_free(scope->classes);
	Array_free(scope->functions);
}


void Scope_appendVariable(Scope* scope, Expression* expression) {
	*Array_push(expressionPtr_t, &scope->variables) = expression;
	Expression_followAsNull(expression);
}

void Scope_appendClass(Scope* scope, Expression* expression) {
	*Array_push(expressionPtr_t, &scope->classes) = expression;
	Expression_followAsNull(expression);
}

void Scope_appendFunction(Scope* scope, Expression* expression) {
	*Array_push(expressionPtr_t, &scope->functions) = expression;
	Expression_followAsNull(expression);
}

