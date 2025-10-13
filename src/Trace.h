#ifndef COMPILER_TRACE_H_
#define COMPILER_TRACE_H_

#include "declarations.h"
#include "castable_t.h"

#include "util/Stack.h"

#include <tools/Array.h>


structdef(TracePack);
structdef(TraceLine);



/**
 * By default, each line is a usage line.
 * If next > TRACE_USAGE_OUT_OF_BOUNDS,
 * then we read an instruction (usage mode diseabled).
 * 
 * If we are in usage mode, the format is
 * usage: offset(10), variable(12), next(10)
 * 
 * If next equals TRACE_USAGE_LAST, the next variable
 * will be used a write
 */

enum {
	TRACE_LINE_POW = 8,
	TRACE_LINE_LEN = (1 << TRACE_LINE_POW),
	TRACE_LINE_MASK = TRACE_LINE_LEN - 1,


	TRACE_TEMP_REGISTERS = 5,
	TRACE_KEPT_REGISTERS = 5,


	TRACE_OFFSET_MAX = 0x3ff,

	TRACE_USAGE_SIZE = 10,
	TRACE_USAGE_MASK = 0x3ff,

	TRACE_USAGE_LAST = 0,
	/// TODO: edit 32
	TRACE_USAGE_OUT_OF_BOUNDS = 0x3ff - 32,
};


typedef unsigned int trline_t;


struct TracePack {
	trline_t line[TRACE_LINE_LEN+1];

	int completion;
	TracePack* next;
};


typedef struct TraceFunctionMapEntry {
	Function* key;
    int position; // -1 if not used
    struct TraceFunctionMapEntry* next;
} TraceFunctionMapEntry;


enum {TRACE_FUNCTIONMAP_LENGTH = 31};
typedef struct {
    TraceFunctionMapEntry buckets[TRACE_FUNCTIONMAP_LENGTH];
	int position;
} TraceFunctionMap;


struct Trace {
	icounter_t icounter;
	int instruction;

	TracePack* first;
	TracePack* last;

	Array variables; // type: VariableTrace
	Stack varPlacements; // type: uint
	TraceFunctionMap calledFunctions;

	int tempRegisters[TRACE_TEMP_REGISTERS];
	int keptRegisters[TRACE_KEPT_REGISTERS];
};



void Trace_create(Trace* trace);
void Trace_delete(Trace* trace);

uint Trace_pushVariable(Trace* trace);
void Trace_removeVariable(Trace* trace, uint index);
int Trace_reachFunction(Trace* trace, Function* fn);


int Trace_packSize(int size);
int Trace_packExprTypeToSize(int type);
void Trace_set(Trace* trace, Expression* expr, uint destVar, uint destOffset, int signedSize, int exprType);

trline_t* Trace_push(Trace* trace, int num);

void TracePack_print(const TracePack* pack);

void Trace_addUsage(Trace* trace, uint variable, uint offset, bool readMode);

uint Trace_ins_create(Trace* trace, Variable* variable, int size, int flags);
void Trace_ins_def(Trace* trace, int variable, int offset, int signedSize, castable_t value);
void Trace_ins_move(Trace* trace, int destVar, int destOffset, int srcVar, int srcOffset, int size);
trline_t* Trace_ins_if(Trace* trace, uint destVar);
void Trace_ins_jmp(Trace* trace, uint instruction);

void Trace_generateAssembly(Trace* trace, FunctionAssembly* fnAsm);


enum {
	TRACETYPE_S8,
	TRACETYPE_S16,
	TRACETYPE_S32,
	TRACETYPE_S64,
};

enum {
	TRACEOP_ADDITION,
	TRACEOP_SUBSTRACTION,
	TRACEOP_MULTIPLICATION,
	TRACEOP_DIVISION,
	TRACEOP_MODULO,
	TRACEOP_INC,
	TRACEOP_DEC,
	TRACEOP_OPPOSE,
};

enum {
	TRACEOP_BITWISE_AND,
	TRACEOP_BITWISE_OR,
	TRACEOP_BITWISE_XOR,
	TRACEOP_LEFT_SHIFT,
	TRACEOP_RIGHT_SHIFT,

	TRACEOP_LOGICAL_AND,
	TRACEOP_LOGICAL_OR,

	TRACEOP_EQUAL,
	TRACEOP_NOT_EQUAL,
	TRACEOP_LESS,
	TRACEOP_LESS_EQUAL,
	TRACEOP_GREATER,
	TRACEOP_GREATER_EQUAL,
};


