#include "Trace.h"

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



static const int ARGUMENT_REGISTERS[] = {
	TRACE_REG_RCX,
	TRACE_REG_R11,
};
enum {
	ARGUMENT_REGISTERS_LENGTH = sizeof(ARGUMENT_REGISTERS)/sizeof(int)
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
	FIRSTUSE_WAITING = -1,
	FIRSTUSE_ELIMINATED = -2,

	LUI_FINAL = (int)-0x80000000,
};

typedef struct {
	trline_t* ptr;
	int lui;
	int scopeId;
	int firstRead;
} Write;

typedef struct {
	union {
		trline_t* lastUsePtr;
		Write* writes;
	};

	int lastUseInstruction;
	int scopeId;
	int firstRead;
	int usageCapacity;
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
	trace->deep = 0;
	trace->scopeId = 0;
	trace->nextScopeId = 1;
	TraceFunctionMap_create(&trace->calledFunctions);
}


void Trace_delete(Trace* trace) {
	TracePack* p = trace->first;

	while (p) {
		TracePack* next = p->next;
		free(p);
		p = next;
	}

	Array_free(trace->replaces);
	free(trace->varInfos);
	free(trace->regs);

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
	vt->firstRead = FIRSTUSE_WAITING;
	
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
		Trace_ins_create(trace, v, v->proto.size, 2, v->proto.cl->isRegistrable);
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
					printf("[%04d] STAR action=%d\n", i+position, (n>>10)&0xf);
					if ((n>>10) == 0)
						goto finish;
					break;

				case TRACECODE_CREATE: {
					uint32_t n2   = pack->line[++i];
					int variable  = (n >> 16) & 0xFFF;
					int size      = n2 >> 16;
					int regable   = n & (1<<28);
					int next2     = n2& 0x3FF;
					printf(
						"[%04d] CREATE v%d size=%d next=+%d %s\n",
						i-1+position,
						variable,
						size,
						next2,
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
						printf("[%04d] DEF value=%d (S16)\n", i+position, (int16_t)value);
					}
					else if (type == TRACETYPE_S8) {
						int value = (n>>16) & 0xFF;
						printf("[%04d] DEF value=%d (S8)\n", i+position, (int8_t)value);
					}
					else if (type == TRACETYPE_S32) {
						uint32_t n2 = pack->line[i+1];
						printf("[%04d] DEF value=%d (S32)\n", i+position, (int32_t)n2);
						i++;
					}
					else {
						uint32_t n2 = pack->line[i];
						uint32_t n3 = pack->line[i+1];
						printf("[%04d] DEF value_lo=%u value_hi=%u\n", i+position, n2, n3);
						i += 2;
					}

					break;
				}

				case TRACECODE_MOVE:
				{
					printf("[%04d] MOVE size=%d\n", i+position, n >> 16);
					break;
				}

				case TRACECODE_PLACE:
				{
					int size       = (n >> 10) & 0x3;
					int receiver   = (n >> 12) & 0x1;
					int reg_value  = (n >> 16) & 0xFF;

					printf("[%04d] PLACE size=%d ", i+position, size);
					printf("reg=%d edit:%s\n", reg_value,
							receiver ? "VARIABLE" : "REGISTER");

					break;
				}

				case TRACECODE_ARITHMETIC: {
					int op   = (n >> 10) & 0x7;
					int type = (n >> 13) & 0x3;

					printf("[%04d] ARITH op=%d (", i+position, op);
					switch (op) {
						case TRACEOP_ADDITION:      printf("ADDITION"); break;
						case TRACEOP_SUBSTRACTION:  printf("SUBSTRACTION"); break;
						case TRACEOP_MULTIPLICATION:printf("MULTIPLICATION"); break;
						case TRACEOP_DIVISION:      printf("DIVISION"); break;
						case TRACEOP_MODULO:        printf("MODULO"); break;
						case TRACEOP_INC:           printf("INC"); break;
						case TRACEOP_DEC:           printf("DEC"); break;
						default:                    printf("UNKNOWN"); break;
					}
					printf(") ps=%d\n", type);
				} break;

				case TRACECODE_ARITHMETIC_IMM: {
					int op        = (n >> 10) & 0x7;
					int type      = (n >> 13) & 0x3;
					int side      = (n >> 15) & 0x1;
					uint32_t star = (n >> 16);

					printf("[%04d] ARITH/IMM op=%d (", i+position, op);
					switch (op) {
						case TRACEOP_ADDITION:      printf("ADDITION"); break;
						case TRACEOP_SUBSTRACTION:  printf("SUBSTRACTION"); break;
						case TRACEOP_MULTIPLICATION:printf("MULTIPLICATION"); break;
						case TRACEOP_DIVISION:      printf("DIVISION"); break;
						case TRACEOP_MODULO:        printf("MODULO"); break;
						case TRACEOP_INC:           printf("INC"); break;
						case TRACEOP_DEC:           printf("DEC"); break;
						default:                    printf("UNKNOWN"); break;
					}
					printf(") type=%d ", type);

					if (op == TRACEOP_INC || op == TRACEOP_DEC) {
						printf("on %s operand\n", side ? "RIGHT" : "LEFT");
					} else {
						if (type == 0 || type == 1) {
							printf("VALUE=%u (%s) on %s operand\n", star,
								type == 0 ? "char" : "short", side ? "RIGHT" : "LEFT");
						} else if (type == 2) {
							i++;
							printf("VALUE=%d (int) on %s operand\n", (int)pack->line[i], side ? "RIGHT" : "LEFT");
						} else if (type == 3) {
							i++;
							uint32_t lo = pack->line[i];
							i++;
							uint32_t hi = pack->line[i];
							printf("VALUE=%u%u (long) on %s operand\n", hi, lo, side ? "RIGHT" : "LEFT");
						}
					}
				} break;

				case TRACECODE_LOGIC: {
					int op   = (n >> 10) & 0xF;
					int type = (n >> 14) & 0x3;

					printf("[%04d] LOGIC op=%d (", i+position, op);
					switch (op) {
						case TRACEOP_BITWISE_AND:   printf("BITWISE_AND"); break;
						case TRACEOP_BITWISE_OR:    printf("BITWISE_OR"); break;
						case TRACEOP_BITWISE_XOR:   printf("BITWISE_XOR"); break;
						case TRACEOP_LEFT_SHIFT:    printf("LEFT_SHIFT"); break;
						case TRACEOP_RIGHT_SHIFT:   printf("RIGHT_SHIFT"); break;
						case TRACEOP_LOGICAL_AND:   printf("LOGICAL_AND"); break;
						case TRACEOP_LOGICAL_OR:    printf("LOGICAL_OR"); break;
						case TRACEOP_EQUAL:         printf("EQUAL"); break;
						case TRACEOP_NOT_EQUAL:     printf("NOT_EQUAL"); break;
						case TRACEOP_LESS:          printf("LESS"); break;
						case TRACEOP_LESS_EQUAL:    printf("LESS_EQUAL"); break;
						case TRACEOP_GREATER:       printf("GREATER"); break;
						case TRACEOP_GREATER_EQUAL: printf("GREATER_EQUAL"); break;
						default:                    printf("UNKNOWN"); break;
					}
					printf(") type=%d\n", type);
				} break;

				case TRACECODE_LOGIC_IMM_LEFT:
				case TRACECODE_LOGIC_IMM_RIGHT:
				{
					int op   = (n >> 10) & 0xF;
					int type = (n >> 14) & 0x3;

					printf("[%04d] LOGIC/IMM op=%d (", i+position, op);
					switch (op) {
						case TRACEOP_BITWISE_AND:   printf("BITWISE_AND"); break;
						case TRACEOP_BITWISE_OR:    printf("BITWISE_OR"); break;
						case TRACEOP_BITWISE_XOR:   printf("BITWISE_XOR"); break;
						case TRACEOP_LEFT_SHIFT:    printf("LEFT_SHIFT"); break;
						case TRACEOP_RIGHT_SHIFT:   printf("RIGHT_SHIFT"); break;
						case TRACEOP_LOGICAL_AND:   printf("LOGICAL_AND"); break;
						case TRACEOP_LOGICAL_OR:    printf("LOGICAL_OR"); break;
						case TRACEOP_EQUAL:         printf("EQUAL"); break;
						case TRACEOP_NOT_EQUAL:     printf("NOT_EQUAL"); break;
						case TRACEOP_LESS:          printf("LESS"); break;
						case TRACEOP_LESS_EQUAL:    printf("LESS_EQUAL"); break;
						case TRACEOP_GREATER:       printf("GREATER"); break;
						case TRACEOP_GREATER_EQUAL: printf("GREATER_EQUAL"); break;
						default:                    printf("UNKNOWN"); break;
					}
					printf(") type=%d immediate:%s ", type, next == TRACECODE_LOGIC_IMM_LEFT ? "LEFT" : "RIGHT");

					if (type < 2) {
						printf("VALUE=%u (int)\n", (unsigned int)(n >> 16));
					} else if (type == 2) {
						i++;
						uint32_t value = pack->line[i];
						printf("VALUE=%d (int)\n", (int)value);
					} else {
						i++;
						uint32_t lo = pack->line[i];
						i++;
						uint32_t hi = pack->line[i];
						printf("VALUE_LO=%u VALUE_HI=%u\n", lo, hi);
					}
				} break;


				case TRACECODE_FNCALL:
				{
					printf("[%04d] FNCALL idx=%02d\n", i+position, n >> 10);
					break;
				}

				case TRACECODE_IF:
				{
					printf("[%04d] IF to=%04d\n", i+position, n >> 10);
					break;
				}

				case TRACECODE_JMP:
				{
					printf("[%04d] JMP to=%04d\n", i+position, n >> 10);
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

					printf("[%04d] CAST src{", i);

						if (src_signed)
							printf("signed ");
						if (src_float)
							printf("float ");

						printf("ps: %d} dest{", src_size);

						if (dest_signed)
							printf("signed ");
						if (dest_float)
							printf("float ");

						printf("ps: %d}\n", dest_size);
						break;
				}



				default:
					printf("[%04d] UNKNOWN INSTRUCTION code=%d\n", i+position, next);
					break;
			}



		} else {
			// Usage mode
			int read = n & (1<<10);
			int variable = n >> 11;
			if (next == TRACE_USAGE_LAST) {
				printf("[%04d] !f v%d %s", i+position, variable, read ? "" : "edit");
			} else {
				printf("[%04d] !u v%d %snext=+%d", i+position, variable, read ? "" : "edit ", next);
			}

			if (registrables[variable]) {
				printf("\n");
			} else {
				i++;
				n = pack->line[i];
				printf(" offset=%d\n", n);
			}

		}
	}	


	finish:
	free(registrables);
}


