#ifndef COMPILER_LANGSTD_H_
#define COMPILER_LANGSTD_H_

#include "declarations.h"

typedef struct {
	Class* pointer;
	Class* type;
	Class* variadic;
} langstd_t;

extern langstd_t _langstd;

#endif
