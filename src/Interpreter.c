#include "Interpreter.h"

#include "Variable.h"
#include "Expression.h"
#include "Prototype.h"
#include "Function.h"
#include "Trace.h"
#include "Type.h"

#include "helper.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


static int intComparator(const void* a, const void* b) {
	int x = *(int*)a;
	int y = *(int*)b;
	return (x > y) - (x < y);
}




static void buildPositions(Interpreter* itp, TracePack* firstPack, int varCount) {
	uint32_t* const varMalloced = malloc((varCount+31)/32 * sizeof(int));

	int waitedIns = -1;
	int* nextInsPos = NULL;
	int* insPosSnapshot = NULL;
	InterpretPosition* position;
	Array posArray; // type: int

	Array_create(&posArray, sizeof(int));

	for (char type = 0; type < 2; type++) {
		int ins = 0;
		TracePack* pack = firstPack;
		trline_t* ptr = firstPack->line;
		#define move() {line = *ptr; ptr++; ins++;}

		if (type) {
			// Once posArray has been filled,
			// we can create positions
			Array_sort(&posArray, intComparator);
			position = malloc(posArray.length * sizeof(InterpretPosition));

			itp->positions = position;
			itp->pos_len = posArray.length;

			// Snapshot indépendant pour éviter l'invalidation du pointeur
			// lors d'un éventuel realloc dans Array_push
			insPosSnapshot = malloc((posArray.length + 1) * sizeof(int));
			memcpy(insPosSnapshot, posArray.data, posArray.length * sizeof(int));
			insPosSnapshot[posArray.length] = -1; // sentinelle de fin

			if (posArray.length > 0) {
				nextInsPos = insPosSnapshot;
				waitedIns = *nextInsPos;
				nextInsPos++;
			}
		}

		while (true) {
			if (waitedIns == ins) {
				waitedIns = *nextInsPos;
				nextInsPos++;
				position->pack = pack;
				position->line = ptr;
				position->ins = ins;
				position++;
			}

			trline_t line;
			move();

			int code = line & 0x3ff;

			if (code <= TRACE_USAGE_OUT_OF_BOUNDS) {
				trline_t variable = line >> 11;
				if (varMalloced[variable/32] & (1u<<(variable%32))) {
					move();
				}
				continue;
			}


			switch (code) {
			case TRACECODE_STAR:
				if (((line >> 10) & 0xf) == 0) {
					pack = pack->next;
					if (!pack)
						goto finishMainWhile;

					ptr = pack->line;
				}
				break;

			case TRACECODE_CREATE:
			{
				trline_t variable = (line >> 16) & 0xfff;
				trline_t registrable = line & (1<<28);
				if (registrable) {
					varMalloced[variable / 32] &= ~(1u << (variable % 32));
				} else {
					varMalloced[variable / 32] |=  (1u << (variable % 32));
				}
				move(); // skip

				break;
			}

			case TRACECODE_DEF:
				switch ((line >> 10) & 0x3) {
				case 0:
				case 1:
					break;

				case 2:
					move();
					break;

				case 3:
					move();
					move();
					break;
				}
				break;

			case TRACECODE_MOVE:
				break;

			case TRACECODE_PLACE:
				break;

			case TRACECODE_ARITHMETIC:
				break;

			case TRACECODE_ARITHMETIC_IMM:
				switch ((line >> 13) & 0x3) {
				case 0:
				case 1:
					break;

				case 2:
					move();
					break;

				case 3:
					move();
					move();
					break;
				}
				break;

			case TRACECODE_LOGIC:
				break;

			case TRACECODE_LOGIC_IMM_LEFT:
			case TRACECODE_LOGIC_IMM_RIGHT:
				switch ((line >> 14) & 0x3) {
				case 0:
				case 1:
					break;

				case 2:
					move();
					break;

				case 3:
					move();
					move();
					break;
				}
				break;

			case TRACECODE_FNCALL:
				break;

			case TRACECODE_IF:
				if (type == 0)
					*Array_push(int, &posArray) = line >> 10;
				break;

			case TRACECODE_JMP:
				if (type == 0)
					*Array_push(int, &posArray) = line >> 10;
				break;

			case TRACECODE_CAST:
				break;

			case TRACECODE_STACK_PTR:
				move();
				break;
			}
		}


		finishMainWhile:
		#undef move
	}

	free(insPosSnapshot);
	free(varMalloced);
	Array_free(posArray);
}


Interpreter* Interpreter_build(Trace* trace) {
	Interpreter* itp = malloc(sizeof(Interpreter));
	TracePack* pack = trace->first;
	int varCount = trace->variables.length;
	itp->pack = pack;
	itp->varCount = varCount;
	buildPositions(itp, pack, varCount);
	return itp;
}

void Interpreter_delete(Interpreter* itp) {
	TracePack* p = itp->pack;
	while (p) {
		TracePack* next = p->next;
		free(p);
		p = next;
	}

	free(itp->positions);
}


Type* Interpret_call(Expression* fncallExpr, Scope* scope) {
	Function* fn = fncallExpr->data.fncall.fn;

	// Check metaDefinitionState
	switch (fn->metaDefinitionState) {
	case DEFINITIONSTATE_UNDEFINED:
		raiseError("[Undefined] Meta interpret status is undefined for this function");
		return NULL;

	case DEFINITIONSTATE_READING:
		raiseError("[Intern] Meta interpret status is reading for this function");
		return NULL;

	case DEFINITIONSTATE_DONE:
		break;

	case DEFINITIONSTATE_NOEXIST:
		return NULL;

	}

	Expression** exprArgs = fncallExpr->data.fncall.args;

	// Check for requires
	Function_makeRequiresReal(fn, NULL);
	Array_for(realRequireCouple_t, fn->realRequires, fn->requires_len, require) {
		Function* testfn = require->fn;

		// Check definitionState
		switch (testfn->definitionState) {
		case DEFINITIONSTATE_UNDEFINED:
			raiseError("[Undefined] Interpret status is undefined for this function");
			return NULL;

		case DEFINITIONSTATE_READING:
			raiseError("[Intern] Interpret status is reading for this function");
			return NULL;
			
		case DEFINITIONSTATE_DONE:
			break;
			
		case DEFINITIONSTATE_NOEXIST:
			raiseError("[Intern] Interpret does not exists for this function");
		}
		
		/// TODO: always use this and no argStartIndex=0?
		if ((fncallExpr->data.fncall.fn->flags & FUNCTIONFLAGS_THIS) == 0) 
			raiseError("[TODO] Interpret_call: no this");


		// Define mblock
		Expression* arg = exprArgs[require->position];
		interpreterSlot_t result = Interpret_interpret(
			testfn->interpreter, scope, &arg,
			0, 0, 1,
			true, true
		);

		if (result.u8 == 0) {
			raiseError("[Runtime] Require failed");
			return NULL;
		}
	}

	// Run final code
	Function* meta = fn->meta;
	if (Interpreter_checkDefinitionState(fn->definitionState))
		return NULL;
		
	interpreterSlot_t result = Interpret_interpret(
		meta->interpreter,
		scope,
		exprArgs,
		0,
		0,
		fn->projections_len + fn->settings_len,
		fncallExpr->data.fncall.fn->flags & FUNCTIONFLAGS_THIS,
		meta->returnPrototype ? true:false
	);

	// Produce a type from the result
	Prototype* returnPrototype = fn->returnPrototype;
	if (returnPrototype == NULL) {
		return NULL;
	}

	Prototype* mproto = meta->returnPrototype;
	if (mproto == NULL) {
		return NULL;
	}

	Type* type = Prototype_generateType(returnPrototype, scope, TYPE_CWAY_DEFAULT);

	/// TODO: handle type
	return type;
}