static Write* reachWrite(int idx, int usageCapacity, VariableTrace* vt) {
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
		printf("nui%d\n", i);
		nextWrites[i].lui = LUI_FINAL;
		nextWrites[i].scopeId = -1;
	}

	if (usageCapacity == 0) {
		nextWrites[0].ptr = vt->lastUsePtr;
		nextWrites[0].lui = vt->lastUseInstruction;
		nextWrites[0].firstRead = vt->firstRead;
	} else {
		nextWrites[usageCapacity].lui = LUI_FINAL;
	}
	
	nextWrites[usageCapacity].scopeId = idx;

	vt->usageCapacity = nextCapacity;
	vt->writes = nextWrites;
	return &nextWrites[idx];
}

void Trace_addUsageAt(Trace* trace, uint variable, bool readMode, trline_t* ptr) {
	if (variable == TRACE_VARIABLE_NONE) {
		return;
	}

	VariableTrace* vt = Array_get(VariableTrace, trace->variables, variable);
	int traceInstruction = trace->instruction;
	
	int usageCapacity = vt->usageCapacity;
	
	// Read mode
	if (readMode) {
		// Simple
		if (usageCapacity == 0) {
			int lui = vt->lastUseInstruction;
			if (lui != LUI_FINAL) {
				int diff = traceInstruction - lui;
				if (diff > TRACE_USAGE_OUT_OF_BOUNDS) {
					diff = TRACE_USAGE_OUT_OF_BOUNDS;
				}
				*vt->lastUsePtr |= diff;
			}
	
			if (vt->firstRead == FIRSTUSE_WAITING) {
				vt->firstRead = traceInstruction;
			}

			vt->lastUseInstruction = traceInstruction;
			vt->lastUsePtr = ptr;
			goto finish;
		}

		// Handle multiusages	
		Write* const writes = vt->writes;
		
		// Mark usages
		int tScopeId = trace->scopeId;
		int vScopeId = vt->scopeId;
		for (int i = 0; i < usageCapacity; i++) {
			Write* write = &writes[i];

			if (write->scopeId <= tScopeId && write->firstRead == FIRSTUSE_WAITING) {
				write->firstRead = traceInstruction;
			}


			int lui = write->lui;
			if (lui == LUI_FINAL)
				continue;

			int diff = traceInstruction - lui;
			if (diff > TRACE_USAGE_OUT_OF_BOUNDS) {
				diff = TRACE_USAGE_OUT_OF_BOUNDS;
			}

			printf("lui%d %d diff=%d\n", i, lui, diff);
			*(write->ptr) |= diff;

			write->lui = LUI_FINAL;
		}

		// Add our usage
		Write* write = reachWrite(tScopeId, usageCapacity, vt);
		write->lui = traceInstruction;
		write->ptr = ptr;


		goto finish;
	}
	


	// Write mode
	int tScopeId = trace->scopeId;
	int vScopeId = vt->scopeId;
	// Simple
	if (usageCapacity == 0) {
		if (tScopeId > vScopeId) {
			// switch to multiusage
			goto writeMulti;
		}

		vt->lastUseInstruction = traceInstruction;
		vt->lastUsePtr = ptr;

		if (vt->firstRead == FIRSTUSE_WAITING) {
			vt->firstRead = FIRSTUSE_ELIMINATED;
		}

	} else {
		// Handle multiusages
		writeMulti:
		Write* write = reachWrite(tScopeId, usageCapacity, vt);
		usageCapacity = vt->usageCapacity;

		printf("set %d\n", traceInstruction);
		write->lui = traceInstruction;
		write->ptr = ptr;

		if (write->firstRead == FIRSTUSE_WAITING) {
			write->firstRead = FIRSTUSE_ELIMINATED;
		}

		
		/// TODO: check for whiles
		// Mark final usage in upper scopes
		Write* writes = vt->writes;
		for (int i = 0; i < usageCapacity; i++) {
			write = &writes[i];
			if (write->scopeId <= tScopeId)
				continue;

			if (write->firstRead == FIRSTUSE_WAITING) {
				write->firstRead = FIRSTUSE_ELIMINATED;
			}

			write->lui = LUI_FINAL;
		}
	}


	finish:
	*ptr = TRACE_USAGE_LAST | (variable << 11) | (readMode ? (1<<10) : 0);
}

