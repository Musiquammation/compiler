#ifndef COMPILER_PROPERTY_H_
#define COMPILER_PROPERTY_H_

#include "declarations.h"

#include <tools/Array.h>

structdef(ExpressionProperty);

struct Property {
	Variable* defined;
	Property* children;
	ExpressionProperty* expression; // to copy
};


void Property_fill(Property* property, ExpressionProperty* parent, Variable* variable, const TypeCall* typeCall);

void Property_delete(Property* property);


Expression* Property_getValue(ExpressionProperty* property, Variable* variable);

#endif