typedef interpreterSlot_t slot_t;


static bool fillSlotArgument(Scope* scope, Expression* expr, slot_t* slot) {
	expr = Expression_cross(expr);
	switch (expr->type) {
	case EXPRESSION_ADDR_OF:
		expr = expr->data.linked;
		switch (expr->type) {
		case EXPRESSION_PROPERTY:
		{
			if (expr->data.property.origin) {
				raiseError("[TODO] Handle origin case");
			}

			Variable** path = expr->data.property.varr;
			Type* type = Scope_searchType(scope, path[0]);
			int offset = Prototype_getVariableOffset(path, expr->data.property.varr_len);
			if (offset < 0) {offset = 0;}
			slot->ptr = type->data + offset;

			return true;
		}

		default:
			raiseError("[Expression] Invalid expression in addrOf argument");
			return false;
		}
		break;

	case EXPRESSION_TYPE:
		slot->ptr = expr->data.type->data;
		return true;

	case EXPRESSION_MBLOCK:
		slot->ptr = expr->data.mblock;
		return true;

	case EXPRESSION_PROPERTY:
	{
		if (expr->data.property.origin) {
			raiseError("[Expression] Cannot handle origin in property");
			return false;
		}

		Variable* first = expr->data.property.varr[0];
		Type* type = Scope_searchType(scope, first);
		if (!type) {
			raiseError("[Unfound] Cannot find type");
		}


		int offset = Prototype_getGlobalVariableOffset(
			first->proto,
			&expr->data.property.varr[1], expr->data.property.varr_len-1);

		slot->ptr = type->data + offset;
		return true;
	}



	case EXPRESSION_U8:
		slot->u8 = expr->data.num.u8;  return true;
	case EXPRESSION_I8:
		slot->u8 = expr->data.num.i8;  return true;

	case EXPRESSION_U16:
		slot->u16 = expr->data.num.u16; return true;
	case EXPRESSION_I16:
		slot->u16 = expr->data.num.i16; return true;

	case EXPRESSION_U32:
		slot->u32 = expr->data.num.u32; return true;
	case EXPRESSION_I32:
	case EXPRESSION_INTEGER:
		slot->u32 = expr->data.num.i32; return true;

	case EXPRESSION_U64:
		slot->u64 = expr->data.num.u64; return true;
	case EXPRESSION_I64:
		slot->u64 = expr->data.num.i64; return true;

	default:
		raiseError("[Expression] Invalid expression in argument");
		return false;

	}
}

static bool fillSlotSetting(Expression* expr, slot_t* slot) {
	switch (expr->type) {
	case EXPRESSION_U8:
		slot->u8 = expr->data.num.u8;  return true;
	case EXPRESSION_I8:
		slot->u8 = expr->data.num.i8;  return true;

	case EXPRESSION_U16:
		slot->u16 = expr->data.num.u16; return true;
	case EXPRESSION_I16:
		slot->u16 = expr->data.num.i16; return true;

	case EXPRESSION_U32:
		slot->u32 = expr->data.num.u32; return true;
	case EXPRESSION_I32:
	case EXPRESSION_INTEGER:
		slot->u32 = expr->data.num.i32; return true;

	case EXPRESSION_U64:
		slot->u64 = expr->data.num.u64; return true;
	case EXPRESSION_I64:
		slot->u64 = expr->data.num.i64; return true;


	default:
		raiseError("[Expression] Invalid expression in setting");
		return false;
	}
}

static bool fillSlotProjection(Expression* expr, slot_t* slot) {
	switch (expr->type) {
	


	default:
		raiseError("[Expression] Invalid expression in setting");
		return false;
	}
}



static void fillInputSlots(
	slot_t slots[],
	Scope* scope,
	Expression** args,
	int projectionLength,
	int settingsLength,
	int argsLen,
	bool useThis
) {
	slot_t* slot = slots;
	Expression** argptr = &args[projectionLength + settingsLength];
	Expression** endArgs = argptr + argsLen;

	// Fill args
	for (; argptr < endArgs; argptr++) {
		if (fillSlotArgument(scope, *argptr, slot))
			slot++;
	}

	// Fill settings
	argptr = &args[projectionLength];
	endArgs = argptr + settingsLength;
	for (; argptr < endArgs; argptr++) {
		if (fillSlotSetting(*argptr, slot))
			slot++;
	}
}


typedef struct {
	int variable;
	int offset;
} Usage;

typedef struct {
	TracePack* pack;
	trline_t* ptr;
	Usage* vars;
	slot_t* const  slots;
	int* const registers;
	uint32_t* const varMalloced;
	InterpretPosition* jumps;
	int jmp_len;
	int usedVariablesCapacity;
	int usedVariablesCount;
	int rememberedSize;
} Cursor;




static void irun_create(Cursor* c, trline_t line);
static void irun_def(Cursor* c, trline_t line);
static void irun_move(Cursor* c, trline_t line);
static void irun_place(Cursor* c, trline_t line);
static void irun_arithmetic(Cursor* c, trline_t line);
static void irun_arithmetic_imm(Cursor* c, trline_t line);
static void irun_logic(Cursor* c, trline_t line);
static void irun_logic_imm_right(Cursor* c, trline_t line);
static void irun_logic_imm_left(Cursor* c, trline_t line);
static void irun_fncall(Cursor* c, trline_t line);
static void irun_if(Cursor* c, trline_t line);
static void irun_jmp(Cursor* c, trline_t line);
static void irun_cast(Cursor* c, trline_t line);
static void irun_stackPtr(Cursor* c, trline_t line);
static void irun_memory(Cursor* c, trline_t line);


