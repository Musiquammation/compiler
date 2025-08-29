#ifndef COMPILER_EXPRESSION_H_
#define COMPILER_EXPRESSION_H_

#include "declarations.h"
#include "Type.h"
#include "label_t.h"

#include <tools/Array.h>

typedef Expression* expressionPtr_t;
typedef char expression_t;

struct Expression {
	expression_t type;
	int followerCount;
	Array followers; // type: expressionPtr_t
};



#define Expression_cast(expr) ((Expression*)(expr))

void Expression_create(expression_t type, Expression* e);
void Expression_delete(expression_t type, Expression* e);
void Expression_free(expression_t type, Expression* e);

void Expression_follow(Expression* follower, Expression* followed);
void Expression_unfollow(Expression* follower, Expression* followed);

void Expression_followAsNull(Expression* followed);
void Expression_unfollowAsNull(expression_t type, Expression* followed);


int Expression_getFollowingExpressionNumber(expression_t type, Expression* e);
void Expression_collectFollowedExpressions(expression_t type, Expression* e, expressionPtr_t buffer[]);






enum {
	EXPRESSION_NONE,
	EXPRESSION_INVALID,

	EXPRESSION_ARRAY,
	EXPRESSION_VARIABLE,
	EXPRESSION_PROPERTY,
	EXPRESSION_CLASS,
	EXPRESSION_FUNCTION,
	EXPRESSION_TYPE,


	EXPRESSION_CHAR,
	EXPRESSION_BOOL,
	EXPRESSION_SHORT,
	EXPRESSION_INT,
	EXPRESSION_LONG,
	EXPRESSION_FLOAT,
	EXPRESSION_DOUBLE,
	EXPRESSION_STRING,
	EXPRESSION_LABEL,

	EXPRESSION_ADDITION,        // a + b
	EXPRESSION_SUBSTRACTION,    // a - b
	EXPRESSION_MULTIPLICATION,  // a * b
	EXPRESSION_DIVISION,        // a / b
	EXPRESSION_MODULO,          // a % b

	EXPRESSION_NEGATION,        // -a
	EXPRESSION_INCREMENT,       // a + 1
	EXPRESSION_DECREMENT,       // a - 1
	EXPRESSION_BITWISE_NOT,     // ~a
	EXPRESSION_LOGICAL_NOT,     // !a

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
	EXPRESSION_GREATER_EQUAL    // a >= b
};




structdef(ExpressionNone);
structdef(ExpressionInvalid);
structdef(ExpressionArray);
structdef(ExpressionProperty);
structdef(ExpressionChar);
structdef(ExpressionBool);
structdef(ExpressionShort);
structdef(ExpressionInt);
structdef(ExpressionLong);
structdef(ExpressionFloat);
structdef(ExpressionDouble);
structdef(ExpressionString);
structdef(ExpressionLabel);
structdef(ExpressionNegation);
structdef(ExpressionIncrement);
structdef(ExpressionDecrement);
structdef(ExpressionBitwiseNot);
structdef(ExpressionLogicalNot);
structdef(ExpressionAddition);
structdef(ExpressionSubstraction);
structdef(ExpressionMultiplication);
structdef(ExpressionDivision);
structdef(ExpressionModulo);
structdef(ExpressionBitwiseAnd);
structdef(ExpressionBitwiseOr);
structdef(ExpressionBitwiseXor);
structdef(ExpressionLeftShift);
structdef(ExpressionRightShift);
structdef(ExpressionLogicalAnd);
structdef(ExpressionLogicalOr);
structdef(ExpressionEqual);
structdef(ExpressionNotEqual);
structdef(ExpressionLess);
structdef(ExpressionLessEqual);
structdef(ExpressionGreater);
structdef(ExpressionGreaterEqual);






struct ExpressionNone {
	Expression prefix;
};

struct ExpressionInvalid {
	Expression prefix;
};

struct ExpressionArray {
	Expression prefix;
	Array expressions; // type: expressionPtr_t
};

struct ExpressionProperty {
	Expression prefix;
	Type type;
	ExpressionProperty* parent;
	Expression* value;
	const Variable* variable;
};





struct ExpressionChar {
	Expression prefix;
	char value;
};

struct ExpressionBool {
	Expression prefix;
	_Bool value;
};

struct ExpressionShort {
	Expression prefix;
	short value;
};

struct ExpressionInt {
	Expression prefix;
	int value;
};

struct ExpressionLong {
	Expression prefix;
	long value;
};

struct ExpressionFloat {
	Expression prefix;
	float value;
};

struct ExpressionDouble {
	Expression prefix;
	double value;
};

struct ExpressionString {
	Expression prefix;
	char* value;
};

struct ExpressionLabel {
	Expression prefix;
	label_t label;
};





struct ExpressionNegation {
	Expression prefix;
	Expression* expr;
};

struct ExpressionIncrement {
	Expression prefix;
	Expression* expr;
};

struct ExpressionDecrement {
	Expression prefix;
	Expression* expr;
};

struct ExpressionBitwiseNot {
	Expression prefix;
	Expression* expr;
};

struct ExpressionLogicalNot {
	Expression prefix;
	Expression* expr;
};



struct ExpressionAddition {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionSubstraction {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionMultiplication {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionDivision {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionModulo {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionBitwiseAnd {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionBitwiseOr {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionBitwiseXor {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionLeftShift {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionRightShift {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionLogicalAnd {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionLogicalOr {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionEqual {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionNotEqual {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionLess {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionLessEqual {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionGreater {
	Expression prefix;
	Expression* left;
	Expression* right;
};

struct ExpressionGreaterEqual {
	Expression prefix;
	Expression* left;
	Expression* right;
};





#endif