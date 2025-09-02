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

void Scope_pushVariable(Scope* scope, label_t name, const TypeCall* typeCall);




struct ScopeFile {
	Scope scope;
	label_t name;
	char* filepath;
	char definitionMode; // 0=all, -1=tc only , +1=th only
};

struct ScopeFolder {
	Scope scope;
	label_t name;
	char* folderpath;
};


enum {
	SCOPE_MODULE,
	SCOPE_FILE,
	SCOPE_FOLDER,
};


void ScopeFile_free(ScopeFile* file);

#endif