interpreterSlot_t Interpret_interpret(
	const Interpreter* itp,
	Scope* scope,
	Expression** args,
	int projectionLength,
	int settingsLength,
	int argsLen,
	bool useThis,
	bool shouldReturn
) {
   	if (!itp) {
		raiseError("[Undefined] Interpreter not defined for this function");
		return (interpreterSlot_t){};
	}

	// Slots
	const int varCount = itp->varCount;
	// slot_t slots[varCount];
	slot_t* const slots = malloc(sizeof(slot_t) * varCount);
	uint32_t varMalloced[(varCount+31)/32];
	int registers[TRACE_REG_NONE];
	memset(varMalloced, 0, sizeof(varMalloced));
	memset(registers, -1, sizeof(registers));

	// Fill args
	fillInputSlots(slots, scope, args, 
		projectionLength, settingsLength, argsLen, useThis);

	// Run
	#define move() {line = *c.ptr; c.ptr++;}
	typedef Variable* vptr_t;

	Cursor c = {
		.pack = itp->pack,
		.ptr = itp->pack->line,
		.vars = malloc(sizeof(Usage) * 4),
		.slots = slots,
		.registers = registers,
		.varMalloced = varMalloced,
		.jumps = itp->positions,
		.jmp_len = itp->pos_len,
		.usedVariablesCapacity = 4,
		.usedVariablesCount = 0,
	};

	while (true) {
		// printf("itp %04ld\n", c.ptr - itp->pack->line);

		trline_t line;
		move();

		int code = line & 0x3ff;

		// Handle usages
		if (code <= TRACE_USAGE_OUT_OF_BOUNDS) {
			if (c.usedVariablesCount == c.usedVariablesCapacity) {
				c.usedVariablesCapacity *= 2;
				c.vars = realloc(c.vars,
					c.usedVariablesCapacity * sizeof(Usage));
			}

			int variable = line >> 11;
			c.vars[c.usedVariablesCount].variable = variable;
			if (varMalloced[variable/32] & (1u<<(variable%32))) {
				move();	
				c.vars[c.usedVariablesCount].offset = line;
			} else {
				c.vars[c.usedVariablesCount].offset = -1;
			}

			c.usedVariablesCount++;
			continue;
		}

		// Run instruction
		switch (code) {
		case TRACECODE_STAR:
		{
			int action = (line >> 10) & 0xf;
			switch (action) {
				case 0:
				// change pack
				c.pack = c.pack->next;
				if (!c.pack)
					goto finishMainWhile;

				c.ptr = c.pack->line;
				break;

			case 1: // quick skip
				break;

			case 2: // return
				goto finishMainWhile;
				break;

			case 3: // forbid moves
			case 4: // protect RAX for fncall
			case 5: // protect RAX and RDX for fncall
			case 6: // mark label
			case 7: // save placements
			case 8: // open placements
			case 9: // save shadow placements
			case 10: // open shadow placements
			case 11: // trival usages
			case 12: // fncall return dst variable (for transpiler)
				break;

			case 13: // remember size of next variable
				c.rememberedSize = line >> 16;
				break;
			}

			break;
		}

		case TRACECODE_CREATE:
			irun_create(&c, line);
			break;

		case TRACECODE_DEF:
			irun_def(&c, line);
			break;

		case TRACECODE_MOVE:
			irun_move(&c, line);
			break;

		case TRACECODE_PLACE:
			irun_place(&c, line);
			break;

		case TRACECODE_ARITHMETIC:
			irun_arithmetic(&c, line);
			break;

		case TRACECODE_ARITHMETIC_IMM:
			irun_arithmetic_imm(&c, line);
			break;

		case TRACECODE_LOGIC:
			irun_logic(&c, line);
			break;

		case TRACECODE_LOGIC_IMM_LEFT:
			irun_logic_imm_left(&c, line);
			break;

		case TRACECODE_LOGIC_IMM_RIGHT:
			irun_logic_imm_right(&c, line);
			break;

		case TRACECODE_FNCALL:
			irun_fncall(&c, line);
			break;

		case TRACECODE_IF:
			irun_if(&c, line);
			break;

		case TRACECODE_JMP:
			irun_jmp(&c, line);
			break;

		case TRACECODE_CAST:
			irun_cast(&c, line);
			break;

		case TRACECODE_STACK_PTR:
			irun_stackPtr(&c, line);
			break;

		case TRACECODE_MEMORY:
			irun_memory(&c, line);
			break;

		}
	
		c.usedVariablesCount = 0;
	}


	/// TODO: when doing create, skip if it is an argument

	finishMainWhile:

	interpreterSlot_t result;
	// Take rax (if we need to return)
	int skippedRemovedVariable;
	if (shouldReturn) {
		int variable = registers[TRACE_REG_RAX];
		if (varMalloced[variable / 32] & (1u << (variable % 32))) {
			skippedRemovedVariable = variable;
		}

		result = slots[variable];

	} else {
		skippedRemovedVariable = -1;
	}
	
	// Clean unmalloced variables
	free(c.vars);
	for (int variable = 0; variable < varCount; variable++) {
		if (varMalloced[variable / 32] & (1u << (variable % 32))) {
			free(slots[variable].ptr);
		}
	}
	free(slots);

	return result;

	
	#undef move
}



bool Interpreter_checkDefinitionState(definitionState_t definitionState) {
	switch (definitionState) {
	case DEFINITIONSTATE_UNDEFINED:
		raiseError("[Undefined] Interpret status is undefined for this function");
		return true;

	case DEFINITIONSTATE_READING:
		raiseError("[Intern] Interpret status is reading for this function");
		return true;

	case DEFINITIONSTATE_DONE:
		return false;

	case DEFINITIONSTATE_NOEXIST:
		return true;

	}
}








#define move() {line = *(c->ptr); c->ptr++;}


static uint8_t* eval8(slot_t* slots, Usage usage) {
	if (usage.offset < 0) {
		return &slots[usage.variable].u8;
	}
	
	return slots[usage.variable].ptr + usage.offset;
}

static uint16_t* eval16(slot_t* slots, Usage usage) {
	if (usage.offset < 0) {
		return &slots[usage.variable].u16;
	}

	return slots[usage.variable].ptr + usage.offset;
}

static uint32_t* eval32(slot_t* slots, Usage usage) {
	if (usage.offset < 0) {
		return &slots[usage.variable].u32;
	}

	return slots[usage.variable].ptr + usage.offset;
}

static uint64_t* eval64(slot_t* slots, Usage usage) {
	if (usage.offset < 0) {
		return &slots[usage.variable].u64;
	}

	return slots[usage.variable].ptr + usage.offset;
}


static void irun_create(Cursor* c, trline_t line) {
	trline_t isArg = line & (1<<11);
	if (isArg) {
		move();
		return; // skip
	}

	trline_t variable = (line >> 16) & 0xfff;
	trline_t registrable = line & (1<<28);
	move();
	trline_t size = line >> 16;

	if (c->varMalloced[variable / 32] & (1u << (variable % 32))) {
		free(c->slots[variable].ptr);
	}

	if (registrable) {
		c->varMalloced[variable / 32] &= ~(1u << (variable % 32));
	} else {
		c->slots[variable].ptr = malloc(size);
		c->varMalloced[variable / 32] |=  (1u << (variable % 32));
	}
}

static void irun_def(Cursor* c, trline_t line) {
	int type = (line >> 10) & 0x3;
	switch (type) {
	case 0:
		*eval8(c->slots, c->vars[0]) = (line >> 16) & 0xff;
		break;

	case 1:
		*eval16(c->slots, c->vars[0]) = line >> 16;
		break;
		
	case 2:
		move();
		*eval32(c->slots, c->vars[0]) = line;
		break;

	case 3:
	{
		move();
		uint64_t l = line;
		move();
		uint64_t r = line;
		*eval64(c->slots, c->vars[0]) = l | (r << 32);
		break;
	}
	}

}

static void irun_move(Cursor* c, trline_t line) {
	Usage* src = &c->vars[0];
	Usage* dst = &c->vars[1];
	int size = line >> 16;
			
	trline_t loadSrc = line & (1<<11);
	trline_t loadDst = line & (1<<12);

	if (loadDst && loadSrc)
		raiseError("[Intern] Load src and dst at once is an illegal operation");

	if (loadDst) {
		void* final = (void*)*eval64(c->slots, *dst);
		switch (size) {
		case 1:	*( uint8_t*)final = *eval8 (c->slots, *src); break;
		case 2:	*(uint16_t*)final = *eval16(c->slots, *src); break;
		case 4: *(uint32_t*)final = *eval32(c->slots, *src); break;
		case 8: *(uint64_t*)final = *eval64(c->slots, *src); break;

		default:
			memcpy(
				final,
				c->slots[src->variable].ptr + src->offset,
				size
			);
			break;
		}
		return;
	}

	if (loadSrc) {
		void* final = (void*)*eval64(c->slots, *src);
		switch (size) {
		case 1: *eval8 (c->slots, *dst) = *( uint8_t*)final; break;
		case 2: *eval16(c->slots, *dst) = *(uint16_t*)final; break;
		case 4: *eval32(c->slots, *dst) = *(uint32_t*)final; break;
		case 8: *eval64(c->slots, *dst) = *(uint64_t*)final; break;

		default:
			memcpy(
				c->slots[dst->variable].ptr + dst->offset,
				final,
				size
			);
			break;
		}
		return;
	}

	
	switch (size) {
	case 1: *eval8 (c->slots, *dst) = *eval8 (c->slots, *src); break;
	case 2:	*eval16(c->slots, *dst) = *eval16(c->slots, *src); break;
	case 4: *eval32(c->slots, *dst) = *eval32(c->slots, *src); break;
	case 8: *eval64(c->slots, *dst) = *eval64(c->slots, *src); break;

	default:
		memcpy(
			c->slots[dst->variable].ptr + dst->offset,
			c->slots[src->variable].ptr + src->offset,
			size
		);
		break;
	}



}

