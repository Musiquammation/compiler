#include "Property.h"

#include "Expression.h"
#include "TypeCall.h"
#include "Class.h"
#include "Variable.h"

void Property_fill(Property* property, ExpressionProperty* parent, Variable* variable, const TypeCall* typeCall) {
	// Generate expression
	ExpressionProperty* expression = malloc(sizeof(ExpressionProperty));
	Expression_create(EXPRESSION_PROPERTY, Expression_cast(expression));
	
	TypeCall_generateType(typeCall, &expression->type);
	expression->variable = variable;

	if (parent) {
		Expression_follow(Expression_cast(expression), Expression_cast(parent));
		expression->parent = parent;
	} else {
		expression->parent = NULL;
	}
	
	Expression_followAsNull(Expression_cast(expression));
	

	expression->value = NULL;



	// Fill defined variables
	const Array* variableArray = &typeCall->class->variables;
	int childrenLength = variableArray->length;
	Variable** variables = variableArray->data;

	Property* children = malloc(sizeof(Property) * childrenLength);
	property->defined = variable;
	property->children = children;
	property->expression = expression;

	for (int i = 0; i < childrenLength; i++) {
		Variable* v = variables[i];
		Property_fill(&children[i], expression, v, &v->typeCall);
	}
}


void Property_delete(Property* property) {
	int childrenLength = property->defined->typeCall.class->variables.length;

	Array_for(Property, property->children, childrenLength, child)
		Property_delete(child);

	Expression* e = Expression_cast(property->expression);
	Expression_unfollowAsNull(e->type, e);
	free(property->children);
}
