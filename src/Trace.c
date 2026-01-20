#include "Trace.h"

#include "chooseSign.h"
#include "TraceStackHandler.h"

#include "Variable.h"
#include "Function.h"
#include "Class.h"
#include "Expression.h"

#include "helper.h"


#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>





#include <stdlib.h>
#include <stdint.h>


// print trace
#if 1
#include <stdarg.h>
void trprintf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
#else
void trprintf(const char* format, ...){}
#endif



static const int ARGUMENT_REGISTERS[] = {
	TRACE_REG_RCX,
	TRACE_REG_R11,
};
enum {
	ARGUMENT_REGISTERS_LENGTH = sizeof(ARGUMENT_REGISTERS)/sizeof(int),
	USE_LENGTH_BITS = 3,
	USE_LENGTH = 1<<3,
};


static size_t hash_ptr(void* ptr) {
	uintptr_t val = (uintptr_t)ptr;
	return (val >> 5) % TRACE_FUNCTIONMAP_LENGTH;
}


static void TraceFunctionMap_create(TraceFunctionMap* map) {
	map->position = 0;

	for (int i = 0; i < TRACE_FUNCTIONMAP_LENGTH; ++i) {
		map->buckets[i].key = NULL;
		map->buckets[i].position = -1;
		map->buckets[i].next = NULL;
	}
}


static void TraceFunctionMap_insert(TraceFunctionMap* map, Function* fn) {
	size_t idx = hash_ptr(fn);
	TraceFunctionMapEntry* entry = &map->buckets[idx];

	if (entry->position == -1) {
		entry->key = fn;
		entry->position = map->position++;
		entry->next = NULL;
	} else {
		TraceFunctionMapEntry* new_entry = malloc(sizeof(TraceFunctionMapEntry));
		new_entry->key = fn;
		new_entry->position = map->position++;
		new_entry->next = entry->next;
		entry->next = new_entry;
	}
}


static int TraceFunctionMap_get(TraceFunctionMap* map, Function* fn) {
	size_t idx = hash_ptr(fn);
	TraceFunctionMapEntry* entry = &map->buckets[idx];

	while (entry) {
		if (entry->key == fn) return entry->position;
		entry = entry->next;
	}
	return -1;
}


static int TraceFunctionMap_reach(TraceFunctionMap* map, Function* fn) {
	size_t idx = hash_ptr(fn);
	TraceFunctionMapEntry* entry = &map->buckets[idx];

	
	while (entry) {
		if (entry->key == fn) return entry->position;
		if (!entry->next) break; 
		entry = entry->next;
	}

	
	TraceFunctionMapEntry* new_entry = malloc(sizeof(TraceFunctionMapEntry));
	new_entry->key = fn;
	int p = map->position++;
	new_entry->position = p;
	new_entry->next = NULL;

	if (entry->position == -1) {
		
		*entry = *new_entry;
		free(new_entry); 
	} else {
		entry->next = new_entry;
	}

	return p;
}


static Function* TraceFunctionMap_getIdx(TraceFunctionMap* map, int idx) {
	if (idx < 0 || idx >= map->position) return NULL; // index invalide

	for (int i = 0; i < TRACE_FUNCTIONMAP_LENGTH; ++i) {
		TraceFunctionMapEntry* entry = &map->buckets[i];

		while (entry) {
			if (entry->position == idx) return entry->key;
			entry = entry->next;
		}
	}

	return NULL; // pas trouvé
}


void TraceFunctionMap_free(TraceFunctionMap* map) {
	for (int i = 0; i < TRACE_FUNCTIONMAP_LENGTH; ++i) {
		TraceFunctionMapEntry* entry = map->buckets[i].next;
		while (entry) {
			TraceFunctionMapEntry* next = entry->next;
			free(entry);
			entry = next;
		}
		map->buckets[i].next = NULL;
	}
}











enum {
	FIRSTREAD_WAITING = -1,
	FIRSTREAD_ELIMINATED = -2,
	FIRSTREAD_STOP = -3,
	LUI_FINAL = (int)-0x80000000,
};

typedef struct {
	trline_t* ptr;
	int lui;
	int scopeId;
	int deep;
	int firstRead;
} Write;

typedef struct {
	union {
		trline_t* lastUsePtr;
		struct {
			Write* writes;
			int* firstReads;
		};
	};

	int lastUseInstruction;
	int scopeId;
	int deep;
	int usageCapacity;
	int globalFirstRead;
	trline_t* creationLinePtr;
} VariableTrace;













typedef int fnplacement_t[TRACE_REG_R11 - TRACE_REG_RAX];

void Trace_create(Trace* trace) {
	TracePack* pack = malloc(sizeof(TracePack));
	/// TODO: remove this line
	memset(pack, -1, sizeof(TracePack));
	pack->completion = 0;
	pack->next = NULL;
	
	trace->first = pack;
	trace->last = pack;
	trace->instruction = 0;



	Array_create(&trace->variables, sizeof(VariableTrace));
	Stack_create(uint, &trace->varPlacements);
	Stack_create(int, &trace->scopeIdStack);
	*Stack_push(int, &trace->scopeIdStack) = 0;
	trace->deep = 0;
	trace->scopeId = 0;
	trace->nextScopeId = 1;
	TraceFunctionMap_create(&trace->calledFunctions);
}


void Trace_delete(Trace* trace, bool hasGeneratedAssembly) {
	TracePack* p = trace->first;

	while (p) {
		TracePack* next = p->next;
		free(p);
		p = next;
	}

	if (hasGeneratedAssembly) {
		Array_free(trace->replaces);
		free(trace->varInfos);
		free(trace->regs);
	} else {
		Array_free(trace->variables);
		Stack_free(trace->varPlacements);
		Stack_free(trace->scopeIdStack);
	}

	TraceFunctionMap_free(&trace->calledFunctions);
}

uint Trace_pushVariable(Trace* trace) {
	uint pos;
	VariableTrace* vt;

	if (Stack_isEmpty(trace->varPlacements)) {
		pos = trace->variables.length;
		vt = Array_push(VariableTrace, &trace->variables);
	} else {
		pos = *Stack_pop(uint, &trace->varPlacements);
		vt = Array_get(VariableTrace, trace->variables, pos);

	}
	
	vt->lastUseInstruction = LUI_FINAL;
	vt->usageCapacity = 0;	
	return pos;
}

void Trace_removeVariable(Trace* trace, uint index) {
	*Stack_push(uint, &trace->varPlacements) = index;
	VariableTrace* vt = Array_get(VariableTrace, trace->variables, index);

	vt->scopeId = -1;
	if (vt->usageCapacity) {
		free(vt->writes);
	}

	/// TODO: move TRACE_USAGE_LAST if variable is referenced (in stack) => call Trace_addUsage

}



int Trace_reachFunction(Trace* trace, Function* fn) {
	return TraceFunctionMap_reach(&trace->calledFunctions, fn);
}

Function* Trace_getFunction(Trace* trace, int index) {
	return TraceFunctionMap_getIdx(&trace->calledFunctions, index);
}


void Trace_pushArgs(Trace* trace, Variable** args, int arglen) {
	for (int i = 0; i < arglen; i++) {
		Variable* v = args[i];
		
		// '2' for argument
		Trace_ins_create(
			trace, v,
			Prototype_getSizes(v->proto).size, 2,
			Prototype_getPrimitiveSizeCode(v->proto)
		);
	}

}





trline_t* Trace_push(Trace* trace, int num) {
	TracePack* pack = trace->last;
	int c = pack->completion;
	int nc = c + num;
	// Follow current block
	if (nc < TRACE_LINE_LEN) {
		pack->completion = nc;
		trace->instruction += num;
		return &pack->line[c];
	}

	// Create a new block
	pack->line[c] = TRACECODE_STAR;

	TracePack* next = malloc(sizeof(TracePack));
	pack->next = next;
	next->next = NULL;
	next->completion = num;
	trace->last = next;
	trace->instruction += num+1;
	return &next->line[0];
}


void TracePack_print(const TracePack* pack, int position) {
	int registrables_len = 32;
	char* registrables = malloc(registrables_len);

	for (int i = 0; true; i++) {
		trline_t n = pack->line[i];
		int next = n & 0x3FF; // always extract next (10 bits)

		// Instruction mode ?
		if (next > TRACE_USAGE_OUT_OF_BOUNDS) {
			switch (next) {
				case TRACECODE_STAR:
					trprintf("[%04d] STAR action=%d\n", i+position, (n>>10)&0xf);
					if ((n>>10) == 0)
						goto finish;
					break;

				case TRACECODE_CREATE: {
					uint32_t n2   = pack->line[++i];
					int flags     = (n >> 10) & 0x3f;
					int variable  = (n >> 16) & 0xFFF;
					int size      = n2 >> 16;
					int regable   = n & (1<<28);
					int next2     = n2& 0x3FF;

					trprintf(
						"[%04d] CREATE v%d size=%d next=+%d {flags=%d} %s\n",
						i-1+position,
						variable,
						size,
						next2,
						flags,
						regable ? "registrable" : "complex"
					);

					if (variable >= registrables_len) {
						int nl = variable * 2;
						char* bff = malloc(nl);
						memcpy(bff, registrables, nl);
						registrables_len = nl;

						free(registrables);
						registrables = bff;
					}

					registrables[variable] = regable ? 1 : 0;

				} break;


				case TRACECODE_DEF:
				{
					int type = (n >> 10) & 0x3;
					if (type == TRACETYPE_S16) {
						int value = (n >> 16) & 0xFFFF;
						trprintf("[%04d] DEF value=%d (S16)\n", i+position, (int16_t)value);
					}
					else if (type == TRACETYPE_S8) {
						int value = (n>>16) & 0xFF;
						trprintf("[%04d] DEF value=%d (S8)\n", i+position, (int8_t)value);
					}
					else if (type == TRACETYPE_S32) {
						uint32_t n2 = pack->line[i+1];
						trprintf("[%04d] DEF value=%d (S32)\n", i+position, (int32_t)n2);
						i++;
					}
					else {
						uint32_t n2 = pack->line[i+1];
						uint32_t n3 = pack->line[i+2];
						trprintf("[%04d] DEF value_lo=%u value_hi=%u\n", i+position, n2, n3);
						i += 2;
					}

					break;
				}

				case TRACECODE_MOVE:
				{
					trline_t loadSrc = (n >> 11) & 1;
					trline_t loadDst = (n >> 12) & 1;
					trprintf("[%04d] MOVE size=%d", i+position, n >> 16);

					if (loadSrc) {
						trprintf(" loadSrc");
					}

					if (loadDst) {
						trprintf(" loadDst");
					}

					if (n & (1<<10)) {
						trprintf("\n"); // registrable
					} else {
						trprintf(" complex\n");
					}

					break;
				}

				case TRACECODE_PLACE:
				{
					int size       = (n >> 10) & 0x3;
					int receiver   = (n >> 12) & 0x1;
					int reg_value  = (n >> 16) & 0xFF;

					trprintf("[%04d] PLACE size=%d ", i+position, size);
					trprintf("reg=%d edit:%s\n", reg_value,
							receiver ? "VARIABLE" : "REGISTER");

					break;
				}

				case TRACECODE_ARITHMETIC: {
					int op   = (n >> 10) & 0x7;
					int type = (n >> 13) & 0x3;

					trprintf("[%04d] ARITH op=%d (", i+position, op);
					switch (op) {
						case TRACEOP_ADDITION:      trprintf("ADDITION"); break;
						case TRACEOP_SUBSTRACTION:  trprintf("SUBSTRACTION"); break;
						case TRACEOP_MULTIPLICATION:trprintf("MULTIPLICATION"); break;
						case TRACEOP_DIVISION:      trprintf("DIVISION"); break;
						case TRACEOP_MODULO:        trprintf("MODULO"); break;
						case TRACEOP_INC:           trprintf("INC"); break;
						case TRACEOP_DEC:           trprintf("DEC"); break;
						default:                    trprintf("UNKNOWN"); break;
					}
					trprintf(") ps=%d\n", type);
				} break;

				case TRACECODE_ARITHMETIC_IMM: {
					int op        = (n >> 10) & 0x7;
					int type      = (n >> 13) & 0x3;
					int side      = (n >> 15) & 0x1;
					uint32_t star = (n >> 16);

					trprintf("[%04d] ARITH/IMM op=%d (", i+position, op);
					switch (op) {
						case TRACEOP_ADDITION:      trprintf("ADDITION"); break;
						case TRACEOP_SUBSTRACTION:  trprintf("SUBSTRACTION"); break;
						case TRACEOP_MULTIPLICATION:trprintf("MULTIPLICATION"); break;
						case TRACEOP_DIVISION:      trprintf("DIVISION"); break;
						case TRACEOP_MODULO:        trprintf("MODULO"); break;
						case TRACEOP_INC:           trprintf("INC"); break;
						case TRACEOP_DEC:           trprintf("DEC"); break;
						default:                    trprintf("UNKNOWN"); break;
					}
					trprintf(") type=%d ", type);

					if (op == TRACEOP_INC || op == TRACEOP_DEC) {
						trprintf("on %s operand\n", side ? "RIGHT" : "LEFT");
					} else {
						if (type == 0 || type == 1) {
							trprintf("VALUE=%u (%s) on %s operand\n", star,
								type == 0 ? "char" : "short", side ? "RIGHT" : "LEFT");
						} else if (type == 2) {
							i++;
							trprintf("VALUE=%d (int) on %s operand\n", (int)pack->line[i], side ? "RIGHT" : "LEFT");
						} else if (type == 3) {
							i++;
							uint32_t lo = pack->line[i];
							i++;
							uint32_t hi = pack->line[i];
							trprintf("VALUE=%u%u (long) on %s operand\n", hi, lo, side ? "RIGHT" : "LEFT");
						}
					}
				} break;

				case TRACECODE_LOGIC: {
					int op   = (n >> 10) & 0xF;
					int type = (n >> 14) & 0x3;

					trprintf("[%04d] LOGIC op=%d (", i+position, op);
					switch (op) {
						case TRACEOP_BITWISE_AND:   trprintf("BITWISE_AND"); break;
						case TRACEOP_BITWISE_OR:    trprintf("BITWISE_OR"); break;
						case TRACEOP_BITWISE_XOR:   trprintf("BITWISE_XOR"); break;
						case TRACEOP_LEFT_SHIFT:    trprintf("LEFT_SHIFT"); break;
						case TRACEOP_RIGHT_SHIFT:   trprintf("RIGHT_SHIFT"); break;
						case TRACEOP_LOGICAL_AND:   trprintf("LOGICAL_AND"); break;
						case TRACEOP_LOGICAL_OR:    trprintf("LOGICAL_OR"); break;
						case TRACEOP_EQUAL:         trprintf("EQUAL"); break;
						case TRACEOP_NOT_EQUAL:     trprintf("NOT_EQUAL"); break;
						case TRACEOP_LESS:          trprintf("LESS"); break;
						case TRACEOP_LESS_EQUAL:    trprintf("LESS_EQUAL"); break;
						case TRACEOP_GREATER:       trprintf("GREATER"); break;
						case TRACEOP_GREATER_EQUAL: trprintf("GREATER_EQUAL"); break;
						default:                    trprintf("UNKNOWN"); break;
					}
					trprintf(") type=%d\n", type);
				} break;

				case TRACECODE_LOGIC_IMM_LEFT:
				case TRACECODE_LOGIC_IMM_RIGHT:
				{
					int op   = (n >> 10) & 0xF;
					int type = (n >> 14) & 0x3;

					trprintf("[%04d] LOGIC/IMM op=%d (", i+position, op);
					switch (op) {
						case TRACEOP_BITWISE_AND:   trprintf("BITWISE_AND"); break;
						case TRACEOP_BITWISE_OR:    trprintf("BITWISE_OR"); break;
						case TRACEOP_BITWISE_XOR:   trprintf("BITWISE_XOR"); break;
						case TRACEOP_LEFT_SHIFT:    trprintf("LEFT_SHIFT"); break;
						case TRACEOP_RIGHT_SHIFT:   trprintf("RIGHT_SHIFT"); break;
						case TRACEOP_LOGICAL_AND:   trprintf("LOGICAL_AND"); break;
						case TRACEOP_LOGICAL_OR:    trprintf("LOGICAL_OR"); break;
						case TRACEOP_EQUAL:         trprintf("EQUAL"); break;
						case TRACEOP_NOT_EQUAL:     trprintf("NOT_EQUAL"); break;
						case TRACEOP_LESS:          trprintf("LESS"); break;
						case TRACEOP_LESS_EQUAL:    trprintf("LESS_EQUAL"); break;
						case TRACEOP_GREATER:       trprintf("GREATER"); break;
						case TRACEOP_GREATER_EQUAL: trprintf("GREATER_EQUAL"); break;
						default:                    trprintf("UNKNOWN"); break;
					}
					trprintf(") type=%d immediate:%s ", type, next == TRACECODE_LOGIC_IMM_LEFT ? "LEFT" : "RIGHT");

					if (type < 2) {
						trprintf("VALUE=%u (int)\n", (unsigned int)(n >> 16));
					} else if (type == 2) {
						i++;
						uint32_t value = pack->line[i];
						trprintf("VALUE=%d (int)\n", (int)value);
					} else {
						i++;
						uint32_t lo = pack->line[i];
						i++;
						uint32_t hi = pack->line[i];
						trprintf("VALUE_LO=%u VALUE_HI=%u\n", lo, hi);
					}
				} break;


				case TRACECODE_FNCALL:
				{
					trprintf("[%04d] FNCALL idx=%02d\n", i+position, n >> 10);
					break;
				}

				case TRACECODE_IF:
				{
					trprintf("[%04d] IF to=%04d\n", i+position, n >> 10);
					break;
				}

				case TRACECODE_JMP:
				{
					trprintf("[%04d] JMP to=%04d\n", i+position, n >> 10);
					break;
				}

				case TRACECODE_CAST:
				{

					int src_signed   = (n >> 10) & 1;
					int src_float    = (n >> 11) & 1;
					int dest_signed  = (n >> 12) & 1;
					int dest_float   = (n >> 13) & 1;
					int src_size     = (n >> 16) & 0x3;
					int dest_size    = (n >> 18) & 0x3;

					trprintf("[%04d] CAST src{", i);

						if (src_signed)
							trprintf("signed ");
						if (src_float)
							trprintf("float ");

						trprintf("ps: %d} dest{", src_size);

						if (dest_signed)
							trprintf("signed ");
						if (dest_float)
							trprintf("float ");

						trprintf("ps: %d}\n", dest_size);
						break;
				}

				case TRACECODE_STACK_PTR:
				{
					uint32_t variable = (n >> 16) & 0xfff;
					uint32_t offset = pack->line[i+1];
					
					if (offset == -1) {
						trprintf("[%04d] STACK_PTR of=v%d offset=no\n", i, variable);
					} else {
						trprintf("[%04d] STACK_PTR of=v%d offset=+%02d\n", i, variable, offset);
					}
					i++;
					break;
				}

				default:
					trprintf("[%04d] UNKNOWN INSTRUCTION code=%d\n", i+position, next);
					break;
			}



		} else {
			// Usage mode
			int read = n & (1<<10);
			int variable = n >> 11;
			if (next == TRACE_USAGE_LAST) {
				trprintf("[%04d] !f v%d %s", i+position, variable, read ? "" : "edit");
			} else {
				trprintf("[%04d] !u v%d %snext=+%d", i+position, variable, read ? "" : "edit ", next);
			}

			if (registrables[variable]) {
				trprintf("\n");
			} else {
				i++;
				n = pack->line[i];
				trprintf(" offset=%d\n", n);
			}

		}
	}	


	finish:
	free(registrables);
}


