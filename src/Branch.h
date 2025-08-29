#ifndef COMPILER_BRANCH_H_
#define COMPILER_BRANCH_H_

#include "declarations.h"

#include <tools/Array.h>

struct Branch {
	Branch* parent;

	Array properties; // type: Property*
	Array types; // type: Type*
};


#endif