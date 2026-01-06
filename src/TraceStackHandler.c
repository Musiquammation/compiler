#include "declarations.h"
#include "TraceStackHandler.h"


// --- Initialize stack handler ---
void TraceStackHandler_create(TraceStackHandler* handler, int labelLength) {
	handler->totalSize = 0;
	handler->rsp = 0;
	handler->holes = NULL;
	handler->positions = malloc(sizeof(TraceStackHandlerPosition) * labelLength);
	for (int i = 0; i < labelLength; i++) {
		handler->positions[i].position = -1;
		handler->positions[i].freeze = 0;
	}
}

// --- Free memory used by stack handler ---
void TraceStackHandler_free(TraceStackHandler* handler) {
	free(handler->positions);

	TraceStackHandlerHole* hole = handler->holes;
	while (hole) {
		TraceStackHandlerHole* next = hole->next;
		free(hole);
		hole = next;
	}
}

// --- Internal helper: find aligned position ---
static int alignPosition(int pos, int anchor) {
	if (pos % anchor == 0) return pos;
	return pos + (anchor - (pos % anchor));
}

// --- Add a new variable in the stack ---
TraceStackHandlerRspAdapter TraceStackHandler_add(TraceStackHandler* handler, int size, int anchor, int label) {
	TraceStackHandlerHole** prev = &handler->holes;
	TraceStackHandlerHole* hole = handler->holes;

	// Search for a hole that fits
	while (hole) {
		if (hole->size >= size) {
			int addr = alignPosition(hole->position, anchor);
			int move;
			
			// Update hole
			if (hole->size == size) {
				*prev = hole->next;
				free(hole);
			} else {
				hole->position += size;
				hole->size -= size;
			}

			if (label >= 0) {
				handler->positions[label].position = addr;
			}
			handler->totalSize = (addr + size > handler->totalSize) ? addr + size : handler->totalSize;

			return (TraceStackHandlerRspAdapter){addr, 0};
		}
		prev = &hole->next;
		hole = hole->next;
	}

	// No suitable hole, append at the end
	int addr = alignPosition(handler->totalSize, anchor);
	int move = addr + size - handler->rsp;
	if (move < 0) {move = 0;}

	if (label >= 0) {
		handler->positions[label].position = addr;
	}

	handler->totalSize = addr + size;

	return (TraceStackHandlerRspAdapter){addr, -move};
}

TraceStackHandlerRspAdapter TraceStackHandler_guarantee(TraceStackHandler* handler, int size, int anchor, int label) {
	if (handler->positions[label].position >= 0) {
		return (TraceStackHandlerRspAdapter){handler->positions[label].position, 0};
	}
	
	return TraceStackHandler_add(handler, size, anchor, label);
}

int TraceStackHandler_reach(TraceStackHandler* handler, int label) {
	return handler->positions[label].position;
}


// --- Remove variable from stack ---
int TraceStackHandler_remove(TraceStackHandler* handler, int label, int size) {
	int pos;
	if (label >= 0) {
		// If frozen, do nothing
		if (handler->positions[label].freeze > 0) return 0;
		
		pos = handler->positions[label].position;
		if (pos < 0) return 0;	// Already removed
	} else {
		pos = -label;
	}

	int oldRsp = handler->rsp;
	int rspAdjust = 0;

	// Case 1: The block is at the top of the stack
	if (pos + size == handler->totalSize) {

		// Shrink totalSize by this block
		handler->totalSize = pos;

		// Try to collapse consecutive holes at the new top
		TraceStackHandlerHole** prev = &handler->holes;
		TraceStackHandlerHole* h = handler->holes;

		while (h) {
			if (h->position + h->size == handler->totalSize) {
				// Extend the top downward
				handler->totalSize = h->position;

				// Remove this hole from the list
				*prev = h->next;
				free(h);

				// Restart scanning (safe because list is small)
				prev = &handler->holes;
				h = handler->holes;
				continue;
			}
			prev = &h->next;
			h = h->next;
		}

		// After collapsing, compute how much rsp must grow
		rspAdjust = oldRsp - handler->totalSize;
	}
	else {
		// Case 2: The block is not at the top → create a hole
		TraceStackHandlerHole* newHole = malloc(sizeof(TraceStackHandlerHole));

		newHole->position = pos;
		newHole->size = size;
		newHole->next = handler->holes;
		handler->holes = newHole;

		// No rsp adjust here
		rspAdjust = 0;
	}

	if (label >= 0) {
		handler->positions[label].position = -1;
	}

	return rspAdjust;
}




static void TraceStackHandler_realAdaptAllocation(TraceStackHandler* handler, int move, FILE* output, const char* suffix) {
	if (move == 0)
		return;

	if (move <= 0) {
		// Allow data
		handler->rsp -= move;
	} else {
		// Free data
		handler->rsp -= move;
	}
}


// --- Handle push operation (simulate stack growth) ---
int TraceStackHandler_handlePush(TraceStackHandler* handler, int size, FILE* output) {
	TraceStackHandlerRspAdapter adp = TraceStackHandler_add(handler, size, size, -1);
	TraceStackHandler_realAdaptAllocation(handler, adp.rspMove, output, "; for push\n");
	return adp.address;
}

// --- Handle pop operation (simulate stack shrink) ---
void TraceStackHandler_handlePop(TraceStackHandler* handler, int size, int data, FILE* output) {
	int move = TraceStackHandler_remove(handler, -data, size);
	TraceStackHandler_realAdaptAllocation(handler, move, output, "; from pop\n");
}

// --- Adapt RSP output for assembler ---
void TraceStackHandler_adaptAllocation(TraceStackHandler* handler, int move, FILE* output) {
	TraceStackHandler_realAdaptAllocation(handler, move, output, "\n");
}


// --- Freeze/unfreeze positions ---
void TraceStackHandler_freeze(TraceStackHandler* handler, int label) {
	handler->positions[label].freeze++;
}

void TraceStackHandler_unfreeze(TraceStackHandler* handler, int label) {
	handler->positions[label].freeze--;
}