static Write* reachWrite(int idx, int deep, int usageCapacity, VariableTrace* vt) {
	int emptyIndex = -1;
	Write* writes = vt->writes;
	for (int i = 0; i < usageCapacity; i++) {
		int si = writes[i].scopeId;
		if (si == idx)
			return &writes[i];

		if (si == -1) {
			emptyIndex = i;
		}
	}

	// Object absent from the list
	if (emptyIndex >= 0) {
		writes[emptyIndex].scopeId = idx;
		writes[emptyIndex].deep = deep;
		writes[emptyIndex].firstRead = FIRSTREAD_WAITING;
		return &writes[emptyIndex];
	}


	int nextCapacity = usageCapacity*2 + 4;
	Write* nextWrites = malloc(nextCapacity * sizeof(Write));
	if (usageCapacity > 0) {
		Write* oldWrites = vt->writes;
		memcpy(nextWrites, oldWrites, usageCapacity * sizeof(Write));
		free(oldWrites);
	}

	for (int i = usageCapacity+1; i < nextCapacity; i++) {
		trprintf("nui%d\n", i);
		nextWrites[i].lui = LUI_FINAL;
		nextWrites[i].scopeId = -1;
	}

	if (usageCapacity == 0) {
		// Init nextwrite
		nextWrites[0].ptr = vt->lastUsePtr;
		nextWrites[0].lui = vt->lastUseInstruction;

		int vScopeId = vt->scopeId;
		
		if (idx != vScopeId) {
			nextWrites[0].scopeId = vScopeId;
			nextWrites[0].deep = deep;
			nextWrites[0].firstRead = FIRSTREAD_WAITING;

			usageCapacity = 1;
			nextWrites[usageCapacity].lui = LUI_FINAL;
		}
	} else {
		nextWrites[usageCapacity].lui = LUI_FINAL;
	}

	nextWrites[usageCapacity].scopeId = idx;
	nextWrites[usageCapacity].deep = deep;
	nextWrites[usageCapacity].firstRead = FIRSTREAD_WAITING;


	vt->usageCapacity = nextCapacity;
	vt->writes = nextWrites;
	return &nextWrites[usageCapacity];
}

static void addUsageAt(Trace* trace, uint variable, int traceInstruction, bool readMode, trline_t* ptr) {
	if (variable == TRACE_VARIABLE_NONE) {
		return;
	}

	VariableTrace* vt = Array_get(VariableTrace, trace->variables, variable);
	
	int usageCapacity = vt->usageCapacity;
	
	// Read mode
	if (readMode) {
		int tScopeId = trace->scopeId;
		int vScopeId = vt->scopeId;

		// Simple
		if (usageCapacity == 0) {
			if (tScopeId > vScopeId) {
				// switch to multiusage
				goto readMulti;
			}

			int lui = vt->lastUseInstruction;
			if (lui != LUI_FINAL) {
				int diff = traceInstruction - lui;
				if (diff > TRACE_USAGE_OUT_OF_BOUNDS) {
					diff = TRACE_USAGE_OUT_OF_BOUNDS;
				}
				*vt->lastUsePtr |= diff;
			}
	
			vt->lastUseInstruction = traceInstruction;
			vt->lastUsePtr = ptr;
			goto finish;
		}

		// Handle multiusages
		readMulti:
		trprintf("vis %d\n", variable);
		Write* ourWrite = reachWrite(tScopeId, trace->deep, usageCapacity, vt);
		usageCapacity = vt->usageCapacity;


		Write* const writes = vt->writes;
		
		// Mark usages
		trprintf("readsession\n");
		for (int i = 0; i < usageCapacity; i++) {
			Write* write = &writes[i];
			
			int lui = write->lui;
			trprintf("reading v%d %d %d %d\n", variable, write->scopeId, tScopeId, lui);
			if (lui == LUI_FINAL)
				continue;

			int diff = traceInstruction - lui;
			if (diff > TRACE_USAGE_OUT_OF_BOUNDS) {
				diff = TRACE_USAGE_OUT_OF_BOUNDS;
			}

			trprintf("lui[%d] %d diff=%d\n", traceInstruction, lui, diff);
			*(write->ptr) |= diff;
			write->lui = LUI_FINAL;
		}

		
		trprintf("pushfirstread v%d\n", variable);
		int* scopeIds = (int*)trace->scopeIdStack.data;
		for (int i = trace->scopeIdStack.top; i >= 0; i--) {
			int scopeId = scopeIds[i];
			trprintf("\tsid %d\n", scopeId);
			for (int i = 0; i < usageCapacity; i++) {
				Write* write = &writes[i];
				if (write->scopeId != scopeId)
					continue;

				int firstRead = write->firstRead;
				trprintf("\t\tat %d\n", firstRead);
				if (firstRead == FIRSTREAD_ELIMINATED) {
					ourWrite->firstRead = FIRSTREAD_ELIMINATED;
					goto finishReadParents;
				}

				if (firstRead <= FIRSTREAD_STOP) {
					trprintf("\t\tpush\n");
					write->firstRead = traceInstruction;
					if (vt->globalFirstRead == FIRSTREAD_WAITING) {
						vt->globalFirstRead = traceInstruction;
					}
					goto finishReadParents;
				}
			}
		}

		finishReadParents:


		// Add our usage
		ourWrite->lui = traceInstruction;
		ourWrite->ptr = ptr;
		
		goto finish;
	}
	


	// Write mode
	int tScopeId = trace->scopeId;
	int vScopeId = vt->scopeId;
	// Simple
	if (usageCapacity == 0) {
		trprintf("simple %d %d | %d\n", tScopeId, vScopeId, variable);
		if (tScopeId > vScopeId) {
			// switch to multiusage
			goto writeMulti;
		}

		vt->lastUseInstruction = traceInstruction;
		vt->lastUsePtr = ptr;

	} else {
		// Handle multiusages
		writeMulti:
		Write* write = reachWrite(tScopeId, trace->deep, usageCapacity, vt);
		usageCapacity = vt->usageCapacity;

		trprintf("set(v%d) %d %d %d\n", variable, traceInstruction, tScopeId, write->firstRead);
		trprintf("with %d %d\n", vt->writes[0].lui, vt->writes[0].scopeId);
		write->lui = traceInstruction;
		write->ptr = ptr;

		// Handle first read
		if (write->firstRead == FIRSTREAD_WAITING) {
			write->firstRead = FIRSTREAD_ELIMINATED;
		}

		// Mark final usage in upper scopes
		Write* writes = vt->writes;
		for (int i = 0; i < usageCapacity; i++) {
			write = &writes[i];
			int wsi = write->scopeId;
			if (wsi > tScopeId) {
				write->lui = LUI_FINAL;
			}
		}
	}


	finish:
	*ptr = TRACE_USAGE_LAST | (variable << 11) | (readMode ? (1<<10) : 0);
}

void Trace_addUsage(Trace* trace, uint variable, int offset, bool readMode) {
	trprintf("usage v%d off=%02d | %s\n", variable, offset, readMode ? "read" : "write");

	if (offset < 0) {
		trline_t* ptr = Trace_push(trace, 1);
		addUsageAt(trace, variable, trace->instruction, readMode, ptr);
	} else {
		trline_t* ptr = Trace_push(trace, 2);
		addUsageAt(trace, variable, trace->instruction-1, readMode, &ptr[0]);
		ptr[1] = (uint)offset;

	}
}







void Trace_ins_savePlacement(Trace* trace) {
	*Trace_push(trace, 1) = TRACECODE_STAR | (7<<10);
}

void Trace_ins_openPlacement(Trace* trace) {
	*Trace_push(trace, 1) = TRACECODE_STAR | (8<<10);
}

void Trace_ins_saveShadowPlacement(Trace* trace) {
	*Trace_push(trace, 1) = TRACECODE_STAR | (9<<10);
}

void Trace_ins_openShadowPlacement(Trace* trace) {
	*Trace_push(trace, 1) = TRACECODE_STAR | (10<<10);
}



int* Trace_prepareWhileUsages(Trace* trace) {
	int deep = trace->deep;
	int scopeId = trace->scopeId;
	VariableTrace* variables = trace->variables.data;
	int length = trace->variables.length;
	
	int* backupPtr = malloc((length+1) * sizeof(int));
	backupPtr[0] = length;
	int* backup = &backupPtr[1];

	for (int i = 0; i < length; i++) {
		VariableTrace* vt = &variables[i];
		backup[i] = vt->globalFirstRead;

		Write* write = reachWrite(scopeId, deep, vt->usageCapacity, vt);
		write->firstRead = FIRSTREAD_STOP;
	}

	return backupPtr;
}


void Trace_addWhileUsages(Trace* trace, int scopeId, int startIns, int endIns, int* backup) {
	VariableTrace* variables = trace->variables.data;
	
	// Add usages
	int numToPut = 0;
	int varlength = trace->variables.length;
	int ins = trace->instruction;

	trprintf("START REAL scopeid\n");
	for (int i = 0; i < varlength; i++) {
		VariableTrace* vt = &variables[i];
		trprintf("globalfirstread(v%d) %d|%d\n", i, vt->usageCapacity, vt->globalFirstRead);
		int usageCapacity = vt->usageCapacity;
		if (usageCapacity == 0)
			continue;

		int firstRead = vt->globalFirstRead;
		if (firstRead < 0)
			continue;

		// Update writes
		Array_for(Write, vt->writes, usageCapacity, w) {			
			trprintf("scopeid %d %d %d\n", w->scopeId, scopeId, w->lui);
			int wsi = w->scopeId;
			if (wsi < scopeId) {
				continue;
			}

			int lui = w->lui;
			if (lui == LUI_FINAL)
				continue;

			trline_t diff = (endIns - w->lui) + (firstRead - startIns);
			trprintf("editdiff %d %d %d %d\n", diff, w->lui, *w->ptr & 0x3ff, wsi);

			if (diff > TRACE_USAGE_OUT_OF_BOUNDS) {
				diff = TRACE_USAGE_OUT_OF_BOUNDS;
			}
			*(w->ptr) |= diff;
		}

		// Add final usage
		Write* w = reachWrite(scopeId, trace->deep, usageCapacity, vt);
		if (numToPut == 7) {
			trline_t* line = Trace_push(trace, 2);
			line[0] = TRACE_USAGE_LAST | (0<<10) | (i << 11);
			line[1] = TRACECODE_STAR | (11<<10);
			w->lui = ins;
			w->ptr = line;
			ins++;
			numToPut = 0;
		} else {
			if (numToPut == 0) {
				*Trace_push(trace, 1) = TRACECODE_STAR | (3<<10); // forbid moves
				ins++;
			}

			numToPut++;
			trline_t* line = Trace_push(trace, 1);
			*line = TRACE_USAGE_LAST | (0<<10) | (i << 11);
			w->lui = ins;
			w->ptr = line;
		}
		
		
		
	}


	// Final numToPut
	if (numToPut > 0) {
		*Trace_push(trace, 1) = TRACECODE_STAR | (11<<10);
		numToPut = 0;
	}


	// Consume backup usages
	int length = backup[0];
	int* ptr = &backup[1];
	int deep = trace->deep;
	for (int i = 0; i < length; i++) {
		int fr = *ptr;
		VariableTrace* vt = &variables[i];
		Write* write = reachWrite(scopeId, deep, vt->usageCapacity, vt);
		write->firstRead = fr;
		vt->globalFirstRead = fr;
	}

	free(backup);
}





static int Trace_tryPackSize(int size) {
	switch (size) {
		case 1: return 0;
		case 2: return 1;
		case 4: return 2;
		case 8: return 3;
		default: return -1;
	}
}

int Trace_packSize(int size) {
	switch (size) {
		case 1: return 0;
		case 2: return 1;
		case 4: return 2;
		case 8: return 3;
		default: raiseError("[Trace] Invalid size of operation"); return -1;
	}
}

int Trace_unpackSize(int psize) {
	switch (psize) {
		case 0: return 1;
		case 1: return 2;
		case 2: return 4;
		case 3: return 8;
		default: raiseError("[Trace] Invalid size of operation"); return -1;
	}
}

int Trace_packExprTypeToSize(int type) {
	switch (type) {
	case EXPRESSION_I8:
	case EXPRESSION_U8:
		return 0;

	case EXPRESSION_I16:
	case EXPRESSION_U16:
		return 1;

	case EXPRESSION_I32:
	case EXPRESSION_U32:
	case EXPRESSION_F32:
		return 2;

	case EXPRESSION_I64:
	case EXPRESSION_U64:
	case EXPRESSION_F64:
		return 3;

	}

	return -1;
}





typedef struct {
	bool normalBehavior;
	uint variable;
} HandleOriginResult;

static void handleOrigin(Trace* trace, Expression* origin,
	Expression* next, int nextType, uint destVar, int destOffset
) {
	switch (origin->type) {
	case EXPRESSION_FAST_ACCESS:
	{
		Function* accessor = origin->data.fastAccess.accessor;

		int stdBehavior = accessor->stdBehavior;
		if (stdBehavior < 0) {
			raiseError("[TODO] real stdbehavior");
			return;
		}

		switch (stdBehavior) {
		// pointer
		case 0:
		{
			Expression* base = origin->data.fastAccess.origin;
			uint ptrVariable = Trace_ins_create(trace, NULL, 8, 0, true);
			Trace_set(trace, base, ptrVariable, -1, 8, base->type); // base should be a pointer

			switch (nextType) {
			case EXPRESSION_PROPERTY:
			{
				Variable** varr = next->data.property.variableArr;
				int length = next->data.property.length;
				int offset = Prototype_getGlobalVariableOffset(NULL, varr, length);
				trline_t* arr = Trace_push(trace, 3);
				arr[0] = TRACECODE_ARITHMETIC_IMM |
					(0 << 10) | // addition
					(3 << 13) | // size = 8 bytes
					(1 << 15); // value is the right operand
				arr[1] = offset;
				arr[2] = 0;


				ExtendedPrototypeSize eps = Prototype_getSizes(varr[length-1]->proto);
				Trace_ins_loadSrc(
					trace, destVar, ptrVariable,
					destOffset, -1, eps.size, eps.primitiveSizeCode);
				
				break;
			}

			default:
				raiseError("[Intern] Unfound type in handleOrigin");
				return;
			}

			break;
		}

		default:
			raiseError("[Intern] Invalid stdbehavior code");
		}

		break;
	}


	default:
		raiseError("[Trace] Expression type cannot be an origin");
	}
}

