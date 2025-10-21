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



static size_t hash_ptr(void* ptr) {
    uintptr_t val = (uintptr_t)ptr;
    return (val >> 3) % TRACE_FUNCTIONMAP_LENGTH;
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











typedef struct {
	trline_t* lastUsePtr;
	int lastUseInstruction;
} VariableTrace;













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
	Stack_create(&trace->varPlacements, sizeof(uint));
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
		vt->lastUseInstruction = -1;
		return pos;
	}
	
	pos = *Stack_pop(uint, &trace->varPlacements);
	vt = Array_get(VariableTrace, trace->variables, pos);
	
	return pos;
}

void Trace_removeVariable(Trace* trace, uint index) {
	*Stack_push(uint, &trace->varPlacements) = index;

	/// TODO: move TRACE_USAGE_LAST if variable is referenced (in stack) => call Trace_addUsage

}



int Trace_reachFunction(Trace* trace, Function* fn) {
	return TraceFunctionMap_reach(&trace->calledFunctions, fn);
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
	for (int i = 0; true; i++) {
		trline_t n = pack->line[i];
		int next = n & 0x3FF; // always extract next (10 bits)

		// Instruction mode ?
		if (next > TRACE_USAGE_OUT_OF_BOUNDS) {
			switch (next) {
				case TRACECODE_STAR:
					printf("[%04d] END action=%d\n", i+position, (n>>10)&0x3);
					if ((n>>10) == 0)
						return;
					break;

				case TRACECODE_CREATE: {
					uint32_t n2 = pack->line[++i];
					int variable = (n >> 16) & 0xFFF;
					int size     = n2 >> 16;
					int next2    = n2& 0x3FF;
					printf("[%04d] CREATE v%d size=%d next=+%d\n", i-1+position, variable, size, next2);
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
			int variable = n >> 10;

			if (next == TRACE_USAGE_LAST) {
				printf("[%04d] !f v%d\n", i+position, variable);
			} else {
				printf("[%04d] !u v%d next=+%d\n", i+position, variable, next);
			}
		}
	}	
}


void Trace_addUsageAt(Trace* trace, uint variable, bool readMode, trline_t* ptr) {
	VariableTrace* vt = Array_get(VariableTrace, trace->variables, variable);
	int traceInstruction = trace->instruction;
	if (readMode) {
		int diff = traceInstruction - vt->lastUseInstruction;
		if (diff >= TRACE_USAGE_OUT_OF_BOUNDS) {
			diff = TRACE_USAGE_OUT_OF_BOUNDS;
		}
		*vt->lastUsePtr |= diff;
	}

	vt->lastUseInstruction = traceInstruction;

	*ptr = TRACE_USAGE_LAST | (variable << 10);
	vt->lastUsePtr = ptr;
}

void Trace_addUsage(Trace* trace, uint variable, bool readMode) {
	trline_t* ptr = Trace_push(trace, 1);
	Trace_addUsageAt(trace, variable, readMode, ptr);
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




void Trace_set(Trace* trace, Expression* expr, uint destVar, int signedSize, int exprType) {
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
			signedObjSize = varr[subLength]->proto.cl->size;
		}

		// Size matches
		if (signedObjSize == signedSize) {
			Trace_ins_move(
				trace,
				destVar,
				varr[0]->id,
				size
			);
			return;
		}

		// Let's cast
		int objSize = signedObjSize >= 0 ? signedObjSize : -signedObjSize;
		uint tempVar = Trace_ins_create(trace, NULL, objSize, 0);
		Trace_ins_move(
			trace,
			tempVar,
			varr[0]->id,
			objSize
		);

		Trace_addUsage(trace, tempVar, true);
		Trace_addUsage(trace, destVar, false);

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
			uint v = Trace_ins_create(
				trace,
				NULL,
				arguments[i]->proto.cl->size,
				0
			);
			variables[i] = v;

			Trace_set(
				trace,
				fncall.args[i],
				v,
				Prototype_getSignedSize(&arguments[i]->proto),
				fncall.args[i]->type
			);
		}

		/// TODO: use rax for output
		int fnIndex = Trace_reachFunction(trace, fncall.fn);

		// Add usages
		for (int i = 0; i < fncall.argsLength; i++) {
			Trace_addUsage(trace, variables[i], true);
		}

		// Function call
		*Trace_push(trace, 1) = TRACECODE_FNCALL | (fnIndex << 10);

		// Remove variables
		for (int i = fncall.argsLength-1; i >= 0; i--) {
			Trace_removeVariable(trace, variables[i]);
		}

		// Output
		int signedOutputSize = Prototype_getSignedSize(&fncall.fn->returnType);
		if (signedOutputSize == signedSize) {
			Trace_ins_place(trace, destVar, TRACE_REG_RAX, Trace_packSize(size), 1);				

		} else {
			int outputSize = signedOutputSize >= 0 ? signedOutputSize : -signedOutputSize;
			uint temp = Trace_ins_create(trace, NULL, outputSize, 0);

			// Put output in temp variable
			Trace_ins_place(trace, temp, TRACE_REG_RAX, Trace_packSize(size), 1);

			Trace_addUsage(trace, temp, true);
			Trace_addUsage(trace, destVar, true);
			*Trace_push(trace, 1) = TRACECODE_CAST |
				(signedOutputSize < 0 ? (1<<10) : 0) |
				(0<<11) |
				(signedSize < 0 ? (1<<12) : 0) |
				(0<<13) |
				Trace_packSize(outputSize) << 16 |
				Trace_packSize(size) << 18;


			Trace_removeVariable(trace, temp);
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
			Trace_set(trace, real, destVar, signedSize, EXPRESSION_PROPERTY);
			return;

		}

		case EXPRESSION_FNCALL:
		{
			Trace_set(trace, real, destVar, signedSize, EXPRESSION_FNCALL);
			return;
		}

		}

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
			printf("sign %d %d\n", signedLeftSize, leftType);

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
			
			uint leftVar = Trace_ins_create(trace, NULL, maxSize, 0);
			Trace_set(trace, leftExpr, leftVar, signedMaxSize, leftType);

			uint rightVar = Trace_ins_create(trace, NULL, maxSize, 0);
			Trace_set(trace, rightExpr, rightVar, signedMaxSize, rightType);

			int tempDestVar;
			
			// Add usages
			if (signedMaxSize == signedSize) {
				Trace_addUsage(trace, leftVar, true);
				Trace_addUsage(trace, rightVar, true);
				Trace_addUsage(trace, destVar, false);
			} else {
				tempDestVar = Trace_ins_create(trace, NULL, maxSize, 0);
				Trace_addUsage(trace, leftVar, true);
				Trace_addUsage(trace, rightVar, true);
				Trace_addUsage(trace, tempDestVar, false);
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
				Trace_addUsage(trace, tempDestVar, true);
				Trace_addUsage(trace, destVar, false);
				

				/// TODO: handle floating numbers
				*Trace_push(trace, 1) = TRACECODE_CAST |
					(signedMaxSize< 0 ? (1<<10) : 0) |
					(0<<11) |
					(signedSize < 0 ? (1<<12) : 0) |
					(0<<13) |
					packedMaxSize << 16 |
					Trace_packSize(size) << 18;


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
				variable = Trace_ins_create(trace, NULL, size, 0);
				signedDestImmediateSize = signedSize;
				Trace_set(trace, operand, variable, signedSize, operandType);

				// Add usages
				Trace_addUsage(trace, variable, true);
				Trace_addUsage(trace, destVar, false);

			} else {
				// Load operand
				variable = Trace_ins_create(trace, NULL, size, 0);
				signedDestImmediateSize = signedOperandSize;
				Trace_set(trace, operand, variable, signedOperandSize, operandType);

				resultVariable = Trace_ins_create(trace, NULL, size, 0);

				// Add usages
				Trace_addUsage(trace, variable, true);
				Trace_addUsage(trace, resultVariable, false);
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
				Trace_addUsage(trace, resultVariable, true);
				Trace_addUsage(trace, destVar, false);

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
			trace, destVar, signedSize,
			castable_cast(signedObjSize, signedSize, expr->data.num)
		);

		return;		
	}

	




	default:
		raiseError("[Trace] Expression type not handled");
		return;
	}
}


