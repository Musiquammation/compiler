#pragma once

#include <stdio.h> // for FILE

#include "declarations.h"
#include "tools/Array.h"

typedef struct TraceStackHandlerHole {
	int position;
	int size;
	struct TraceStackHandlerHole* next;
} TraceStackHandlerHole;

typedef struct {
	int position;
	int freeze;
} TraceStackHandlerPosition;

typedef struct {
	int address;
	int rspMove;
} TraceStackHandlerRspAdapter;

struct TraceStackHandler {
	int totalSize;
	int rsp;
	TraceStackHandlerPosition* positions;
	TraceStackHandlerHole* holes;
};



void TraceStackHandler_create(TraceStackHandler* handler, int labelLength);
void TraceStackHandler_free(TraceStackHandler* handler);
TraceStackHandlerRspAdapter TraceStackHandler_add(TraceStackHandler* handler, int size, int anchor, int label);
TraceStackHandlerRspAdapter TraceStackHandler_guarantee(TraceStackHandler* handler, int size, int anchor, int label);
int TraceStackHandler_reach(TraceStackHandler* handler, int label);
int TraceStackHandler_remove(TraceStackHandler* handler, int label, int size);

int TraceStackHandler_handlePush(TraceStackHandler* handler, int size, FILE* output);
void TraceStackHandler_handlePop(TraceStackHandler* handler, int size, int data, FILE* output);
void TraceStackHandler_adaptAllocation(TraceStackHandler* handler, int decalage, FILE* output);

void TraceStackHandler_freeze(TraceStackHandler* handler, int label);
void TraceStackHandler_unfreeze(TraceStackHandler* handler, int label);