static void irun_place(Cursor* c, trline_t line) {
	trline_t reg = (line >> 16) & 0xff;
	trline_t psize = (line >> 10) & 0x3;
	
	// Edit variable
	if (line & (1<<12)) {
		c->registers[reg] = c->vars[1].variable;
	} else {
		c->registers[reg] = c->vars[0].variable;
	}

	// Move value
	switch (psize) {
	case 0: *eval8 (c->slots, c->vars[1]) = *eval8 (c->slots, c->vars[0]); break;
	case 1:	*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]); break;
	case 2: *eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]); break;
	case 3: *eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]); break;
	}
	
}

static void irun_arithmetic(Cursor* c, trline_t line) {
	int operation = (line >> 10) & 0x7;
	int psize = (line >> 13) & 0x3;

	switch (psize) {
	case 0:
		switch (operation) {
		case TRACEOP_ADDITION:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) +
				*eval8(c->slots, c->vars[2]); break;

		case TRACEOP_SUBSTRACTION:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) -
				*eval8(c->slots, c->vars[2]); break;

		case TRACEOP_MULTIPLICATION:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) *
				*eval8(c->slots, c->vars[2]); break;

		case TRACEOP_DIVISION:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) /
				*eval8(c->slots, c->vars[2]); break;

		case TRACEOP_MODULO:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) %
				*eval8(c->slots, c->vars[2]); break;

		case TRACEOP_INC:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) + 1;
			break;

		case TRACEOP_DEC:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) - 1;
			break;

		case TRACEOP_OPPOSE:
			*eval8 (c->slots, c->vars[1]) = -(*eval8(c->slots, c->vars[0]));
			break;
		}

		break;


	case 1:
		switch (operation) {
		case TRACEOP_ADDITION:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) +
				*eval16(c->slots, c->vars[2]); break;

		case TRACEOP_SUBSTRACTION:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) -
				*eval16(c->slots, c->vars[2]); break;

		case TRACEOP_MULTIPLICATION:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) *
				*eval16(c->slots, c->vars[2]); break;

		case TRACEOP_DIVISION:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) /
				*eval16(c->slots, c->vars[2]); break;

		case TRACEOP_MODULO:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) %
				*eval16(c->slots, c->vars[2]); break;

		case TRACEOP_INC:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) + 1;
			break;

		case TRACEOP_DEC:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) - 1;
			break;

		case TRACEOP_OPPOSE:
			*eval16(c->slots, c->vars[1]) = -(*eval16(c->slots, c->vars[0]));
			break;
		}

		break;

	
	case 2:
		switch (operation) {
		case TRACEOP_ADDITION:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) +
				*eval32(c->slots, c->vars[2]); break;

		case TRACEOP_SUBSTRACTION:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) -
				*eval32(c->slots, c->vars[2]); break;

		case TRACEOP_MULTIPLICATION:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) *
				*eval32(c->slots, c->vars[2]); break;

		case TRACEOP_DIVISION:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) /
				*eval32(c->slots, c->vars[2]); break;

		case TRACEOP_MODULO:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) %
				*eval32(c->slots, c->vars[2]); break;

		case TRACEOP_INC:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) + 1;
			break;

		case TRACEOP_DEC:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) - 1;
			break;

		case TRACEOP_OPPOSE:
			*eval32(c->slots, c->vars[1]) = -(*eval32(c->slots, c->vars[0]));
			break;
		}

		break;


	case 3:
		switch (operation) {
		case TRACEOP_ADDITION:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) +
				*eval64(c->slots, c->vars[2]); break;

		case TRACEOP_SUBSTRACTION:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) -
				*eval64(c->slots, c->vars[2]); break;

		case TRACEOP_MULTIPLICATION:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) *
				*eval64(c->slots, c->vars[2]); break;

		case TRACEOP_DIVISION:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) /
				*eval64(c->slots, c->vars[2]); break;

		case TRACEOP_MODULO:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) %
				*eval64(c->slots, c->vars[2]); break;

		case TRACEOP_INC:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) + 1;
			break;

		case TRACEOP_DEC:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) - 1;
			break;

		case TRACEOP_OPPOSE:
			*eval64(c->slots, c->vars[1]) = -(*eval64(c->slots, c->vars[0]));
			break;
		}

		break;
	}
}

