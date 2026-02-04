#pragma once

#include "declarations.h"
#include "globalLabelPool.h"

#include "definitionState_t.h"

#include "label_t.h"

#include "tools/Array.h"
#include <stdio.h>


struct Scope {
	Scope* parent;
	int type;
};


enum {
	SCOPEFILEDEFFLAGS_TH = 1,
	SCOPEFILEDEFFLAGS_TL = 2,
	SCOPEFILEDEFFLAGS_TC = 4,
};

enum {
	SCOPEADDFN_OP_DEFAULT = 0,
	SCOPEADDFN_OP_CONSTRUCTOR = 1,
};



struct ScopeFile {
	Scope scope;
	label_t name;
	char* filepath;
	char definitionMode;
	definitionState_t state_th;
	definitionState_t state_tl;
	definitionState_t state_tc;

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


enum {SCOPE_POOL_NUM = 2};

typedef struct {
	Scope scope;
	Scope* scopes[SCOPE_POOL_NUM];
} ScopePool;


enum {
	SCOPE_NONE,
	SCOPE_MODULE,
	SCOPE_FILE,
	SCOPE_FOLDER,
	SCOPE_POOL,
	SCOPE_CLASS,
	SCOPE_FUNCTION,
	SCOPE_TYPE,
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
Type* Scope_searchType(Scope* scope, Variable* variable);


void Scope_addVariable(Scope* scope, int scopeType, Variable* v);
void Scope_addClass(Scope* scope, int scopeType, Class* cl);
void Scope_addFunction(Scope* scope, int scopeType, int flags, Function* fn);



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
void ScopeFile_addFunction(ScopeFile* file, Function* fn, int flags);



Variable* ScopePool_searchVariable(ScopePool* file, label_t name, ScopeSearchArgs* args);
Class* ScopePool_searchClass(ScopePool* file, label_t namel, ScopeSearchArgs* args);
Function* ScopePool_searchFunction(ScopePool* file, label_t namen, ScopeSearchArgs* args);

void ScopePool_addVariable(ScopePool* file, Variable* v);
void ScopePool_addClass(ScopePool* file, Class* cl);
void ScopePool_addFunction(ScopePool* file, Function* fn, int flags);




