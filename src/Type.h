#ifndef COMPILER_TYPE_H_
#define COMPILER_TYPE_H_

#include "declarations.h"
#include <tools/Array.h>

typedef void* mblock_t;

struct Type {
	Prototype* proto;
	mblock_t data;
	char primitiveSizeCode;
};

Type* Type_newCopy(Type* src);
void Type_free(Type* type);


#endif