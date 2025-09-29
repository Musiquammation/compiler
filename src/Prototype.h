#ifndef COMPILER_PROTOTYPE_H_
#define COMPILER_PROTOTYPE_H_

#include "declarations.h"

struct Prototype {
	Class* cl;
	char isPrimitive;
};


Type* Prototype_generateType(Prototype* proto);

bool Prototype_accepts(const Prototype* proto, const Type* type);

#endif