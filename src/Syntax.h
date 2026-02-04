#pragma once


#include <stdbool.h>


#include "declarations.h"
#include "label_t.h"
#include "tools/Array.h"


enum {
	// Function declaration
	SYNTAXDATAFLAG_FNDCL_REQUIRES_DEFINITION = 1<<0,
	SYNTAXDATAFLAG_FNDCL_REQUIRES_DECLARATION = 1<<1,
	SYNTAXDATAFLAG_FNDCL_REQUIRES_NAME = 1<<2,
	SYNTAXDATAFLAG_FNDCL_FORBID_NAME = 1<<3,
	SYNTAXDATAFLAG_FNDCL_FORBID_RETURN = 1<<4,
	SYNTAXDATAFLAG_FNDCL_FAST_ACCESS = 1<<5,
	SYNTAXDATAFLAG_FNDCL_NO_META = 1<<6,
	SYNTAXDATAFLAG_FNDCL_STD_FAST_ACCESS = 1<<7,
	SYNTAXDATAFLAG_FNDCL_REQUIRES_RETURN = 1<<8,
	SYNTAXDATAFLAG_FNDCL_THIS = 1<<9,
	SYNTAXDATAFLAG_FNDCL_CONSTRUCTOR = 1<<10,
	SYNTAXDATAFLAG_FNDCL_ARGUMENT_CONSTRUCTOR = 1<<11,
	SYNTAXDATAFLAG_FNDCL_TEST = 1<<12,
	SYNTAXDATAFLAG_FNDCL_CONDITION = 1<<13,


	// Class declaration
	SYNTAXDATAFLAG_CLDCL_REQUIRES_DEFINITION = 1<<0,
	SYNTAXDATAFLAG_CLDCL_REQUIRES_DECLARATION = 1<<1,
	SYNTAXDATAFLAG_CLDCL_REQUIRES_NAME = 1<<2,
};

typedef struct {
	label_t defaultName;	
	Class* definedClass;
	int stdBehavior;
	int args_len;
	Variable** args;
	Array* requireList; // type: labelRequireCouple_t
} Syntax_FunctionDeclarationArg;

typedef struct {
	label_t defaultName;
} Syntax_ClassDeclarationArg;

typedef struct {
	Array controlFunctions; // type: label_t
} Syntax_ClassDefinitionArg;



void Syntax_thFile(ScopeFile* scope);
void Syntax_tcFile(ScopeFile* scope);
void Syntax_tlFile(ScopeFile* scope);
void Syntax_module(Module* module, const char* filepath);

void Syntax_order(Scope* scope, Parser* parser);
void Syntax_declarationList(Scope* scope, Parser* parser);

void Syntax_classDeclaration(Scope* scope, Parser* parser, int flags, const Syntax_ClassDeclarationArg* defaultData);
void Syntax_classDefinition(Scope* scope, Parser* parser, Class* cl, Syntax_ClassDefinitionArg* cdefs);
Prototype* Syntax_proto(Parser* parser, Scope* scope);

Function* Syntax_functionDeclaration(Scope* scope, Scope* variadicScope, Parser* parser, int flags, const Syntax_FunctionDeclarationArg* defaultData);
Array Syntax_functionArgumentsDecl(Scope* scope, Parser* parser);
bool Syntax_functionDefinition(Scope* scope, Parser* parser, Function* fn, Class* thisclass);
int Syntax_functionScope(ScopeFunction* scope, Trace* trace, Parser* parser);


/**
 * Returns an expression which type can be:
 * - `EXPRESSION_VALUE`
 * - `EXPRESSION_PROPERTY`
 * - `EXPRESSION_FNCALL`
 */
Expression* Syntax_readPath(label_t label, Parser* parser, Scope* scope);
void Syntax_annotation(Annotation* annotation, Parser* parser, LabelPool* labelPool);

