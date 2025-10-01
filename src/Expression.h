#ifndef COMPILER_EXPRESSION_H_
#define COMPILER_EXPRESSION_H_

#include "declarations.h"
#include "label_t.h"



struct Expression {
	int type;

	union {
		Function* method;
		char* string;

		char i8;
		unsigned char u8;

		short i16;
		unsigned short u16;

		int i32;
		unsigned int u32;

		float f32;
		double f64;

		long i64;
		unsigned long u64;

		struct {
			Expression* left;
			Expression* right;
		} operands;

		Expression* operand;

		Expression* target;

		struct {
			Variable** variableArr;
			int length;
			bool next;
		} property;
	} data;
};



enum {
	EXPRESSION_NONE,
	EXPRESSION_INVALID,
	EXPRESSION_GROUP,

	EXPRESSION_PROPERTY,
	EXPRESSION_PATH,

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


#endif
