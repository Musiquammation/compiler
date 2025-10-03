#ifndef COMPILER_SYNTAX_H_
#define COMPILER_SYNTAX_H_

#include <stdbool.h>


#include "declarations.h"
#include "label_t.h"



enum {
	// Function declaration
	SYNTAXDATAFLAG_FN_DCL_REQUIRES_DEFINITION = 1,
	SYNTAXDATAFLAG_FN_DCL_REQUIRES_DECLARATION = 2,
	SYNTAXDATAFLAG_FNDCL_REQUIRES_NAME = 4,
	SYNTAXDATAFLAG_FNDCL_FORBID_NAME = 8,

	// Class declaration
	SYNTAXDATAFLAG_CL_DCL_REQUIRES_DEFINITION = 1,
	SYNTAXDATAFLAG_CL_DCL_REQUIRES_DECLARATION = 2,
	SYNTAXDATAFLAG_CL_DCL_REQUIRES_NAME = 4,
};

typedef struct {
	label_t defaultName;	
} Syntax_FunctionDeclaration;

typedef struct {
	label_t defaultName;
} Syntax_ClassDeclaration;

void Syntax_thFile(ScopeFile* scope);
void Syntax_tcFile(ScopeFile* scope);
void Syntax_module(Module* module, const char* filepath);

void Syntax_order(Scope* scope, Parser* parser);
void Syntax_declarationList(Scope* scope, Parser* parser);

void Syntax_classDeclaration(Scope* scope, Parser* parser, int flags, const Syntax_ClassDeclaration* defaultData);
void Syntax_classDefinition(Scope* scope, Parser* parser, Class* cl);

void Syntax_functionDeclaration(Scope* scope, Parser* parser, int flags, const Syntax_FunctionDeclaration* defaultData);
Array Syntax_functionArguments(Scope* scope, Parser* parser);
bool Syntax_functionDefinition(Scope* scope, Parser* parser, Function* fn);

Expression* Syntax_readPath(label_t label, Parser* parser, Scope* scope);
void Syntax_annotation(Annotation* annotation, Parser* parser, LabelPool* labelPool);

#endif

