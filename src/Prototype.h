#ifndef COMPILER_PROTOTYPE_H_
#define COMPILER_PROTOTYPE_H_

#include "declarations.h"

struct Prototype {
	Class* cl;
	bool isPrimitive;
};


void Prototype_generateType(Prototype* proto, Type* type);


#endif