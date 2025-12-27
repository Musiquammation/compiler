#ifndef COMPILER_TRACE_H_
#define COMPILER_TRACE_H_

#include "declarations.h"
#include "castable_t.h"
#include "TraceStackHandler.h"

#include "util/Stack.h"

#include "tools/Array.h"


structdef(TracePack);
structdef(TraceLine);



/**
 * By default, each line is a usage line.
 * If next > TRACE_USAGE_LIMIT,
 * then we read an instruction (usage mode diseabled).
 * 
 * If we are in usage mode, the format is
 * usage: variable(21), read(1), next(10)
 * 
 * If next equals TRACE_USAGE_LAST, the next variable
 * will be used a write
 * 
 * 
 * The convention for usage order is the following:
 * - first input (if !f lets the store be replaced)
 * - output
 * - next inputs
 */

enum {
	TRACE_LINE_POW = 10,
	TRACE_LINE_LEN = (1 << TRACE_LINE_POW),
	TRACE_LINE_MASK = TRACE_LINE_LEN - 1,


	TRACE_TEMP_REGISTERS = 5,
	TRACE_KEPT_REGISTERS = 5,


	TRACE_USAGE_SIZE = 10,
	TRACE_USAGE_MASK = 0x3ff,

	TRACE_USAGE_LAST = 0,
	TRACE_USAGE_OUT_OF_BOUNDS = 0x3ff - 18,
	TRACE_USAGE_LIMIT,


	TRACE_OFFSET_NONE = -1,
	TRACE_CALLESAVED_LIMIT = 128,
	TRACE_VARIABLE_NONE = 0xffffffff,
	TRACE_VARIABLE_KILL_FINAL = 0xffffffff,
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


enum {TRACE_FUNCTIONMAP_LENGTH = 15};
typedef struct {
    TraceFunctionMapEntry buckets[TRACE_FUNCTIONMAP_LENGTH];
	int position;
} TraceFunctionMap;

typedef struct {
	int instruction;
	uint variable;
	int victim;
	int destination;
	int victimStore; // for the victim
	char reading;
	bool becauseOfStackOnly; // destination will be a stackId
} TraceReplace;

typedef struct {
	uint variable;
	int nextUse;
} TraceRegister;

typedef struct {
	int size;
	int nextUse;
	int store; // positive => stackId, negative => register
	char registrable;
	bool stackOnly;
} VarInfoTrace;


struct Trace {
	int instruction;

	TracePack* first;
	TracePack* last;

	union {
		struct {
			Array variables; // type: VariableTrace
			Stack varPlacements; // type: uint
			int deep;
			int scopeId;
			int nextScopeId;
			Stack scopeIdStack; // type: int
		};
		
		struct {
			Array fncallPlacements; // type: fnplacement_t
			Array replaces; // type: Replace
			VarInfoTrace* varInfos; // type: VarInfo
			TraceRegister* regs;
			int stackId;
			int varlength;
			TraceStackHandler stackHandler;
		};
	};

	TraceFunctionMap calledFunctions;


	int tempRegisters[TRACE_TEMP_REGISTERS];
	int keptRegisters[TRACE_KEPT_REGISTERS];
};



void Trace_create(Trace* trace);
void Trace_delete(Trace* trace);

uint Trace_pushVariable(Trace* trace);
void Trace_removeVariable(Trace* trace, uint index);
int Trace_reachFunction(Trace* trace, Function* fn);
Function* Trace_getFunction(Trace* trace, int index);
void Trace_pushArgs(Trace* trace, Variable** args, int arglen);

int Trace_packSize(int size);
int Trace_unpackSize(int psize);
int Trace_packExprTypeToSize(int type);
void Trace_set(Trace* trace, Expression* expr, uint destVar, int destOffset, int signedSize, int exprType);

trline_t* Trace_push(Trace* trace, int num);

void TracePack_print(const TracePack* pack, int position);

void Trace_addUsage(Trace* trace, uint variable, int offset, bool readMode);


uint Trace_ins_create(Trace* trace, Variable* variable, int size, int flags, char registrable);
void Trace_ins_def(Trace* trace, int variable, int offset, int signedSize, castable_t value);
void Trace_ins_move(Trace* trace, int destVar, int srcVar, int destOffset, int srcOffset, int size, char isRegistrable);
void Trace_ins_moveWithPtrs(
	Trace* trace, int destVar, int srcVar, int destOffset, int srcOffset, int size,
	char isRegistrable, bool srcIsPointer, bool dstIsPointer);
trline_t* Trace_ins_if(Trace* trace, uint destVar);
void Trace_ins_jmp(Trace* trace, uint instruction);
void Trace_ins_placeReg(Trace* trace, int srcVariable, int dstVariable, int reg, int packedSize);
void Trace_ins_placeVar(Trace* trace, int dstVariable, int reg, int packedSize);
void Trace_ins_getStackPtr(Trace* trace, int destVar, int srcVar, int destOffset, int srcOffset);

void Trace_ins_savePlacement(Trace* trace);
void Trace_ins_openPlacement(Trace* trace);
void Trace_ins_saveShadowPlacement(Trace* trace);
void Trace_ins_openShadowPlacement(Trace* trace);

int* Trace_prepareWhileUsages(Trace* trace);
void Trace_addWhileUsages(Trace* trace, int scopeId, int startIns, int endIns, int* backup);

void Trace_placeRegisters(Trace* trace);
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
	TRACE_REG_RAX = 1,
	TRACE_REG_RCX,
	// TRACE_REG_RDX,
	// TRACE_REG_RSI,
	// TRACE_REG_RDI,
	// TRACE_REG_R8,
	// TRACE_REG_R9,
	// TRACE_REG_R10,
	TRACE_REG_R11,

	TRACE_REG_RBX,
	// TRACE_REG_R12,
	// TRACE_REG_R13,
	// TRACE_REG_R14,
	TRACE_REG_R15,

    TRACE_REG_NONE,
};


/// TODO: remove this line
enum {
	TRACE_REG_RDX = 420
};




enum {
	/**
	 * +00: CODE
	 * +10: ACTION
	 * +14: [blank]
	 * +16: DATA
	 * 
	 * ACTION=00: end
	 * ACTION=01: quick skip
	 * ACTION=02: return
	 * ACTION=03: forbid moves
	 * ACTION=04: protect RAX for fncall
	 * ACTION=05: protect RAX and RDX for fncall
	 * ACTION=06: mark label
	 * ACTION=07: save placements
	 * ACTION=08: open placements
	 * ACTION=09: save shadow placements
	 * ACTION=10: open shadow placements
	 * ACTION=11: trival usages
	 */
	TRACECODE_STAR = TRACE_USAGE_LIMIT+1,

	/**
	 * +00: CODE
	 * +10: FLAGS (6 bits)
	 * +16: VARIABLE
	 * +28: REGISTRABLE
	 * +29
	 * 
	 * +00: SIZE
	 * +17: [blank]
	 * +22: NEXT
	 * 
	 * FLAGS are:
	 * +0 (at +10): CASTABLE
	 * +1 (at +11): ARGUMENT
	 * +2 (at +12): STACK_ONLY
	 * 
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
	 * +10: REGISTRABLE
	 * +11: SRC_POINTER
	 * +12: DST_POINTER
	 * +13: [blank]
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
	TRACECODE_CAST,


	/**
	 * +00: CODE
	 * +10: [blank]
	 * +16: VARIABLE
	 * +28
	 * 
	 * +00: OFFSET
	 * +32
	 */
	TRACECODE_STACK_PTR,
};




#endif

