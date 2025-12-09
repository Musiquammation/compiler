#ifndef COMPILER_TYPE_H_
#define COMPILER_TYPE_H_

#include "declarations.h"
#include <tools/Array.h>

typedef void* mblock_t;

struct Type {
	Prototype* proto;
	Type* meta;
	mblock_t data;
	Type* reference;
	int refCount;
	char primitiveSizeCode;
};

void Type_free(Type* type);


void Type_defaultConstructors(
	void* data, Class* cl, ProtoSetting* settings,
	int settingLength, Type meta, Scope* scope);

void Type_defaultDestructors(void* data, Type* meta);


#endif
