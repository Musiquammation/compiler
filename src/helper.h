#ifndef COMPILER_HELPER_H_
#define COMPILER_HELPER_H_

#include <stdbool.h>

bool isNullPointerRef(const void*);


void raiseError(const char* reason);

#endif