uint Trace_ins_create(Trace* trace, Variable* variable, int size, int flags) {
	if (size >= 0xffff) {
		raiseError("[TODO] handle create big size variables");
		return -1;
	}

	uint id = Trace_pushVariable(trace);
	trline_t* ptr = Trace_push(trace, 2);
	ptr[0] = TRACECODE_CREATE | (flags << 10) | (id << 16);
	ptr[1] = TRACE_USAGE_LAST | (size << 16);
	
	VariableTrace* vt = Array_get(VariableTrace, trace->variables, id);
	vt->lastUsePtr = &ptr[1];
	vt->lastUseInstruction = trace->instruction - 1;


	return id;
}


void Trace_ins_def(Trace* trace, int variable, int signedSize, castable_t value) {
	Trace_addUsage(trace, variable, false);

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


void Trace_ins_move(Trace* trace, int destVar, int srcVar, int size) {
	Trace_addUsage(trace, srcVar, true);
	Trace_addUsage(trace, destVar, false);

	*Trace_push(trace, 1) = TRACECODE_MOVE | (size << 16);
}

trline_t* Trace_ins_if(Trace* trace, uint destVar) {
	Trace_addUsage(trace, destVar, true);
	trline_t* l = Trace_push(trace, 1);
	*l = TRACECODE_IF;
	return l;
}

void Trace_ins_jmp(Trace* trace, uint instruction) {
	*Trace_push(trace, 1) = TRACECODE_JMP | (instruction << 10);
}

void Trace_ins_place(Trace* trace, int variable, int reg, int packedSize, int setVariable) {
	Trace_addUsage(trace, variable, true);
	
	*Trace_push(trace, 1) = TRACECODE_PLACE |
		(packedSize << 10) |
		(setVariable << 12) |
		(reg << 16);
}

































enum {
	NO_VICTIM = 0xffffffff,
	STACK_BLOCKS = 64
};

typedef struct {
	int instruction;
	uint variable;
	int victim;
	int destRegister;
	int source; // stack region to free
	int victimStackId; // for the victim
} Replace;









int Trace_reg_chooseFast(const TraceRegister regs[]) {
	int biggerNextUse = 0;
	int index = TRACE_REG_NONE;
	for (int i = TRACE_REG_RAX; i <= TRACE_REG_R11; i++) {
		int nu = regs[i].nextUse;
		if (nu < 0) {
			return -i;
		}

		if (nu > biggerNextUse) {
			biggerNextUse = nu;
			index = i;
		}
	}

	return index;
}


int Trace_reg_place(Trace* trace, TraceRegister regs[], int ip, int variable, int source, int nextUse, int reg) {
	Replace* replace = Array_push(Replace, &trace->replaces);
	
	if (reg >= 0) {
		uint victim = regs[reg].variable;
		replace->victim = victim;

		if (victim != NO_VICTIM) {
			int stackId = trace->stackId;
			replace->victimStackId = stackId;
			trace->varInfos[victim].store = stackId;
			trace->stackId++;
		}

		
	} else {
		// No victim
		replace->victim = NO_VICTIM;
		reg = -reg;
	}


	replace->instruction = ip;
	replace->variable = variable;
	replace->destRegister = reg;
	replace->source = source;
	
	trace->varInfos[variable].nextUse = nextUse;
	trace->varInfos[variable].store = -reg;
	

	regs[reg].nextUse = nextUse;
	regs[reg].variable = variable;
	return reg;
}




void Trace_placeRegisters(Trace* trace) {
	// Generate next data
	VarInfoTrace* varInfos = malloc(sizeof(VarInfoTrace) * trace->variables.length);
	TraceRegister* regs = malloc(TRACE_REG_NONE * sizeof(TraceRegister));
	Array_for(VarInfoTrace, varInfos, trace->variables.length, v) {
		v->size = -1;
		v->nextUse = -1;
		v->store = -TRACE_REG_NONE;
	}

	Array_free(trace->variables);
	Stack_free(trace->varPlacements);
	Array_create(&trace->replaces, sizeof(Replace));
	trace->varInfos = varInfos;
	trace->regs = regs;
	trace->stackId = 0;


	for (int i = 1; i < TRACE_REG_NONE; i++) {
		regs[i].nextUse = -1;
	}


	trline_t line = 0;
	int ip = 0;
	TracePack* pack = trace->first;
	trline_t* linePtr = pack->line;

	while (true) {
		trline_t previousLine = line;
		line = *linePtr;
		linePtr++;
		trline_t code = line & 0x3ff;

		// Usage
		if (code <= TRACE_USAGE_OUT_OF_BOUNDS) {
			trline_t varIdx = line >> 10;
			VarInfoTrace* vi = &varInfos[varIdx];
			int store = vi->store;


			if (code == 0) {
				// Last usage => destroy variable
				vi->nextUse = -1;

				if (store == -TRACE_REG_NONE)
					goto continueWhile;

				// Register
				if (store < 0) {
					regs[-store].nextUse = -1;
					regs[-store].variable = -1;
					vi->store = -TRACE_REG_NONE;
					goto continueWhile;
				}

				/// TODO: free stack


				goto continueWhile;
			}
			
			int nextUse = code == TRACE_USAGE_OUT_OF_BOUNDS ?
				0x7ffffffe : ip + code;

			vi->nextUse = nextUse;
			
			
			
			if (store >= 0 || store == -TRACE_REG_NONE) {
				int reg = Trace_reg_chooseFast(regs);
				vi->store = -Trace_reg_place(trace, regs, ip, varIdx, store, nextUse, reg);
			} else {
				regs[-store].nextUse = nextUse;
			}

			goto continueWhile;
		}


		

		// Simple instruction
		switch (code) {
		case TRACECODE_STAR:
		{
			int action = (line >> 10) & 0x3;
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
			
			case 2: // place signal
				break;

			}
			break;
		}

		case TRACECODE_CREATE:
		{
			trline_t size = line >> 16;
			line = *linePtr;
			linePtr++;
			ip++;

			trline_t next = line & 0x3FF;
			trline_t variable = (line >> 16) & 0xFFF;

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
				raiseError("[TODO]: PLACE > edit variable");
			} else {
				int reg = line >> 16;
				uint variable = previousLine >> 10;
				int source = varInfos[variable].store;
				int nextUse = 1;
				Trace_reg_place(trace, regs, ip, variable, source, nextUse, reg);
			}

			break;
		}

		case TRACECODE_ARITHMETIC:
		{
			break;
		}

		case TRACECODE_ARITHMETIC_IMM:
		{
			break;	
		}

		case TRACECODE_LOGIC:
		{
			break;	
		};

		case TRACECODE_LOGIC_IMM_LEFT:
		case TRACECODE_LOGIC_IMM_RIGHT:
		{
			break;	
		};



		case TRACECODE_FNCALL:
		{
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
	
}





static const char* REGISTER_NAMES[][TRACE_REG_NONE] = {
	{"Zl", "al", "cl", "r11b"},
	{"Zx", "ax", "cx", "r11w"},
	{"eZx", "eax", "ecx", "r11d"},
	{"rZx", "rax", "rcx", "r11"},
};

static const char* REGISTER_SIZENAMES[] = {
	"byte",
	"word",
	"dword",
	""
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

void Trace_printUsage(Trace* trace, FILE* output, int psize, int use) {
	if (use >= 0) {
		printf("use %d\n", use);
		int size = Trace_unpackSize(psize);
		int pos = TraceStackHandler_guarantee(&trace->stackHandler, size, size, use);
		fprintf(output, "%s[rsp-%d]", REGISTER_SIZENAMES[psize], pos);
	} else {
		if (use >= TRACE_REG_NONE) {use = 0;}
		fprintf(output, "%s", REGISTER_NAMES[psize][-use]);
	}	
} 

void Trace_generateAssembly(Trace* trace, FunctionAssembly* fnAsm) {
	int labelCounter = 0;
	FILE* output = fnAsm->output;

	// Init data
	VarInfoTrace* varInfos = trace->varInfos;
	TraceRegister* regs = trace->regs;
	TraceStackHandler stackHandler;
	TraceStackHandler_create(&trace->stackHandler, trace->stackId);


	fprintf(output, "fn_%p: ; %s\n", fnAsm->fn, fnAsm->fn->name);
	
	Replace* replace = trace->replaces.data;
	Replace* finalReplace = &replace[trace->replaces.length];
	TracePack* pack = trace->first;
	trline_t* linePtr = pack->line;

	int uses[4];
	int ip = 0;
	int usageCompletion = 0;


	while (true) {
		trline_t line = *linePtr;
		linePtr++;
		trline_t code = line & 0x3ff;

		// Apply replaces
		while (replace < finalReplace && replace->instruction == ip) {
			varInfos[replace->variable].store = -replace->destRegister;
			int victim = replace->victim;
			if (victim >= 0) {
				fprintf(output, "\tmov ");
				int psize = Trace_packSize(varInfos[victim].size);
				int victimStackId = replace->victimStackId;

				Trace_printUsage(
					trace, output, psize,
					victimStackId
				);
				fprintf(output, ", ");
				Trace_printUsage(
					trace, output, psize,
					varInfos[victim].store
				);
				fprintf(output, "; replacement\n");

				varInfos[victim].store = victimStackId;
			}

			int source = replace->source;
			
			/// TODO: free source region ?

			replace++;
		}

		ip++;



		// Usage
		if (code <= TRACE_USAGE_OUT_OF_BOUNDS) {
			trline_t varIdx = line >> 10;
			int store = varInfos[varIdx].store;
			if (store >= 0 && code == 0) {
				printf("rem %d\n" , store);
				store = TraceStackHandler_remove(
					&trace->stackHandler,
					store,
					varInfos[varIdx].size
				);
			}
			uses[usageCompletion] = store;
			usageCompletion++;
			continue;
		}

		usageCompletion = 0;



		// Instruction
		switch (code) {
		case TRACECODE_STAR:
		{
			int action = line >> 10;
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

			}
			
			break;
		}

		case TRACECODE_CREATE:
		{
			varInfos[(line >> 16) & 0xFFF].size = (*linePtr) >> 16;
			linePtr++;
			ip++;
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
				raiseError("[TODO] edit variable in place (so with a register)");
			} else {
				// edit register
				int psize = (line >> 10) & 0x3;
				int reg = line >> 16;
				fprintf(output, "\tmov ");
				Trace_printUsage(trace, output, psize, -reg);
				fprintf(output, ", ");
				Trace_printUsage(trace, output, psize, uses[0]);
				fprintf(output, "\n");
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
			break;	
		};

		case TRACECODE_LOGIC_IMM_LEFT:
		case TRACECODE_LOGIC_IMM_RIGHT:
		{
			break;	
		};



		case TRACECODE_FNCALL:
		{
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

	}

	finishWhile:
	TraceStackHandler_free(&trace->stackHandler);

	fprintf(output, "\tret\n");
}






