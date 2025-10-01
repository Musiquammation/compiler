#ifndef COMPILER_TYPENODE_H_
#define COMPILER_TYPENODE_H_

#include "declarations.h"
#include "Expression.h"

#include <tools/Array.h>


enum {
	TYPENODE_ALIGN = 4,
};

typedef struct {
	Variable* defined;
	TypeNode* node;
} TypeNodePair;




struct TypeNode {
	TypeNodePair* pairs;

	int length; // length < 0 for number ; length >= 0 for a node
	int usage;
	

	union {
		Type* type;

		char i8;
		unsigned char u8;

		short i16;
		unsigned u16;

		int i32;
		unsigned u32;

		float f32;
		double f64;

		long i64;
		unsigned long u64;
	} value;
};



enum {
	TYPENODE_LENGTH_I8 = -127,
	TYPENODE_LENGTH_UNDEF_I8,

	TYPENODE_LENGTH_U8,
	TYPENODE_LENGTH_UNDEF_U8,
	
	TYPENODE_LENGTH_I16,
	TYPENODE_LENGTH_UNDEF_I16,

	TYPENODE_LENGTH_U16,
	TYPENODE_LENGTH_UNDEF_U16,

	TYPENODE_LENGTH_I32,
	TYPENODE_LENGTH_UNDEF_I32,
	
	TYPENODE_LENGTH_U32,
	TYPENODE_LENGTH_UNDEF_U32,
	
	TYPENODE_LENGTH_I64,
	TYPENODE_LENGTH_UNDEF_I64,

	TYPENODE_LENGTH_U64,
	TYPENODE_LENGTH_UNDEF_U64,

	TYPENODE_LENGTH_F32,
	TYPENODE_LENGTH_UNDEF_F32,
	
	TYPENODE_LENGTH_F64,
	TYPENODE_LENGTH_UNDEF_F64,

	
};




void TypeNode_pushPair(
	TypeNode* node,
	Variable* defined,
	TypeNode* value
);

TypeNode* TypeNode_get(
	TypeNode* node,
	Variable* path[],
	int pathLength
);

TypeNode* TypeNode_tryReach(
	TypeNode* node,
	Variable* path[],
	int pathLength
);

bool TypeNode_set(
	TypeNode* node,
	Variable* path[],
	TypeNode* value,
	int pathLength
);

TypeNode* TypeNode_push(TypeNode* root, Variable* v, Expression* value);

void TypeNode_copyValue(TypeNode* dest, const TypeNode* src, int srcLength);

void TypeNode_copy(TypeNode* dest, const TypeNode* src);

bool TypeNode_fillValue(TypeNode* node, Prototype* proto);

int TypeNode_getPrimitiveLength(const Class* primitiveClass, bool isDefined);

bool TypeNode_checkProto(Prototype* prototype, TypeNode* node);

/**
 * mode=0 : free
 * mode=1 : unfollow children directly
 * mode=2 : never free
 */
void TypeNode_unfollow(TypeNode* node, char mode);




#endif

