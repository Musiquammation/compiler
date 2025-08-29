#include "Property.h"

#include "Expression.h"
#include "Type.h"

#include "TypeCall.h"
#include "Class.h"
#include "Variable.h"


void Property_fill(Property* property, Property* parent, const Variable* variable, const TypeCall* typeCall) {
	// Generate expression
	Expression_create(EXPRESSION_PROPERTY, Expression_cast(property));
	
	Type* type = malloc(sizeof(Type));
	TypeCall_generateType(typeCall, type);
	property->type = type;
	property->variable = variable;

	Expression_follow(Expression_cast(property), Expression_cast(type));

	if (parent) {
		Expression_follow(Expression_cast(property), Expression_cast(parent));
		property->parent = parent;
	} else {
		property->parent = NULL;
	}
	
	// Scope following
	Expression_followAsNull((Expression*)property);


	// Fill defined variables
	const Array* variableArray = &typeCall->class->variables;
	int childrenLength = variableArray->length;
	Variable** variables = variableArray->data;

	Property** children = malloc(sizeof(Property*) * childrenLength);
	property->children = children;

	for (int i = 0; i < childrenLength; i++) {
		Variable* v = variables[i];
		Property* child = malloc(sizeof(Property));
		children[i] = child;
		Property_fill(child, property, v, &v->typeCall);
	}
}


void Property_unfollowAsNull(Property* property) {
	typedef Property* Property_ptr_t;

	int length = property->type->class->variables.length;
	Array_for(Property_ptr_t, property->children, length, child)
		Property_unfollowAsNull(*child);

	Expression_unfollowAsNull(EXPRESSION_PROPERTY, Expression_cast(property));
}

void Property_delete(Property* property) {
	int childrenLength = property->type->class->variables.length;
	Expression_unfollow(Expression_cast(property), Expression_cast(property->type));

	Property* parent = property->parent;
	if (parent) {
		Expression_unfollow(Expression_cast(property), Expression_cast(property->parent));
	}
	free(property->children);
}


Property* Property_getSubProperty(Property* property, const Variable* variable) {
	int length = property->type->class->variables.length;
	
	typedef Property* Property_ptr_t;
	Array_for(Property_ptr_t, property->children, length, child_ptr) {
		Property* child = *child_ptr;
		if (child->variable == variable)
			return child;
	}

	return NULL;	
}


Property* Property_copy(Property* property) {
	/// TODO: this fonction
}