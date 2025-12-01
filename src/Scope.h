#ifndef COMPILER_SCOPE_H_
#define COMPILER_SCOPE_H_

#include "declarations.h"
#include "globalLabelPool.h"

#include "definitionState_t.h"

#include "label_t.h"

#include <tools/Array.h>
#include <stdio.h>


struct Scope {
	Scope* parent;
	int type;
};






struct ScopeFile {
	Scope scope;
	label_t name;
	char* filepath;
	char definitionMode; // 0=all, -1=tc only , +1=th only
	definitionState_t state_tc;
	definitionState_t state_th;

	char generationState;
	char* id;
	FILE* output;

	Array variables; // type: Variable*
	Array classes; // type: Class*
	Array functions; // type: Function*
};

struct ScopeFolder {
	Scope scope;
	label_t name;
	char* folderpath;
	char readState;
};


enum {
	SCOPE_NONE,
	SCOPE_MODULE,
	SCOPE_FILE,
	SCOPE_FOLDER,
	SCOPE_CLASS,
	SCOPE_FUNCTION,
};


enum {
	SCOPEFILE_GENERATION_NONE,
	SCOPEFILE_GENERATION_ID,
	SCOPEFILE_GENERATION_ASM,
};

struct ScopeSearchArgs {
	int resultType;
};


enum {
	SCOPESEARCH_VARIABLE = 1,
	SCOPESEARCH_CLASS = 2,
	SCOPESEARCH_FUNCTION = 4,
	SCOPESEARCH_TYPE = 8,

	SCOPESEARCH_ANY = SCOPESEARCH_VARIABLE|SCOPESEARCH_CLASS|SCOPESEARCH_FUNCTION|SCOPESEARCH_TYPE,
};


void* Scope_search(Scope* scope, label_t name, ScopeSearchArgs* args, int searchFlags);
Module* Scope_reachModule(Scope* scope);
ScopeFile* Scope_reachFile(Scope* scope);

bool Scope_defineOnFly(Scope* scope, label_t name);


Variable* Scope_searchVariable(Scope* scope, int scopeType, label_t name, ScopeSearchArgs* args);
Class* Scope_searchClass(Scope* scope, int scopeType, label_t name, ScopeSearchArgs* args);
Function* Scope_searchFunction(Scope* scope, int scopeType, label_t name, ScopeSearchArgs* args);

void Scope_addVariable(Scope* scope, int scopeType, Variable* v);
void Scope_addClass(Scope* scope, int scopeType, Class* cl);
void Scope_addFunction(Scope* scope, int scopeType, Function* fn);



void ScopeFile_create(ScopeFile* file);
void ScopeFile_free(ScopeFile* file);

char* ScopeFile_generateId(ScopeFile* file);
char* ScopeFile_requireId(ScopeFile* file, char generationState);
FILE* ScopeFile_requireAssembly(ScopeFile* file, char generationState);

Variable* ScopeFile_searchVariable(ScopeFile* module, label_t name, ScopeSearchArgs* args);
Class* ScopeFile_searchClass(ScopeFile* module, label_t name, ScopeSearchArgs* args);
Function* ScopeFile_searchFunction(ScopeFile* module, label_t name, ScopeSearchArgs* args);

void ScopeFile_addVariable(ScopeFile* file, Variable* v);
void ScopeFile_addClass(ScopeFile* file, Class* cl);
void ScopeFile_addFunction(ScopeFile* file, Function* fn);




#endif
