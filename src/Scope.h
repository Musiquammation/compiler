#ifndef COMPILER_SCOPE_H_
#define COMPILER_SCOPE_H_

typedef char scope_t;

#include <tools/Array.h>
#include "Expression.h"

typedef struct {
	Scope* parent;
	scope_t type;
	
	Array variables; // type: Variable
	Array classes; // type: Class
	Array function; // type: Function

	Array properties; // type: expressionPtr_t
	Array types; // type: expressionPtr_t
} Scope;


void Scope_create(Scope* scope);
void Scope_delete(Scope* scope);

void Scope_appendVariable(Scope* scope, Expression* expression);
void Scope_appendClass(Scope* scope, Expression* expression);
void Scope_appendFunction(Scope* scope, Expression* expression);




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
