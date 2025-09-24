#include "TypeNode.h"

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
	Array_for(variable_t, path, pathLength, vPtr) {
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
			child->length = 0;
			child->usage = 1;

			TypeNode_pushPair(node, variable, child);
			node = child;
		}
		
		
		pairFound:
	}

	return node;
}


bool TypeNode_set(
	TypeNode* node,
	Variable* path[],
	TypeNode* value,
	int pathLength,
	bool copyValue
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
				if (copyValue) {
					/// TODO: check copy
					TypeNode_unfollow(node, 2);
					node->value = value->value;
					node->length = value->length; // should be < 0
					node->usage = -1;
					return true;
				}
				
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
				if (copyValue) {
					raiseError("[TODO]: copyValue");
					goto pairFound;
				}

				TypeNode_pushPair(node, variable, value);
				value->usage++;

				return true;
			}

			TypeNode* child = malloc(sizeof(TypeNode));
			printf("new %p for %p\n", node, child);
			child->length = 0;
			child->usage = 1;

			TypeNode_pushPair(node, variable, child);
			node = child;
		}
		
		
		pairFound:{}

	}

	


	return node;
}


void TypeNode_copy(TypeNode* dest, TypeNode* src) {
	int length = src->length;
	
	// Quick copy
	dest->usage = 1;
	dest->length = length;
	dest->value = src->value;

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


		if (mode == 0)
			free(node);
		return;
	}

	node->usage = usage - 1;
}





