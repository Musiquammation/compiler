#ifndef COMPILER_INTERPRETER_H_
#define COMPILER_INTERPRETER_H_

#include "Trace.h"
#include "castable_t.h"

typedef union {
	castable_t value;
	void* ptr;
} itpr_var_t;

void Interpreter_start(TracePack* pack, int varlen);



#endif