static void irun_arithmetic_imm(Cursor* c, trline_t line) {
	int operation = (line >> 10) & 0x7;
	int psize = (line >> 13) & 0x3;

	if (line & (1<<15)) {
		// Immediate is at right
		switch (psize) {
		case 0:
		{
			uint8_t imm = (line >> 16) & 0xff;
			switch (operation) {
			case TRACEOP_ADDITION:
				*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) + imm;
				break;

			case TRACEOP_SUBSTRACTION:
				*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) - imm;
				break;

			case TRACEOP_MULTIPLICATION:
				*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) * imm;
				break;

			case TRACEOP_DIVISION:
				*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) / imm;
				break;

			case TRACEOP_MODULO:
				*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) % imm;
				break;

			case TRACEOP_INC:
				*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) + 1;
				break;

			case TRACEOP_DEC:
				*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) - 1;
				break;

			case TRACEOP_OPPOSE:
				*eval8 (c->slots, c->vars[1]) = -(*eval8(c->slots, c->vars[0]));
				break;
			}

			break;
		}


		case 1:
		{
			uint16_t imm = line >> 16;
			switch (operation) {
			case TRACEOP_ADDITION:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) + imm;
				break;

			case TRACEOP_SUBSTRACTION:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) - imm;
				break;

			case TRACEOP_MULTIPLICATION:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) * imm;
				break;

			case TRACEOP_DIVISION:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) / imm;
				break;

			case TRACEOP_MODULO:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) % imm;
				break;

			case TRACEOP_INC:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) + 1;
				break;

			case TRACEOP_DEC:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) - 1;
				break;

			case TRACEOP_OPPOSE:
				*eval16(c->slots, c->vars[1]) = -(*eval16(c->slots, c->vars[0]));
				break;
			}

			break;
		}

		
		case 2:
		{
			move();
			uint32_t imm = line;
			switch (operation) {
			case TRACEOP_ADDITION:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) + imm;

			case TRACEOP_SUBSTRACTION:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) - imm;

			case TRACEOP_MULTIPLICATION:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) * imm;

			case TRACEOP_DIVISION:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) / imm;

			case TRACEOP_MODULO:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) % imm;

			case TRACEOP_INC:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) + 1;
				break;

			case TRACEOP_DEC:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) - 1;
				break;

			case TRACEOP_OPPOSE:
				*eval32(c->slots, c->vars[1]) = -(*eval32(c->slots, c->vars[0]));
				break;
			}

			break;
		}


		case 3:
		{
			move();
			uint64_t l = line;
			move();
			uint64_t r = line;
			uint64_t imm = l | (r << 32);
			
			switch (operation) {
			case TRACEOP_ADDITION:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) + imm;
				break;

			case TRACEOP_SUBSTRACTION:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) - imm;
				break;

			case TRACEOP_MULTIPLICATION:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) * imm;
				break;

			case TRACEOP_DIVISION:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) / imm;
				break;

			case TRACEOP_MODULO:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) % imm;
				break;

			case TRACEOP_INC:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) + 1;
				break;

			case TRACEOP_DEC:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) - 1;
				break;

			case TRACEOP_OPPOSE:
				*eval64(c->slots, c->vars[1]) = -(*eval64(c->slots, c->vars[0]));
				break;
			}

			break;
		}



		}
		
		
	} else {
		// Immediate is at left
		switch (psize) {
		case 0:
		{
			uint8_t imm = (line >> 16) & 0xff;
			switch (operation) {
			case TRACEOP_ADDITION:
				*eval8 (c->slots, c->vars[1]) = imm + *eval8(c->slots, c->vars[0]);
				break;

			case TRACEOP_SUBSTRACTION:
				*eval8 (c->slots, c->vars[1]) = imm - *eval8(c->slots, c->vars[0]);
				break;

			case TRACEOP_MULTIPLICATION:
				*eval8 (c->slots, c->vars[1]) = imm * *eval8(c->slots, c->vars[0]);
				break;

			case TRACEOP_DIVISION:
				*eval8 (c->slots, c->vars[1]) = imm / *eval8(c->slots, c->vars[0]);
				break;

			case TRACEOP_MODULO:
				*eval8 (c->slots, c->vars[1]) = imm % *eval8(c->slots, c->vars[0]);
				break;

			case TRACEOP_INC:
				*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) + 1;
				break;

			case TRACEOP_DEC:
				*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) - 1;
				break;

			case TRACEOP_OPPOSE:
				*eval8 (c->slots, c->vars[1]) = -(*eval8(c->slots, c->vars[0]));
				break;
			}

			break;
		}


		case 1:
		{
			uint16_t imm = line >> 16;
			switch (operation) {
			case TRACEOP_ADDITION:
				*eval16(c->slots, c->vars[1]) = imm + *eval16(c->slots, c->vars[0]);
				break;

			case TRACEOP_SUBSTRACTION:
				*eval16(c->slots, c->vars[1]) = imm - *eval16(c->slots, c->vars[0]);
				break;

			case TRACEOP_MULTIPLICATION:
				*eval16(c->slots, c->vars[1]) = imm * *eval16(c->slots, c->vars[0]);
				break;

			case TRACEOP_DIVISION:
				*eval16(c->slots, c->vars[1]) = imm / *eval16(c->slots, c->vars[0]);
				break;

			case TRACEOP_MODULO:
				*eval16(c->slots, c->vars[1]) = imm % *eval16(c->slots, c->vars[0]);
				break;

			case TRACEOP_INC:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) + 1;
				break;

			case TRACEOP_DEC:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) - 1;
				break;

			case TRACEOP_OPPOSE:
				*eval16(c->slots, c->vars[1]) = -(*eval16(c->slots, c->vars[0]));
				break;
			}

			break;
		}

		
		case 2:
		{
			move();
			uint32_t imm = line;
			switch (operation) {
			case TRACEOP_ADDITION:
				*eval32(c->slots, c->vars[1]) = imm + *eval32(c->slots, c->vars[0]);

			case TRACEOP_SUBSTRACTION:
				*eval32(c->slots, c->vars[1]) = imm - *eval32(c->slots, c->vars[0]);

			case TRACEOP_MULTIPLICATION:
				*eval32(c->slots, c->vars[1]) = imm * *eval32(c->slots, c->vars[0]);

			case TRACEOP_DIVISION:
				*eval32(c->slots, c->vars[1]) = imm / *eval32(c->slots, c->vars[0]);

			case TRACEOP_MODULO:
				*eval32(c->slots, c->vars[1]) = imm % *eval32(c->slots, c->vars[0]);

			case TRACEOP_INC:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) + 1;
				break;

			case TRACEOP_DEC:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) - 1;
				break;

			case TRACEOP_OPPOSE:
				*eval32(c->slots, c->vars[1]) = -(*eval32(c->slots, c->vars[0]));
				break;
			}

			break;
		}


		case 3:
		{
			move();
			uint64_t l = line;
			move();
			uint64_t r = line;
			uint64_t imm = l | (r << 32);
			
			switch (operation) {
			case TRACEOP_ADDITION:
				*eval64(c->slots, c->vars[1]) = imm + *eval64(c->slots, c->vars[0]);
				break;

			case TRACEOP_SUBSTRACTION:
				*eval64(c->slots, c->vars[1]) = imm - *eval64(c->slots, c->vars[0]);
				break;

			case TRACEOP_MULTIPLICATION:
				*eval64(c->slots, c->vars[1]) = imm * *eval64(c->slots, c->vars[0]);
				break;

			case TRACEOP_DIVISION:
				*eval64(c->slots, c->vars[1]) = imm / *eval64(c->slots, c->vars[0]);
				break;

			case TRACEOP_MODULO:
				*eval64(c->slots, c->vars[1]) = imm % *eval64(c->slots, c->vars[0]);
				break;

			case TRACEOP_INC:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) + 1;
				break;

			case TRACEOP_DEC:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) - 1;
				break;

			case TRACEOP_OPPOSE:
				*eval64(c->slots, c->vars[1]) = -(*eval64(c->slots, c->vars[0]));
				break;
			}

			break;
		}



		}

	}
}

