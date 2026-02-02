#ifndef COMPILER_TYPE_H_
#define COMPILER_TYPE_H_

#include "mblock_t.h"

#include "Scope.h"
#include "declarations.h"
#include "tools/Array.h"

enum {
	TYPE_CWAY_DEFAULT,
	TYPE_CWAY_ARGUMENT,
};


struct Type {
	Prototype* proto;
	Type* meta;
	mblock_t data;
	Type* reference;
	int refCount;
	char primitiveSizeCode;
};

struct ScopeType {
	Scope scope;
	mblock_t data;
};


void Type_free(Type* type);

void Type_defaultConstructors(
	mblock_t data, Class* meta, ProtoSetting* settings,
	int settingLength, int way, Scope* scope);

void Type_defaultDestructors(mblock_t data, Class* cl);

void Type_defaultCopy(mblock_t dst, const mblock_t src, Class* cl);

Type* Type_deepCopy(Type* root, Prototype* proto, Variable** varr, int varr_len);


Variable* ScopeType_searchVariable(ScopeType* scope, label_t name, ScopeSearchArgs* args);
Class* ScopeType_searchClass(ScopeType* scope, label_t name, ScopeSearchArgs* args);
Function* ScopeType_searchFunction(ScopeType* scope, label_t name, ScopeSearchArgs* args);

void ScopeType_addVariable(ScopeType* scope, Variable* v);
void ScopeType_addClass(ScopeType* scope, Class* cl);
void ScopeType_addFunction(ScopeType* scope, Function* fn);

Type* ScopeType_searchType(ScopeType* scope, Variable* variable);

#endif

