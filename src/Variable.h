#pragma once

#include "label_t.h"
#include "declarations.h"

#include "Prototype.h"


struct Variable {
	label_t name;
	Variable* meta;
	Prototype* proto;
	union {int offset; int id;};
};

void Variable_create(Variable* variable);
void Variable_delete(Variable* variable);
void Variable_destroy(Variable* variable);