void Trace_set(Trace* trace, Expression* expr, uint destVar, int destOffset, int signedSize, int exprType) {
	int size = signedSize >= 0 ? signedSize : -signedSize;

	retry:
	switch (exprType) {
	case EXPRESSION_PROPERTY:
	{
		Expression* origin = expr->data.property.origin;
		if (origin) {
			handleOrigin(trace, origin, expr, EXPRESSION_PROPERTY, destVar, destOffset	);
			break;
		}


		int length = expr->data.property.length;
		Variable** varr = expr->data.property.variableArr;
		char primitiveSizeCode = Prototype_getPrimitiveSizeCode(varr[length-1]->proto);
		int signedObjSize;
		if (primitiveSizeCode) {
			if (primitiveSizeCode == 5) {
				signedObjSize = CASTABLE_FLOAT;
			} else if (primitiveSizeCode == 9) {
				signedObjSize = CASTABLE_DOUBLE;
			} else {
				signedObjSize = primitiveSizeCode;
			}
		} else {
			signedObjSize = Prototype_getSizes(varr[length-1]->proto).size;

			if (signedObjSize != signedSize) {
				raiseError("[Type] Sizes do not match on copy");
				return;
			}
		}


		// Size matches
		if (signedObjSize == signedSize) {
			Trace_ins_move(
				trace,
				destVar,
				varr[0]->id,
				destOffset,
				Prototype_getVariableOffset(varr, length),
				size,
				primitiveSizeCode
			);
			return;
		}

		// Let's cast
		int objSize = signedObjSize >= 0 ? signedObjSize : -signedObjSize;
		uint tempVar = Trace_ins_create(trace, NULL, objSize, 0, primitiveSizeCode);
		Trace_ins_move(
			trace,
			tempVar,
			varr[0]->id,
			-1,
			destOffset,
			objSize,
			1
		);

		Trace_addUsage(trace, tempVar, TRACE_OFFSET_NONE, true);
		Trace_addUsage(trace, destVar, destOffset, false);

		/// TODO: handle floating numbers
		*Trace_push(trace, 1) = TRACECODE_CAST |
			(signedObjSize < 0 ? (1<<10) : 0) |
			(0<<11) |
			(signedSize < 0 ? (1<<12) : 0) |
			(0<<13) |
			Trace_packSize(objSize) << 16 |
			Trace_packSize(size) << 18;


		Trace_removeVariable(trace, tempVar);


		return;

	}

	case EXPRESSION_FNCALL:
	{
		int argsLength = expr->data.fncall.argsLength;
		Function* fn = expr->data.fncall.fn;
		Variable** arguments = fn->arguments;
		Expression** args = expr->data.fncall.args;
		uint* variables = malloc(argsLength * sizeof(int));

		for (int i = 0; i < argsLength; i++) {
			/// TODO: check sizes
			char primitiveSizeCode = Prototype_getPrimitiveSizeCode(arguments[i]->proto);
			int size = Prototype_getSizes(arguments[i]->proto).size;
			uint bufferVar = Trace_ins_create(trace, NULL, size, 0, primitiveSizeCode);
			
			Trace_set(
				trace,
				args[i],
				bufferVar,
				primitiveSizeCode ? TRACE_OFFSET_NONE : 0,
				Prototype_getSignedSize(arguments[i]->proto),
				args[i]->type
			);

			uint finalVar = Trace_ins_create(trace, NULL, size, 0, primitiveSizeCode);
			variables[i] = finalVar;
			
			if (i < ARGUMENT_REGISTERS_LENGTH) {
				Trace_ins_placeReg(
					trace,
					bufferVar,
					finalVar,
					ARGUMENT_REGISTERS[i],
					Trace_packSize(size)
				);
			} else {
				raiseError("[TODO] handle a lot of arguements");
				return;
			}

			Trace_removeVariable(trace, bufferVar);
		}


		// Mark rax will be used and mark variable
		if (destVar != TRACE_VARIABLE_NONE) {
			*Trace_push(trace, 1) = TRACECODE_STAR | (4 << 10);

			if (destVar > 0xffff) {
				trline_t* arr = Trace_push(trace, 2);
				arr[0] = TRACECODE_STAR | (12 << 10) | (destVar << 16);
				arr[1] = TRACECODE_STAR | (12 << 10) | (destVar & 0xffff0000);
			} else {
				*Trace_push(trace, 1) = TRACECODE_STAR | (12 << 10) | (destVar << 16);
			}
		}

		int fnIndex = Trace_reachFunction(trace, fn);

		// Add usages
		for (int i = 0; i < argsLength; i++) {
			char psc = Prototype_getPrimitiveSizeCode(arguments[i]->proto);
			if (psc == PSC_UNKNOWN) {
				raiseError("[Architecture] Registrable status is unknown");
			}

			Trace_addUsage(
				trace, variables[i],
				psc ? TRACE_OFFSET_NONE : 0,
				true
			);
		}

		// Function call
		*Trace_push(trace, 1) = TRACECODE_FNCALL | (fnIndex << 10);

		// Remove variables
		for (int i = argsLength-1; i >= 0; i--) {
			Trace_removeVariable(trace, variables[i]);
		}

		// Output
		if (destVar != TRACE_VARIABLE_NONE) {
			int signedOutputSize = Prototype_getSignedSize(fn->returnType);
			if (signedOutputSize == signedSize) {
				Trace_ins_placeVar(trace, destVar, TRACE_REG_RAX, Trace_packSize(size));
	
			} else {
				int outputSize = signedOutputSize >= 0 ? signedOutputSize : -signedOutputSize;
				uint temp = Trace_ins_create(
					trace, NULL, outputSize, 0,
					Prototype_getPrimitiveSizeCode(fn->returnType)
				);
	
				// Put output in temp variable
				/// TODO: check TRACE_VARIABLE_NONE
				Trace_ins_placeVar(trace, temp, TRACE_REG_RAX, Trace_packSize(size));
	
				Trace_addUsage(trace, temp, TRACE_OFFSET_NONE, true);
				Trace_addUsage(trace, destVar, destOffset, true);
				*Trace_push(trace, 1) = TRACECODE_CAST |
					(signedOutputSize < 0 ? (1<<10) : 0) |
					(0<<11) |
					(signedSize < 0 ? (1<<12) : 0) |
					(0<<13) |
					Trace_packSize(outputSize) << 16 |
					Trace_packSize(size) << 18;
	
	
				Trace_removeVariable(trace, temp);
			}
		}

		free(variables);

		/// TODO: handle return value
		return;
	}

	case EXPRESSION_GROUP:
	{
		Expression* target = expr->data.target;
		Trace_set(trace, target, destVar, destOffset, signedSize, target->type);
		break;
	}

	case EXPRESSION_LINK:
		expr = expr->data.linked;
		exprType = expr->type;
		goto retry;


	case EXPRESSION_ADDITION:
	case EXPRESSION_SUBSTRACTION:
	case EXPRESSION_MULTIPLICATION:
	case EXPRESSION_DIVISION:
	case EXPRESSION_MODULO:

	case EXPRESSION_BITWISE_AND:
	case EXPRESSION_BITWISE_OR:
	case EXPRESSION_BITWISE_XOR:
	case EXPRESSION_LEFT_SHIFT:
	case EXPRESSION_RIGHT_SHIFT:

	case EXPRESSION_LOGICAL_AND:
	case EXPRESSION_LOGICAL_OR:

	case EXPRESSION_EQUAL:
	case EXPRESSION_NOT_EQUAL:
	case EXPRESSION_LESS:
	case EXPRESSION_LESS_EQUAL:
	case EXPRESSION_GREATER:
	case EXPRESSION_GREATER_EQUAL:
	{
		Expression* leftExpr  = expr->data.operands.left;
		Expression* rightExpr = expr->data.operands.right;

		int leftType = leftExpr->type;
		int rightType = rightExpr->type;

		bool leftValueGiven;
		bool rightValueGiven;

		castable_t immValue;

		if (leftType >= EXPRESSION_U8 && leftType <= EXPRESSION_F64) {
			leftValueGiven = true;
			immValue = leftExpr->data.num;
		} else {
			leftValueGiven = false;
		}

		if (rightType >= EXPRESSION_U8 && rightType <= EXPRESSION_F64) {
			immValue = rightExpr->data.num;
			rightValueGiven = true;
		} else {
			rightValueGiven = false;
		}

		if (leftValueGiven && rightValueGiven) {
			raiseError("[Trace] left and right operands cannot be immediates");
			return;
		}

		if (!leftValueGiven && !rightValueGiven) {
			int signedLeftSize = Expression_reachSignedSize(leftType, leftExpr);
			int signedRightSize = Expression_reachSignedSize(rightType, rightExpr);

			int signedMaxSize = chooseFinalSign(chooseSign(signedLeftSize, signedRightSize));
			int maxSize = signedMaxSize >= 0 ? signedMaxSize : -signedMaxSize;
			
			uint leftVar = Trace_ins_create(trace, NULL, maxSize, 0, 1);
			Trace_set(trace, leftExpr, leftVar, TRACE_OFFSET_NONE, signedMaxSize, leftType);

			uint rightVar = Trace_ins_create(trace, NULL, maxSize, 0, 1);
			Trace_set(trace, rightExpr, rightVar, TRACE_OFFSET_NONE, signedMaxSize, rightType);

			int tempDestVar;
			
			// Add usages
			if (signedMaxSize == signedSize) {
				Trace_addUsage(trace, leftVar, TRACE_OFFSET_NONE, true);
				Trace_addUsage(trace, destVar, destOffset, false);
				Trace_addUsage(trace, rightVar, TRACE_OFFSET_NONE, true);
			} else {
				tempDestVar = Trace_ins_create(trace, NULL, maxSize, 0, 1);
				Trace_addUsage(trace, leftVar, TRACE_OFFSET_NONE, true);
				Trace_addUsage(trace, tempDestVar, destOffset, false);
				Trace_addUsage(trace, rightVar, TRACE_OFFSET_NONE, true);
			}
			
			// Perform operation
			int packedMaxSize = Trace_packSize(maxSize);
			if (exprType <= EXPRESSION_MODULO) {
				// Arithmetic operation
				*Trace_push(trace, 1) = TRACECODE_ARITHMETIC |
					((exprType - (EXPRESSION_ADDITION - TRACEOP_ADDITION)) << 10) |
					(packedMaxSize << 13);

			} else {
				// Logic operation
				*Trace_push(trace, 1) = TRACECODE_LOGIC |
					((exprType - (EXPRESSION_BITWISE_AND - TRACEOP_BITWISE_AND)) << 10) |
					(packedMaxSize << 14);
			}

			// Add cast
			if (signedMaxSize != signedSize) {
				Trace_addUsage(trace, tempDestVar, TRACE_OFFSET_NONE, true);
				Trace_addUsage(trace, destVar, destOffset, false);
				

				/// TODO: handle floating numbers
				*Trace_push(trace, 1) = TRACECODE_CAST |
					(signedMaxSize< 0 ? (1<<10) : 0) |
					(0<<11) |
					(signedSize < 0 ? (1<<12) : 0) |
					(0<<13) |
					(packedMaxSize << 16) |
					(Trace_packSize(size) << 18);


				Trace_removeVariable(trace, tempDestVar);
			}

			Trace_removeVariable(trace, rightVar);
			Trace_removeVariable(trace, leftVar);

		} else {
			Expression* operand;
			int operandType;
			int signedSrcImmediateSize;
			if (rightValueGiven) {
				operand = leftExpr;
				operandType = leftType;
				signedSrcImmediateSize = Expression_getSignedSize(rightType);
			} else {
				operand = rightExpr;
				operandType = rightType;
				signedSrcImmediateSize = Expression_getSignedSize(leftType);
			}

			int signedOperandSize = chooseFinalSign(Expression_reachSignedSize(operandType, operand));
			int operandSize = signedOperandSize >= 0 ? signedOperandSize : -signedOperandSize;
			int signedDestImmediateSize;

			uint variable;
			uint resultVariable;
			bool notCast = operandSize <= size;

			/// TODO: handle case for same size but different sign
			if (notCast) {
				// Load operand
				variable = Trace_ins_create(trace, NULL, size, 0, 1);
				signedDestImmediateSize = signedSize;
				Trace_set(trace, operand, variable, TRACE_OFFSET_NONE, signedSize, operandType);

				// Add usages
				Trace_addUsage(trace, variable, TRACE_OFFSET_NONE, true);
				Trace_addUsage(trace, destVar, destOffset, false);

			} else {
				// Load operand
				variable = Trace_ins_create(trace, NULL, size, 0, 1);
				signedDestImmediateSize = signedOperandSize;
				Trace_set(trace, operand, variable, TRACE_OFFSET_NONE, signedOperandSize, operandType);

				resultVariable = Trace_ins_create(trace, NULL, size, 0, 1);

				// Add usages
				Trace_addUsage(trace, variable, TRACE_OFFSET_NONE, true);
				Trace_addUsage(trace, resultVariable, TRACE_OFFSET_NONE, false);
			}

			// Perform operation
			trline_t* ptr;
			trline_t first;

			int decPos;

			// Put some first data
			if (exprType <= EXPRESSION_MODULO) {
				decPos = 13;
				first = TRACECODE_ARITHMETIC_IMM |
					((exprType - (EXPRESSION_ADDITION - TRACEOP_ADDITION)) << 10) |
					(rightValueGiven<<15);
				
			} else {
				decPos = 14;
				first = (leftValueGiven ? TRACECODE_LOGIC_IMM_LEFT : TRACECODE_LOGIC_IMM_RIGHT) |
					((exprType - (EXPRESSION_BITWISE_AND - TRACEOP_BITWISE_AND)) << 10);
			}

			// Add value and type
			/// TODO: add sign to operation
			immValue = castable_cast(signedSrcImmediateSize, signedDestImmediateSize, immValue);
			switch (signedDestImmediateSize) {
			case 1:
			case -1:
				ptr = Trace_push(trace, 1);
				ptr[0] = first | (0 << decPos) | (immValue.u8 << 16);
				break;

			case 2:
			case -2:
				ptr = Trace_push(trace, 1);
				ptr[0] = first | (1 << decPos) | (immValue.u16 << 16);
				break;

			case 4:
			case -4:
				ptr = Trace_push(trace, 2);
				ptr[0] = first | (2 << decPos);
				ptr[1] = immValue.u32;
				break;
				
			case 8:
			case -8:
				ptr = Trace_push(trace, 3);
				ptr[0] = first | (3 << decPos);
				ptr[1] = immValue.u64;
				ptr[2] = immValue.u64 >> 32;
				break;

			default:
				raiseError("[TODO]: handle floats");
				break;
			}

			
			Trace_removeVariable(trace, variable);

			// Perform final cast
			if (!notCast) {
				Trace_addUsage(trace, resultVariable, TRACE_OFFSET_NONE, true);
				Trace_addUsage(trace, destVar, destOffset, false);

				*Trace_push(trace, 1) = TRACECODE_CAST |
					(signedOperandSize < 0 ? (1<<10) : 0) |
					(0<<11) |
					(signedSize < 0 ? (1<<12) : 0) |
					(0<<13) |
					Trace_packSize(operandSize) << 16 |
					Trace_packSize(size) << 18;
				
				Trace_removeVariable(trace, resultVariable);
			}


		}
		
		
		/// TODO: return value
		return;
	}


	case EXPRESSION_I8:
	case EXPRESSION_U8:
	case EXPRESSION_I16:
	case EXPRESSION_U16:
	case EXPRESSION_I32:
	case EXPRESSION_U32:
	case EXPRESSION_F32:
	case EXPRESSION_I64:
	case EXPRESSION_U64:
	case EXPRESSION_F64:
	case EXPRESSION_INTEGER:
	case EXPRESSION_FLOATING:
	{
		int signedObjSize = Expression_getSignedSize(exprType);
		int objSize = signedObjSize >= 0 ? signedObjSize : -signedObjSize;

		// Size matches
		Trace_ins_def(
			trace, destVar, destOffset, signedSize,
			castable_cast(signedObjSize, signedSize, expr->data.num)
		);

		return;		
	}

	case EXPRESSION_ADDR_OF:
	{
		Expression* reference = Expression_cross(expr->data.operand);
		if (reference->type != EXPRESSION_PROPERTY) {
			raiseError("[Syntax] Can only get the address of a variable");
			return;
		}
		
		if (reference->data.property.origin) {
			raiseError("[TODO] Handle origin in addrOf (this)");
		}
		
		int refArrLength = reference->data.property.length;
		Variable** refVarArr = reference->data.property.variableArr;
		int srcOffset = Prototype_getVariableOffset(refVarArr, refArrLength);
		int srcVar = refVarArr[0]->id;
		
		printf("ds %d %d\n", destVar, srcVar);
		Trace_ins_getStackPtr(trace, destVar, srcVar, destOffset, srcOffset);
		return;
	}	




	default:
		raiseError("[Trace] Expression type not handled");
		return;
	}
}


uint Trace_ins_create(Trace* trace, Variable* variable, int size, int flags, char registrable) {
	if (registrable == PSC_UNKNOWN) {
		raiseError("[Architecture] Registrable status is unknown");
		return -1;
	}

	if (size >= 0xffff) {
		raiseError("[TODO] handle create big size variables");
		return -1;
	}


	uint id = Trace_pushVariable(trace);
	trline_t* ptr = Trace_push(trace, 2);
	ptr[0] = TRACECODE_CREATE | (flags << 10) | (id << 16) | (registrable ? (1<<28) : 0);
	ptr[1] = TRACE_USAGE_LAST | (size << 16);
	
	VariableTrace* vt = Array_get(VariableTrace, trace->variables, id);
	if (vt->usageCapacity) {
		free(vt->writes);
	} else {
		vt->usageCapacity = 0;
	}

	vt->lastUsePtr = &ptr[1];
	vt->lastUseInstruction = trace->instruction - 1;
	vt->scopeId = trace->scopeId;
	vt->deep = trace->deep;
	vt->globalFirstRead = FIRSTREAD_WAITING;
	vt->creationLinePtr = ptr;
	return id;
}


void Trace_ins_def(Trace* trace, int variable, int offset, int signedSize, castable_t value) {
	Trace_addUsage(trace, variable, offset, false);

	trline_t* ptr;
	switch (signedSize) {
	case -1:
	case 1:
	 	ptr = Trace_push(trace, 1);
		ptr[0] = TRACECODE_DEF | (TRACETYPE_S8 << 10) | (value.u8 << 16);
		break;

	case 2:
	case -2:
	 	ptr = Trace_push(trace, 1);
		ptr[0] = TRACECODE_DEF | (TRACETYPE_S16 << 10) | (value.u16 << 16);
		break;
		
	case -4:
	case 4:
	case 5:
		ptr = Trace_push(trace, 2);
		ptr[0] = TRACECODE_DEF | (TRACETYPE_S32 << 10);
		ptr[1] = value.u32;
		break;

	case -8:
	case 8:
	case 9:
		ptr = Trace_push(trace, 3);
		ptr[0] = TRACECODE_DEF | (TRACETYPE_S64 << 10);
		ptr[1] = (unsigned int)(value.u64);
		ptr[2] = (unsigned int)(value.u64 >> 32);
		break;
	}
}


void Trace_ins_move(Trace* trace, int destVar, int srcVar,
	int destOffset, int srcOffset, int size, char isRegistrable) {

	Trace_addUsage(trace, srcVar, srcOffset, true);
	Trace_addUsage(trace, destVar, destOffset, false);

	*Trace_push(trace, 1) = TRACECODE_MOVE | (size << 16) |
		(isRegistrable ? 1<<10 : 0);
}

void Trace_ins_loadSrc(Trace* trace, int destVar, int srcVar,
	int destOffset, int srcOffset, int size, char isRegistrable) {

	Trace_addUsage(trace, srcVar, srcOffset, true);
	Trace_addUsage(trace, destVar, destOffset, false);

	*Trace_push(trace, 1) = TRACECODE_MOVE | (size << 16) |
		(isRegistrable ? 1<<10 : 0) | (1<<11);
}

void Trace_ins_loadDst(Trace* trace, int destVar, int srcVar,
	int destOffset, int srcOffset, int size, char isRegistrable) {

	Trace_addUsage(trace, srcVar, srcOffset, true);
	Trace_addUsage(trace, destVar, destOffset, true);

	*Trace_push(trace, 1) = TRACECODE_MOVE | (size << 16) |
		(isRegistrable ? 1<<10 : 0) | (1<<12);
}


trline_t* Trace_ins_if(Trace* trace, uint destVar) {
	Trace_addUsage(trace, destVar, TRACE_OFFSET_NONE, true);
	trline_t* l = Trace_push(trace, 1);
	*l = TRACECODE_IF;
	return l;
}