enum {
    TRACE_REGISTER_RAX,
    TRACE_REGISTER_RBX,
    TRACE_REGISTER_RCX,
    TRACE_REGISTER_RDX,
    TRACE_REGISTER_RSI,
    TRACE_REGISTER_RDI,
    TRACE_REGISTER_RBP,
    TRACE_REGISTER_RSP,
    TRACE_REGISTER_R8,
    TRACE_REGISTER_R9,
    TRACE_REGISTER_R10,
    TRACE_REGISTER_R11,
    TRACE_REGISTER_R12,
    TRACE_REGISTER_R13,
    TRACE_REGISTER_R14,
    TRACE_REGISTER_R15
};



enum {
	TRACECODE_END = TRACE_USAGE_OUT_OF_BOUNDS+1,

	/**
	 * +00: CODE
	 * +10: FLAGS (6 bits)
	 * +16: VARIABLE
	 * +28: [blank]
	 * 
	 * +00: SIZE
	 * +16: [blank]
	 * +22: NEXT
	 * 
	 * FLAGS are:
	 * +0: CASTABLE
	 */
	TRACECODE_CREATE,

	/**
	 * if TYPE == TRACETYPE_S8:
	 *   +00: CODE
	 *   +10: TYPE
	 *   +12: [blank]
	 *   +16: VALUE
	 *   +24: [blank]
	 * 
	 * 
	 * else if TYPE == TRACETYPE_S16:
	 *   +00: CODE
	 *   +10: TYPE
	 *   +12: [blank]
	 *   +16: VALUE
	 * 
	 * else if TYPE == TRACETYPE_S32:
	 *   +00: CODE
	 *   +10: TYPE
	 *   +12: [blank]
	 * 
	 *   +00: VALUE
	 * 
	 * else:
	 *   +00: CODE
	 *   +10: TYPE
	 *   +12: [blank]
	 * 
	 *   +00: VALUE
	 *   
	 *   +00: VALUE
	 * 
	 */
	TRACECODE_DEF,

	/**
	 * +00: CODE
	 * +10: [blank]
	 * +16: SIZE
	 */
	TRACECODE_MOVE,

	/**
	 * +00: CODE
	 * +10: TYPE
	 * +12: EDIT (0=register edited ; 1=var edited)
	 * +13: [blank]
	 * +16: register id
	 * +24: [blank]
	 */
	TRACECODE_PLACE,

	/**
	 * +00: CODE
	 * +10: OPERATION
	 * +13: TYPE
	 * +15: [blank]
	 */
	TRACECODE_ARITHMETIC,

	/**
	 * +00: CODE
	 * +10: OPERATION
	 * +13: TYPE
	 * +15: SIDE
	 * +16: <star>
	 * 
	 * SIDE=0 means immediate is at left
	 * SIDE=1 means immediate is at right
	 * 
	 * if OPERATION == INC or DEC,
	 *  IMMEDIATE is ignored
	 * 	SIDE=0 means left  inc/dec
	 *  SIDE=1 means right inc/dec
	 *  
	 * else if IMMEDIATE is true:
	 *   if SIDE == 0, VALUE is left operand,
	 *   else, VALUE is right operand
	 * 
	 *   if TYPE == 0, <star> = VALUE (from +16 to +24)
	 *   if TYPE == 1, <star> = VALUE
	 *   if TYPE == 2, add 1 line (int) for VALUE
	 *   if TYPE == 3, add 2 lines (long) for VALUE
	 */
	TRACECODE_ARITHMETIC_IMM,

	/**
	 * +00: CODE
	 * +10: OPERATION
	 * +14: TYPE
	 * +16: [blank]
	 */
	TRACECODE_LOGIC,

	/**
	 * +00: CODE
	 * +10: OPERATION
	 * +14: TYPE
	 * +16: VALUE if TYPE < 2,
	 * 
	 * else if TYPE == 2:
	 * +00: VALUE 
	 * 
	 * else:
	 * +00: VALUE
	 * 
	 * +00: VALUE
	 */
	TRACECODE_LOGIC_IMM_LEFT,
	TRACECODE_LOGIC_IMM_RIGHT,

	/**
	 * +00: CODE
	 * +10: LABEL (on 22 bits)
	 */
	TRACECODE_FNCALL,

	/**
	 * +00: CODE
	 * +10: ADDR (22 bits)
	 */
	TRACECODE_IF,

	/**
	 * +00: CODE
	 * +10: ADDR (22 bits)
	 */
	TRACECODE_JMP,

	/**
	 * +00: CODE
	 * +10: src  signed?
	 * +11: src  float?
	 * +12: dest signed?
	 * +13: dest float?
	 * +14: [blank]
	 * +16: src  size (packed)
	 * +18: dest size (packed)
	 * +20
	 */
	TRACECODE_CAST
};




#endif