static void irun_logic(Cursor* c, trline_t line) {
	int operation = (line >> 10) & 0xf;
	int psize = (line >> 14) & 0x3;

	switch (psize) {
	case 0:
		switch (operation) {
		case TRACEOP_BITWISE_AND:
			*eval8(c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) &
				*eval8(c->slots, c->vars[2]); break;

		case TRACEOP_BITWISE_OR:
			*eval8(c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) |
				*eval8(c->slots, c->vars[2]); break;

		case TRACEOP_BITWISE_XOR:
			*eval8(c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) ^
				*eval8(c->slots, c->vars[2]); break;

		case TRACEOP_LEFT_SHIFT:
			*eval8(c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) <<
				*eval8(c->slots, c->vars[2]); break;

		case TRACEOP_RIGHT_SHIFT:
			*eval8(c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) >>
				*eval8(c->slots, c->vars[2]); break;


		case TRACEOP_LOGICAL_AND:
			*eval8(c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) &&
				*eval8(c->slots, c->vars[2]); break;

		case TRACEOP_LOGICAL_OR:
			*eval8(c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) ||
				*eval8(c->slots, c->vars[2]); break;


		case TRACEOP_EQUAL:
			*eval8(c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) ==
				*eval8(c->slots, c->vars[2]); break;

		case TRACEOP_NOT_EQUAL:
			*eval8(c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) !=
				*eval8(c->slots, c->vars[2]); break;

		case TRACEOP_LESS:
			*eval8(c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) <
				*eval8(c->slots, c->vars[2]); break;

		case TRACEOP_LESS_EQUAL:
			*eval8(c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) <=
				*eval8(c->slots, c->vars[2]); break;

		case TRACEOP_GREATER:
			*eval8(c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) >
				*eval8(c->slots, c->vars[2]); break;

		case TRACEOP_GREATER_EQUAL:
			*eval8(c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) >=
				*eval8(c->slots, c->vars[2]); break;


		}

		break;


	case 1:
		switch (operation) {
			case TRACEOP_BITWISE_AND:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) &
					*eval16(c->slots, c->vars[2]); break;

			case TRACEOP_BITWISE_OR:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) |
					*eval16(c->slots, c->vars[2]); break;

			case TRACEOP_BITWISE_XOR:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) ^
					*eval16(c->slots, c->vars[2]); break;

			case TRACEOP_LEFT_SHIFT:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) <<
					*eval16(c->slots, c->vars[2]); break;

			case TRACEOP_RIGHT_SHIFT:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) >>
					*eval16(c->slots, c->vars[2]); break;


			case TRACEOP_LOGICAL_AND:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) &&
					*eval16(c->slots, c->vars[2]); break;

			case TRACEOP_LOGICAL_OR:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) ||
					*eval16(c->slots, c->vars[2]); break;


			case TRACEOP_EQUAL:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) ==
					*eval16(c->slots, c->vars[2]); break;

			case TRACEOP_NOT_EQUAL:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) !=
					*eval16(c->slots, c->vars[2]); break;

			case TRACEOP_LESS:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) <
					*eval16(c->slots, c->vars[2]); break;

			case TRACEOP_LESS_EQUAL:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) <=
					*eval16(c->slots, c->vars[2]); break;

			case TRACEOP_GREATER:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) >
					*eval16(c->slots, c->vars[2]); break;

			case TRACEOP_GREATER_EQUAL:
				*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) >=
					*eval16(c->slots, c->vars[2]); break;


			}

			break;


		case 2:
			switch (operation) {
			case TRACEOP_BITWISE_AND:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) &
					*eval32(c->slots, c->vars[2]); break;

			case TRACEOP_BITWISE_OR:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) |
					*eval32(c->slots, c->vars[2]); break;

			case TRACEOP_BITWISE_XOR:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) ^
					*eval32(c->slots, c->vars[2]); break;

			case TRACEOP_LEFT_SHIFT:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) <<
					*eval32(c->slots, c->vars[2]); break;

			case TRACEOP_RIGHT_SHIFT:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) >>
					*eval32(c->slots, c->vars[2]); break;


			case TRACEOP_LOGICAL_AND:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) &&
					*eval32(c->slots, c->vars[2]); break;

			case TRACEOP_LOGICAL_OR:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) ||
					*eval32(c->slots, c->vars[2]); break;


			case TRACEOP_EQUAL:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) ==
					*eval32(c->slots, c->vars[2]); break;

			case TRACEOP_NOT_EQUAL:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) !=
					*eval32(c->slots, c->vars[2]); break;

			case TRACEOP_LESS:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) <
					*eval32(c->slots, c->vars[2]); break;

			case TRACEOP_LESS_EQUAL:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) <=
					*eval32(c->slots, c->vars[2]); break;

			case TRACEOP_GREATER:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) >
					*eval32(c->slots, c->vars[2]); break;

			case TRACEOP_GREATER_EQUAL:
				*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) >=
					*eval32(c->slots, c->vars[2]); break;


			}

			break;


		case 3:
			switch (operation) {
			case TRACEOP_BITWISE_AND:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) &
					*eval64(c->slots, c->vars[2]); break;

			case TRACEOP_BITWISE_OR:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) |
					*eval64(c->slots, c->vars[2]); break;

			case TRACEOP_BITWISE_XOR:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) ^
					*eval64(c->slots, c->vars[2]); break;

			case TRACEOP_LEFT_SHIFT:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) <<
					*eval64(c->slots, c->vars[2]); break;

			case TRACEOP_RIGHT_SHIFT:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) >>
					*eval64(c->slots, c->vars[2]); break;


			case TRACEOP_LOGICAL_AND:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) &&
					*eval64(c->slots, c->vars[2]); break;

			case TRACEOP_LOGICAL_OR:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) ||
					*eval64(c->slots, c->vars[2]); break;


			case TRACEOP_EQUAL:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) ==
					*eval64(c->slots, c->vars[2]); break;

			case TRACEOP_NOT_EQUAL:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) !=
					*eval64(c->slots, c->vars[2]); break;

			case TRACEOP_LESS:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) <
					*eval64(c->slots, c->vars[2]); break;

			case TRACEOP_LESS_EQUAL:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) <=
					*eval64(c->slots, c->vars[2]); break;

			case TRACEOP_GREATER:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) >
					*eval64(c->slots, c->vars[2]); break;

			case TRACEOP_GREATER_EQUAL:
				*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) >=
					*eval64(c->slots, c->vars[2]); break;


			}

			break;
	}
}

static void irun_logic_imm_right(Cursor* c, trline_t line) {
	int operation = (line >> 10) & 0xf;
	int psize = (line >> 14) & 0x3;

	switch (psize) {
	case 0:
	{
		uint8_t imm = (line >> 16) & 0xff;
		switch (operation) {
		case TRACEOP_BITWISE_AND:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) & imm;
			break;

		case TRACEOP_BITWISE_OR:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) | imm;
			break;

		case TRACEOP_BITWISE_XOR:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) ^ imm;
			break;

		case TRACEOP_LEFT_SHIFT:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) << imm;
			break;

		case TRACEOP_RIGHT_SHIFT:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) >> imm;
			break;


		case TRACEOP_LOGICAL_AND:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) && imm;
			break;

		case TRACEOP_LOGICAL_OR:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) || imm;
			break;


		case TRACEOP_EQUAL:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) == imm;
			break;

		case TRACEOP_NOT_EQUAL:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) != imm;
			break;

		case TRACEOP_LESS:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) < imm;
			break;

		case TRACEOP_LESS_EQUAL:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) <= imm;
			break;

		case TRACEOP_GREATER:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) > imm;
			break;

		case TRACEOP_GREATER_EQUAL:
			*eval8 (c->slots, c->vars[1]) = *eval8(c->slots, c->vars[0]) >= imm;
			break;
		}


		break;
	}


	case 1:
	{
		uint16_t imm = line >> 16;
		switch (operation) {
		case TRACEOP_BITWISE_AND:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) & imm;
			break;

		case TRACEOP_BITWISE_OR:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) | imm;
			break;

		case TRACEOP_BITWISE_XOR:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) ^ imm;
			break;

		case TRACEOP_LEFT_SHIFT:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) << imm;
			break;

		case TRACEOP_RIGHT_SHIFT:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) >> imm;
			break;


		case TRACEOP_LOGICAL_AND:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) && imm;
			break;

		case TRACEOP_LOGICAL_OR:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) || imm;
			break;


		case TRACEOP_EQUAL:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) == imm;
			break;

		case TRACEOP_NOT_EQUAL:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) != imm;
			break;

		case TRACEOP_LESS:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) < imm;
			break;

		case TRACEOP_LESS_EQUAL:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) <= imm;
			break;

		case TRACEOP_GREATER:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) > imm;
			break;

		case TRACEOP_GREATER_EQUAL:
			*eval16(c->slots, c->vars[1]) = *eval16(c->slots, c->vars[0]) >= imm;
			break;
		}

		break;
	}

	
	case 2:
	{
		move();
		uint32_t imm = line;
		
		switch (operation) {
		case TRACEOP_BITWISE_AND:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) & imm;
			break;

		case TRACEOP_BITWISE_OR:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) | imm;
			break;

		case TRACEOP_BITWISE_XOR:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) ^ imm;
			break;

		case TRACEOP_LEFT_SHIFT:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) << imm;
			break;

		case TRACEOP_RIGHT_SHIFT:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) >> imm;
			break;


		case TRACEOP_LOGICAL_AND:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) && imm;
			break;

		case TRACEOP_LOGICAL_OR:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) || imm;
			break;


		case TRACEOP_EQUAL:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) == imm;
			break;

		case TRACEOP_NOT_EQUAL:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) != imm;
			break;

		case TRACEOP_LESS:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) < imm;
			break;

		case TRACEOP_LESS_EQUAL:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) <= imm;
			break;

		case TRACEOP_GREATER:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) > imm;
			break;

		case TRACEOP_GREATER_EQUAL:
			*eval32(c->slots, c->vars[1]) = *eval32(c->slots, c->vars[0]) >= imm;
			break;
		}
		break;
	}


	case 3:
	{
		move();
		uint64_t l = line;
		move();
		uint64_t r = line;
		uint64_t imm = l | (r << 32);
		
		switch (operation) {
		case TRACEOP_BITWISE_AND:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) & imm;
			break;

		case TRACEOP_BITWISE_OR:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) | imm;
			break;

		case TRACEOP_BITWISE_XOR:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) ^ imm;
			break;

		case TRACEOP_LEFT_SHIFT:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) << imm;
			break;

		case TRACEOP_RIGHT_SHIFT:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) >> imm;
			break;


		case TRACEOP_LOGICAL_AND:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) && imm;
			break;

		case TRACEOP_LOGICAL_OR:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) || imm;
			break;


		case TRACEOP_EQUAL:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) == imm;
			break;

		case TRACEOP_NOT_EQUAL:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) != imm;
			break;

		case TRACEOP_LESS:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) < imm;
			break;

		case TRACEOP_LESS_EQUAL:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) <= imm;
			break;

		case TRACEOP_GREATER:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) > imm;
			break;

		case TRACEOP_GREATER_EQUAL:
			*eval64(c->slots, c->vars[1]) = *eval64(c->slots, c->vars[0]) >= imm;
			break;
		}
		break;
	}
	}
}

