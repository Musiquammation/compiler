#ifndef COMPILER_EXPRESSION_H_
#define COMPILER_EXPRESSION_H_

typedef char expression_t;

#include <tools/Array.h>



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

	EXPRESSION_KNOWN_CHAR,
	EXPRESSION_KNOWN_BOOL,
	EXPRESSION_KNOWN_SHORT,
	EXPRESSION_KNOWN_INT,
	EXPRESSION_KNOWN_LONG,
	EXPRESSION_KNOWN_FLOAT,
	EXPRESSION_KNOWN_DOUBLE,
	EXPRESSION_KNOWN_STRING,
	EXPRESSION_KNOWN_LABEL,

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






typedef struct {
	expression_t type;
	int followerCount;
	Array followers; // type: expressionPtr_t
} Expression;

typedef Expression* expressionPtr_t;

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














typedef struct {
	Expression prefix;
} ExpressionNone;

typedef struct {
	Expression prefix;
} ExpressionInvalid;

typedef struct {
	Expression prefix;
	Array expressions; // type: expressionPtr_t
} ExpressionArray;

/// TODO: remove ExpressionVariable ?
typedef struct {
	Expression prefix;
} ExpressionVariable;

typedef struct {
	Expression prefix;
} ExpressionProperty;

/// TODO: remove ExpressionClass ?
typedef struct {
	Expression prefix;
} ExpressionClass;

/// TODO: remove ExpressionFunction ?
typedef struct {
	Expression prefix;
} ExpressionFunction;

typedef struct {
	Expression prefix;
} ExpressionType;




typedef struct {
	Expression prefix;
	char value;
} ExpressionChar;

typedef struct {
	Expression prefix;
	_Bool value;
} ExpressionBool;

typedef struct {
	Expression prefix;
	short value;
} ExpressionShort;

typedef struct {
	Expression prefix;
	int value;
} ExpressionInt;

typedef struct {
	Expression prefix;
	long value;
} ExpressionLong;

typedef struct {
	Expression prefix;
	float value;
} ExpressionFloat;

typedef struct {
	Expression prefix;
	double value;
} ExpressionDouble;

typedef struct {
	Expression prefix;
	char* value;
} ExpressionString;

typedef struct {
	Expression prefix;
	char* value;
} ExpressionLabel;



typedef struct {
	Expression prefix;
	char value;
} ExpressionKnownChar;

typedef struct {
	Expression prefix;
	bool value;
} ExpressionKnownBool;

typedef struct {
	Expression prefix;
	short value;
} ExpressionKnownShort;

typedef struct {
	Expression prefix;
	int value;
} ExpressionKnownInt;

typedef struct {
	Expression prefix;
	long value;
} ExpressionKnownLong;

typedef struct {
	Expression prefix;
	float value;
} ExpressionKnownFloat;

typedef struct {
	Expression prefix;
	double value;
} ExpressionKnownDouble;

typedef struct {
	Expression prefix;
	char* value;
} ExpressionKnownString;

typedef struct {
	Expression prefix;
	char* value;
} ExpressionKnownLabel;



typedef struct {
	Expression prefix;
	Expression* expr;
} ExpressionNegation;

typedef struct {
	Expression prefix;
	Expression* expr;
} ExpressionIncrement;

typedef struct {
	Expression prefix;
	Expression* expr;
} ExpressionDecrement;

typedef struct {
	Expression prefix;
	Expression* expr;
} ExpressionBitwiseNot;

typedef struct {
	Expression prefix;
	Expression* expr;
} ExpressionLogicalNot;



typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionAddition;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionSubstraction;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionMultiplication;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionDivision;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionModulo;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionBitwiseAnd;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionBitwiseOr;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionBitwiseXor;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionLeftShift;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionRightShift;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionLogicalAnd;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionLogicalOr;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionEqual;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionNotEqual;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionLess;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionLessEqual;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionGreater;

typedef struct {
	Expression prefix;
	Expression* left;
	Expression* right;
} ExpressionGreaterEqual;








#endif
