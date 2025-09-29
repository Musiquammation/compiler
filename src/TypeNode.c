#include "TypeNode.h"

#include "Class.h"
#include "Type.h"
#include "Prototype.h"
#include "Variable.h"
#include "helper.h"
#include <string.h>



void TypeNode_pushPair(
	TypeNode* node,
	Variable* defined,
	TypeNode* value
) {
	int length = node->length;
	TypeNodePair* pairs;

	if (length == 0) {
		pairs = malloc(TYPENODE_ALIGN * sizeof(TypeNodePair));
		node->pairs = pairs;
		
	} else if ((length % TYPENODE_ALIGN) == 0) {
		TypeNodePair* last = node->pairs;
		size_t size = length * sizeof(TypeNodePair);
		pairs = malloc(size + TYPENODE_ALIGN * sizeof(TypeNodePair));
		memcpy(pairs, last, size);
		free(last);

		node->pairs = pairs;
	} else {
		pairs = node->pairs;
	}

	pairs[length].defined = defined;
	pairs[length].node = value;
	node->length = length + 1;
}




TypeNode* TypeNode_get(
	TypeNode* node,
	Variable* path[],
	int pathLength
) {
	// Search sub node
	typedef Variable* variable_t;
	for (
		variable_t *vPtr = path,
		*const vPtr_end = vPtr + pathLength - 1;
		vPtr <= vPtr_end;
		vPtr++
	) {
		variable_t variable = *vPtr;
		
		TypeNodePair* pairs = node->pairs;
		if (pairs == NULL)
			return NULL;

		// Search variable
		Array_for(TypeNodePair, pairs, node->length, pair) {
			if (pair->defined == variable) {
				node = pair->node;
				goto pairFound;
			}
		}

		// If here, not found so let's create a TypeNode
		{
			TypeNode* child = malloc(sizeof(TypeNode));
			child->usage = 1;
			
			TypeNode_fillValue(child, &variable->proto);
			TypeNode_pushPair(node, variable, child);
			node = child;

			if (vPtr == vPtr_end)
				return node; // skip fillValue

		}
		
		
		pairFound:
	}

	return node;
}


bool TypeNode_set(
	TypeNode* node,
	Variable* path[],
	TypeNode* value,
	int pathLength
) {	
	typedef Variable* variable_t;


	// Search sub node
	for (
		variable_t *vPtr = path,
		*const vPtr_end = vPtr + pathLength - 1;
		vPtr <= vPtr_end;
		vPtr++
	) {
		variable_t variable = *vPtr;
		
		int nodeLength = node->length;
		if (nodeLength == 0)
			goto pairNotFound;

		// Search 
		Array_for(TypeNodePair, node->pairs, nodeLength, pair) {
			if (pair->defined != variable)
				continue;
			
			node = pair->node;

			if (vPtr == vPtr_end) {				
				TypeNode_unfollow(node, 0);
				pair->node = value;
				value->usage++;

				return true;
			}
			
			int usage = node->usage;
			if (usage > 1) {
				node->usage = usage - 1;
				TypeNode* copy = malloc(sizeof(TypeNode));
				TypeNode_copy(copy, node);
				pair->node = copy;
				node = copy;
			}

			goto pairFound;
		}

		pairNotFound:

		{
			if (vPtr == vPtr_end) {
				TypeNode_pushPair(node, variable, value);
				value->usage++;

				return true;
			}

			TypeNode* child = malloc(sizeof(TypeNode));
			TypeNode_fillValue(child, &variable->proto);
			child->usage = 1;

			TypeNode_pushPair(node, variable, child);
			node = child;
		}
		
		
		pairFound:{}

	}

	


	return node;
}


TypeNode* TypeNode_push(TypeNode* root, Variable* v, Expression* value) {
	TypeNode* node = malloc(sizeof(TypeNode));
	node->usage = 0;

	if (v->proto.isPrimitive) {
		node->length = TypeNode_getPrimitiveLength(v->proto.cl, value != NULL);
		if (value) {
			/// TODO: set value
		}

	} else {
		node->length = 0;
		if (value) {

		} else {
			Type* type = Prototype_generateType(&v->proto);
			node->value.type = type;
		}
	}

	TypeNode_set(root, &v, node, 1);
	

	return node;
}