static void irun_logic_imm_left(Cursor* c, trline_t line) {
	int operation = (line >> 10) & 0xf;
	int psize = (line >> 14) & 0x3;

	switch (psize) {
	case 0:
	{
		uint8_t imm = (line >> 16) & 0xff;
		switch (operation) {
		case TRACEOP_BITWISE_AND:
			*eval8 (c->slots, c->vars[1]) = imm & *eval8(c->slots, c->vars[0]);
			break;

		case TRACEOP_BITWISE_OR:
			*eval8 (c->slots, c->vars[1]) = imm | *eval8(c->slots, c->vars[0]);
			break;

		case TRACEOP_BITWISE_XOR:
			*eval8 (c->slots, c->vars[1]) = imm ^ *eval8(c->slots, c->vars[0]);
			break;

		case TRACEOP_LEFT_SHIFT:
			*eval8 (c->slots, c->vars[1]) = imm << *eval8(c->slots, c->vars[0]);
			break;

		case TRACEOP_RIGHT_SHIFT:
			*eval8 (c->slots, c->vars[1]) = imm >> *eval8(c->slots, c->vars[0]);
			break;


		case TRACEOP_LOGICAL_AND:
			*eval8 (c->slots, c->vars[1]) = imm && *eval8(c->slots, c->vars[0]);
			break;

		case TRACEOP_LOGICAL_OR:
			*eval8 (c->slots, c->vars[1]) = imm || *eval8(c->slots, c->vars[0]);
			break;


		case TRACEOP_EQUAL:
			*eval8 (c->slots, c->vars[1]) = imm == *eval8(c->slots, c->vars[0]);
			break;

		case TRACEOP_NOT_EQUAL:
			*eval8 (c->slots, c->vars[1]) = imm != *eval8(c->slots, c->vars[0]);
			break;

		case TRACEOP_LESS:
			*eval8 (c->slots, c->vars[1]) = imm < *eval8(c->slots, c->vars[0]);
			break;

		case TRACEOP_LESS_EQUAL:
			*eval8 (c->slots, c->vars[1]) = imm <= *eval8(c->slots, c->vars[0]);
			break;

		case TRACEOP_GREATER:
			*eval8 (c->slots, c->vars[1]) = imm > *eval8(c->slots, c->vars[0]);
			break;

		case TRACEOP_GREATER_EQUAL:
			*eval8 (c->slots, c->vars[1]) = imm >= *eval8(c->slots, c->vars[0]);
			break;
		}


		break;
	}


	case 1:
	{
		uint16_t imm = line >> 16;
		switch (operation) {
		case TRACEOP_BITWISE_AND:
			*eval16(c->slots, c->vars[1]) = imm & *eval16(c->slots, c->vars[0]);
			break;

		case TRACEOP_BITWISE_OR:
			*eval16(c->slots, c->vars[1]) = imm | *eval16(c->slots, c->vars[0]);
			break;

		case TRACEOP_BITWISE_XOR:
			*eval16(c->slots, c->vars[1]) = imm ^ *eval16(c->slots, c->vars[0]);
			break;

		case TRACEOP_LEFT_SHIFT:
			*eval16(c->slots, c->vars[1]) = imm << *eval16(c->slots, c->vars[0]);
			break;

		case TRACEOP_RIGHT_SHIFT:
			*eval16(c->slots, c->vars[1]) = imm >> *eval16(c->slots, c->vars[0]);
			break;


		case TRACEOP_LOGICAL_AND:
			*eval16(c->slots, c->vars[1]) = imm && *eval16(c->slots, c->vars[0]);
			break;

		case TRACEOP_LOGICAL_OR:
			*eval16(c->slots, c->vars[1]) = imm || *eval16(c->slots, c->vars[0]);
			break;


		case TRACEOP_EQUAL:
			*eval16(c->slots, c->vars[1]) = imm == *eval16(c->slots, c->vars[0]);
			break;

		case TRACEOP_NOT_EQUAL:
			*eval16(c->slots, c->vars[1]) = imm != *eval16(c->slots, c->vars[0]);
			break;

		case TRACEOP_LESS:
			*eval16(c->slots, c->vars[1]) = imm < *eval16(c->slots, c->vars[0]);
			break;

		case TRACEOP_LESS_EQUAL:
			*eval16(c->slots, c->vars[1]) = imm <= *eval16(c->slots, c->vars[0]);
			break;

		case TRACEOP_GREATER:
			*eval16(c->slots, c->vars[1]) = imm > *eval16(c->slots, c->vars[0]);
			break;

		case TRACEOP_GREATER_EQUAL:
			*eval16(c->slots, c->vars[1]) = imm >= *eval16(c->slots, c->vars[0]);
			break;
		}

		break;
	}

	
	case 2:
	{
		move();
		uint32_t imm = line;
		
		switch (operation) {
		case TRACEOP_BITWISE_AND:
			*eval32(c->slots, c->vars[1]) = imm & *eval32(c->slots, c->vars[0]);
			break;

		case TRACEOP_BITWISE_OR:
			*eval32(c->slots, c->vars[1]) = imm | *eval32(c->slots, c->vars[0]);
			break;

		case TRACEOP_BITWISE_XOR:
			*eval32(c->slots, c->vars[1]) = imm ^ *eval32(c->slots, c->vars[0]);
			break;

		case TRACEOP_LEFT_SHIFT:
			*eval32(c->slots, c->vars[1]) = imm << *eval32(c->slots, c->vars[0]);
			break;

		case TRACEOP_RIGHT_SHIFT:
			*eval32(c->slots, c->vars[1]) = imm >> *eval32(c->slots, c->vars[0]);
			break;


		case TRACEOP_LOGICAL_AND:
			*eval32(c->slots, c->vars[1]) = imm && *eval32(c->slots, c->vars[0]);
			break;

		case TRACEOP_LOGICAL_OR:
			*eval32(c->slots, c->vars[1]) = imm || *eval32(c->slots, c->vars[0]);
			break;


		case TRACEOP_EQUAL:
			*eval32(c->slots, c->vars[1]) = imm == *eval32(c->slots, c->vars[0]);
			break;

		case TRACEOP_NOT_EQUAL:
			*eval32(c->slots, c->vars[1]) = imm != *eval32(c->slots, c->vars[0]);
			break;

		case TRACEOP_LESS:
			*eval32(c->slots, c->vars[1]) = imm < *eval32(c->slots, c->vars[0]);
			break;

		case TRACEOP_LESS_EQUAL:
			*eval32(c->slots, c->vars[1]) = imm <= *eval32(c->slots, c->vars[0]);
			break;

		case TRACEOP_GREATER:
			*eval32(c->slots, c->vars[1]) = imm > *eval32(c->slots, c->vars[0]);
			break;

		case TRACEOP_GREATER_EQUAL:
			*eval32(c->slots, c->vars[1]) = imm >= *eval32(c->slots, c->vars[0]);
			break;
		}
		break;
	}


	case 3:
	{
		move();
		uint64_t l = line;
		move();
		uint64_t r = line;
		uint64_t imm = l | (r << 32);
		
		switch (operation) {
		case TRACEOP_BITWISE_AND:
			*eval64(c->slots, c->vars[1]) = imm & *eval64(c->slots, c->vars[0]);
			break;

		case TRACEOP_BITWISE_OR:
			*eval64(c->slots, c->vars[1]) = imm | *eval64(c->slots, c->vars[0]);
			break;

		case TRACEOP_BITWISE_XOR:
			*eval64(c->slots, c->vars[1]) = imm ^ *eval64(c->slots, c->vars[0]);
			break;

		case TRACEOP_LEFT_SHIFT:
			*eval64(c->slots, c->vars[1]) = imm << *eval64(c->slots, c->vars[0]);
			break;

		case TRACEOP_RIGHT_SHIFT:
			*eval64(c->slots, c->vars[1]) = imm >> *eval64(c->slots, c->vars[0]);
			break;


		case TRACEOP_LOGICAL_AND:
			*eval64(c->slots, c->vars[1]) = imm && *eval64(c->slots, c->vars[0]);
			break;

		case TRACEOP_LOGICAL_OR:
			*eval64(c->slots, c->vars[1]) = imm || *eval64(c->slots, c->vars[0]);
			break;


		case TRACEOP_EQUAL:
			*eval64(c->slots, c->vars[1]) = imm == *eval64(c->slots, c->vars[0]);
			break;

		case TRACEOP_NOT_EQUAL:
			*eval64(c->slots, c->vars[1]) = imm != *eval64(c->slots, c->vars[0]);
			break;

		case TRACEOP_LESS:
			*eval64(c->slots, c->vars[1]) = imm < *eval64(c->slots, c->vars[0]);
			break;

		case TRACEOP_LESS_EQUAL:
			*eval64(c->slots, c->vars[1]) = imm <= *eval64(c->slots, c->vars[0]);
			break;

		case TRACEOP_GREATER:
			*eval64(c->slots, c->vars[1]) = imm > *eval64(c->slots, c->vars[0]);
			break;

		case TRACEOP_GREATER_EQUAL:
			*eval64(c->slots, c->vars[1]) = imm >= *eval64(c->slots, c->vars[0]);
			break;
		}
		break;
	}
	}
}

