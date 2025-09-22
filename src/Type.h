#ifndef COMPILER_TYPE_H_
#define COMPILER_TYPE_H_

#include "declarations.h"
#include <tools/Array.h>

struct Type {
	Class* cl;
	bool isPrimitive;
};


#endif