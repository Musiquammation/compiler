#ifndef COMPILER_TYPECALL_H_
#define COMPILER_TYPECALL_H_

#include "declarations.h"

struct TypeCall {
	const Class* class;
};

void TypeCall_generateType(const TypeCall* tcall, Type* type);

#endif