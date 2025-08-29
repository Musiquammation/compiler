#ifndef COMPILER_TYPE_H_
#define COMPILER_TYPE_H_

#include "declarations.h"

#include "Expression.h"

struct Type {
	Expression prefix;
	const Class* class;

	// liste des propriétés vérifiées
};
	

void Type_delete(Type* property);

#endif