void Trace_ins_jmp(Trace* trace, uint instruction) {
	*Trace_push(trace, 1) = TRACECODE_JMP | (instruction << 10);
}

void Trace_ins_placeReg(Trace* trace, int srcVariable, int dstVariable, int reg, int packedSize) {
	*Trace_push(trace, 1) = TRACECODE_STAR | (3<<10); // forbid read

	/// TODO: check if variable is registrable
	Trace_addUsage(trace, srcVariable, TRACE_OFFSET_NONE, true);
	Trace_addUsage(trace, dstVariable, TRACE_OFFSET_NONE, false);
	
	*Trace_push(trace, 1) = TRACECODE_PLACE |
		(packedSize << 10) |
		(0 << 12) |
		(reg << 16);
}

void Trace_ins_placeVar(Trace* trace, int dstVariable, int reg, int packedSize) {
	*Trace_push(trace, 1) = TRACECODE_STAR | (3<<10); // forbid read

	/// TODO: check if variable is registrable
	Trace_addUsage(trace, dstVariable, TRACE_OFFSET_NONE, false);
	
	*Trace_push(trace, 1) = TRACECODE_PLACE |
		(packedSize << 10) |
		(1 << 12) |
		(reg << 16);
}


void Trace_ins_getStackPtr(Trace* trace, int destVar, int srcVar, int destOffset, int srcOffset) {
	Trace_addUsage(trace, destVar, destOffset, false);

	trline_t* arr = Trace_push(trace, 2);
	arr[0] = TRACECODE_STACK_PTR | (srcVar<<16);
	arr[1] = srcOffset;

	trline_t* ptr = Array_get(VariableTrace, trace->variables, srcVar)->creationLinePtr;
	*ptr = (*ptr) | (1<<12); // append stack-only attribute
}


























enum {
	STACK_BLOCKS = 64
};










static int Trace_reg_searchEmpty(const TraceRegister regs[]) {
	for (int i = TRACE_REG_RAX; i <= TRACE_REG_R11; i++) {
		int nu = regs[i].nextUse;
		if (nu == -1) {
			return i;
		}
	}

	return -TRACE_REG_NONE;
}

static int Trace_reg_chooseFast(const TraceRegister regs[], int biggerNextUse) {
	trprintf("choose: ");
	for (int i = TRACE_REG_RAX; i <= TRACE_REG_R11; i++) {
		trprintf("%d ", regs[i].nextUse);
	}
	trprintf("\n");

	int index = TRACE_REG_NONE;
	for (int i = TRACE_REG_RAX; i <= TRACE_REG_R11; i++) {
		int nu = regs[i].nextUse;
		if (nu == -1) {
			return -i;
		}

		if (nu > biggerNextUse) {
			biggerNextUse = nu;
			index = i;
		}
	}

	return index;
}



int Trace_reg_place(
	Trace* trace,
	TraceRegister regs[],
	int ip,
	uint variable,
	int source,
	int nextUse,
	int reg,
	char reading
) {
	if (trace->varInfos[variable].store == -reg)
		return reg;
		
	TraceReplace* replace;
	
	// regs[reg].replace = replace;
	
	
	if (reg >= 0) {
		trprintf("replace(%d:v%d) v%d in %d / %d\n", -reg, regs[reg].variable, variable, trace->varInfos[variable].store, nextUse);
		// regs[reg].replace = replace;


		uint victim = regs[reg].variable;
		
		if (victim == TRACE_VARIABLE_NONE) {
			replace = Array_push(TraceReplace, &trace->replaces);
			replace->victim = -TRACE_REG_NONE;
		} else {
			replace = Array_push(TraceReplace, &trace->replaces);
			int stackId = trace->stackId;
			replace->victimStore = stackId;
			trace->varInfos[victim].store = stackId;
			trace->stackId++;
			replace->victim = victim;

			/// TODO: develop this comment

			/*int victimNextUse = regs[reg].nextUse;
			regs[reg].nextUse = -3; // temporary: to forbid reg in Trace_reg_chooseFast

			int victimRegister = Trace_reg_chooseFast(regs, nextUse);
			if (victimRegister == TRACE_REG_NONE) {
				replace = Array_push(TraceReplace, &trace->replaces);
				
				int stackId = trace->stackId;
				replace->victimStore = stackId;
				trace->varInfos[victim].store = stackId;
				trace->stackId++;
				replace->victim = victim;
			} else {
				trprintf("nextvictim %d, %d %d\n", victim, reg, victimNextUse);
				victimRegister = Trace_reg_place(trace, regs, ip, victim, -reg, victimNextUse, victimRegister);
				replace = Array_push(TraceReplace, &trace->replaces);
				replace->victim = victim;
				replace->victimStore = -victimRegister;

			}*/
		}

		
	} else {
		trprintf("replace(%d) v%d in %d\n", reg, variable, trace->varInfos[variable].store);
		
		// No victim
		replace = Array_push(TraceReplace, &trace->replaces);
		replace->victim = -TRACE_REG_NONE;
		reg = -reg;
		
	}


	replace->instruction = ip;
	replace->variable = variable;
	replace->destination = -reg;
	replace->reading = reading;
	replace->becauseOfStackOnly = false;
	// replace->source = source;
	
	trace->varInfos[variable].nextUse = nextUse;
	trace->varInfos[variable].store = -reg;
	
	regs[reg].nextUse = nextUse;
	regs[reg].variable = variable;
	// regs[reg].replace = replace;

	return reg;
}


typedef struct {
	int nextUse;
	int pos;
} IndexedTraceRegister;

static int IndexedTraceRegister_compare(const void* pa, const void* pb) {
	int a = ((const IndexedTraceRegister*)pa)->nextUse;
	int b = ((const IndexedTraceRegister*)pb)->nextUse;

	if (a < 0 && b < 0)
		return 0;

	if (a < 0)
		return 1;

	if (b < 0)
		return -1;

	if (a > b)
		return 1;

	if (a < b)
		return -1;

	return 0;
}




static uint searchStackId(VarInfoTrace varInfos[], int stackId, uint length) {
	for (uint i = 0; i < length; i++)
		if (varInfos[i].store == stackId)
			return i;
	

	return TRACE_VARIABLE_NONE;
}



typedef struct {
	int store;
	int nextUse;
} ShadowPlacement;

static void saveShadowPlacements(
	VarInfoTrace varInfos[],
	Stack* shadowPlacementSaves,
	TraceStackHandler* stackHandler,
	uint length
) {
	ShadowPlacement* placements;
	void* placementsPtr;

	if (stackHandler) {
		placementsPtr = malloc(sizeof(int) + length * sizeof(ShadowPlacement));
		placements = placementsPtr + sizeof(int);
		*(int*)placementsPtr = stackHandler->rsp;
	} else {
		placementsPtr = malloc(length * sizeof(ShadowPlacement));
		placements = placementsPtr;
	}


	trprintf("shadowsave %p\n", placements);

	for (uint i = 0; i < length; i++) {
		int nextUse = varInfos[i].nextUse;
		placements[i].nextUse = nextUse;
		trprintf("shadowsv[v%02d] %d %2d\n", i, nextUse, varInfos[i].store);
		if (nextUse >= 0) {
			int store = varInfos[i].store;
			placements[i].store = store;

			if (store >= 0 && stackHandler) {
				TraceStackHandler_freeze(stackHandler, store);
			}
		}
	}

	*Stack_push(ShadowPlacement*, shadowPlacementSaves) = placementsPtr;
}

static void openShadowPlacements(
	VarInfoTrace varInfos[],
	TraceRegister regs[],
	Stack* shadowPlacementSaves,
	TraceStackHandler* stackHandler,
	FILE* output,
	uint length
) {
	void* placementsPtr = *Stack_pop(ShadowPlacement*, shadowPlacementSaves);
	trprintf("shadowopen %p\n", placementsPtr);

	ShadowPlacement* placements;
	if (stackHandler) {
		int nextRsp = *(int*)placementsPtr;
		stackHandler->rsp = nextRsp;
		placements = placementsPtr + sizeof(int);
	} else {
		placements = placementsPtr;
	}


	for (uint i = 0; i < length; i++) {
		int nextUse = placements[i].nextUse;
		varInfos[i].nextUse = nextUse;

		trprintf("shadowpl %d %d\n", placements[i].nextUse, placements[i].store);
		if (nextUse >= 0) {
			int store = placements[i].store;
			varInfos[i].store = store;
			trprintf("give %d %d\n", i, store);

			if (store >= 0) {
				if (stackHandler) {
					TraceStackHandler_unfreeze(stackHandler, store);
				}
			} else if (store != -TRACE_REG_NONE) {
				store = -store;
				regs[store].nextUse = nextUse;
				regs[store].variable = i;
			}
		} else {
			varInfos[i].store = -TRACE_REG_NONE;
		}
	}

	
	free(placementsPtr);
}







static void placeRegisters_savePlacements(
	VarInfoTrace varInfos[],
	Stack* placementSaves,
	uint length
) {
	int* placements = malloc(length * sizeof(int));

	for (uint i = 0; i < length; i++) {
		placements[i] = varInfos[i].nextUse < 0 ?
			-TRACE_REG_NONE : varInfos[i].store;
	}

	*Stack_push(int*, placementSaves) = placements;
}



static void placeRegisters_openPlacements(
	Trace* trace,
	VarInfoTrace varInfos[],
	TraceRegister regs[],
	Stack* placementSaves,
	uint length
) {
	int* stores = *Stack_pop(int*, placementSaves);

	for (uint v = 0; v < length; v++) {
		int dest = stores[v];
		if (dest == -TRACE_REG_NONE)
			continue;
		
		int source = varInfos[v].store;
		if (dest == source)
			continue;

		if (source >= 0) {
			if (dest >= 0) {
				uint victim = searchStackId(
					varInfos, dest,
					trace->variables.length
				);

				if (victim == TRACE_VARIABLE_NONE) {

				} else if (victim != v) {
					// exchange placements
					varInfos[victim].store = source;
				} else {
					continue;
				}

				varInfos[v].store = dest;
				
			} else {
				int dt = -dest;
				int destNextUse = regs[dt].nextUse;
				if (destNextUse >= 0) {
					uint victim = regs[dt].variable;
					varInfos[victim].store = source;
				}

				varInfos[v].store = dest;
				regs[dt].variable = v;
				regs[dt].nextUse = varInfos[v].nextUse;
			}
		
		} else if (dest >= 0) {
			uint victim = searchStackId(
				varInfos, dest,
				trace->variables.length
			);

			int st = -source;
			if (victim == TRACE_VARIABLE_NONE) {
				// exchange placements
				varInfos[v].store = dest;
				regs[st].variable = TRACE_VARIABLE_NONE;
				regs[st].nextUse = -1;

			} else if (victim != v) {
				varInfos[v].store = dest;
				varInfos[victim].store = source;
				regs[st].variable = v;
				regs[st].nextUse = varInfos[victim].nextUse;
			}


		} else {
			int st = -source;
			int dt = -dest;

			trprintf("%d\n", dt);
			int destNextUse = regs[dt].nextUse;
			if (destNextUse >= 0) {
				uint victim = regs[dt].variable;
				varInfos[victim].store = source;
				regs[st].variable = v;
				regs[st].nextUse = destNextUse;
			}

			varInfos[v].store = dest;
			regs[dt].variable = v;
			regs[dt].nextUse = varInfos[v].nextUse;
		}
	}

	free(stores);
}






void Trace_placeRegisters(Trace* trace) {
	// Generate next data
	int varlength = trace->variables.length;
	VarInfoTrace* varInfos = malloc(sizeof(VarInfoTrace) * varlength);
	TraceRegister* regs = malloc((TRACE_REG_NONE+1) * sizeof(TraceRegister));
	Stack placementSaves; // type: int*
	Stack shadowPlacementSaves; // type: ShadowPlacement*
	int slowRegisters = 0;
	int placedRegisters = 0;
	int argumentPosition = 0;
	
	Array_for(VarInfoTrace, varInfos, varlength, v) {
		v->size = -1;
		v->nextUse = -1;
		v->store = -TRACE_REG_NONE;
	}

	Array_free(trace->variables);
	Stack_free(trace->varPlacements);
	Stack_free(trace->scopeIdStack);

	Array_create(&trace->replaces, sizeof(TraceReplace));
	Array_create(&trace->fncallPlacements, sizeof(fnplacement_t));
	Stack_create(int*, &placementSaves);
	Stack_create(ShadowPlacement*, &shadowPlacementSaves);
	trace->fncallPlacements.data = NULL;

	trace->varInfos = varInfos;
	trace->regs = regs;
	trace->stackId = 0;
	trace->varlength = varlength;


	for (int i = 1; i < TRACE_REG_NONE; i++) {
		regs[i].nextUse = -1;
		regs[i].variable = TRACE_VARIABLE_NONE;
		// regs[i].replace = NULL;
	}


	trline_t line = 0;
	int ip = 0;
	TracePack* pack = trace->first;
	trline_t* linePtr = pack->line;

	enum {USAGE_BUFFER_LEN = 4};
	trline_t usageBuffer[USAGE_BUFFER_LEN];
	int usageBufferLine = 0;
	bool movesAllowed = true;

	while (true) {
		trprintf("> at %d\n", ip);
		line = *linePtr;
		linePtr++;
		trline_t code = line & 0x3ff;

		// Usage
		if (code <= TRACE_USAGE_LIMIT) {
			usageBuffer[usageBufferLine] = line;
			usageBufferLine++;

			trline_t readmode = (line>>10) & 1;
			trline_t varIdx = line >> 11;
			VarInfoTrace* vi = &varInfos[varIdx];
			int store = vi->store;


			if (code == 0) {
				// Last usage => destroy variable
				vi->nextUse = -1;
				trprintf("kill v%d\n", varIdx);

				if (store == -TRACE_REG_NONE)
					goto continueWhile;
				
				// Register
				if (store < 0) {
					trprintf("destroy %d (v%d == v%d) \n", store, varIdx, regs[-store].variable);
					regs[-store].nextUse = -1;
					regs[-store].variable = -1;
					vi->store = -TRACE_REG_NONE;
					goto continueWhile;
				}

				goto continueWhile;
			}
			
			int nextUse = code == TRACE_USAGE_OUT_OF_BOUNDS ?
				0x7ffffffe : ip + code;

			vi->nextUse = nextUse;
			
			
			// Put variable in a register
			if (movesAllowed && varInfos[varIdx].registrable) {
				if (varInfos[varIdx].stackOnly) {
					if (store == -TRACE_REG_NONE) {
						int dest = trace->stackId;
						trace->stackId++;
						varInfos[varIdx].store = dest;

						TraceReplace* replace = Array_push(TraceReplace, &trace->replaces);
						replace->instruction = ip;
						replace->variable = varIdx;
						replace->victim = -TRACE_REG_NONE;
						replace->destination = dest;
						replace->victimStore = -TRACE_REG_NONE;
						replace->reading = readmode;
						replace->becauseOfStackOnly = true;
					}

				} else if (store >= 0 || store == -TRACE_REG_NONE) {
					trprintf("placedRegisters %d | %d for v%d\n", placedRegisters, store, varIdx);
					int reg = Trace_reg_chooseFast(regs, -1);
					vi->store = -Trace_reg_place(trace, regs, ip, varIdx, store, nextUse, reg, readmode);
				} else {
					regs[-store].nextUse = nextUse;
				}


			}

			goto continueWhile;
		}

		
		usageBufferLine = 0;
		movesAllowed = true;

		// Simple instruction
		switch (code) {
		case TRACECODE_STAR:
		{
			int action = (line >> 10) & 0xf;
			switch (action) {
				case 0:
				// change pack
				pack = pack->next;
				if (!pack)
					goto finishWhile;

				linePtr = pack->line;
				break;

			case 1: // quick skip
				break;
			
			case 2: // return
				break;

			case 3: // forbid moves
				movesAllowed = false;
				break;

			case 4: // protect RAX for fncall
				break;

			case 5: // protect RAX and RDX for fncall
				break;

			case 6: // mark label
				break;

			case 7: // save placements
				placeRegisters_savePlacements(varInfos, &placementSaves, varlength);
				break;

			case 8: // open placements
				placeRegisters_openPlacements(trace, varInfos, regs,
					&placementSaves, varlength);
				break;
				
			case 9: // save shadow placements
				saveShadowPlacements(varInfos,
					&shadowPlacementSaves, NULL, varlength);
				break;

			case 10: // open shadow placements
				openShadowPlacements(varInfos, regs, 
					&shadowPlacementSaves, NULL, NULL, varlength);
				break;

			case 11: // trival usages
				break;

			case 12: // fncall return dst variable (for transpiler)
				break;
				
			}
			break;
		}

		case TRACECODE_CREATE:
		{
			char registrable = (line >> 28) & 1;
			char argument = (line >> 11) & 1;
			bool stackOnly = (!registrable) || ((line >> 12) & 1);
			trline_t size = line >> 16;
			trline_t variable = (line >> 16) & 0xFFF;

			line = *linePtr;
			linePtr++;
			ip++;

			trline_t next = line & 0x3FF;
			int nextUse = next == 0 ? -1 : next;

			trprintf("create %d\n", variable);
			varInfos[variable].size = size;
			varInfos[variable].nextUse = nextUse;
			varInfos[variable].registrable = registrable;
			varInfos[variable].stackOnly = stackOnly;

			if (registrable) {
				if (argument && nextUse > 0) {
					int reg = ARGUMENT_REGISTERS[argumentPosition];
					varInfos[variable].store = -reg;
					regs[reg].nextUse = ip + nextUse;
					regs[reg].variable = variable;
					argumentPosition++;
				} else {
					varInfos[variable].store = -TRACE_REG_NONE;
				}
			} else {
				if (argument) {
					raiseError("[TODO] handle args on stack");
				} else {
					varInfos[variable].store = trace->stackId;
					trace->stackId++;
				}
			}


			break;	
		};


		case TRACECODE_DEF:
		{
			if (((line >> 10) & 0x3) >= 2) {
				linePtr++;
				ip++;
			}
			break;
		}

		case TRACECODE_MOVE:
		{
			break;
		}

		case TRACECODE_PLACE:
		{
			if (line & (1<<12)) {
				// Edit variable
				uint destVar = usageBuffer[0] >> 11;
				int nextUse = usageBuffer[0] & 0x3ff;
				nextUse = (nextUse == TRACE_USAGE_OUT_OF_BOUNDS ?
					0x7ffffffe : ip + nextUse - 1);

				int reg = line >> 16;
				varInfos[destVar].nextUse = nextUse;
				
			} else {
				int reg = line >> 16;
				uint srcVar = usageBuffer[0] >> 11;
				uint destVar = usageBuffer[1] >> 11;
				int source = varInfos[srcVar].store;
				trprintf(
					"plce %d|%d src %d(%d) ; dst %d(%d)\n",
					regs[reg].variable, reg,
					srcVar, source,
					destVar, varInfos[destVar].store);

				varInfos[srcVar].nextUse = -1;
				varInfos[srcVar].store = -TRACE_REG_NONE;

				if (source >= 0) {
					/// TODO: handle this case ?
					// raiseError("[TODO]: free stack");

				} else if (source != -TRACE_REG_NONE) {
					regs[-source].nextUse = -1;
					regs[-source].variable = TRACE_VARIABLE_NONE;
				}

				Trace_reg_place(trace, regs, ip, destVar, source, -2, reg, 1);
				placedRegisters |= 1<<reg;
			}

			break;
		}

		case TRACECODE_ARITHMETIC:
		{
			break;
		}

		case TRACECODE_ARITHMETIC_IMM:
		{
			int psize = (line >> 13) & 0x3;
			if (psize >= 2) {
				linePtr++;
				ip++;
			}
			break;	
		}

		case TRACECODE_LOGIC:
		{
			break;	
		};

		case TRACECODE_LOGIC_IMM_LEFT:
		case TRACECODE_LOGIC_IMM_RIGHT:
		{
			int psize = (line >> 14) & 0x3;
			if (psize >= 2) {
				linePtr++;
				ip++;
			}
			break;
		};



		case TRACECODE_FNCALL:
		{
			trprintf("situat: ");
			for (int i = TRACE_REG_RAX; i <= TRACE_REG_R11; i++) {
				trprintf("%d ", regs[i].nextUse);
			}
			trprintf("\n");

			fnplacement_t* placement = Array_push(fnplacement_t, &trace->fncallPlacements);
	
			// generate IndexedTraceRegister list
			IndexedTraceRegister sorted[TRACE_REG_R11];
			for (int i = 0; i < TRACE_REG_R11; i++) {
				int j = i+TRACE_REG_RAX;
				sorted[i].pos = j;
				sorted[i].nextUse = regs[j].nextUse;
			}
			
			qsort(sorted, TRACE_REG_R11, sizeof(IndexedTraceRegister), IndexedTraceRegister_compare);
			int keptRegisters = 0;

			// Use rbx, etc.
			int regPos = TRACE_REG_RBX;
			for (int i = 0; i < TRACE_REG_R15 - TRACE_REG_RBX + 1; i++) {			
				int p = sorted[i].pos;
				if (sorted[p-1].nextUse == -1)
					continue;

				if (sorted[p-1].nextUse == -2) {
					trprintf("REFUSE %d\n", p);
					keptRegisters |= 1 << (p-1);
					(*placement)[p-1] = -TRACE_REG_NONE;
					continue;
				}
				
				int flag = 1<<i;
				if (sorted[p-1].nextUse <= ip + TRACE_CALLESAVED_LIMIT || (slowRegisters & flag)) {
					keptRegisters |= 1 << (p-1);
					slowRegisters |= flag;
					
					regs[regPos].variable = regs[p].variable;
					regs[regPos].nextUse = regs[p].nextUse;
					// regs[j].replace = regs[p].replace;

					regs[p].variable = TRACE_VARIABLE_NONE;
					regs[p].nextUse = -1;

					trprintf("give %d\n", -regPos);
					(*placement)[p-1] = -regPos;
					regPos++;
				}
			}

			// Save lost registers
			for (int i = 0; i < TRACE_REG_R11 - TRACE_REG_RAX+1; i++) {
				if ((keptRegisters & (1<<i)))
					continue;
				
				int j = i+TRACE_REG_RAX;
				if (regs[j].variable == TRACE_VARIABLE_NONE) {
					((int*)placement)[i] = -TRACE_REG_NONE;
				} else {
					((int*)placement)[i] = trace->stackId;
					trace->stackId++;
					regs[j].variable = TRACE_VARIABLE_NONE;
					regs[j].nextUse = -1;
				}
			}

			break;
		}

		case TRACECODE_IF:
		{

			break;
		}

		case TRACECODE_JMP:
		{
			break;
		}

		case TRACECODE_CAST:
		{
			break;
		}

		case TRACECODE_STACK_PTR:
		{
			trline_t variable = (line >> 16) & 0xfff;
			trline_t offset = *linePtr;

			linePtr++;
			ip++;

			break;
		}

		}
	
		continueWhile:
		ip++;
	}


	finishWhile:
	// if (fncallPositionsOrigin)
		// free(fncallPositionsOrigin);
	

	Stack_free(placementSaves);
	Stack_free(shadowPlacementSaves);
}





