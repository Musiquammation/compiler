#pragma once

#include "declarations.h"

#include "Variable.h"

typedef struct {
	Class* pointer;
	Class* type;
	Class* token;

	Function* pointer_fastAccess;
} langstd_t;

extern langstd_t _langstd;


void langstd_init();
void langstd_cleanup();

