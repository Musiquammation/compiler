#ifndef COMPILER_TRACESTACKHANDLER_H_
#define COMPILER_TRACESTACKHANDLER_H_

#include "declarations.h"
#include <tools/Array.h>

typedef struct TraceStackHandlerHole {
    int position;
    int size;
    struct TraceStackHandlerHole* next;
} TraceStackHandlerHole;

struct TraceStackHandler {
    int totalSize;
    int* labels;
    TraceStackHandlerHole* holes;
};

void TraceStackHandler_create(TraceStackHandler* handler, int labelLength);
void TraceStackHandler_free(TraceStackHandler* handler);
int TraceStackHandler_add(TraceStackHandler* handler, int size, int anchor, int label);
int TraceStackHandler_reach(TraceStackHandler* handler, int label);
int TraceStackHandler_guarantee(TraceStackHandler* handler, int size, int anchor, int label);
int TraceStackHandler_remove(TraceStackHandler* handler, int label, int size);
	

#endif