void TypeNode_copyValue(TypeNode* dest, const TypeNode* src, int length) {
	dest->length = length;

	// Copy type
	if (length >= 0) {
		dest->value.type = Type_newCopy(src->value.type);
		return;
	}



	switch (length) {
	TYPENODE_LENGTH_I8:  dest->value.i8 = src->value.i8; break; 
	TYPENODE_LENGTH_U8:  dest->value.u8 = src->value.u8; break;
	TYPENODE_LENGTH_I16: dest->value.i16 = src->value.i16; break;
	TYPENODE_LENGTH_U16: dest->value.u16 = src->value.u16; break;
	TYPENODE_LENGTH_I32: dest->value.i32 = src->value.i32; break;
	TYPENODE_LENGTH_U32: dest->value.u32 = src->value.u32; break;
	TYPENODE_LENGTH_I64: dest->value.i64 = src->value.i64; break;
	TYPENODE_LENGTH_U64: dest->value.u64 = src->value.u64; break;
	TYPENODE_LENGTH_F32: dest->value.f32 = src->value.f32; break;
	TYPENODE_LENGTH_F64: dest->value.f64 = src->value.f64; break;

	TYPENODE_LENGTH_UNDEF_I8:
	TYPENODE_LENGTH_UNDEF_U8:
	TYPENODE_LENGTH_UNDEF_I16:
	TYPENODE_LENGTH_UNDEF_U16:
	TYPENODE_LENGTH_UNDEF_I32:
	TYPENODE_LENGTH_UNDEF_U32:
	TYPENODE_LENGTH_UNDEF_I64:
	TYPENODE_LENGTH_UNDEF_U64:
	TYPENODE_LENGTH_UNDEF_F32:
	TYPENODE_LENGTH_UNDEF_F64:
		break;
	}
}



void TypeNode_copy(TypeNode* dest, const TypeNode* src) {
	int length = src->length;
	
	// Quick copy
	dest->usage = 1;
	TypeNode_copyValue(dest, src, length);

	if (length <= 0)
		return;

	// Handle pairs
	int lengthToAllow = length % TYPENODE_ALIGN;
	lengthToAllow = length == 0 ? length : length + TYPENODE_ALIGN - length;

	TypeNodePair* destPairs = malloc(lengthToAllow * sizeof(TypeNodePair));
	TypeNodePair* srcPairs = src->pairs;

	dest->pairs = destPairs;

	for (int i = 0; i < length; i++) {
		TypeNode* node = srcPairs[i].node;
		destPairs[i].defined = srcPairs[i].defined;
		destPairs[i].node = node;
		node->usage++;
	}
}

void TypeNode_fillValue(TypeNode* node, Prototype* proto) {
	if (proto->isPrimitive) {
		node->length = TypeNode_getPrimitiveLength(proto->cl, false);
		return;
	}

	printf("fill\n");
	Type* type = Prototype_generateType(proto);
	node->value.type = type;
	node->length = 0;
}



int TypeNode_getPrimitiveLength(const Class* primitiveClass, bool isDefined) {
	int result;
	switch (primitiveClass->isPrimitive) {
	case  1: result = TYPENODE_LENGTH_I8;  break;
	case  2: result = TYPENODE_LENGTH_U8;  break;
	case  3: result = TYPENODE_LENGTH_I16; break;
	case  4: result = TYPENODE_LENGTH_U16; break;
	case  5: result = TYPENODE_LENGTH_I32; break;
	case  6: result = TYPENODE_LENGTH_U32; break;
	case  7: result = TYPENODE_LENGTH_I64; break;
	case  8: result = TYPENODE_LENGTH_U64; break;
	case  9: result = TYPENODE_LENGTH_F32; break;
	case 10: result = TYPENODE_LENGTH_F64; break;

	default:
		return -1;
	}


	if (!isDefined) {
		result++;
	}

	return result;
}



void TypeNode_unfollow(TypeNode* node, char mode) {
	int usage;
	if (mode == 1) {
		goto unfollowChildren;
	}
	

	usage = node->usage;

	if (usage == 1) {
		unfollowChildren:

		// Unfollow children
		int childrenLength = node->length;
		if (childrenLength > 0) {
			TypeNodePair* pairs = node->pairs;
			Array_for(TypeNodePair, pairs, childrenLength, i) {
				TypeNode_unfollow(i->node, 0);
			}

			free(pairs);
		}


		// Free node
		if (mode == 0) {
			printf("free %d %p\n", node->length, node->value.type);
			if (childrenLength >= 0)
				Type_free(node->value.type);

			free(node);
		}
		return;
	}

	node->usage = usage - 1;
}





