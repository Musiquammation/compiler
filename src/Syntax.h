#ifndef COMPILER_SYNTAX_H_
#define COMPILER_SYNTAX_H_

#include <stdbool.h>

#include "declarations.h"
#include "label_t.h"



typedef struct {
    bool initialized;
    label_t defaultName;
    
} Syntax_FunctionDeclaration;

void Syntax_file(ScopeFile* scope);

void Syntax_order(Scope* scope, Parser* parser);
void Syntax_declarationList(Scope* scope, Parser* parser);

Function* Syntax_functionDeclaration(Scope* scope, Parser* parser, const Syntax_FunctionDeclaration* defaultData);
bool Syntax_functionDefinition(Scope* scope, Parser* parser);


#endif