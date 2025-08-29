#ifndef COMPILER_SCOPE_H_
#define COMPILER_SCOPE_H_

#include "declarations.h"

#include "Branch.h"
#include "label_t.h"
#include <tools/Array.h>

typedef char scope_t;


struct Scope {
	Scope* parent;
	scope_t type;
	
	Array variables; // type: Variable*
	Array classes; // type: Class*
	Array functions; // type: Function*
	Array properties; // type: Property*
};


void Scope_create(Scope* scope);
void Scope_delete(Scope* scope);

void Scope_pushVariable(Scope* scope, label_t name, const TypeCall* typeCall, Expression* value);




typedef struct {
	Scope scope;
	label_t name;
	char* filename;
} ScopeFile;

typedef struct {
	Scope scope;
	label_t name;
	char* foldername;
} ScopeFolder;


#endif
