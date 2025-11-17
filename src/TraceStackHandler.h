#ifndef COMPILER_TRACESTACKHANDLER_H_
#define COMPILER_TRACESTACKHANDLER_H_

#include "declarations.h"
#include <tools/Array.h>

typedef struct TraceStackHandlerHole {
	int position;
	int size;
	struct TraceStackHandlerHole* next;
} TraceStackHandlerHole;

typedef struct {
	int position;
	int freeze;
} TraceStackHandlerPosition;

struct TraceStackHandler {
	int totalSize;
	TraceStackHandlerPosition* positions;
	TraceStackHandlerHole* holes;
};

void TraceStackHandler_create(TraceStackHandler* handler, int labelLength);
void TraceStackHandler_free(TraceStackHandler* handler);
int TraceStackHandler_add(TraceStackHandler* handler, int size, int anchor, int label);
int TraceStackHandler_reach(TraceStackHandler* handler, int label);
int TraceStackHandler_guarantee(TraceStackHandler* handler, int size, int anchor, int label);
void TraceStackHandler_remove(TraceStackHandler* handler, int label, int size);

void TraceStackHandler_freeze(TraceStackHandler* handler, int label);
void TraceStackHandler_unfreeze(TraceStackHandler* handler, int label);

#endif