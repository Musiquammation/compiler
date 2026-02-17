#include "Trace.h"

#include "chooseSign.h"

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
#if 0
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


	trace->varCount = 0;
	Stack_create(uint, &trace->varPlacements);
	TraceFunctionMap_create(&trace->calledFunctions);
}


void Trace_delete(Trace* trace, bool hasGeneratedAssembly) {
	Stack_free(trace->varPlacements);

	if (hasGeneratedAssembly) {
		Array_free(trace->replaces);
		free(trace->varInfos);
		free(trace->regs);
	}

	TraceFunctionMap_free(&trace->calledFunctions);
}

uint Trace_pushVariable(Trace* trace) {
	uint pos;

	if (Stack_isEmpty(trace->varPlacements)) {
		pos = trace->varCount;
		trace->varCount++; // add a variable
	} else {
		pos = *Stack_pop(uint, &trace->varPlacements);
	}
	
	return pos;
}

void Trace_popVariable(Trace* trace, uint index) {
	*Stack_push(uint, &trace->varPlacements) = index;

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
		
		// '1<<1' for argument
		v->id = Trace_ins_create(
			trace, v,
			Prototype_getSizes(v->proto).size, 1<<1,
			Prototype_getPrimitiveSizeCode(v->proto)
		);
	}
}

void Trace_popArgs(Trace* trace, Variable** args, int arglen) {
	for (int i = 0; i < arglen; i++) {
		Variable* v = args[i];
		Trace_popVariable(trace, v->id);
		v->id = -1;
	}
}

