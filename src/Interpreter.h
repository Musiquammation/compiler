#ifndef COMPILER_INTERPRETER_H_
#define COMPILER_INTERPRETER_H_

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


Type* Intrepret_call(Expression* fncallExpr, Scope* scope);

Type* Intrepret_interpret(
	const Interpreter* interpreter,
	Scope* scope,
	Expression** args,
	int argsLen,
	int startArgIndex,
	bool useThis
);


#endif