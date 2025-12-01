#ifndef COMPILER_SCOPEBUFFER_H_
#define COMPILER_SCOPEBUFFER_H_

#include "Scope.h"
#include "Class.h"

union ScopeBuffer {
    ScopeClass cl;
};

#endif