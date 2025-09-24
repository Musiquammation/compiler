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
	int usage; // -1 for a number
	

	union {
		Variable* v;
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

bool TypeNode_set(
	TypeNode* node,
	Variable* path[],
	TypeNode* value,
	int pathLength,
	bool copyValue
);


void TypeNode_copy(TypeNode* dest, TypeNode* src);


/**
 * mode=0 : free
 * mode=1 : unfollow children directly
 * mode=2 : never free
 */
void TypeNode_unfollow(TypeNode* node, char mode);




#endif
