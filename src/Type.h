#ifndef COMPILER_TYPE_H_
#define COMPILER_TYPE_H_

#include "declarations.h"
#include <tools/Array.h>

struct Type {
	Class* cl;
	char isPrimitive;
	void* data;
};

Type* Type_newCopy(Type* src);
void Type_free(Type* type);


#endif