static const char REGISTER_NAMES[][TRACE_REG_NONE+1][5] = {
	{"Zl", "al", "cl", "r11b", "bl", "r15b", "Zl"},
	{"Zx", "ax", "cx", "r11w", "bx", "r15w", "Zx"},
	{"eZx", "eax", "ecx", "r11d", "ebx", "r15d", "eZx"},
	{"rZx", "rax", "rcx", "r11", "rbx", "r15", "rZx"},
};



/*
static const char REGISTER_NAMES[][TRACE_REG_NONE+1][5] = {
	{"Zl", "al", "cl", "dl", "r8b", "r9b", "r10b", "r11b", "bl", "r15b", "Zl"},
	{"Zx", "ax", "cx", "dx", "r8w", "r9w", "r10w", "r11w", "bx", "r15w", "Zx"},
	{"eZx", "eax", "ecx", "edx", "r8d", "r9d", "r10d", "r11d", "ebx", "r15d", "eZx"},
	{"rZx", "rax", "rcx", "rdx", "r8", "r9", "r10", "r11", "rbx", "r15", "rZx"},
};
*/

static const char* REGISTER_SIZENAMES[] = {
	"byte",
	"word",
	"dword",
	"qword"
};

static const char* ARITHMETIC_NAMES[] = {
	"add",
	"sub",
	"mul",
	"div",
	"mod",
	"inc",
	"dec",
	"neg"
};


static const char* LOGIC_NAMES[] = {
	"and",
	"or",
	"xor",
	"shl",
	"shr",
	NULL, // logical and
	NULL, // logical or
	"sete",
	"setne",
	"setl",
	"setle",
	"setg",
	"setge",
};


static const char* ARITHMETIC_SYMBOLS[] = {
	"+",
	"-",
	"*",
	"/",
	"%",
	"++",
	"--",
	"-"
};

static const char* LOGIC_SYMBOLS[] = {
	"&",
	"|",
	"^",
	"<<",
	">>",
	"&&",
	"||",
	"==",
	"!=",
	"<",
	"<=",
	">",
	">=",
};





typedef struct {
	int store;
	int offset;
} Use;

void Trace_printUsage(Trace* trace, FILE* output, int psize, Use use) {
	if (use.store >= 0) {
		int size = Trace_unpackSize(psize);
		int pos = use.store;
		pos += use.offset;
		trprintf("rsp_to %d-%d-%d = %d\n", trace->stackHandler.rsp, pos, size, trace->stackHandler.rsp - pos - size);
		fprintf(output, "%s[rsp+%d]", REGISTER_SIZENAMES[psize], trace->stackHandler.rsp - pos - size);
	} else {
		if (use.store == -TRACE_REG_NONE) {use.store = 0;}
		int s = -use.store;
		fprintf(output, "%s", REGISTER_NAMES[psize][s]);
	}	
}





void restoreAfterFnCall(
	int p, int i,
	uint variable,
	VarInfoTrace varInfos[],
	TraceRegister regs[],
	FILE* output,
	Trace* trace
) {
	if (p >= 0) {
		int j = i + TRACE_REG_RAX;
		int size = varInfos[variable].size;
		int psize = Trace_packSize(size);

		regs[j].variable = variable;
		regs[j].nextUse = varInfos[variable].nextUse;

		varInfos[variable].store = -(i+TRACE_REG_RAX);

		fprintf(output, "\tmov %s, ", REGISTER_NAMES[psize][j]);
		Trace_printUsage(trace, output, psize, (Use){p, 0});
		fprintf(output, "; from fncall \n");

		int move = TraceStackHandler_remove(&trace->stackHandler, p, size);
		TraceStackHandler_adaptAllocation(&trace->stackHandler, move, output);

	} else if (p != -TRACE_REG_NONE) {
		int j = i + TRACE_REG_RAX;
		trprintf("vis %d %d\n", i, variable);
		int size = varInfos[variable].size;
		int psize = Trace_packSize(size);
		
		varInfos[variable].store = -j;

		regs[j].variable = variable;
		regs[j].nextUse = varInfos[variable].nextUse;

		p = -p;
		regs[p].variable = TRACE_VARIABLE_NONE;
		regs[p].nextUse = -1;

		fprintf(
			output,
			"\tmov %s, %s; from fncall\n",
			REGISTER_NAMES[psize][j],
			REGISTER_NAMES[psize][p]
		);
	}
}






void generateAssembly_savePlacements(
	VarInfoTrace varInfos[],
	Stack* placementSaves,
	TraceStackHandler* stackHandler,
	uint length
) {
	int* placementsPtr = malloc((length+1) * sizeof(int));
	placementsPtr[0] = stackHandler->rsp;
	trprintf("saversp %d %p\n", stackHandler->rsp, placementsPtr);
	int* placements = &placementsPtr[1];

	for (uint i = 0; i < length; i++) {
		int store = varInfos[i].store;
		placements[i] = store;
		if (store >= 0) {
			TraceStackHandler_freeze(stackHandler, store);
		}
	}

	*Stack_push(int*, placementSaves) = placementsPtr;
}




typedef struct {
	int reg;
	int size;
	int previousNextUse;
	int stack;
} TempReg;

#define subToRsp(pos, size) (stackHandler->rsp - (pos) - (size))



static TempReg pushTempReg(
	int rplceReg,
	TraceRegister regs[],
	VarInfoTrace varInfos[],
	TraceStackHandler* stackHandler,
	FILE* output
) {
	// Get register
	TempReg tmp;
	tmp.reg = Trace_reg_searchEmpty(regs);
	if (tmp.reg == -TRACE_REG_NONE) {
		uint variable = regs[rplceReg].variable;
		tmp.reg = rplceReg;
		tmp.previousNextUse = -1;
		tmp.size = variable == TRACE_VARIABLE_NONE ? 8 : varInfos[variable].size;
		tmp.stack = TraceStackHandler_handlePush(stackHandler, tmp.size, output);
		trprintf("handle push %d %d\n", tmp.size, tmp.stack);
		int psize = Trace_packSize(tmp.size);
		fprintf(
			output,
			"\tmov %s[rsp+%d], %s; push\n",
			REGISTER_SIZENAMES[psize],
			subToRsp(tmp.stack, tmp.size),
			REGISTER_NAMES[psize][tmp.reg]
		);
		
	} else {
		regs[tmp.reg].nextUse = -2;
		tmp.previousNextUse = regs[tmp.reg].nextUse;
		tmp.size = varInfos[regs[tmp.reg].variable].size;
	}

	return tmp;
}

static void popTempReg(TempReg tmp, TraceRegister regs[], TraceStackHandler* stackHandler, FILE* output) {
	int previousNextUse = tmp.previousNextUse;
	if (tmp.previousNextUse == -1) {
		int psize = Trace_packSize(tmp.size);
		fprintf(
			output,
			"\tmov %s, %s[rsp+%d]; pop\n",
			REGISTER_NAMES[psize][tmp.reg],
			REGISTER_SIZENAMES[psize],
			subToRsp(tmp.stack, tmp.size)
		);

		TraceStackHandler_handlePop(stackHandler, tmp.size, tmp.stack, output);

	} else if (tmp.reg != -TRACE_REG_NONE) {
		regs[tmp.reg].nextUse = previousNextUse;
	}
}

#undef subToRsp
#define toRsp(pos, size) (trace->stackHandler.rsp - (pos) - (size))


void generateAssembly_openPlacements(
	Trace* trace,
	VarInfoTrace varInfos[],
	TraceRegister regs[],
	Stack* placementSaves,
	TraceStackHandler* stackHandler,
	FILE* output,
	uint length
) {
	int* storesPtr = *Stack_pop(int*, placementSaves);

	int* stores = &storesPtr[1];

	for (uint v = 0; v < length; v++) {
		trprintf("before %d %2d\n", v, varInfos[v].store);
	}

	for (uint v = 0; v < length; v++) {
		int dest = stores[v];
		trprintf("destof(v%d): %2d from %2d [rsp=%d]\n", v, dest, varInfos[v].store, stackHandler->rsp);
		if (dest == -TRACE_REG_NONE)
			continue;

		if (dest >= 0) {
			TraceStackHandler_unfreeze(stackHandler, dest);
		}

		int source = varInfos[v].store;
		if (dest == source)
			continue;


		int size = varInfos[v].size;
		// int psize = Trace_tryPackSize(size);
		int psize = Trace_packSize(size);

		if (source >= 0) {
			int sourceAddress = TraceStackHandler_reach(&trace->stackHandler, source);
			trprintf("guarenteegot %d of %d\n", sourceAddress, source);
			trprintf("sourceAddress %d[%d]\n", source, sourceAddress);

			if (dest >= 0) {
				varInfos[v].store = dest;

				uint victim = searchStackId(
					varInfos, dest,
					trace->variables.length
				);

				if (victim == TRACE_VARIABLE_NONE) {
					TempReg tmp = pushTempReg(TRACE_REG_RAX, regs, varInfos, &trace->stackHandler, output);
					TraceStackHandler_remove(&trace->stackHandler, source, size);

					TraceStackHandlerRspAdapter adp = TraceStackHandler_guarantee(&trace->stackHandler, size, size, dest);
					int destAddress = adp.address;

					fprintf(
						output,
						"\tmov %s, %s[rsp+%d]; restore@0\n",
						REGISTER_NAMES[psize][tmp.reg],
						REGISTER_SIZENAMES[psize],
						toRsp(sourceAddress, size)
					);

					trprintf("moversp %d %d %d\n", adp.rspMove, trace->stackHandler.rsp, destAddress);
					TraceStackHandler_adaptAllocation(&trace->stackHandler, adp.rspMove, output);

					fprintf(
						output,
						"\tmov %s[rsp+%d], %s\n",
						REGISTER_SIZENAMES[psize],
						toRsp(destAddress, size),
						REGISTER_NAMES[psize][tmp.reg]
					);

					popTempReg(tmp, regs, &trace->stackHandler, output);
				} else {
					TraceStackHandler_remove(&trace->stackHandler, source, size);
					raiseError("[TODO] victim");
				}

				continue;
			}

			// Here, dest < 0 is a register
			int dt = -dest;
			int destNextUse = regs[dt].nextUse;
			trprintf("destNextUse(v%d) %d %d\n", v, destNextUse, dt);
			
			
			if (destNextUse >= 0) {
				uint victim = regs[dt].variable;
				varInfos[victim].store = source;
				
				TempReg tmp = pushTempReg(TRACE_REG_RAX, regs, varInfos, &trace->stackHandler, output);
				TraceStackHandler_remove(&trace->stackHandler, source, size);
				TraceStackHandlerRspAdapter adp = TraceStackHandler_guarantee(&trace->stackHandler, size, size, source);
				int destAddress = adp.address;

				fprintf(
					output,
					"\tmov %s, %s; restore@1\n\tmov %s, %s[rsp+%d]\n",

					REGISTER_NAMES[psize][tmp.reg],
					REGISTER_NAMES[psize][dt],

					REGISTER_NAMES[psize][dt],
					REGISTER_SIZENAMES[psize],
					toRsp(sourceAddress, size)
				);

				TraceStackHandler_adaptAllocation(&trace->stackHandler, adp.rspMove, output);

				fprintf(
					output,
					"\tmov %s[rsp+%d], %s\n",
					
					REGISTER_SIZENAMES[psize],
					toRsp(destAddress, size),
					REGISTER_NAMES[psize][tmp.reg]

				);


				popTempReg(tmp, regs, &trace->stackHandler, output);

			} else {
				TraceStackHandler_remove(&trace->stackHandler, source, size);

				fprintf(
					output,
					"\tmov %s, %s[rsp+%d]; restore@2\n",
					REGISTER_NAMES[psize][dt],
					REGISTER_SIZENAMES[psize],
					toRsp(sourceAddress, size)
				);
			}

			varInfos[v].store = dest;
			regs[dt].variable = v;
			regs[dt].nextUse = varInfos[v].nextUse;


			continue;
		}

		// Here, source < 0 is a register
		if (dest >= 0) {
			uint victim = searchStackId(
				varInfos, dest,
				trace->variables.length
			);

			int st = -source;
			int destAddress = TraceStackHandler_guarantee(
				&trace->stackHandler, size, size, dest).address;

			if (victim == TRACE_VARIABLE_NONE) {
				varInfos[v].store = dest;
				regs[st].variable = TRACE_VARIABLE_NONE;
				regs[st].nextUse = -1;


				fprintf(
					output,
					"\tmov %s[rsp+%d], %s; restore@3\n",
					REGISTER_SIZENAMES[psize],
					toRsp(destAddress, size),
					REGISTER_NAMES[psize][st]
				);

			} else if (victim != v) {
				varInfos[v].store = dest;
				varInfos[victim].store = source;
				regs[st].variable = v;
				regs[st].nextUse = varInfos[victim].nextUse;

				TempReg tmp = pushTempReg(TRACE_REG_RAX, regs, varInfos, &trace->stackHandler, output);

				fprintf(
					output,
					"\tmov %s %s[rsp+%d]; restore@4\n\tmov %s[rsp+%d] %s\n\tmov %s %s\n",

					REGISTER_NAMES[psize][tmp.reg],
					REGISTER_SIZENAMES[psize],
					toRsp(destAddress, size),

					REGISTER_SIZENAMES[psize],
					toRsp(destAddress, size),
					REGISTER_NAMES[psize][st],

					REGISTER_NAMES[psize][st],
					REGISTER_NAMES[psize][tmp.reg]
				);

				popTempReg(tmp, regs, &trace->stackHandler, output);

			} else {
				continue;
			}

			continue;
		}

		// Here, source < 0 and dest < 0 are in a register
		int st = -source;
		int dt = -dest;
		
		int destNextUse = regs[dt].nextUse;
		if (destNextUse >= 0) {
			uint victim = regs[dt].variable;
			varInfos[victim].store = source;
			regs[st].variable = v;
			regs[st].nextUse = destNextUse;
			regs[dt].nextUse = -2; // temporary forbid

			// Swap st and dt
			int temp = Trace_reg_searchEmpty(regs);
			if (temp == -TRACE_REG_NONE) {
				fprintf(
					output,
					"\txor %s, %s; restore@5\n\txor %s, %s\n\txor %s %s\n",

					REGISTER_NAMES[psize][st],
					REGISTER_NAMES[psize][dt],

					REGISTER_NAMES[psize][dt],
					REGISTER_NAMES[psize][st],

					REGISTER_NAMES[psize][st],
					REGISTER_NAMES[psize][dt]
				);
			} else {

				fprintf(
					output,
					"\tmov %s, %s; restore@6\n\tmov %s, %s\n\tmov %s %s\n",

					REGISTER_NAMES[psize][temp],
					REGISTER_NAMES[psize][st],

					REGISTER_NAMES[psize][st],
					REGISTER_NAMES[psize][dt],

					REGISTER_NAMES[psize][dt],
					REGISTER_NAMES[psize][temp]
				);
			}
		
		} else {
			trprintf("restoremov %d %d\n", st, dt);
			fprintf(
				output,
				"\tmov %s, %s; restore@7\n",
				REGISTER_NAMES[psize][st],
				REGISTER_NAMES[psize][dt]
			);
		}

		varInfos[v].store = dest;
		regs[dt].variable = v;
		regs[dt].nextUse = varInfos[v].nextUse;
	}


	int nextRsp = storesPtr[0];
	trprintf("openrsp=%d [%p]\n", nextRsp, storesPtr);
	trprintf("cameto %d\n", stackHandler->rsp);
	fprintf(output, "\tadd rsp, %d; readapt\n", stackHandler->rsp - nextRsp);
	stackHandler->rsp = nextRsp;


	free(storesPtr);
}





