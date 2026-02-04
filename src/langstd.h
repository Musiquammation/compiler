#pragma once

#include "declarations.h"

typedef struct {
	Class* pointer;
	Class* type;
	Class* token;

	Function* pointer_fastAccess;
} langstd_t;

extern langstd_t _langstd;


