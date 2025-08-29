#ifndef COMPILER_PROPERTY_H_
#define COMPILER_PROPERTY_H_

#include "declarations.h"
#include "Expression.h"


#include <tools/Array.h>


struct Property {
	Expression prefix;
	Type* type;
	Property* parent;
	Property** children;
	const Variable* variable;
};



void Property_fill(Property* property, Property* parent, const Variable* variable, const TypeCall* typeCall);

void Property_unfollowAsNull(Property* property);

void Property_delete(Property* property);

Property* Property_getSubProperty(Property* property, const Variable* variable);

Property* Property_copy(Property* property);


#endif
