#ifndef COMPILER_CLASS_H_
#define COMPILER_CLASS_H_

#include "declarations.h"
#include "definitionState_t.h"

#include "Scope.h"
#include "MemoryCursor.h"

#include <tools/Array.h>

enum {
	CLASSFLAGS_META = 1
};

enum {
	CLASSSIZE_UNDEFINED = -1,
	CLASSSIZE_NOEXIST = -2,
	CLASSSIZE_LATER = -3,
	CLASSSIZE_TOSEARCH = -4,
};

enum {
	REGISTRABLE_UNKNOWN = -1,
	REGISTRABLE_FALSE = 0,
	REGISTRABLE_TRUE = 1,
};

struct Class {
	Array variables; // type: Variable*
	Array functions; // type: Function*

	Class* meta;
	definitionState_t definitionState;
	definitionState_t metaDefinitionState;
	label_t name;
	
	char primitiveSizeCode;
	char isRegistrable;

	int size;
	int maxMinimalSize;
};

struct ScopeClass {
	Scope scope;
	Class* cl;
	bool allowThis;
};

void Class_create(Class* cl);
void Class_delete(Class* cl);


Class* Class_appendMeta(Class* cl, Class* meta, bool containsMetaArguments);

void ScopeClass_delete(ScopeClass* scope);

Variable* ScopeClass_searchVariable(ScopeClass* scope, label_t name, ScopeSearchArgs* args);
Class* ScopeClass_searchClass(ScopeClass* scope, label_t name, ScopeSearchArgs* args);
Function* ScopeClass_searchFunction(ScopeClass* scope, label_t name, ScopeSearchArgs* args);

void ScopeClass_addVariable(ScopeClass* scope, Variable* v);
void ScopeClass_addClass(ScopeClass* scope, Class* cl);
void ScopeClass_addFunction(ScopeClass* scope, Function* fn);



#endif