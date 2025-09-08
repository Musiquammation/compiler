#ifndef COMPILER_SYNTAX_H_
#define COMPILER_SYNTAX_H_

#include <stdbool.h>

#include "declarations.h"
#include "label_t.h"



enum {
	// Function declaration
	SYNTAXDATAFLAG_FN_DCL_REQUIRES_DEFINITION = 1,
	SYNTAXDATAFLAG_FN_DCL_REQUIRES_DECLARATION = 2,
	SYNTAXDATAFLAG_FNçDCL_REQUIRES_NAME = 4,

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

void Syntax_file(ScopeFile* scope);
void Syntax_module(Module* module, const char* filepath);

void Syntax_order(Scope* scope, Parser* parser);
void Syntax_declarationList(Scope* scope, Parser* parser);

void Syntax_classDeclaration(Scope* scope, Parser* parser, int flags, const Syntax_ClassDeclaration* defaultData);
void Syntax_classDefinition(Scope* scope, Parser* parser, Class* cl);

void Syntax_functionDeclaration(Scope* scope, Parser* parser, int flags, const Syntax_FunctionDeclaration* defaultData);
void Syntax_functionArguments(Parser* parser, Function* fn);
bool Syntax_functionDefinition(Scope* scope, Parser* parser, Function* fn);


void Syntax_annotation(Annotation* annotation, Parser* parser, LabelPool* labelPool);

#endif