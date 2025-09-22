#ifndef COMPILER_VARIABLE_H_
#define COMPILER_VARIABLE_H_

#include "label_t.h"
#include "declarations.h"

#include "Prototype.h"


struct Variable {
	label_t name;
	Prototype proto;
	int offset;
};

void Variable_create(Variable* variable);
void Variable_delete(Variable* variable);


#endif