void Trace_generateAssembly(Trace* trace, FunctionAssembly* fnAsm) {
	#define printNumber()\
	switch (psize) {\
		case 0:\
		{fprintf(output, ", %d", (line>>16) & 0xff); break;}\
		case 1:\
		{fprintf(output, ", %d", line>>16);break;}\
		case 2:\
		{\
			line = *linePtr; linePtr++; ip++;\
			fprintf(output, ", %d", line);\
			break;\
		}\
		case 3:\
		{\
			uint lo = *linePtr;\
			linePtr++;\
			uint hi = *linePtr;\
			linePtr++;\
			ip += 2;\
			uint64_t value = (uint64_t)lo | (((uint64_t)hi) << 32);\
			fprintf(output, ", %ld", value);\
		}\
	}

	FILE* output = fnAsm->output;
	int varlength = trace->varlength;

	// Init data
	VarInfoTrace* varInfos = trace->varInfos;
	TraceRegister* regs = trace->regs;
	
	for (int i = 0; i < varlength; i++) {
		varInfos[i].store = -TRACE_REG_NONE;
		varInfos[i].nextUse = -1;
	}

	for (int i = 0; i < TRACE_REG_NONE; i++) {
		regs[i].nextUse = -1;
		regs[i].variable = TRACE_VARIABLE_NONE;
	}

	TraceStackHandler_create(&trace->stackHandler, trace->stackId);

	if (fnAsm->fn->returnType) {
		fprintf(output, "fn_%016lx: ; %s(...) -> %s;\n",
			fnAsm->fn->traceId, fnAsm->fn->name, 
			fnAsm->fn->returnType->direct.cl->c_name);
	} else {
		fprintf(output, "fn_%016lx: ; %s(...);\n", fnAsm->fn->traceId, fnAsm->fn->name);
	}


	Stack placementSaves; // type: int*
	Stack shadowPlacementSaves; // type: ShadowPlacement*
	Stack_create(int*, &placementSaves);
	Stack_create(ShadowPlacement*, &shadowPlacementSaves);


	fnplacement_t* const placementsObj = trace->fncallPlacements.data;
	fnplacement_t* placement = placementsObj;
	TraceReplace* replace = trace->replaces.data;
	TraceReplace* finalReplace = &replace[trace->replaces.length];
	TracePack* pack = trace->first;
	trline_t* linePtr = pack->line;

	int labelCounter = 0;
	int slowRegisters = 0;
	int argumentPosition = 0;
	int ip = 0;
	int usageCompletion = 0;
	trline_t line = 0;
	trline_t previousLine;
	char protectMode = 0;

	enum {
		WAITING_ENABLED = (int)-0x80000000,
		WAITING_DISABLED = 0,
	};
	typedef struct {
		int store;
		int size;
		uint variable;
	} WaitingRegister;


	WaitingRegister waitRax = {-TRACE_REG_NONE, WAITING_DISABLED};
	WaitingRegister waitRdx = {-TRACE_REG_NONE, WAITING_DISABLED};
	
	typedef struct {
		int store;
		int offset;
		int rspMove;
	} Usemv;

	Usemv uses[USE_LENGTH];

	#define useAt(idx) ((Use){uses[idx].store, uses[idx].offset})

	while (true) {
		// Print state
		trprintf("Stack: ");
		for (int i = 0; i < trace->stackId; i++) {
			trprintf("%2d ", trace->stackHandler.positions[i].position);
		}
		trprintf("\n");

		trprintf("Regs: ");
		for (int i = 1; i < TRACE_REG_NONE; i++) {
			trprintf("%4d ", trace->regs[i].nextUse);
		}
		trprintf("\n");

		for (int i = 0; i < varlength; i++) {
			int store = varInfos[i].store;
			if (store >= 0) {
				trprintf("@v%02d : %d[%d]", i, store, TraceStackHandler_reach(&trace->stackHandler, store));
			} else if (store != -TRACE_REG_NONE) {
				store = -store;
				trprintf("@v%02d : %s", i, REGISTER_NAMES[3][store]);

				if (regs[store].variable == i) {
					
				} else {
					trprintf("*");
				}
			} else {
				trprintf("@v%02d : NONE", i);
			}

			if (varInfos[i].nextUse < 0)
				trprintf("\t(!f)");

			trprintf("\n");
		}

		trprintf("> to %d [rsp=%d]\n", ip, trace->stackHandler.rsp);

		

		previousLine = line;
		line = *linePtr;
		linePtr++;
		trline_t code = line & 0x3ff;

		// Apply replaces
		while (replace < finalReplace && replace->instruction == ip) {
			uint variable = replace->variable;

			if (replace->becauseOfStackOnly) {
				varInfos[variable].store = replace->destination;
				replace++;

				continue;
			}
			
			// Move victim
			#define adapt(p, move, sstore) if (sstore < 0) {p = sstore; move=0;} else {\
				TraceStackHandlerRspAdapter apt = TraceStackHandler_guarantee(\
					&trace->stackHandler, size, size, sstore);\
					p = apt.address;\
					if (apt.rspMove < 0) {\
						TraceStackHandler_adaptAllocation(&trace->stackHandler, apt.rspMove, output);\
						move = 0;\
					} else if (apt.rspMove > 0) {\
						move = apt.rspMove;\
					} else {move=0;}\
				}
			

			int victim = replace->victim;
			trprintf("victim %d for v%d in %d reading=%d\n", victim, variable, varInfos[variable].store, replace->reading);
			if (victim >= 0) {
				int p1, move1;
				int p2, move2;
				int size = varInfos[victim].size;
				int psize = Trace_packSize(size);
				int victimStore = replace->victimStore;
				
				adapt(p1, move1, victimStore);
				adapt(p2, move2, varInfos[victim].store);
				trprintf("dest %d > %d => %d\n", replace->destination, victimStore, p2);
				fprintf(output, "\tmov ");
				Trace_printUsage(
					trace, output, psize,
					(Use){p1, 0}
				);
				fprintf(output, ", ");

				Trace_printUsage(
					trace, output, psize,
					(Use){p2, 0}
				);
				fprintf(output, "; throw v%d\n", victim);

				TraceStackHandler_adaptAllocation(&trace->stackHandler, move1, output);
				TraceStackHandler_adaptAllocation(&trace->stackHandler, move2, output);

				varInfos[victim].store = victimStore;
			
			} else if (victim != -TRACE_REG_NONE) {
				victim = -victim;
				
				regs[victim].variable = TRACE_VARIABLE_NONE;
				regs[victim].nextUse = -1;
			}


			// Free stack area
			int destination = replace->destination;
			if (varInfos[variable].store >= 0 && replace->reading) {
				int size = varInfos[variable].size;
				int psize = Trace_packSize(size);
				int p1, move1;
				int p2, move2;

				adapt(p1, move1, destination);
				adapt(p2, move2, varInfos[variable].store);
				fprintf(output, "\tmov ");
				Trace_printUsage(
					trace, output, psize,
					(Use){p1, 0}
				);
				fprintf(output, ", ");
				Trace_printUsage(
					trace, output, psize,
					(Use){p2, 0}
				);
				fprintf(output, "; take v%d\n", variable);


				trprintf("store %d %d\n", variable, varInfos[variable].store);
				// Free taken value
				int move3 = TraceStackHandler_remove(
					&trace->stackHandler,
					varInfos[variable].store,
					varInfos[variable].size
				);
				
				TraceStackHandler_adaptAllocation(&trace->stackHandler, move1, output);
				TraceStackHandler_adaptAllocation(&trace->stackHandler, move2, output);
				TraceStackHandler_adaptAllocation(&trace->stackHandler, move3, output);

			}

			// Update store
			varInfos[variable].store = destination;
			if (destination < 0) {
				destination = -destination;
				regs[destination].nextUse = varInfos[variable].nextUse;
				regs[destination].variable = variable;
			}

			#undef adapt


			replace++;
		}


		ip++;



		// Usage
		if (code <= TRACE_USAGE_LIMIT) {
			trline_t readmode = line & (1<<10);
			trline_t varIdx = line >> 11;
			#define getNextUse() (code == TRACE_USAGE_OUT_OF_BOUNDS ?\
				0x7ffffffe : ip + code - 1)

			int store = varInfos[varIdx].store;
			trprintf("reada %d in %d next=%d\n", varIdx, store, getNextUse());
			uses[usageCompletion].rspMove = 0;
			
			if (varInfos[varIdx].registrable) {
				if (store >= 0) {
					int size = varInfos[varIdx].size;
					trprintf("shouldadd %d\n", varIdx);
					TraceStackHandlerRspAdapter rspAdapter = TraceStackHandler_guarantee(
						&trace->stackHandler,
						size,
						size,
						store
					);
					
					if (rspAdapter.rspMove != 0) {
						uses[usageCompletion].rspMove = rspAdapter.rspMove;
						trprintf("remove setting %d\n", rspAdapter.rspMove);
					}

					if (code == 0) {
						// Remove variable
						int move = TraceStackHandler_remove(
							&trace->stackHandler,
							store,
							varInfos[varIdx].size
						);
						uses[usageCompletion].rspMove = move;
						trprintf("remove moving %d\n", move);

						varInfos[varIdx].nextUse = -1;
					} else {
						varInfos[varIdx].nextUse = getNextUse();
					}

					store = rspAdapter.address;
				
				} else if (code == 0) {
					regs[-store].nextUse = -1;
					varInfos[varIdx].nextUse = -1;
				} else if (store != -TRACE_REG_NONE) {
					int nextUse = getNextUse();
					regs[-store].nextUse = nextUse;
					varInfos[varIdx].nextUse = nextUse;
				} else {
					raiseError("[Trace] A variable must be placed");
					return;
				}
				
				
				uses[usageCompletion].store = store;
				uses[usageCompletion].offset = 0;
				usageCompletion++;

			} else {
				line = *linePtr;
				linePtr++;
				ip++;

				if (store >= 0) {
					int size = varInfos[varIdx].size;
					store = TraceStackHandler_guarantee(
						&trace->stackHandler,
						size,
						size,
						store
					).address;
				}
				varInfos[varIdx].nextUse = getNextUse();
				uses[usageCompletion].store = store;
				uses[usageCompletion].offset = line;
				usageCompletion++;
			}

			if (store != -TRACE_REG_NONE && store <= -TRACE_REG_RBX) {
				int r = -store;
				int flag = 1 << r;
				if ((slowRegisters & flag) == 0) {
					trprintf("ID %d\n", r);
					fprintf(output, "\tpush %s\n", REGISTER_NAMES[3][r]);
					slowRegisters |= flag;
				}
			}
			

			#undef getNextUse
			
			continue;
		}


		// Reset usage completion
		usageCompletion = 0;

		#define allowRsp(id) {if (uses[id].rspMove < 0) {\
			TraceStackHandler_adaptAllocation(&trace->stackHandler, uses[id].rspMove, output);}}
		
		#define desallowRsp(id) {if (uses[id].rspMove > 0) {\
			TraceStackHandler_adaptAllocation(&trace->stackHandler, uses[id].rspMove, output);}}

		// Instruction
		switch (code) {
		case TRACECODE_STAR:
		{
			int action = (line >> 10) & 0xf;
			switch (action) {
			case 0:
				// change pack
				pack = pack->next;
				if (!pack)
					goto finishWhile;

				linePtr = pack->line;
				break;

			case 1: // quick skip
				break;

			case 2: // return
				fprintf(output, "\tret\n");
				break;

			case 3: // forbid moves
				break;
			
			case 4: // protect RAX for fncall
				waitRax.store = -TRACE_REG_NONE;
				waitRax.size = WAITING_ENABLED;
				break;

			case 5: // protect RAX and RDX for fncall
				waitRax.store = -TRACE_REG_NONE;
				waitRax.size = WAITING_ENABLED;
				waitRdx.store = -TRACE_REG_NONE;
				waitRdx.size = WAITING_ENABLED;
				break;

			case 6: // mark label
				fprintf(output, ".l%04d:\n", ip);
				break;

			case 7: // save placements
				generateAssembly_savePlacements(varInfos, &placementSaves,
					&trace->stackHandler, varlength);
				break;

			case 8: // open placements
				generateAssembly_openPlacements(trace, varInfos,
					regs, &placementSaves, &trace->stackHandler, output, varlength);
				break;
				
			case 9: // pass placements
				saveShadowPlacements(varInfos, &shadowPlacementSaves,
					&trace->stackHandler, varlength);
				break;

			case 10: // rewind placements
				openShadowPlacements(varInfos, regs, &shadowPlacementSaves,
					&trace->stackHandler, output, varlength);
				break;

			case 11: // trival usages
				break;

			case 12: // fncall return dst variable (for transpiler)
				break;

			}
			
			break;
		}

		case TRACECODE_CREATE:
		{
			int variable = (line >> 16) & 0xfff;
			int size = (*linePtr) >> 16;
			char registrable = (line >> 28) & 1;
			char argument = (line >> 11) & 1;
			bool stackOnly = (!registrable) || ((line >> 12) & 1);


			line = *linePtr;
			linePtr++;
			ip++;

			trline_t next = line & 0x3FF;
			int nextUse = next == 0 ? -1 : next;

			trprintf("create %d\n", variable);
			varInfos[variable].size = size;
			varInfos[variable].nextUse = nextUse;
			varInfos[variable].stackOnly = stackOnly;

			if (argument) {
				if (registrable) {
					int reg = ARGUMENT_REGISTERS[argumentPosition];
					varInfos[variable].store = -reg;
					regs[reg].nextUse = nextUse;
					regs[reg].variable = variable;
					argumentPosition++;
				} else {
					raiseError("[TODO] handle args on stack");
				}
			} else {
				varInfos[variable].store = -TRACE_REG_NONE;
				varInfos[variable].registrable = registrable;
			}
			break;	
		};


		case TRACECODE_DEF:
		{
			int type = (line >> 10) & 0x3;
			allowRsp(0);
			
			fprintf(output, "\tmov ");

			Trace_printUsage(trace, output, type, useAt(0));
			fprintf(output, ", ");
			
			// Print value
			if (type == TRACETYPE_S8) {
				int value = (line >> 16) & 0xFF;
				fprintf(output, "%u \n", value);
			}
			else if (type == TRACETYPE_S16) {
				int value = (line>>16) & 0xFFFF;
				fprintf(output, "%u \n", value);
			}
			else if (type == TRACETYPE_S32) {
				fprintf(output, "%u \n", *linePtr);
				linePtr++;
				ip++;
			}
			else {
				unsigned long l = *(linePtr++);
				unsigned long r = *(linePtr++);
				ip += 2;
				fprintf(output, "%lu \n", l | (r << 32));
			}

			break;
		}

		case TRACECODE_MOVE:
		{
			trline_t registrableOp = line & (1<<10);
			trline_t loadSrc = line & (1<<11);
			trline_t loadDst = line & (1<<12);

			if (loadSrc && loadDst) {
				raiseError("[Intern] Cannot load src and dst at once");
				return;
			}

			if (loadSrc) {
				if (registrableOp) {
					allowRsp(1);

					TempReg tmp;
					int size = (line >> 16);
					int srcStore = uses[0].store;
					int psize = Trace_packSize(size);
					bool useTmpReg = (srcStore >= 0);

					if (useTmpReg) {
						tmp = pushTempReg(TRACE_REG_RAX, regs, varInfos, &trace->stackHandler, output);
						fprintf(
							output,
							"\tmov %s, %s[rsp+%d]; restore@8\n",
							REGISTER_NAMES[psize][tmp.reg],
							REGISTER_SIZENAMES[psize],
							toRsp(TraceStackHandler_reach(&trace->stackHandler, srcStore), size)
						);

						srcStore = -tmp.reg;
					}

					fprintf(output, "\tmov ");
					Trace_printUsage(trace, output, psize, useAt(1));
					fprintf(output, ", %s[", REGISTER_SIZENAMES[psize]);
					Trace_printUsage(trace, output, 3, ((Use){srcStore, -1}));
					fprintf(output, "]\n");

					if (useTmpReg)
						popTempReg(tmp, regs, &trace->stackHandler, output);

					desallowRsp(0);
					break;
				}

				raiseError("[TODO] loadSrc not registrableOp");

			}

			if (loadDst) {
				raiseError("[TODO] loadDst");
				break;
			}

			if (registrableOp) {
				allowRsp(1);
	
				int size = (line >> 16);
				fprintf(output, "\tmov ");
				Trace_printUsage(trace, output, Trace_packSize(size), useAt(1));
				fprintf(output, ", ");
				Trace_printUsage(trace, output, Trace_packSize(size), useAt(0));
				fprintf(output, "\n");
			
				desallowRsp(0);
			} else {
				fprintf(output, "\t; [TODO] move stacks\n");
			}


			break;
		}

		case TRACECODE_PLACE:
		{			
			allowRsp(1);

			if (line & (1<<12)) {
				// edit variable
				uint destVar = previousLine >> 10;
				int nextUse = previousLine & 0x3ff;
				int psize = (line >> 10) & 0x3;

				nextUse = (nextUse == TRACE_USAGE_OUT_OF_BOUNDS ?
					0x7ffffffe : ip + nextUse - 1);

				int reg = line >> 16;
				varInfos[destVar].nextUse = nextUse;
				
				fprintf(output, "\tmov ");
				Trace_printUsage(trace, output, psize, useAt(0));
				fprintf(output, ", ");
				Trace_printUsage(trace, output, psize, (Use){-reg, 0});
				fprintf(output, "; place(var)\n");

				// Finally perform "from fncall operation"
				if (reg == TRACE_REG_RAX && waitRax.store != -TRACE_REG_NONE) {
					/// TODO: complete here
					restoreAfterFnCall(waitRax.store, 0, waitRax.variable, varInfos, regs, output, trace);
					waitRax.store = -TRACE_REG_NONE;
					waitRax.size = WAITING_DISABLED;
				}
				
				if (reg == TRACE_REG_RDX && waitRdx.store != -TRACE_REG_NONE) {
					raiseError("[TODO] enable restoreAfterFnCall");
					// restoreAfterFnCall(waitRax.store, 0, waitRax.variable, varInfos, regs, output, trace);
					waitRdx.store = -TRACE_REG_NONE;
					waitRax.size = WAITING_DISABLED;
				}

			} else {
				// edit register
				int psize = (line >> 10) & 0x3;
				int reg = line >> 16;

				fprintf(output, "\tmov ");
				Trace_printUsage(trace, output, psize, (Use){-reg, 0});
				fprintf(output, ", ");
				Trace_printUsage(trace, output, psize, useAt(0));
				fprintf(output, "; place(reg)\n");
			}

			desallowRsp(0);

			break;
		}

		case TRACECODE_ARITHMETIC:
		{
			allowRsp(1);

			int operation = (line >> 10) & 0x7;
			int psize = (line >> 13) & 0x3;

			if (operation == 2) {
				// multiplication
				raiseError("[TODO] hanlde special op");
				
			} else if (operation == 3) {
				raiseError("[TODO] hanlde special op");
				// division
				
			} else if (operation == 4) {
				raiseError("[TODO] hanlde special op");
				// modulo

			}

			// Move value
			fprintf(output, "\tmov ");
			Trace_printUsage(trace, output, psize, useAt(1));
			fprintf(output, ", ");
			Trace_printUsage(trace, output, psize, useAt(0));
			fprintf(output, "; prepare arithmetic operation\n");

			// Perform real operation
			fprintf(output, "\t%s ", ARITHMETIC_NAMES[operation]);
			Trace_printUsage(trace, output, psize, useAt(1));
			fprintf(output, ", ");
			Trace_printUsage(trace, output, psize, useAt(2));
			fprintf(output, "\n");

			desallowRsp(0);
			desallowRsp(2);

			break;
		}

		case TRACECODE_ARITHMETIC_IMM:
		{
			allowRsp(1);

			int operation = (line >> 10) & 0x7;
			int psize = (line >> 13) & 0x3;

			// Move value
			fprintf(output, "\tmov ");
			Trace_printUsage(trace, output, psize, useAt(1));
			fprintf(output, ", ");
			Trace_printUsage(trace, output, psize, useAt(0));
			fprintf(output, "; prepare arithmetic operation\n");

			// Perform real operation
			fprintf(output, "\t%s ", ARITHMETIC_NAMES[operation]);
			Trace_printUsage(trace, output, psize, useAt(1));
			
			if (operation >= TRACEOP_INC) {
				fprintf(output, "\n");
				break;
			}

			if (operation >= TRACEOP_MULTIPLICATION) {
				raiseError("[TODO] mult");
				break;
			}

			/// TODO: check sign
			printNumber();
			fprintf(output, "\n");

			desallowRsp(0);
			break;	
		}

		case TRACECODE_LOGIC:
		{
			allowRsp(1);
			int operation = (line >> 10) & 0xf;
			int psize = (line >> 14) & 0x3;

			switch (operation) {
			case TRACEOP_BITWISE_AND:
			case TRACEOP_BITWISE_OR:
			case TRACEOP_BITWISE_XOR:
			case TRACEOP_LEFT_SHIFT:
			case TRACEOP_RIGHT_SHIFT:
			{
				fprintf(output, "\tmov ");
				Trace_printUsage(trace, output, psize, useAt(1));
				fprintf(output, ", ");
				Trace_printUsage(trace, output, psize, useAt(0));
				fprintf(output, "; prepare logic operation\n");

				// Perform real operation
				fprintf(output, "\t%s ", LOGIC_NAMES[operation]);
				Trace_printUsage(trace, output, psize, useAt(1));
				fprintf(output, ", ");
				Trace_printUsage(trace, output, psize, useAt(2));
				fprintf(output, "\n");
				break;
			}

			case TRACEOP_LOGICAL_AND:
			{
				raiseError("[TODO] logical and");
				break;
			}
			
			case TRACEOP_LOGICAL_OR:
			{
				
				raiseError("[TODO] logical or");
				break;
			}


			case TRACEOP_EQUAL:
			{
				raiseError("[TODO] logical operation");
				break;
			}
			
			case TRACEOP_NOT_EQUAL:
			{
				raiseError("[TODO] logical operation");
				break;
			}

			case TRACEOP_LESS:
			{
				raiseError("[TODO] logical operation");
				break;
			}

			case TRACEOP_LESS_EQUAL:
			{
				raiseError("[TODO] logical operation");
				break;
			}

			case TRACEOP_GREATER:
			{
				raiseError("[TODO] logical operation");
				break;
			}

			case TRACEOP_GREATER_EQUAL:
			{
				raiseError("[TODO] logical operation");
				;
				break;
			}


			}
		
			desallowRsp(0);
			desallowRsp(2);
			break;	
		};

		case TRACECODE_LOGIC_IMM_LEFT:
		case TRACECODE_LOGIC_IMM_RIGHT:
		{
			allowRsp(1);

			int operation = (line >> 10) & 0xf;
			int psize = (line >> 14) & 0x3;
			trprintf("psize is %d\n", psize);

			// Move value
			fprintf(output, "\tmov ");
			Trace_printUsage(trace, output, psize, useAt(1));
			fprintf(output, ", ");
			Trace_printUsage(trace, output, psize, useAt(0));
			fprintf(output, "; prepare logic operation\n");
			
			// Perform real operation
			switch (operation) {
			case TRACEOP_BITWISE_AND:
			case TRACEOP_BITWISE_OR:
			case TRACEOP_BITWISE_XOR:
			case TRACEOP_LEFT_SHIFT:
			case TRACEOP_RIGHT_SHIFT:
				if (code == TRACECODE_LOGIC_IMM_LEFT) {
					fprintf(output, "\t%s ", LOGIC_NAMES[operation]);
					printNumber();
					Trace_printUsage(trace, output, psize, useAt(1));
					fprintf(output, "\n");
				} else {
					fprintf(output, "\t%s ", LOGIC_NAMES[operation]);
					Trace_printUsage(trace, output, psize, useAt(1));
					printNumber();
					fprintf(output, "\n");
				}
				break;

			case TRACEOP_LOGICAL_AND:
			case TRACEOP_LOGICAL_OR:

			case TRACEOP_EQUAL:
			case TRACEOP_NOT_EQUAL:
			case TRACEOP_LESS:
			case TRACEOP_LESS_EQUAL:
			case TRACEOP_GREATER:
			case TRACEOP_GREATER_EQUAL:
				break;

			}
			

			desallowRsp(0);

			break;	
		};



		case TRACECODE_FNCALL:
		{
			int newSlowRegisters = slowRegisters;
			// Collect slow registers to add
			for (int i = 0; i < TRACE_REG_R11 - TRACE_REG_RAX + 1; i++) {
				int p = (*placement)[i];
				if (p < 0 && p != TRACE_REG_NONE) {
					newSlowRegisters |= 1<<(-p);
				}
			}

			// Add slow registers
			newSlowRegisters ^= slowRegisters;
			for (int i = TRACE_REG_RBX; i <= TRACE_REG_R15; i++) {
				if (newSlowRegisters & (1<<i)) {
					fprintf(output, "\tpush %s\n", REGISTER_NAMES[3][i]);
				}
			}


			// Save data
			uint variables[TRACE_REG_R11 - TRACE_REG_RAX + 1];

			for (int i = 0; i < TRACE_REG_R11 - TRACE_REG_RAX + 1; i++) {
				int p = (*placement)[i];
				int j = i + TRACE_REG_RAX;

				if (p >= 0) {
					uint variable = regs[j].variable;
					variables[i] = variable;

					int psize = Trace_packSize(varInfos[variable].size);
					regs[j].variable = TRACE_VARIABLE_NONE;
					regs[j].nextUse = -1;

					varInfos[variable].store = p;

					fprintf(output, "\tmov ");
					Trace_printUsage(trace, output, psize, (Use){p, 0});
					fprintf(output, ", %s ; for fncall \n", REGISTER_NAMES[psize][j]);
				
				} else if (p != -TRACE_REG_NONE) {

					// Move to rbx
					uint variable = regs[j].variable;
					variables[i] = variable;
					trprintf("sis %d\n", i);
					int psize = Trace_packSize(varInfos[variable].size);
					
					varInfos[variable].store = p;

					p = -p;
					slowRegisters |= 1 << p;
					regs[p].variable = variable;
					regs[p].nextUse = regs[j].nextUse;


					regs[j].variable = TRACE_VARIABLE_NONE;
					regs[j].nextUse = -1;

					fprintf(
						output,
						"\tmov %s, %s; for fncall\n",
						REGISTER_NAMES[psize][p],
						REGISTER_NAMES[psize][j]
					);

				}

			}

			

			
			// Call function
			Function* fn = Trace_getFunction(trace, line>>10);
			fprintf(output, "\tcall fn_%016lx\n", fn->traceId);
			
			// Restore@data
			for (int i = TRACE_REG_R11 - TRACE_REG_RAX; i >= 0; i--) {
				int p = (*placement)[i];

				if (i == TRACE_REG_RAX-1 && waitRax.size == WAITING_ENABLED) {
					if (p != -TRACE_REG_NONE) {
						uint v = variables[i];
						waitRax.store = p;
						waitRax.size = varInfos[v].size;
						waitRax.variable = v;
					}
					continue;
				}

				if (i == TRACE_REG_RDX-1 && waitRdx.size == WAITING_ENABLED) {
					if (p != -TRACE_REG_NONE) {
						uint v = variables[i];
						waitRdx.store = p;
						waitRdx.size = varInfos[v].size;
						waitRdx.variable = v;
					}
					continue;
				}


				restoreAfterFnCall(p, i, variables[i], varInfos, regs, output, trace);
			}
			
			placement++;
			break;
		}

		case TRACECODE_IF:
		{
			/// TODO: mark real size
			fprintf(output, "\tcmp ");
			Trace_printUsage(trace, output, 2, useAt(0));
			desallowRsp(0);
			fprintf(output, ", 0\n\tjz .l%04d\n", line>>10);


			// trprintf("[TODO] asm IF\n");
			break;
		}
		
		case TRACECODE_JMP:
		{
			fprintf(output, "\tjmp .l%04d\n", line>>10);
			break;
		}
		
		case TRACECODE_CAST:
		{
			allowRsp(1);
			trline_t srcSigned = line & (1<<10);
			trline_t srcFloat = line & (1<<11);
			trline_t destSigned = line & (1<<12);
			trline_t destFloat = line & (1<<13);

			if (srcFloat || destFloat) {
				raiseError("[TODO] handle float casts");
				return;
			}

			
			trline_t srcPSize = (line >> 16) & 0x3;
			trline_t dstPSize = (line >> 18) & 0x3;
			if (dstPSize >= srcPSize) {
				fprintf(output, "\tmov%s ", destSigned ? "sx" : "zx");
				Trace_printUsage(trace, output, dstPSize, useAt(1));
				fprintf(output, ", ");
				Trace_printUsage(trace, output, srcPSize, useAt(0));
				fprintf(output, "\n");
			} else {
				/// TODO: handle something
			}
			
			desallowRsp(0);

			break;
		}


		case TRACECODE_STACK_PTR:
		{
			int variable = (line >> 16) & 0xfff;
			int offset = (*linePtr);
			linePtr++;
			ip++;

			int store = varInfos[variable].store;
			if (store < 0) {
				fprintf(output, "\tmov ");
				Trace_printUsage(trace, output, 3, useAt(0));
				fprintf(output, ", rsp\n\tadd rsp, 42; error\n");
				// raiseError("[Trace] Cannot get the address of a variable stored in a register");
				// return;

				break;
			}

			offset = trace->stackHandler.rsp - varInfos[variable].size;


			fprintf(output, "\tmov ");
			Trace_printUsage(trace, output, 3, useAt(0));
			fprintf(output, ", rsp\n\tadd ");
			Trace_printUsage(trace, output, 3, useAt(0));
			fprintf(output, ", %d\n", offset);

			break;
		}



		}
	
		#undef adaptAllow
		#undef adaptDesalloc
		#undef useAt
	}

	#undef printNumber

	finishWhile:
	if (placementsObj)
		free(placementsObj);

	TraceStackHandler_free(&trace->stackHandler);
	Stack_free(placementSaves);
	Stack_free(shadowPlacementSaves);

	for (int i = TRACE_REG_R15; i >= TRACE_REG_RBX; i--) {
		trprintf("k-%d\n", i);
		if (slowRegisters & (1<<i))
			fprintf(output, "\tpop %s\n", REGISTER_NAMES[3][i]);
	}


	fprintf(output, "\tret\n\n");
}







































