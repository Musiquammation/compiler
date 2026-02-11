#pragma once

#include "declarations.h"
#include "Trace.h"
#include "definitionState_t.h"

#include <stdint.h>

structdef(TracePack);


typedef union {
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;
	void* ptr;
} interpreterSlot_t;


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

interpreterSlot_t Interpret_interpret(
	const Interpreter* interpreter,
	Scope* scope,
	Expression** args,
	int argsLen,
	int startArgIndex,
	bool useThis,
	bool shouldReturn
);


bool Interpreter_checkDefinitionState(definitionState_t definitionState);