void Trace_addUsage(Trace* trace, uint variable, int offset, bool readMode) {
	printf("usage %2d %2d | %s\n", variable, offset, readMode ? "read" : "write");

	if (offset < 0) {
		trline_t* ptr = Trace_push(trace, 1);
		Trace_addUsageAt(trace, variable, readMode, ptr);
	} else {
		trline_t* ptr = Trace_push(trace, 2);
		Trace_addUsageAt(trace, variable, readMode, &ptr[0]);
		ptr[1] = (uint)offset;

	}

}







void Trace_ins_savePlacement(Trace* trace) {
	*Trace_push(trace, 1) = TRACECODE_STAR | (7<<10);
}

void Trace_ins_openPlacement(Trace* trace) {
	*Trace_push(trace, 1) = TRACECODE_STAR | (8<<10);
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




void Trace_set(Trace* trace, Expression* expr, uint destVar, int destOffset, int signedSize, int exprType) {
	int size = signedSize >= 0 ? signedSize : -signedSize;

	switch (exprType) {
	case EXPRESSION_PROPERTY:
	{
		int subLength = expr->data.property.length - 1;
		Variable** varr = expr->data.property.variableArr;
		char primitiveSizeCode = varr[subLength]->proto.cl->primitiveSizeCode;
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
			signedObjSize = Prototype_getSize(&varr[subLength]->proto);
		}


		// Size matches
		if (signedObjSize == signedSize) {
			Trace_ins_move(
				trace,
				destVar,
				varr[0]->id,
				destOffset,
				Prototype_getVariableOffset(varr, subLength),
				size
			);
			return;
		}

		// Let's cast
		int objSize = signedObjSize >= 0 ? signedObjSize : -signedObjSize;
		uint tempVar = Trace_ins_create(trace, NULL, objSize, 0, true);
		Trace_ins_move(
			trace,
			tempVar,
			varr[0]->id,
			-1,
			destOffset,
			objSize
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
		Expression_FnCall fncall = *expr->data.fncall.object;
		Variable** arguments = fncall.fn->arguments.data;
		uint* variables = malloc(fncall.argsLength * sizeof(int));

		for (int i = 0; i < fncall.argsLength; i++) {
			/// TODO: check sizes
			printf("args_are(%d) %p for %s\n", i, arguments, fncall.fn->name);
			char isRegistrable = arguments[i]->proto.cl->isRegistrable;
			int size = Prototype_getSize(&arguments[i]->proto);
			uint bufferVar = Trace_ins_create(trace, NULL, size, 0, isRegistrable);
			
			Trace_set(
				trace,
				fncall.args[i],
				bufferVar,
				arguments[i]->proto.cl->isRegistrable ? TRACE_OFFSET_NONE : 0,
				Prototype_getSignedSize(&arguments[i]->proto),
				fncall.args[i]->type
			);

			uint finalVar = Trace_ins_create(trace, NULL, size, 0, isRegistrable);
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


		// Mark rax will be used
		if (destVar != TRACE_VARIABLE_NONE) {
			*Trace_push(trace, 1) = TRACECODE_STAR | (4 << 10);
		}

		int fnIndex = Trace_reachFunction(trace, fncall.fn);

		// Add usages
		for (int i = 0; i < fncall.argsLength; i++) {
			Trace_addUsage(
				trace, variables[i],
				arguments[i]->proto.cl->isRegistrable ? TRACE_OFFSET_NONE : 0,
				true
			);
		}

		// Function call
		*Trace_push(trace, 1) = TRACECODE_FNCALL | (fnIndex << 10);

		// Remove variables
		for (int i = fncall.argsLength-1; i >= 0; i--) {
			Trace_removeVariable(trace, variables[i]);
		}

		// Output
		if (destVar != TRACE_VARIABLE_NONE) {
			int signedOutputSize = Prototype_getSignedSize(&fncall.fn->returnType);
			if (signedOutputSize == signedSize) {
				Trace_ins_placeVar(trace, destVar, TRACE_REG_RAX, Trace_packSize(size));
	
			} else {
				int outputSize = signedOutputSize >= 0 ? signedOutputSize : -signedOutputSize;
				uint temp = Trace_ins_create(
					trace, NULL, outputSize, 0,
					fncall.fn->returnType.cl->isRegistrable
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

	case EXPRESSION_PATH:
	{
		Expression* const real = expr->data.target;
		int realType = real->type;

		switch (realType) {
		case EXPRESSION_PROPERTY:
		{
			/// TODO: handle this case
			if (real->data.property.next) {
				raiseError("[TODO]: expr such that next==true not handled for Trace");
				return;
			}

			/// TODO: check size
			Trace_set(trace, real, destVar, destOffset, signedSize, EXPRESSION_PROPERTY);
			return;

		}

		case EXPRESSION_FNCALL:
		{
			Trace_set(trace, real, destVar, destOffset, signedSize, EXPRESSION_FNCALL);
			return;
		}

		}

		break;
	}

	case EXPRESSION_GROUP:
	{
		Expression* target = expr->data.target;
		Trace_set(trace, target, destVar, destOffset, signedSize, target->type);
		break;
	}



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

			int leftSize = signedLeftSize >= 0 ? signedLeftSize : -signedLeftSize;
			int rightSize = signedRightSize >= 0 ? signedRightSize : -signedRightSize;

			int signedMaxSize;
			int maxSize;
			if (leftSize == rightSize) {
				maxSize = leftSize;
				signedMaxSize = signedLeftSize >= 0 ? 
					signedLeftSize :
					signedRightSize;


			} else if (leftSize >= rightSize) {
				signedMaxSize = signedLeftSize;
				maxSize = leftSize;
			} else {
				signedMaxSize = signedRightSize;
				maxSize = rightSize;
			}
			
			uint leftVar = Trace_ins_create(trace, NULL, maxSize, 0, true);
			Trace_set(trace, leftExpr, leftVar, TRACE_OFFSET_NONE, signedMaxSize, leftType);

			uint rightVar = Trace_ins_create(trace, NULL, maxSize, 0, true);
			Trace_set(trace, rightExpr, rightVar, TRACE_OFFSET_NONE, signedMaxSize, rightType);

			int tempDestVar;
			
			// Add usages
			if (signedMaxSize == signedSize) {
				Trace_addUsage(trace, leftVar, TRACE_OFFSET_NONE, true);
				Trace_addUsage(trace, rightVar, TRACE_OFFSET_NONE, true);
				Trace_addUsage(trace, destVar, destOffset, false);
			} else {
				tempDestVar = Trace_ins_create(trace, NULL, maxSize, 0, true);
				Trace_addUsage(trace, leftVar, TRACE_OFFSET_NONE, true);
				Trace_addUsage(trace, rightVar, TRACE_OFFSET_NONE, true);
				Trace_addUsage(trace, tempDestVar, destOffset, false);
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

			int signedOperandSize = Expression_reachSignedSize(operandType, operand);
			int operandSize = signedOperandSize >= 0 ? signedOperandSize : -signedOperandSize;
			int signedDestImmediateSize;

			uint variable;
			uint resultVariable;
			bool notCast = operandSize <= size;

			/// TODO: handle case for same size but different sign
			if (notCast) {
				// Load operand
				variable = Trace_ins_create(trace, NULL, size, 0, true);
				signedDestImmediateSize = signedSize;
				Trace_set(trace, operand, variable, TRACE_OFFSET_NONE, signedSize, operandType);

				// Add usages
				Trace_addUsage(trace, variable, TRACE_OFFSET_NONE, true);
				Trace_addUsage(trace, destVar, destOffset, false);

			} else {
				// Load operand
				variable = Trace_ins_create(trace, NULL, size, 0, true);
				signedDestImmediateSize = signedOperandSize;
				Trace_set(trace, operand, variable, TRACE_OFFSET_NONE, signedOperandSize, operandType);

				resultVariable = Trace_ins_create(trace, NULL, size, 0, true);

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

	




	default:
		raiseError("[Trace] Expression type not handled");
		return;
	}
}


uint Trace_ins_create(Trace* trace, Variable* variable, int size, int flags, bool registrable) {
	if (size >= 0xffff) {
		raiseError("[TODO] handle create big size variables");
		return -1;
	}

	uint id = Trace_pushVariable(trace);
	trline_t* ptr = Trace_push(trace, 2);
	ptr[0] = TRACECODE_CREATE | (flags << 10) | (id << 16) | (registrable ? (1<<28) : 0);
	ptr[1] = TRACE_USAGE_LAST | (size << 16);
	
	VariableTrace* vt = Array_get(VariableTrace, trace->variables, id);
	vt->lastUsePtr = &ptr[1];
	vt->lastUseInstruction = trace->instruction - 1;
	vt->scopeId = trace->scopeId;
	

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
		ptr[1] = value.u64 & 0xffffffff;
		ptr[2] = (value.u64 >> 32) & 0xffffffff;
		break;
	}
}


void Trace_ins_move(Trace* trace, int destVar, int srcVar, int destOffset, int srcOffset, int size) {
	Trace_addUsage(trace, srcVar, srcOffset, true);
	Trace_addUsage(trace, destVar, destOffset, false);

	*Trace_push(trace, 1) = TRACECODE_MOVE | (size << 16);
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
	printf("choose: ");
	for (int i = TRACE_REG_RAX; i <= TRACE_REG_R11; i++) {
		printf("%d ", regs[i].nextUse);
	}
	printf("\n");

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
		printf("replace(%d:v%d) v%d in %d / %d\n", -reg, regs[reg].variable, variable, trace->varInfos[variable].store, nextUse);
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
				printf("nextvictim %d, %d %d\n", victim, reg, victimNextUse);
				victimRegister = Trace_reg_place(trace, regs, ip, victim, -reg, victimNextUse, victimRegister);
				replace = Array_push(TraceReplace, &trace->replaces);
				replace->victim = victim;
				replace->victimStore = -victimRegister;

			}*/
		}

		
	} else {
		printf("replace(%d) v%d in %d\n", reg, variable, trace->varInfos[variable].store);
		
		// No victim
		replace = Array_push(TraceReplace, &trace->replaces);
		replace->victim = -TRACE_REG_NONE;
		reg = -reg;
		
	}


	replace->instruction = ip;
	replace->variable = variable;
	replace->destination = -reg;
	replace->reading = reading;
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

static void placeRegisters_savePlacements(
	Trace* trace,
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
	bool take,
	VarInfoTrace varInfos[],
	TraceRegister regs[],
	Stack* placementSaves,
	uint length
) {
	int* stores = take ?
		*Stack_pop(int*, placementSaves) :
		*Stack_seek(int*, placementSaves);

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
					varInfos[v].store = dest;

				} else if (victim != v) {
					// exchange placements
					varInfos[victim].store = source;
					varInfos[v].store = dest;
				}
			
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

			printf("%d\n", dt);
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

	if (take) {
		free(stores);
	}
}




void Trace_placeRegisters(Trace* trace) {
	// Generate next data
	int varlength = trace->variables.length;
	VarInfoTrace* varInfos = malloc(sizeof(VarInfoTrace) * varlength);
	TraceRegister* regs = malloc((TRACE_REG_NONE+1) * sizeof(TraceRegister));
	Stack placementSaves; // type: int*
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

	Array_create(&trace->replaces, sizeof(TraceReplace));
	Array_create(&trace->fncallPlacements, sizeof(fnplacement_t));
	Stack_create(int, &placementSaves);
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
		printf("> at %d\n", ip);
		line = *linePtr;
		linePtr++;
		trline_t code = line & 0x3ff;

		// Usage
		if (code <= TRACE_USAGE_OUT_OF_BOUNDS) {
			usageBuffer[usageBufferLine] = line;
			usageBufferLine++;

			trline_t readmode = line & (1<<10);
			trline_t varIdx = line >> 11;
			VarInfoTrace* vi = &varInfos[varIdx];
			int store = vi->store;

			printf("rememb %d\n", vi->store);

			if (code == 0) {
				// Last usage => destroy variable
				vi->nextUse = -1;
				printf("kill v%d\n", varIdx);

				if (store == -TRACE_REG_NONE)
					goto continueWhile;
				
				// Register
				if (store < 0) {
					printf("destroy %d (v%d == v%d) \n", store, varIdx, regs[-store].variable);
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
			
			
			if (movesAllowed && varInfos[varIdx].registrable) {
				if (store >= 0 || store == -TRACE_REG_NONE) {
					printf("placedRegisters %d\n", placedRegisters);
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
				placeRegisters_savePlacements(trace, varInfos,
					&placementSaves, varlength);
				break;

			case 8: // open placements (take)
				placeRegisters_openPlacements(trace, true, varInfos,
					regs, &placementSaves, varlength);
				break;
				
				case 9: // open placements (seek)
				placeRegisters_openPlacements(trace, false, varInfos,
					regs, &placementSaves, varlength);
				break;
				
			}
			break;
		}

		case TRACECODE_CREATE:
		{
			char registrable = (line >> 28) & 1;
			char argument = (line >> 11) & 1;
			trline_t size = line >> 16;
			trline_t variable = (line >> 16) & 0xFFF;

			line = *linePtr;
			linePtr++;
			ip++;

			trline_t next = line & 0x3FF;
			int nextUse = next == 0 ? -1 : next;

			printf("create %d\n", variable);
			varInfos[variable].size = size;
			varInfos[variable].nextUse = nextUse;
			varInfos[variable].registrable = registrable;
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
				printf("plce %d|%d src %d(%d) ; dst %d(%d)\n", regs[TRACE_REG_R11].variable, reg, srcVar, source, destVar, varInfos[destVar].store);
				varInfos[srcVar].nextUse = -1;
				varInfos[srcVar].store = -TRACE_REG_NONE;

				if (source >= 0) {
					raiseError("[TODO]: free stack");

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
			printf("situat: ");
			for (int i = TRACE_REG_RAX; i <= TRACE_REG_R11; i++) {
				printf("%d ", regs[i].nextUse);
			}
			printf("\n");

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
					printf("REFUSE %d\n", p);
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

					printf("give %d\n", -regPos);
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

		}
	
		continueWhile:
		ip++;
	}


	finishWhile:
	// if (fncallPositionsOrigin)
		// free(fncallPositionsOrigin);
	

	Stack_free(placementSaves);
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

typedef struct {
	int store;
	int offset;
} Use;

void Trace_printUsage(Trace* trace, FILE* output, int psize, Use use) {
	if (use.store >= 0) {
		int size = Trace_unpackSize(psize);
		int pos = use.store;
		pos += use.offset;
		fprintf(output, "%s[rsp-%d]", REGISTER_SIZENAMES[psize], pos);
	} else {
		if (use.store == -TRACE_REG_NONE) {use.store = 0;}
		int s = -use.store;
		fprintf(output, "%s", REGISTER_NAMES[psize][s]);
	}	
}

void Trace_printFastUsage(Trace* trace, FILE* output, int psize, Use use) {
	if (use.store >= 0) {
		int size = Trace_unpackSize(psize);
		int pos = TraceStackHandler_guarantee(&trace->stackHandler, size, size, use.store);
		pos += use.offset;
		fprintf(output, "%s[rsp-%d]", REGISTER_SIZENAMES[psize], pos);
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

		TraceStackHandler_remove(&trace->stackHandler, p, size);

	} else if (p != -TRACE_REG_NONE) {
		int j = i + TRACE_REG_RAX;
		printf("vis %d %d\n", i, variable);
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
	Trace* trace,
	VarInfoTrace varInfos[],
	Stack* placementSaves,
	TraceStackHandler* stackHandler,
	uint length
) {
	int* placements = malloc(length * sizeof(int));

	for (uint i = 0; i < length; i++) {
		int store = varInfos[i].store;
		placements[i] = store;
		if (store >= 0) {
			TraceStackHandler_freeze(stackHandler, store);
		}
	}

	*Stack_push(int*, placementSaves) = placements;
}




typedef struct {
	int reg;
	uint variable;
	int packedSize;
	bool pop;
} TempReg;

static TempReg pushTemp(int rplceReg, TraceRegister regs[], VarInfoTrace varInfos[], FILE* output) {
	// Get register
	TempReg tmp;
	tmp.reg = Trace_reg_searchEmpty(regs);
	if (tmp.reg == -TRACE_REG_NONE) {
		tmp.pop = true;
		tmp.reg = rplceReg;
		tmp.variable = regs[rplceReg].variable;
		tmp.packedSize = tmp.variable == TRACE_VARIABLE_NONE ?
			3 : Trace_packSize(varInfos[tmp.variable].size);

		fprintf(
			output,
			"\tpush %s; restore\n",
			REGISTER_NAMES[tmp.packedSize][rplceReg]
		);

	} else {
		tmp.pop = false;
		regs[tmp.reg].nextUse = -2;
	}

	return tmp;
}

static void popTemp(TempReg* tmp, TraceRegister regs[], FILE* output) {
	if (tmp->pop) {
		fprintf(output, "\tpop rbx; buffer for restore\n");
		fprintf(
			output,
			"\tpop %s; restore\n",
			REGISTER_NAMES[tmp->packedSize][tmp->reg]
		);
	} else if (tmp->reg != -TRACE_REG_NONE) {
		regs[tmp->reg].nextUse = -1;
	}
}


void generateAssembly_openPlacements(
	Trace* trace,
	bool take,
	VarInfoTrace varInfos[],
	TraceRegister regs[],
	Stack* placementSaves,
	TraceStackHandler* stackHandler,
	FILE* output,
	uint length
) {
	int* stores = take ?
		*Stack_pop(int*, placementSaves) :
		*Stack_seek(int*, placementSaves);

	for (uint v = 0; v < length; v++) {
		int dest = stores[v];
		printf("destof(v%d): %2d from %2d\n", v, dest, varInfos[v].store);
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
			TraceStackHandler_remove(&trace->stackHandler, source, size);

			if (dest >= 0) {
				uint victim = searchStackId(
					varInfos, dest,
					trace->variables.length
				);
				

				TempReg tmp1;
				TempReg tmp2;

				int destAddress = TraceStackHandler_reach(&trace->stackHandler, dest);

				if (victim == TRACE_VARIABLE_NONE) {
					varInfos[v].store = dest;
					tmp2.reg = -TRACE_REG_NONE;
					
				} else if (victim != v) {
					// exchange placements
					varInfos[victim].store = source;
					varInfos[v].store = dest;

					tmp2 = pushTemp(
						TRACE_REG_RDX,
						regs, varInfos, output
					);

					// Load victim in register
					fprintf(
						output,
						"\tmov %s, %s[rsp-%d]\n",
						REGISTER_NAMES[psize][TRACE_REG_RDX],
						REGISTER_SIZENAMES[psize],
						destAddress
					);
				} else {
					continue;
				}

				tmp1 = pushTemp(TRACE_REG_RAX, regs, varInfos, output);

				fprintf(
					output,
					"\tmov %s, %s[rsp-%d]\n\tmov %s[rsp-%d], %s\n",
					
					// load source in reg1
					REGISTER_NAMES[psize][tmp1.reg],
					REGISTER_SIZENAMES[psize],
					sourceAddress,
					
					// move source to dest
					REGISTER_SIZENAMES[psize],
					destAddress,
					REGISTER_NAMES[psize][tmp1.reg]
				);

				// Move victim to source
				if (tmp2.reg != -TRACE_REG_NONE) {
					fprintf(
						output,
						"\tmov %s[rsp-%d], %s\n",

						REGISTER_SIZENAMES[psize],
						sourceAddress,
						REGISTER_NAMES[psize][tmp2.reg]
					);
				}

				
				popTemp(&tmp2, regs, output);
				popTemp(&tmp1, regs, output);
				continue;
			}

			// Here, dest < 0 is a register
			int dt = -dest;
			int destNextUse = regs[dt].nextUse;
			if (destNextUse >= 0) {
				uint victim = regs[dt].variable;
				varInfos[victim].store = source;

				TempReg tmp = pushTemp(TRACE_REG_RAX, regs, varInfos, output);
				fprintf(
					output,
					"\tmov %s, %s; restore#1\n\tmov %s, %s[rsp-%d]\n\tmov %s[rsp-%d], %s",

					REGISTER_NAMES[psize][tmp.reg],
					REGISTER_NAMES[psize][dt],

					REGISTER_NAMES[psize][dt],
					REGISTER_SIZENAMES[psize],
					sourceAddress,

					REGISTER_SIZENAMES[psize],
					sourceAddress,
					REGISTER_NAMES[psize][tmp.reg]
				);

				popTemp(&tmp, regs, output);

			} else {
				fprintf(
					output,
					"\tmov %s, %s[rsp-%d]; restore#2\n",
					REGISTER_NAMES[psize][dt],
					REGISTER_SIZENAMES[psize],
					sourceAddress
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
				&trace->stackHandler, size, size, dest);

			if (victim == TRACE_VARIABLE_NONE) {
				varInfos[v].store = dest;
				regs[st].variable = TRACE_VARIABLE_NONE;
				regs[st].nextUse = -1;


				fprintf(
					output,
					"\tmov %s[rsp-%d], %s; restore#3\n",
					REGISTER_SIZENAMES[psize],
					destAddress,
					REGISTER_NAMES[psize][st]
				);

			} else if (victim != v) {
				varInfos[v].store = dest;
				varInfos[victim].store = source;
				regs[st].variable = v;
				regs[st].nextUse = varInfos[victim].nextUse;

				TempReg tmp = pushTemp(TRACE_REG_RAX, regs, varInfos, output);

				fprintf(
					output,
					"\tmov %s %s[rsp-%d]; restore#4\n\tmov %s[rsp-%d] %s\n\tmov %s %s\n",

					REGISTER_NAMES[psize][tmp.reg],
					REGISTER_SIZENAMES[psize],
					destAddress,

					REGISTER_SIZENAMES[psize],
					destAddress,
					REGISTER_NAMES[psize][st],

					REGISTER_NAMES[psize][st],
					REGISTER_NAMES[psize][tmp.reg]
				);

				popTemp(&tmp, regs, output);

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
					"\txor %s, %s; restore#5\n\txor %s, %s\n\txor %s %s\n",

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
					"\tmov %s, %s; restore#6\n\tmov %s, %s\n\tmov %s %s\n",

					REGISTER_NAMES[psize][temp],
					REGISTER_NAMES[psize][st],

					REGISTER_NAMES[psize][st],
					REGISTER_NAMES[psize][dt],

					REGISTER_NAMES[psize][dt],
					REGISTER_NAMES[psize][temp]
				);
			}
		
		} else {
			printf("restoremov %d %d\n", st, dt);
			fprintf(
				output,
				"\tmov %s, %s; restore#7\n",
				REGISTER_NAMES[psize][st],
				REGISTER_NAMES[psize][dt]
			);
		}

		varInfos[v].store = dest;
		regs[dt].variable = v;
		regs[dt].nextUse = varInfos[v].nextUse;
	}


	if (take) {
		free(stores);
	}
}



void Trace_generateAssembly(Trace* trace, FunctionAssembly* fnAsm) {
	FILE* output = fnAsm->output;
	int varlength = trace->varlength;

	// Init data
	VarInfoTrace* varInfos = trace->varInfos;
	TraceRegister* regs = trace->regs;
	
	for (int i = 0; i < varlength; i++) {
		varInfos[i].store = -TRACE_REG_NONE;
	}

	TraceStackHandler_create(&trace->stackHandler, trace->stackId);

	fprintf(output, "fn_%p: ; %s\n", fnAsm->fn, fnAsm->fn->name);


	Stack placementSaves; // type: int*
	Stack_create(int, &placementSaves);


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
	
	Use uses[4];

	while (true) {
		// Print state
		for (int i = 0; i < varlength; i++) {
			int store = varInfos[i].store;
			if (store >= 0) {
				printf("@v%02d : %d[%d]\n", i, store, TraceStackHandler_reach(&trace->stackHandler, store));
			} else if (store != -TRACE_REG_NONE) {
				printf("@v%02d : %s\n", i, REGISTER_NAMES[3][-store]);
			} else {
				printf("@v%02d : NONE\n", i);
			}
		}

		printf("> to %d\n", ip);

		

		previousLine = line;
		line = *linePtr;
		linePtr++;
		trline_t code = line & 0x3ff;

		// Apply replaces
		while (replace < finalReplace && replace->instruction == ip) {
			uint variable = replace->variable;
			
			// Move victim
			int victim = replace->victim;
			printf("victim %d for v%d\n", victim, variable);
			if (victim >= 0) {
				int psize = Trace_packSize(varInfos[victim].size);
				int victimStore = replace->victimStore;
				printf("dest %d > %d\n", replace->destination, victimStore);
				
				fprintf(output, "\tmov ");
				Trace_printFastUsage(
					trace, output, psize,
					(Use){victimStore, 0}
				);
				fprintf(output, ", ");
				Trace_printFastUsage(
					trace, output, psize,
					(Use){varInfos[victim].store, 0}
				);
				fprintf(output, "; throw v%d\n", victim);

				varInfos[victim].store = victimStore;
			
			} else if (victim != -TRACE_REG_NONE) {
				victim = -victim;
				
				regs[victim].variable = TRACE_VARIABLE_NONE;
				regs[victim].nextUse = -1;
			}

			// Free stack area
			int destination = replace->destination;
			if (varInfos[variable].store >= 0 && replace->reading){
				int psize = Trace_packSize(varInfos[variable].size);
				fprintf(output, "\tmov ");
				Trace_printFastUsage(
					trace, output, psize,
					(Use){destination, 0}
				);
				fprintf(output, ", ");
				Trace_printFastUsage(
					trace, output, psize,
					(Use){varInfos[variable].store, 0}
				);
				fprintf(output, "; take v%d\n", variable);


				printf("store %d %d\n", variable, varInfos[variable].store);
				// Free taken value
				TraceStackHandler_remove(
					&trace->stackHandler,
					varInfos[variable].store,
					varInfos[variable].size
				);
			}

			// Update store
			varInfos[variable].store = destination;
			if (destination < 0) {
				destination = -destination;
				regs[destination].nextUse = varInfos[variable].nextUse;
				regs[destination].variable = variable;
			}

			replace++;
		}


		ip++;



		// Usage
		if (code <= TRACE_USAGE_OUT_OF_BOUNDS) {
			trline_t readmode = line & (1<<10);
			trline_t varIdx = line >> 11;
			int store = varInfos[varIdx].store;
			printf("read %d => %d\n", varIdx, store);
			if (varInfos[varIdx].registrable) {

				// Remove variable
				if (store >= 0) {
					int size = varInfos[varIdx].size;
					printf("shouldadd %d\n", varIdx);
					int address = TraceStackHandler_guarantee(
						&trace->stackHandler,
						size,
						size,
						store
					);

					if (code == 0) {
						TraceStackHandler_remove(
							&trace->stackHandler,
							store,
							varInfos[varIdx].size
						);
					}

					store = address;
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
					);
				}
				uses[usageCompletion].store = store;
				uses[usageCompletion].offset = line;
				usageCompletion++;
			}

			if (store != -TRACE_REG_NONE && store <= -TRACE_REG_RBX) {
				int r = -store;
				int flag = 1 << r;
				if ((slowRegisters & flag) == 0) {
					printf("ID %d\n", r);
					fprintf(output, "\tpush %s\n", REGISTER_NAMES[3][r]);
					slowRegisters |= flag;
				}
			}
			
			
			continue;
		}



		usageCompletion = 0;



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
				generateAssembly_savePlacements(trace, varInfos,
					&placementSaves, &trace->stackHandler, varlength);
				break;

			case 8: // open placements (take)
				generateAssembly_openPlacements(trace, true, varInfos,
					regs, &placementSaves, &trace->stackHandler, output, varlength);
				break;

			case 9: // open placements (seek)
				generateAssembly_openPlacements(trace, false, varInfos,
					regs, &placementSaves, &trace->stackHandler, output, varlength);
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

			line = *linePtr;
			linePtr++;
			ip++;

			trline_t next = line & 0x3FF;
			int nextUse = next == 0 ? -1 : next;

			printf("create %d\n", variable);
			varInfos[variable].size = size;
			varInfos[variable].nextUse = nextUse;
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
			fprintf(output, "\tmov ");
			Trace_printUsage(trace, output, type, uses[0]);
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
				unsigned long long l = *(linePtr++);
				unsigned long long r = *(linePtr++);
				unsigned long long value = (l << 32) | r; 
				ip += 2;
				
				fprintf(output, "%u \n", *linePtr);
			}

			break;
		}

		case TRACECODE_MOVE:
		{
			int size = (line >> 16);
			fprintf(output, "\tmov ");
			Trace_printUsage(trace, output, Trace_packSize(size), uses[1]);
			fprintf(output, ", ");
			Trace_printUsage(trace, output, Trace_packSize(size), uses[0]);
			fprintf(output, "\n");

			break;
		}

		case TRACECODE_PLACE:
		{
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
				Trace_printUsage(trace, output, psize, uses[0]);
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
				Trace_printUsage(trace, output, psize, uses[0]);
				fprintf(output, "; place(reg)\n");
			}

			break;
		}

		case TRACECODE_ARITHMETIC:
		{
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
			Trace_printUsage(trace, output, psize, uses[2]);
			fprintf(output, ", ");
			Trace_printUsage(trace, output, psize, uses[0]);
			fprintf(output, "; prepare arithmetic operation\n");

			// Perform real operation
			fprintf(output, "\t%s ", ARITHMETIC_NAMES[operation]);
			Trace_printUsage(trace, output, psize, uses[2]);
			fprintf(output, ", ");
			Trace_printUsage(trace, output, psize, uses[1]);
			fprintf(output, "\n");
			break;
		}

		case TRACECODE_ARITHMETIC_IMM:
		{
			int operation = (line >> 10) & 0x7;
			int psize = (line >> 13) & 0x3;

			// Move value
			fprintf(output, "\tmov ");
			Trace_printUsage(trace, output, psize, uses[1]);
			fprintf(output, ", ");
			Trace_printUsage(trace, output, psize, uses[0]);
			fprintf(output, "; prepare arithmetic operation\n");

			// Perform real operation
			fprintf(output, "\t%s ", ARITHMETIC_NAMES[operation]);
			Trace_printUsage(trace, output, psize, uses[1]);
			
			if (operation >= TRACEOP_INC) {
				fprintf(output, "\n");
				break;
			}

			if (operation >= TRACEOP_MULTIPLICATION) {
				raiseError("[TODO] mult");
				break;
			}

			/// TODO: check sign
			switch (psize) {
			case 0:
			{
				fprintf(output, ", %d\n", (line>>16) & 0xff);
				break;
			}

			case 1:
			{
				fprintf(output, ", %d\n", line>>16);
				break;
			}

			case 2:
			{
				line = *linePtr;
				linePtr++;
				ip++;
				fprintf(output, ", %d\n", line);
				break;
			}

			case 3:
			{
				uint lo = *linePtr;
				linePtr++;
				uint hi = *linePtr;
				linePtr++;
				ip += 2;

				uint64_t value = (uint64_t)lo | (((uint64_t)hi) << 32);
				fprintf(output, ", %ld\n", value);


			}
			}

			break;	
		}

		case TRACECODE_LOGIC:
		{
			int operation = (line >> 10) & 0xf;
			int psize = (line >> 14) & 0x3;

			switch (operation) {
			case TRACEOP_BITWISE_AND:
			case TRACEOP_BITWISE_OR:
			case TRACEOP_BITWISE_XOR:
			{
				fprintf(output, "\tmov ");
				Trace_printUsage(trace, output, psize, uses[2]);
				fprintf(output, ", ");
				Trace_printUsage(trace, output, psize, uses[0]);
				fprintf(output, "; prepare logic operation\n");

				// Perform real operation
				fprintf(output, "\t%s ", LOGIC_NAMES[operation]);
				Trace_printUsage(trace, output, psize, uses[2]);
				fprintf(output, ", ");
				Trace_printUsage(trace, output, psize, uses[1]);
				fprintf(output, "\n");
				break;
			}

			case TRACEOP_LEFT_SHIFT:
			{
				raiseError("[TODO] shl");
				break;
			}
			
			case TRACEOP_RIGHT_SHIFT:
			{
				raiseError("[TODO] shr");
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
			break;	
		};

		case TRACECODE_LOGIC_IMM_LEFT:
		case TRACECODE_LOGIC_IMM_RIGHT:
		{
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
					printf("sis %d\n", i);
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
			fprintf(output, "\tcall fn_%p\n", fn);
			
			// Restore#data
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
			fprintf(output, "\tjz ");
			/// TODO: mark real size
			Trace_printUsage(trace, output, 2, uses[0]);
			fprintf(output, ", .l%04d\n", line>>10);


			// printf("[TODO] asm IF\n");
			break;
		}
		
		case TRACECODE_JMP:
		{
			fprintf(output, "\tjmp .l%04d\n", line>>10);
			break;
		}
		
		case TRACECODE_CAST:
		{
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
				Trace_printUsage(trace, output, dstPSize, uses[1]);
				fprintf(output, ", ");
				Trace_printUsage(trace, output, srcPSize, uses[0]);
				fprintf(output, "\n");
			} else {
				
			}
			

			break;
		}




		}


	}

	finishWhile:
	if (placementsObj)
		free(placementsObj);

	TraceStackHandler_free(&trace->stackHandler);
	Stack_free(placementSaves);

	for (int i = TRACE_REG_R15; i >= TRACE_REG_RBX; i--) {
		printf("k-%d\n", i);
		if (slowRegisters & (1<<i))
			fprintf(output, "\tpop %s\n", REGISTER_NAMES[3][i]);
	}


	fprintf(output, "\tret\n\n");
}






