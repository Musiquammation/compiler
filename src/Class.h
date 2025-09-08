#ifndef COMPILER_CLASS_H_
#define COMPILER_CLASS_H_

#include "declarations.h"
#include "definitionState_t.h"

#include <tools/Array.h>

struct Class {
	Variable* variables; // type: Variable*
	int vlength;
	definitionState_t definitionState;
	label_t name;
};


void Class_create(Class* class);
void Class_delete(Class* class);


#endif