void Trace_pushMembersTypeConstructorCalls(Trace* trace, Class* thisclass) {
	/// TODO: check argsStartIndex 
	Variable* currentArg;
	Expression varExpr = {.type = EXPRESSION_PROPERTY, .data = {.property = {
		.varr = &currentArg,
		.origin = NULL,
		.varr_len = 1,
		.freeVariableArr = false
	}}};


	Expression* varExprList[] = {&varExpr};
	Expression expr = {.type = EXPRESSION_FNCALL};
	expr.data.fncall.args = varExprList;
	
	typedef Variable* var_t;
	typedef Function* fn_t;
	

	
	Array_loop(var_t, thisclass->variables, vptr) {
		Variable* v = *vptr;
		Class* cl = Prototype_getClass(v->proto);
		

		// Search and call constructor
		Array_loop(fn_t, cl->constructors, fptr) {
			Function* fn = *fptr;
			if (fn->name == NULL && fn->flags & FUNCTIONFLAGS_ARGUMENT_CONSTRUCTOR) {
				// constructor found
				currentArg = v;
				expr.data.fncall.fn = fn;

				Trace_set(trace, &expr, TRACE_VARIABLE_NONE, -1, 0, EXPRESSION_FNCALL);
				break;
			}
		}
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
					int flags     = (n >> 10) & 0x3f;
					int variable  = (n >> 16) & 0xFFF;
					int size      = n2 >> 16;
					int regable   = n & (1<<28);
					int next2     = n2& 0x3FF;

					printf(
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
						uint32_t n2 = pack->line[i+1];
						uint32_t n3 = pack->line[i+2];
						printf("[%04d] DEF value_lo=%u value_hi=%u\n", i+position, n2, n3);
						i += 2;
					}

					break;
				}

				case TRACECODE_MOVE:
				{
					trline_t loadSrc = (n >> 11) & 1;
					trline_t loadDst = (n >> 12) & 1;
					printf("[%04d] MOVE size=%d", i+position, n >> 16);

					if (loadSrc) {
						printf(" loadSrc");
					}

					if (loadDst) {
						printf(" loadDst");
					}

					if (n & (1<<10)) {
						printf("\n"); // registrable
					} else {
						printf(" complex\n");
					}

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

				case TRACECODE_STACK_PTR:
				{
					uint32_t variable = (n >> 16) & 0xfff;
					uint32_t offset = pack->line[i+1];
					
					if (offset == -1) {
						printf("[%04d] STACK_PTR of=v%d offset=no\n", i, variable);
					} else {
						printf("[%04d] STACK_PTR of=v%d offset=+%02d\n", i, variable, offset);
					}
					i++;
					break;
				}

				case TRACECODE_MEMORY:
					printf("[%04d] MEMORY\n", i);
					break;

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



static void addUsageAt(Trace* trace, uint variable, int traceInstruction, bool readMode, trline_t* ptr) {
	if (variable == TRACE_VARIABLE_NONE) {
		return;
	}

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
				Variable** varr = next->data.property.varr;
				int length = next->data.property.varr_len;
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



static const int* placeFnArg(
	Trace* trace,
	Prototype* argProto,
	Expression* expr,
	uint* variable,
	const int* currentRegister
) {
	char primitiveSizeCode = Prototype_getPrimitiveSizeCode(argProto);
	int subSize = Prototype_getSizes(argProto).size;
	uint bufferVar = Trace_ins_create(trace, NULL, subSize, 0, primitiveSizeCode);

	Trace_set(
		trace,
		expr,
		bufferVar,	
		primitiveSizeCode ? TRACE_OFFSET_NONE : 0,
		Prototype_getSignedSize(argProto),
		expr->type
	);

	uint finalVar = Trace_ins_create(trace, NULL, subSize, 0, primitiveSizeCode);
	*variable = finalVar;

	if (currentRegister < &ARGUMENT_REGISTERS[ARGUMENT_REGISTERS_LENGTH]) {
		Trace_ins_placeReg(
			trace,
			bufferVar,
			finalVar,
			*currentRegister,
			Trace_packSize(subSize)
		);

		currentRegister++;
	} else {
		raiseError("[TODO] handle a lot of arguements");
		return NULL;
	}

	Trace_popVariable(trace, bufferVar);
	return currentRegister;
}

void Trace_set(Trace* trace, Expression* expr, uint destVar, int destOffset, int signedSize, int exprType) {
	const int destSize = signedSize >= 0 ? signedSize : -signedSize;

	retry:
	switch (exprType) {
	case EXPRESSION_PROPERTY:
	{
		Expression* origin = expr->data.property.origin;
		if (origin) {
			handleOrigin(trace, origin, expr, EXPRESSION_PROPERTY, destVar, destOffset	);
			break;
		}


		int length = expr->data.property.varr_len;
		Variable** varr = expr->data.property.varr;
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
				destSize,
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
			Trace_packSize(destSize) << 18;


		Trace_popVariable(trace, tempVar);


		return;

	}

	case EXPRESSION_FNCALL:
	{
		Expression** args = expr->data.fncall.args;

		Function* fn = expr->data.fncall.fn;
		Variable** arguments = fn->arguments;
		Variable** settings = fn->settings;

		int argsLength = fn->args_len;
		int settingLength = fn->settings_len;

		int argsStartIndex = fn->projections_len + settingLength;
		int settingsStartIndex = settingLength;

		uint variables[argsLength + settingLength];
		const int* currentRegister = ARGUMENT_REGISTERS;


		// Place arguments
		for (int i = 0; i < argsLength; i++) {
			currentRegister = placeFnArg(
				trace,
				arguments[i]->proto,
				args[i+argsStartIndex],
				&variables[i],
				currentRegister
			);
		}

		// Place settings
		for (int i = 0; i < settingLength; i++) {
			currentRegister = placeFnArg(
				trace,
				settings[i]->proto,
				args[i+settingsStartIndex],
				&variables[argsLength+i],
				currentRegister
			);
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

		
		// Add argument usages
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

		// Add setting usages
		for (int i = 0; i < settingLength; i++) {
			char psc = Prototype_getPrimitiveSizeCode(settings[i]->proto);
			if (psc == PSC_UNKNOWN) {
				raiseError("[Architecture] Registrable status is unknown");
			}

			Trace_addUsage(
				trace, variables[argsLength + i],
				psc ? TRACE_OFFSET_NONE : 0,
				true
			);
		}

		// Function call
		int fnIndex = Trace_reachFunction(trace, fn);
		*Trace_push(trace, 1) = TRACECODE_FNCALL | (fnIndex << 10);


		// Remove variables
		for (int i = argsLength + settingLength - 1; i >= 0; i--) {
			Trace_popVariable(trace, variables[i]);
		}





		// Output
		if (destVar != TRACE_VARIABLE_NONE) {
			int signedOutputSize = Prototype_getSignedSize(fn->returnPrototype);
			if (signedOutputSize == signedSize) {
				Trace_ins_placeVar(trace, destVar, TRACE_REG_RAX, Trace_packSize(destSize));

			} else {
				int outputSize = signedOutputSize >= 0 ? signedOutputSize : -signedOutputSize;
				uint temp = Trace_ins_create(
					trace, NULL, outputSize, 0,
					Prototype_getPrimitiveSizeCode(fn->returnPrototype)
				);

				// Put output in temp variable
				/// TODO: check TRACE_VARIABLE_NONE
				Trace_ins_placeVar(trace, temp, TRACE_REG_RAX, Trace_packSize(destSize));

				Trace_addUsage(trace, temp, TRACE_OFFSET_NONE, true);
				Trace_addUsage(trace, destVar, destOffset, true);
				*Trace_push(trace, 1) = TRACECODE_CAST |
					(signedOutputSize < 0 ? (1<<10) : 0) |
					(0<<11) |
					(signedSize < 0 ? (1<<12) : 0) |
					(0<<13) |
					Trace_packSize(outputSize) << 16 |
					Trace_packSize(destSize) << 18;


				Trace_popVariable(trace, temp);
			}
		}

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
					(Trace_packSize(destSize) << 18);


				Trace_popVariable(trace, tempDestVar);
			}

			Trace_popVariable(trace, rightVar);
			Trace_popVariable(trace, leftVar);

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
			bool notCast = operandSize <= destSize;

			/// TODO: handle case for same size but different sign
			if (notCast) {
				// Load operand
				variable = Trace_ins_create(trace, NULL, destSize, 0, 1);
				signedDestImmediateSize = signedSize;
				Trace_set(trace, operand, variable, TRACE_OFFSET_NONE, signedSize, operandType);

				// Add usages
				Trace_addUsage(trace, variable, TRACE_OFFSET_NONE, true);
				Trace_addUsage(trace, destVar, destOffset, false);

			} else {
				// Load operand
				variable = Trace_ins_create(trace, NULL, destSize, 0, 1);
				signedDestImmediateSize = signedOperandSize;
				Trace_set(trace, operand, variable, TRACE_OFFSET_NONE, signedOperandSize, operandType);

				resultVariable = Trace_ins_create(trace, NULL, destSize, 0, 1);

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

			
			Trace_popVariable(trace, variable);

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
					Trace_packSize(destSize) << 18;
				
				Trace_popVariable(trace, resultVariable);
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
		Expression* reference = Expression_cross(expr->data.operand.op);
		switch (reference->type) {
		case EXPRESSION_PROPERTY:
		{
			if (reference->type != EXPRESSION_PROPERTY) {
				raiseError("[Syntax] Can only get the address of a variable");
				return;
			}
			
			if (reference->data.property.origin) {
				raiseError("[TODO] Handle origin in addrOf (this)");
			}
			
			int refArrLength = reference->data.property.varr_len;
			Variable** refVarArr = reference->data.property.varr;
			int srcOffset = Prototype_getVariableOffset(refVarArr, refArrLength);
			int srcVar = refVarArr[0]->id;
			
			Trace_ins_getStackPtr(trace, destVar, srcVar, destOffset, srcOffset);
			return;
		}

		case EXPRESSION_FAST_ACCESS:
		{
			Function* accessor = reference->data.fastAccess.accessor;
			if (accessor->stdBehavior < 0) {
				raiseError("[TODO] handle non standard behaviors in addrOf(fastAccess)");
				return;
			}

			switch (accessor->stdBehavior) {
			case 0: // pointer
			{
				Expression* o = reference->data.fastAccess.origin;
				Trace_set(trace, o, destVar, destOffset, 8, o->type);
				return;
			}

			default:
				raiseError("[BadId]: Invalid id for standard fast access");
			}

			return;
		}

		default:
			raiseError("[TODO] addrOf in trace");
		
		}
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


trline_t* Trace_ins_if(Trace* trace, uint destVar, int varSize) {
	*Trace_push(trace, 1) = TRACECODE_STAR | (13<<10) | (varSize<<16);
	Trace_addUsage(trace, destVar, TRACE_OFFSET_NONE, true);
	trline_t* line = Trace_push(trace, 1);
	*line = TRACECODE_IF;
	return line;
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


































void Trace_generateTranspiled(Trace* trace, FunctionAssembly* fnAsm, bool useThis) {
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

	int variableCount = trace->varCount;
	VarData variables[variableCount];
	char varFlags[variableCount];
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
	if (currentFunction->returnPrototype) {
		fprintf(output, "// %s()\n%s fn_%016lx(",
			currentFunction->name,
			currentFunction->returnPrototype->direct.cl->c_name,
			currentFunction->traceId
		);
	} else {
		fprintf(output, "// %s()\nvoid fn_%016lx(",
			currentFunction->name,
			currentFunction->traceId
		);
	}

	
	typedef Variable* vptr_t;

	// Write args
	int fnArgLen = currentFunction->args_len;
	Variable** fnArgs = currentFunction->arguments;
	for (int i = 0; i < fnArgLen; i++) {
		Variable* v = fnArgs[i];
		if (useThis && i==0) {
			fprintf(output, "/* this */ ");
		}

		fprintf(output, "%s v%03d_%02d",
			Prototype_getClass(v->proto)->c_name, v->id, 1);
		
		if (i < fnArgLen-1)
			fprintf(output, ", ");
	}

	// Write settings
	int fnSettingLen = currentFunction->settings_len;
	Variable** fnSettings = currentFunction->settings;
	for (int i = 0; i < fnSettingLen; i++) {
		if (i == 0 && fnArgLen > 0)
			fprintf(output, ", ");
		
		Variable* v = fnSettings[i];
		fprintf(output, "%s v%03d_%02d",
			Prototype_getClass(v->proto)->c_name, v->id, 1);
		
	}



	// Finish signature
	fprintf(output, ") {\n");

	// Create final variable
	bool finalDefined;
	if (currentFunction->returnPrototype) {
		finalDefined = true;
		int size = Prototype_getSignedSize(currentFunction->returnPrototype);
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
	#define couple(u) u.variable, variables[u.variable].index

	#define writePrimitive(u, size)\
		if (u.offset < 0) {\
			fprintf(output, "v%03d_%02d", couple(u));\
		} else {\
			switch (size) {\
			case 1:\
				fprintf(output, "*(tuint8_t *)(&v%03d_%02d[%d])",\
					couple(u), u.offset);\
				break;\
			case 2:\
				fprintf(output, "*(tuint16_t*)(&v%03d_%02d[%d])",\
					couple(u), u.offset);\
				break;\
			case 4:\
				fprintf(output, "*(tuint32_t*)(&v%03d_%02d[%d])",\
					couple(u), u.offset);\
				break;\
			case 8:\
				fprintf(output, "*(tuint64_t*)(&v%03d_%02d[%d])",\
					couple(u), u.offset);\
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
					fprintf(output, "v%03d_%02d", couple(dst));
				} else {
					switch (realSize) {
					case 1: 
						fprintf(output, "*((uint8_t *)(v%03d_%02d + %d))", couple(dst), dst.offset);	
						size = -1;
						break;
	
					case 2:
						fprintf(output, "*((uint16_t*)(v%03d_%02d + %d))", couple(dst), dst.offset);	
						size = -1;
						break;
	
					case 4:
						fprintf(output, "*((uint32_t*)(v%03d_%02d + %d))", couple(dst), dst.offset);	
						size = -1;
						break;
	
					case 8:
						fprintf(output, "*((uint64_t*)(v%03d_%02d + %d))", couple(dst), dst.offset);	
						size = -1;
						break;
	
	
					default:
						fprintf(output, "memcpy(v%03d_%02d + %d, ",
							couple(dst), dst.offset);
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
						fprintf(output, "v%03d_%02d", couple(src));

					} else {
						fprintf(output, "(*(void**)(v%03d_%02d + %d))",
							couple(src), src.offset);
					}
				
				} else if (src.offset < 0) {
					fprintf(output, ", v%03d_%02d, , %d)", couple(src), realSize);
				} else {
					fprintf(output, ", (*(void**)(v%03d_%02d + %d)), %d)",
						couple(src), src.offset, realSize);
				}

				goto finishMove;
			}

			if (loadDst) {
				if (dst.offset < 0) {
					size = line >> 16;
					realSize = size;
					fprintf(output, "memcpy((void*)v%03d_%02d, ",
						couple(dst));
			
				} else {
					size = line >> 16;
					realSize = size;

					switch (size) {
					case 1: 
						fprintf(output, "**((uint8_t **)(v%03d_%02d + %d))", couple(dst), dst.offset);	
						size = -1;
						break;

					case 2:
						fprintf(output, "**((uint16_t**)(v%03d_%02d + %d))", couple(dst), dst.offset);	
						size = -1;
						break;

					case 4:
						fprintf(output, "**((uint32_t**)(v%03d_%02d + %d))", couple(dst), dst.offset);	
						size = -1;
						break;

					case 8:
						fprintf(output, "**((uint64_t**)(v%03d_%02d + %d))", couple(dst), dst.offset);	
						size = -1;
						break;


					default:
						fprintf(output, "memcpy(v%03d_%02d + %d, ", couple(dst), dst.offset);
						break;

					}
				}
				
			} else if (dst.offset < 0) {
				fprintf(output, "v%03d_%02d", couple(dst));
				size = -1;
				realSize = line >> 16;

			} else {
				size = line >> 16;
				realSize = size;
				switch (size) {
				case 1: 
					fprintf(output, "*((uint8_t *)(v%03d_%02d + %d))", couple(dst), dst.offset);
					size = -1;
					break;

				case 2:
					fprintf(output, "*((uint16_t*)(v%03d_%02d + %d))", couple(dst), dst.offset);
					size = -1;
					break;

				case 4:
					fprintf(output, "*((uint32_t*)(v%03d_%02d + %d))", couple(dst), dst.offset);
					size = -1;
					break;

				case 8:
					fprintf(output, "*((uint64_t*)(v%03d_%02d + %d))", couple(dst), dst.offset);
					size = -1;
					break;


				default:
					fprintf(output, "memcpy(v%03d_%02d + %d, ", couple(dst), dst.offset);
					break;

				}
			}

			if (size < 0) {
				fprintf(output, " = ");
			}

			if (src.offset < 0) {
				if (size < 0) {
					fprintf(output, "v%03d_%02d", couple(src));
				} else {
					fprintf(output, "&v%03d_%02d, %d)",
						couple(src), size);
				}
			
			} else if (size < 0) {
				switch (realSize) {
				case 1: 
					fprintf(output, "*((uint8_t *)(v%03d_%02d + %d))", couple(src), src.offset);	
					break;

				case 2:
					fprintf(output, "*((uint16_t*)(v%03d_%02d + %d))", couple(src), src.offset);	
					break;

				case 4:
					fprintf(output, "*((uint32_t*)(v%03d_%02d + %d))", couple(src), src.offset);	
					break;

				case 8:
					fprintf(output, "*((uint64_t*)(v%03d_%02d + %d))", couple(src), src.offset);	
					break;

				default:
					raiseError("[Intern] size error in transpilation");
					return;
				}

			} else {
				fprintf(output, "v%03d_%02d + %d, %d)", couple(src), src.offset, size);

			}


			finishMove:
			fprintf(output, ";\n");


			break;
		}

		case TRACECODE_PLACE:
		{
			// Edit variable (can be ignored by transpiler)
			trline_t reg = (line >> 16) & 0xff;
			if (line & (1<<12)) {
				if (reg != TRACE_REG_RAX)
					raiseError("[TODO] transpile TRACECODE_PLACE edit var");
					
				break;
			}

			if (reg == TRACE_REG_RAX) {
				Usage u = usedVariables[0];
				fprintf(output, "\tfinal = v%03d_%02d;\n", couple(u));
			} else {
				Usage src = usedVariables[0];
				Usage dst = usedVariables[1];

				fprintf(output, "\tv%03d_%02d = ", couple(dst));

				if (src.offset < 0) {
					fprintf(output, "v%03d_%02d;\n", couple(src));
				} else {
					trline_t size = (line >> 10) & 0x3;

					switch (size) {
					case 1: 
						fprintf(output, " *((uint8_t *)(v%03d_%02d + %d));\n", couple(src), src.offset);	
						break;

					case 2:
						fprintf(output, "*((uint16_t*)(v%03d_%02d + %d));\n", couple(src), src.offset);	
						break;

					case 4:
						fprintf(output, "*((uint32_t*)(v%03d_%02d + %d));\n", couple(src), src.offset);	
						break;

					case 8:
						fprintf(output, "*((uint64_t*)(v%03d_%02d + %d));\n", couple(src), src.offset);	
						break;

					default:
						raiseError("[Intern] size error in transpilation");
						return;
					}
				}

			}
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
				fprintf(output, "\tv%03d_%02d = ", couple(dst));
			} else {
				fprintf(output, "\t*(uint64_t*)(v%03d_%02d + %d) = ",
					couple(dst), dst.offset);
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


		case TRACECODE_MEMORY:
		{
			if (line & (1<<10)) {
				trline_t action = (line >> 11) & 0x3;
				if (action == 0) {
					raiseError("[TODO] TRACECODE_MEMORY");
					break;
				}

				if (action == 1) {
					raiseError("[TODO] TRACECODE_MEMORY");
					break;
				}

				// debug print
				if (action == 2) {
					// skip
					break;
				}
				break;
			}
			
			// fixed malloc
			trline_t size = line >> 11;
			raiseError("[TODO] TRACECODE_MEMORY fixed malloc");
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

	#undef printImmediate
	#undef writePrimitive
	#undef move
	#undef couple
}

