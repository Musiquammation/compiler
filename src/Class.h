#ifndef COMPILER_CLASS_H_
#define COMPILER_CLASS_H_

#include "declarations.h"
#include "definitionState_t.h"

#include "Scope.h"
#include "MemoryCursor.h"

#include "tools/Array.h"

enum {
	CLASSFLAGS_META = 1,

	PSC_UNKNOWN = 42
};

enum {
	CLASSSIZE_UNDEFINED = -1,
	CLASSSIZE_NOEXIST = -2,
	CLASSSIZE_LATER = -3,
	CLASSSIZE_TOSEARCH = -4,
};


struct Class {
	Array variables; // type: Variable*
	Array methods; // type: Function*

	Class* meta;
	label_t name;
	const char* c_name;

	struct {
		Function* fastAccess;
	} std_methods;
	
	definitionState_t definitionState;
	definitionState_t metaDefinitionState;
	char primitiveSizeCode;
	bool isPrimitive;

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

bool Class_getCompleteDefinition(Class* cl, Scope* scope, bool throwError);

char Class_getPrimitiveSizeCode(const Class* cl);
label_t Class_generateMetaName(label_t name, char addChar);

void Class_appendMetas(Class* cl);

void Class_acheiveDefinition(Class* cl);


void ScopeClass_delete(ScopeClass* scope);

Variable* ScopeClass_searchVariable(ScopeClass* scope, label_t name, ScopeSearchArgs* args);
Class* ScopeClass_searchClass(ScopeClass* scope, label_t name, ScopeSearchArgs* args);
Function* ScopeClass_searchFunction(ScopeClass* scope, label_t name, ScopeSearchArgs* args);

void ScopeClass_addVariable(ScopeClass* scope, Variable* v);
void ScopeClass_addClass(ScopeClass* scope, Class* cl);
void ScopeClass_addFunction(ScopeClass* scope, Function* fn);



#endif

