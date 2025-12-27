#include "Interpreter.h"

enum {
	FLAG_REGISTRABLE = 1
};

void Interpreter_start(TracePack* pack, int varlen) {
	itpr_var_t* const variables = malloc(sizeof(itpr_var_t) * varlen);
	char* const varflags = calloc(1, varlen);

	enum {
		USE_LEN = 32
	};

	uint uses[USE_LEN];
	char usesToDelete[USE_LEN];
	trline_t* linePtr = pack->line;
	int useIdx = 0;


	while (true) {
		trline_t line = *linePtr;
		linePtr++;

		trline_t code = line & 0x3ff;
		// Usage
		if (code <= TRACE_USAGE_LIMIT) {
			uses[useIdx] = line >> 11;
			usesToDelete[useIdx] = (code == 0) ? 1:0;
			useIdx++;
			continue;
		}

		// Instruction
		switch (code) {
		case TRACECODE_STAR:
		{

			break;
		}

		case TRACECODE_CREATE:
		{
 			uint variable = (line >> 16) & 0xfff;
			int size = (*linePtr) >> 16;
			char registrable = (line >> 28) & 1;
			char argument = (line >> 11) & 1;

			char flag = 0;
			if (size <= 8) {
				flag |= FLAG_REGISTRABLE;
			} else {
				variables->ptr = malloc(size);
			}


			varflags[variable] = flag;

			break;
		}

		case TRACECODE_DEF:
		{
			int type = (line >> 10) & 0x3;
			if (type == TRACETYPE_S16) {
				int value = (line >> 16) & 0xFFFF;
			}
			else if (type == TRACETYPE_S8) {
				int value = (line >> 16) & 0xFF;
			}
			else if (type == TRACETYPE_S32) {
				line = *linePtr;
				linePtr++;
			}
			else {
				unsigned long l = *(linePtr++);
				unsigned long r = *(linePtr++);
				unsigned long value = l | (r << 32);
				// variables
			}
			break;
		}

		case TRACECODE_MOVE:
		{

			break;
		}

		case TRACECODE_PLACE:
		{

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
		}

		case TRACECODE_LOGIC_IMM_LEFT:
		{

			break;
		}

		case TRACECODE_LOGIC_IMM_RIGHT:
		{

			break;
		}

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

		case TRACECODE_STACK_PTR:
		{

			break;
		}

		case TRACECODE_LOAD:
		{

			break;
		}

		}


		for (int i = 0; i < useIdx; i++) {
			if (usesToDelete[i])
				free(variables[uses[useIdx]].ptr);
		}

		useIdx = 0;
	}


	free(varflags);
	free(variables);
}
