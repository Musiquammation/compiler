#pragma once

#include "declarations.h"
#include "label_t.h"
#include "Scope.h"

/**
 * If name==NULL, use argument,
 * else use proto
 */
struct FunctionArgProjection {
	label_t name;

	union {
		Prototype* proto;
		Variable* argument;
		label_t tmpArgName;
	};
};



struct ScopeFunctionArgProjection {
	Scope scope;
	FunctionArgProjection* projections;
	int length;
};



Prototype* ScopeFunctionArgProjection_globalSearchPrototype(Scope* scope, label_t name);