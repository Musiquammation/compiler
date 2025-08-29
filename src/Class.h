#ifndef COMPILER_CLASS_H_
#define COMPILER_CLASS_H_

#include "declarations.h"

#include <tools/Array.h>

struct Class {
	Array variables; // type: Variable*
	label_t name;
};


void Class_create(Class* class);
void Class_delete(Class* class);

#endif