#ifndef COMPILER_EXPRESSION_H_
#define COMPILER_EXPRESSION_H_

#include "declarations.h"
#include "label_t.h"
#include "castable_t.h"



struct Expression {
	int type;

	union {
		Function* method;
		char* string;

		castable_t num;

		struct {
			Expression* left;
			Expression* right;
		} operands;

		struct {
			Expression* op;
			bool toFree;
		} operand;

		Expression* target;

		struct {
			void* value;
			Prototype* proto;
		} value;

		struct {
			Expression** args;
			Function* fn;
			int args_len;
			int argsStartIndex;
		} fncall;

		struct {
			Variable** variableArr;
			Expression* origin;
			int args_len;
			bool freeVariableArr;
		} property;

		struct {
			Function* accessor;
			Expression* origin;
		} fastAccess;

		Expression* linked;
	} data;
};



enum {
	EXPRESSION_NONE,
	EXPRESSION_INVALID,
	EXPRESSION_GROUP,

	EXPRESSION_VALUE,
	EXPRESSION_PROPERTY,
	EXPRESSION_FNCALL,
	EXPRESSION_FAST_ACCESS,
	EXPRESSION_LINK,


	EXPRESSION_U8,
	EXPRESSION_I8,

	EXPRESSION_U16,
	EXPRESSION_I16,

	EXPRESSION_U32,
	EXPRESSION_I32,
	EXPRESSION_F32,

	EXPRESSION_U64,
	EXPRESSION_I64,
	EXPRESSION_F64,
	
	EXPRESSION_INTEGER,
	EXPRESSION_FLOATING,
	
	EXPRESSION_STRING,

	EXPRESSION_ADDITION,        // a + b
	EXPRESSION_SUBSTRACTION,    // a - b
	EXPRESSION_MULTIPLICATION,  // a * b
	EXPRESSION_DIVISION,        // a / b
	EXPRESSION_MODULO,          // a % b
	
	EXPRESSION_BITWISE_AND,     // a & b
	EXPRESSION_BITWISE_OR,      // a | b
	EXPRESSION_BITWISE_XOR,     // a ^ b
	EXPRESSION_LEFT_SHIFT,      // a << b
	EXPRESSION_RIGHT_SHIFT,     // a >> b

	EXPRESSION_LOGICAL_AND,     // a && b
	EXPRESSION_LOGICAL_OR,      // a || b

	EXPRESSION_EQUAL,           // a == b
	EXPRESSION_NOT_EQUAL,       // a != b
	EXPRESSION_LESS,            // a < b
	EXPRESSION_LESS_EQUAL,      // a <= b
	EXPRESSION_GREATER,         // a > b
	EXPRESSION_GREATER_EQUAL,   // a >= b
	
	EXPRESSION_BITWISE_NOT,     // ~a
	EXPRESSION_LOGICAL_NOT,     // !a
	EXPRESSION_POSITIVE,        // +a
	EXPRESSION_NEGATION,        // -a
	EXPRESSION_R_INCREMENT,     // a++
	EXPRESSION_R_DECREMENT,     // a--
	EXPRESSION_L_INCREMENT,     // ++a
	EXPRESSION_L_DECREMENT,     // --a
	EXPRESSION_A_INCREMENT,     // a++ or ++a (used temporarily)
	EXPRESSION_A_DECREMENT,     // a-- or --a (used temporarily)
	
	EXPRESSION_ADDR_OF,         // &a
	EXPRESSION_VALUE_AT,        // *a
};

enum {
	EXPRSIZE_INTEGER = 0x7ffffffe,
	EXPRSIZE_FLOATING = 0x7fffffff
};


void Expression_free(int type, Expression* e);

void Expression_exchangeReferences(
	Expression* expr,	
	const Expression* base,
	Expression* results,
	int length
);

bool Expression_followsOperatorPlace(int operatorPlace, int type);

Expression* Expression_processLine(Expression* line, int length);

bool Expression_canSimplify(int type, int op1, int op2);

int Expression_simplify(
	int type,
	int type_op1,
	int type_op2,
	Expression* target,
	const Expression* left,
	const Expression* right
);


int Expression_getSignedSize(int exprType);
int Expression_reachSignedSize(int type, const Expression* expr);



Expression* Expression_crossTyped(int type, Expression* expr);
Expression* Expression_cross(Expression* expr);

Prototype* Expression_getPrimitiveProtoFromType(int type);
Type* Expression_getPrimitiveTypeFromType(int type);
Prototype* Expression_getPrimitiveProtoFromSize(int type);


#endif