static void irun_fncall(Cursor* c, trline_t line) {
	raiseError("[TODO] fncalls");
}


static void jumpTo(Cursor* c, int destination) {
	// Binary search accross jumps
	int left = 0;
	int right = c->jmp_len - 1;
	InterpretPosition* const jumps = c->jumps;

	while (left <= right) {
		int mid = left + (right - left) / 2;
		InterpretPosition* jmp = &jumps[mid];
		if (jmp->ins == destination) {
			c->pack = jmp->pack;
			c->ptr = jmp->line;
			return;
		} else if (jmp->ins < destination) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}

	raiseError("[Interpret] Cannot find instruction to jump");
}

static void irun_if(Cursor* c, trline_t line) {
	printf("rmbSize=%d ", c->rememberedSize);
	switch (c->rememberedSize) {
	case 1:
		if (*eval8(c->slots, c->vars[0]))
			return;
		break;

	case 2:
		if (*eval16(c->slots, c->vars[0]))
			return;
		break;
		
	case 4:
		if (*eval32(c->slots, c->vars[0]))
			return;
		break;
	case 8:
		if (*eval64(c->slots, c->vars[0]))
			return;
		break;

	default:
		raiseError("[Interpret] Invalid size of if");
	}

	jumpTo(c, line >> 10);
}

static void irun_jmp(Cursor* c, trline_t line) {
	jumpTo(c, line >> 10);
}

static void irun_cast(Cursor* c, trline_t line) {
	trline_t srcSigned = line & (1<<10);
	trline_t srcFloat  = line & (1<<11);
	trline_t dstSigned = line & (1<<12);
	trline_t dstFloat  = line & (1<<13);
	trline_t srcSize   = (line>>16) & 0x3;
	trline_t dstSize   = (line>>18) & 0x3;

	printf("got %d %d %d %d %d %d\n", srcSigned, srcFloat, dstSigned, dstFloat, srcSize, dstSize);

	raiseError("[TODO] eval irun_cast");
}

static void irun_stackPtr(Cursor* c, trline_t line) {
	trline_t variable = line >> 16;
	move();
	
	*eval64(c->slots, c->vars[0]) = (uint64_t)((void*)&c->slots[variable]) + line;
}

static void irun_memory(Cursor* c, trline_t line) {
	if (line & (1<<10)) {
		trline_t action = (line >> 11) & 0x3;

		if (action == 0) {
			raiseError("[TODO] malloc");
			return;
		}

		if (action == 1) {
			raiseError("[TODO] free");
			return;
		}

		// debug print
		if (action == 2) {
			switch (line >> 13) {
			case 0:
				printf("runtime08: %d\n", (int)*eval8(c->slots, c->vars[0]));
				break;

			case 1:
				printf("runtime16: %d\n", (int)*eval16(c->slots, c->vars[0]));
				break;

			case 2:
				printf("runtime32: %d\n", *eval32(c->slots, c->vars[0]));
				break;

			case 3:
				printf("runtime64: %ld\n", *eval64(c->slots, c->vars[0]));
				break;
			}
		}

		if (action == 3) {
			printf("\n");
			return;
		}

		return;
	}

	raiseError("[TODO] fixed malloc");
}


#undef move