#pragma once

#include "declarations.h"
#include "Trace.h"

structdef(TracePack);


typedef struct {
	TracePack* pack;
	trline_t* line;
	int ins;
} InterpretPosition;

struct Interpreter {
	TracePack* pack;
	InterpretPosition* positions;
	int varCount;
	int pos_len;
};


Interpreter* Interpreter_build(Trace* trace);
void Interpreter_delete(Interpreter* itp);


Type* Interpret_call(Expression* fncallExpr, Scope* scope);


Type* Interpret_interpret(
	const Interpreter* interpreter,
	Scope* scope,
	Expression** args,
	int argsLen,
	int startArgIndex,
	bool useThis
);