void Trace_generateTranspiled(Trace* trace, FunctionAssembly* fnAsm) {
	typedef struct {
		int index;
		int size;
	} VarData;

	typedef struct {
		int variable;
		int offset;
	} Usage;

	enum {
		FLAG_REGISTRABLE = 1
	};

	
	enum {
		JMPIDX_IF,
		JMPIDX_ELSE
	};
	Stack jmpStack;
	Stack closingBraceStack;
	Stack_create(char, &jmpStack);
	Stack_create(int, &closingBraceStack);

	int variableLength = trace->variables.length;
	VarData variables[variableLength];
	char varFlags[variableLength];
	memset(variables, 0, sizeof(variables));
	memset(varFlags, 0, sizeof(varFlags));

	int instruction = 0;
	FILE* output = fnAsm->output;
	TracePack* pack = trace->first;
	Usage* usedVariables = malloc(sizeof(Usage) * 4);
	int usedVariablesCapacity = 4;
	int usedVariablesCount = 0;




	// Write function signature (return type and name)
	Function* currentFunction = fnAsm->fn;
	if (currentFunction->returnType) {
		fprintf(output, "// %s()\n%s fn_%016lx(",
			currentFunction->name,
			currentFunction->returnType->direct.cl->c_name,
			currentFunction->traceId
		);
	} else {
		fprintf(output, "// %s()\nvoid fn_%016lx(",
			currentFunction->name,
			currentFunction->traceId
		);
	}

	// Write args
	typedef Variable* vptr_t;
	int fnArgLen = currentFunction->args_len;
	Variable** fnArgs = currentFunction->arguments;
	for (int i = 0; i < fnArgLen; i++) {
		Variable* v = fnArgs[i];
		fprintf(output, "%s v%03d_%02d",
			Prototype_getClass(v->proto)->c_name, i, 1);
		
		if (i < fnArgLen-1)
			fprintf(output, ", ");
	}



	// Finish signature
	fprintf(output, ") {\n");

	// Create final variable
	bool finalDefined;
	if (currentFunction->returnType) {
		finalDefined = true;
		int size = Prototype_getSignedSize(currentFunction->returnType);
		switch (size) {
			case 1:
			case -1:
				fprintf(output, "\tuint8_t  final;\n");
				break;

			case 2:
			case -2:
				fprintf(output, "\tuint16_t final;\n");
				break;

			case 4:
			case -4:
				fprintf(output, "\tuint32_t final;\n");
				break;

			case 8:
			case -8:
				fprintf(output, "\tuint64_t final;\n");
				break;

			default:
				fprintf(output, "\tuint8_t  final[%d];\n", size);
				break;
			}
	} else {
		finalDefined = false;
	}

	#define move() {line = pack->line[instruction]; instruction++;}

	#define writePrimitive(u, size)\
		if (u.offset < 0) {\
			fprintf(output, "v%03d_%02d", u.variable,\
				variables[u.variable].index);\
		} else {\
			switch (size) {\
			case 1:\
				fprintf(output, "*(tuint8_t *)(&v%03d_%02d[%d])",\
					u.variable, variables[u.variable].index, u.offset);\
				break;\
			case 2:\
				fprintf(output, "*(tuint16_t*)(&v%03d_%02d[%d])",\
					u.variable, variables[u.variable].index, u.offset);\
				break;\
			case 4:\
				fprintf(output, "*(tuint32_t*)(&v%03d_%02d[%d])",\
					u.variable, variables[u.variable].index, u.offset);\
				break;\
			case 8:\
				fprintf(output, "*(tuint64_t*)(&v%03d_%02d[%d])",\
					u.variable, variables[u.variable].index, u.offset);\
				break;\
			}\
		}


	#define printImmediate(line, psize)\
		if (psize == 0 || psize == 1) {\
			fprintf(output, "%d", line >> 16);\
		} else if (psize == 2) {\
			move();\
			fprintf(output, "%d", line);\
		} else if (psize == 3) {\
			move();\
			uint64_t lo = (uint64_t)line;\
			move();\
			uint64_t hi = (uint64_t)line;\
			fprintf(output, "%lu", (hi >> 32) | lo);\
		}
		



	int receivingReturnVariable = -1;

	while (true) {
		if (
			!Stack_isEmpty(closingBraceStack) &&
			instruction == *Stack_seek(int, &closingBraceStack)
		) {
			Stack_pop(int, &closingBraceStack);
			fprintf(output, "\t}\n");
		}

		trline_t line;
		move();

		int code = line & 0x3ff;

		// Append usages
		if (code <= TRACE_USAGE_OUT_OF_BOUNDS) {
			if (usedVariablesCount == usedVariablesCapacity) {
				usedVariablesCapacity *= 2;
				usedVariables = realloc(usedVariables, usedVariablesCapacity * sizeof(Usage));
			}

			int variable = line >> 11;
			usedVariables[usedVariablesCount].variable = variable;
			if (varFlags[variable] & FLAG_REGISTRABLE) {
				usedVariables[usedVariablesCount].offset = -1;
			} else {
				move();
				usedVariables[usedVariablesCount].offset = line;
			}

			usedVariablesCount++;
			continue;
		}

		// Append instructions
		switch (code) {
		case TRACECODE_STAR:
		{
			int action = (line >> 10) & 0xf;
			switch (action) {
				case 0:
				// change pack
				pack = pack->next;
				if (!pack)
					goto finishMainWhile;

				instruction = 0;
				break;

			case 1: // quick skip
				break;
			
			case 2: // return
				fprintf(output, finalDefined ? "\treturn final;\n" : "return;");
				break;

			case 3: // forbid moves
				break;

			case 4: // protect RAX for fncall
				break;

			case 5: // protect RAX and RDX for fncall
				break;

			case 6: // mark label
				fprintf(output, "\tl%d:\n", instruction);
				break;

			case 7: // save placements
				break;

			case 8: // open placements
				break;
				
			case 9: // save shadow placements
				break;

			case 10: // open shadow placements
				break;

			case 11: // trival usages
				break;
				
			case 12: // fncall return dst variable (for transpiler)
				if (receivingReturnVariable == -1) {
					receivingReturnVariable = line >> 16;
				} else {
					receivingReturnVariable |= line & 0xffff0000;
				}
				break;

			}

			break;
		}

		case TRACECODE_CREATE:
		{
			trline_t isArg = line & (1<<11);
			trline_t variable = (line >> 16) & 0xfff;
			trline_t registrable = line & (1<<28);
			move();
			trline_t size = line >> 16;

			variables[variable].index++;
			variables[variable].size = size;
			varFlags[variable] = (registrable ? FLAG_REGISTRABLE : 0);

			// Arguments are already created, so let's skip them
			if (isArg)
				break;

			switch (size) {
			case 1:
				fprintf(output, "\tuint8_t  v%03d_%02d;\n", variable, variables[variable].index);
				break;

			case 2:
				fprintf(output, "\tuint16_t v%03d_%02d;\n", variable, variables[variable].index);
				break;

			case 4:
				fprintf(output, "\tuint32_t v%03d_%02d;\n", variable, variables[variable].index);
				break;

			case 8:
				fprintf(output, "\tuint64_t v%03d_%02d;\n", variable, variables[variable].index);
				break;

			default:
				fprintf(output, "\tuint8_t  v%03d_%02d[%d];\n", variable, variables[variable].index, size);
				
			}

			break;
		}

		case TRACECODE_DEF:
		{
			int type = (line >> 10) & 0x3;
			Usage usage = usedVariables[0];

			// Operand
			if (usage.offset < 0) {
				fprintf(output, "\tv%03d_%02d = ", usage.variable, variables[usage.variable].index);

			} else {
				uint8_t * prefix;
				switch (type) {
					case TRACETYPE_S8:  prefix = "(uint8_t *)"; break;
					case TRACETYPE_S16: prefix = "(uint16_t*)"; break;
					case TRACETYPE_S32: prefix = "(uint32_t*)"; break;
					case TRACETYPE_S64: prefix = "(uint64_t*)"; break;
					default:            prefix = "(void    *)"; break;
				}
				fprintf(output, "\t*(%s(v%03d_%02d + %d)) = ", prefix, usage.variable,
					variables[usage.variable].index, usage.offset);

			}

			// Value
			if (type == TRACETYPE_S8) {
				int value = (line >> 16) & 0xFF;
				fprintf(output, "%u;\n", value);
			} else if (type == TRACETYPE_S16) {
				int value = (line>>16) & 0xFFFF;
				fprintf(output, "%u;\n", value);
			} else if (type == TRACETYPE_S32) {
				move();
				fprintf(output, "%u;\n", line);
			} else {
				move();
				unsigned long l = line;
				move();
				unsigned long r = line;

				fprintf(output, "%lu;\n", l | (r << 32));
			}
			break;
		}

		case TRACECODE_MOVE:
		{
			Usage src = usedVariables[0];
			Usage dst = usedVariables[1];
			
			trline_t loadSrc = line & (1<<11);
			trline_t loadDst = line & (1<<12);
			int size = -1;
			int realSize;

			if (loadSrc && loadDst) {
				raiseError("[Intern] Load src and dst at once is an illegal operation");
				return;
			}

			fprintf(output, "\t");


			if (loadSrc) {
				realSize = line >> 16;

				if (dst.offset < 0) {
					size = -1;
					fprintf(output, "v%03d_%02d", dst.variable, variables[dst.variable].index);
				} else {
					switch (realSize) {
					case 1: 
						fprintf(output, "*((uint8_t *)(v%03d_%02d + %d))", dst.variable,
							variables[dst.variable].index, dst.offset);	
						size = -1;
						break;
	
					case 2:
						fprintf(output, "*((uint16_t*)(v%03d_%02d + %d))", dst.variable,
							variables[dst.variable].index, dst.offset);	
						size = -1;
						break;
	
					case 4:
						fprintf(output, "*((uint32_t*)(v%03d_%02d + %d))", dst.variable,
							variables[dst.variable].index, dst.offset);	
						size = -1;
						break;
	
					case 8:
						fprintf(output, "*((uint64_t*)(v%03d_%02d + %d))", dst.variable,
							variables[dst.variable].index, dst.offset);	
						size = -1;
						break;
	
	
					default:
						fprintf(output, "memcpy(v%03d_%02d + %d, ",
							dst.variable, variables[dst.variable].index, dst.offset);
						size = realSize;
						break;
	
					}
				}

				
				if (size < 0) {
					fprintf(output, " = ");
					if (src.offset < 0) {
						switch (realSize) {
							case 1: fprintf(output, "*(uint8_t *)"); break;
							case 2: fprintf(output, "*(uint16_t*)"); break;
							case 4: fprintf(output, "*(uint32_t*)"); break;
							case 8: fprintf(output, "*(uint64_t*)"); break;
						}
						fprintf(output, "v%03d_%02d", src.variable, variables[src.variable].index);

					} else {
						fprintf(output, "(*(void**)(v%03d_%02d + %d))",
							src.variable, variables[src.variable].index, src.offset);
					}
				
				} else if (src.offset < 0) {
					fprintf(output, ", v%03d_%02d, , %d)", src.variable, variables[src.variable].index, realSize);
				} else {
					fprintf(output, ", (*(void**)(v%03d_%02d + %d)), %d)",
						src.variable, variables[src.variable].index, src.offset, realSize);
				}

				goto finishMove;
			}

			if (loadDst) {
				if (dst.offset < 0) {
					size = line >> 16;
					realSize = size;
					fprintf(output, "memcpy((void*)v%03d_%02d, ",
						dst.variable, variables[dst.variable].index);
			
				} else {
					size = line >> 16;
					realSize = size;

					switch (size) {
					case 1: 
						fprintf(output, "**((uint8_t **)(v%03d_%02d + %d))", dst.variable,
							variables[dst.variable].index, dst.offset);	
						size = -1;
						break;

					case 2:
						fprintf(output, "**((uint16_t**)(v%03d_%02d + %d))", dst.variable,
							variables[dst.variable].index, dst.offset);	
						size = -1;
						break;

					case 4:
						fprintf(output, "**((uint32_t**)(v%03d_%02d + %d))", dst.variable,
							variables[dst.variable].index, dst.offset);	
						size = -1;
						break;

					case 8:
						fprintf(output, "**((uint64_t**)(v%03d_%02d + %d))", dst.variable,
							variables[dst.variable].index, dst.offset);	
						size = -1;
						break;


					default:
						fprintf(output, "memcpy(v%03d_%02d + %d, ",
							dst.variable, variables[dst.variable].index, dst.offset);
						break;

					}
				}
				
			} else if (dst.offset < 0) {
				fprintf(output, "v%03d_%02d", dst.variable, variables[dst.variable].index);
				size = -1;
				realSize = line >> 16;

			} else {
				size = line >> 16;
				realSize = size;
				switch (size) {
				case 1: 
					fprintf(output, "*((uint8_t *)(v%03d_%02d + %d))", dst.variable,
						variables[dst.variable].index, dst.offset);	
					size = -1;
					break;

				case 2:
					fprintf(output, "*((uint16_t*)(v%03d_%02d + %d))", dst.variable,
						variables[dst.variable].index, dst.offset);	
					size = -1;
					break;

				case 4:
					fprintf(output, "*((uint32_t*)(v%03d_%02d + %d))", dst.variable,
						variables[dst.variable].index, dst.offset);	
					size = -1;
					break;

				case 8:
					fprintf(output, "*((uint64_t*)(v%03d_%02d + %d))", dst.variable,
						variables[dst.variable].index, dst.offset);	
					size = -1;
					break;


				default:
					fprintf(output, "memcpy(v%03d_%02d + %d, ",
						dst.variable, variables[dst.variable].index, dst.offset);
					break;

				}
			}

			if (size < 0) {
				fprintf(output, " = ");
			}

			if (src.offset < 0) {
				if (size < 0) {
					fprintf(output, "v%03d_%02d", src.variable, variables[src.variable].index);
				} else {
					fprintf(output, "&v%03d_%02d, %d)",
						src.variable, variables[src.variable].index, size);
				}
			
			} else if (size < 0) {
				switch (realSize) {
				case 1: 
					fprintf(output, "*((uint8_t *)(v%03d_%02d + %d))", src.variable,
						variables[src.variable].index, src.offset);	
					break;

				case 2:
					fprintf(output, "*((uint16_t*)(v%03d_%02d + %d))", src.variable,
						variables[src.variable].index, src.offset);	
					break;

				case 4:
					fprintf(output, "*((uint32_t*)(v%03d_%02d + %d))", src.variable,
						variables[src.variable].index, src.offset);	
					break;

				case 8:
					fprintf(output, "*((uint64_t*)(v%03d_%02d + %d))", src.variable,
						variables[src.variable].index, src.offset);	
					break;

				default:
					raiseError("[Intern] size error in transpilation");
					return;
				}

			} else {
				fprintf(output, "v%03d_%02d + %d, %d)",
					src.variable, variables[src.variable].index, src.offset, size);

			}


			finishMove:
			fprintf(output, ";\n");


			break;
		}

		case TRACECODE_PLACE:
		{
			// Edit variable (can be ignored by transpiler)
			if (line & (1<<12)) {
				break;
			}


			Usage u = usedVariables[0];
			fprintf(output, "\tfinal = v%03d_%02d;\n", u.variable, variables[u.variable].index);
			break;
		}

		case TRACECODE_ARITHMETIC:
		{
			int operation = (line >> 10) & 0x7;
			int psize = (line >> 13) & 0x3;

			Usage u = usedVariables[1];
			fprintf(output, "\t");
			writePrimitive(u, variables[u.variable].size);
			fprintf(output, " = ");
			if (operation <= TRACEOP_MODULO) {
				u = usedVariables[0];
				writePrimitive(u, variables[u.variable].size);
				fprintf(output, " %s ", ARITHMETIC_SYMBOLS[operation]);
				u = usedVariables[2];
				writePrimitive(u, variables[u.variable].size);
				fprintf(output, ";\n");

			} else {
				raiseError("[TODO] transpile this operation");
			}
			break;
		}

		case TRACECODE_ARITHMETIC_IMM:
		{
			int operation = (line >> 10) & 0x7;
			int psize = (line >> 13) & 0x3;
			int immAtRight = line & (1<<15);

			Usage u = usedVariables[1];
			fprintf(output, "\t");
			writePrimitive(u, variables[u.variable].size);
			fprintf(output, " = ");

			
			if (operation <= TRACEOP_MODULO) {
				if (immAtRight) {
					u = usedVariables[0];
					writePrimitive(u, variables[u.variable].size);
					fprintf(output, " %s ", ARITHMETIC_SYMBOLS[operation]);
					printImmediate(line, psize);
					fprintf(output, ";\n");
				} else {
					u = usedVariables[0];
					printImmediate(line, psize);
					fprintf(output, " %s ", ARITHMETIC_SYMBOLS[operation]);
					writePrimitive(u, variables[u.variable].size);
					fprintf(output, ";\n");

				}
			} else {
				raiseError("[TODO] Handle imm arithmetic operation");
			}

			break;
		}

		case TRACECODE_LOGIC:
		{
			int operation = (line >> 10) & 0xf;
			int psize = (line >> 14) & 0x3;

			Usage u = usedVariables[1];
			fprintf(output, "\t");
			writePrimitive(u, variables[u.variable].size);
			fprintf(output, " = ");
			u = usedVariables[0];
			writePrimitive(u, variables[u.variable].size);
			fprintf(output, " %s ", LOGIC_SYMBOLS[operation]);
			u = usedVariables[2];
			writePrimitive(u, variables[u.variable].size);
			fprintf(output, ";\n");			
			break;
		}

		case TRACECODE_LOGIC_IMM_LEFT:
		{
			raiseError("[TODO] TRACECODE_LOGIC_IMM_LEFT");
			break;
		}
		
		case TRACECODE_LOGIC_IMM_RIGHT:
		{
			raiseError("[TODO] TRACECODE_LOGIC_IMM_RIGHT");
			break;
		}

		case TRACECODE_FNCALL:
		{
			// Write destination
			if (receivingReturnVariable >= 0) {
				fprintf(output, "\tv%03d_%02d = ",
					receivingReturnVariable, variables[receivingReturnVariable].index);
			} else {
				fprintf(output, "\t");
			}

			// Call fn
			Function* fn = TraceFunctionMap_getIdx(&trace->calledFunctions, line >> 10);
			fprintf(output, "fn_%016lx(", fn->traceId);
			
			// Put args
			for (int i = 0; i < usedVariablesCount; i++) {
				Usage u = usedVariables[i];
				writePrimitive(u, variables[u.variable].size);

				if (i < usedVariablesCount-1) {
					fprintf(output, ", ");
				}
			}

			fprintf(output, ");\n");
			receivingReturnVariable = -1;
			break;
		}

		case TRACECODE_IF:
		{
			fprintf(output, "\tif (");
			Usage u = usedVariables[0];
			writePrimitive(u, variables[u.variable].size);
			fprintf(output, ") {\n");

			*Stack_push(char, &jmpStack) = JMPIDX_IF;
			*Stack_push(int, &closingBraceStack) = line >> 10;
			break;
		}

		case TRACECODE_JMP:
		{
			trline_t target = line >> 10;
			switch (*Stack_pop(char, &jmpStack)) {
			case JMPIDX_IF:
				if (target >= instruction) {
					fprintf(output, "\t} else {\n");
				} else {
					fprintf(output, "\tgoto l%d;\n\t}\n", target);
				}
				Stack_pop(int, &closingBraceStack); // ignore the closing brace added by if
				// *Stack_push(int, &jmpStack) = JMPIDX_ELSE;
				break;

			case JMPIDX_ELSE:
				// raiseError("[Intern] Should not ");

			default:
				raiseError("[Intern] jmpStack is corrupted");
				return;
			}
			break;
		}

		case TRACECODE_CAST:
		{
			int operation = (line >> 10) & 0x7;
			int psize = (line >> 13) & 0x3;

			// Destination
			Usage u = usedVariables[1];
			fprintf(output, "\t");
			writePrimitive(u, variables[u.variable].size);

			const char* prefix;
			switch (psize) {
				case TRACETYPE_S8:  prefix = "(uint8_t)"; break;
				case TRACETYPE_S16: prefix = "(uint16_t)"; break;
				case TRACETYPE_S32: prefix = "(uint32_t)"; break;
				case TRACETYPE_S64: prefix = "(uint64_t)"; break;
				default:            prefix = "(void)"; break;
			}

			fprintf(output, " = %s(", prefix);

			// Source
			u = usedVariables[0];
			writePrimitive(u, variables[u.variable].size);
			fprintf(output, ");\n");
			
			break;
		}

		case TRACECODE_STACK_PTR:
		{
			trline_t variable = (line >> 16) & 0xfff;
			move();
			int offset = line;

			Usage dst = usedVariables[0];
			if (dst.offset < 0) {
				fprintf(output, "\tv%03d_%02d = ", dst.variable, variables[dst.variable].index);
			} else {
				fprintf(output, "\t*(uint64_t*)(v%03d_%02d + %d) = ",
					dst.variable, variables[dst.variable].index, dst.offset);
			}

			if (offset == 0) {
				fprintf(output, "(uint64_t)(v%03d_%02d)", variable, variables[variable].index);
			} else if (offset == -1) {
				fprintf(output, "(uint64_t)(&v%03d_%02d)", variable, variables[variable].index);
			} else {
				fprintf(output, "(uint64_t)(&v%03d_%02d[%d])", variable, variables[variable].index, offset);
			}

			fprintf(output, ";\n");
			break;
		}

		case TRACECODE_LOAD:
		{
			break;
		}


		}


		usedVariablesCount = 0;	
	}

	finishMainWhile:

	Stack_free(jmpStack);
	Stack_free(closingBraceStack);

	free(usedVariables);
	fprintf(output, "}\n");
	#undef move
	#undef writePrimitive
	#undef printImmediate
}

