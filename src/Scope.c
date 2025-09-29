#include "Scope.h"

#include "Expression.h"
#include "Module.h"
#include "Variable.h"
#include "Class.h"
#include "Function.h"

#include "Syntax.h"
#include "helper.h"


#include <stdio.h>



void Scope_delete(Scope* scope, int scopeType) {
	switch (scopeType) {
	case SCOPE_MODULE:
		Module_delete((Module*)scope);
		break;

	case SCOPE_FILE:
		ScopeFile_free((ScopeFile*)scope);
		break;

	case SCOPE_CLASS:
		ScopeClass_delete((ScopeClass*)scope);
		break;

	case SCOPE_FUNCTION:
		ScopeFunction_delete((ScopeFunction*)scope);
		break;
	}
}







void* Scope_search(Scope* scope, label_t name, ScopeSearchArgs* args, int searchFlags) {
	// Search primitives
	if (searchFlags & SCOPESEARCH_CLASS) {
		Class* cl = primitives_getClass(name);
		if (cl) {
			if (args) {
				args->resultType = SCOPESEARCH_CLASS;
			}
			return cl;
		}
	}

	// Search object
	while (scope) {
		const int scopeType = scope->type;

		if (searchFlags & SCOPESEARCH_VARIABLE) {
			Variable* variable = Scope_searchVariable(scope, scopeType, name, args);
			if (variable) {
				if (args) {
					args->resultType = SCOPESEARCH_VARIABLE;
				}

				return variable;
			}
		}

		if (searchFlags & SCOPESEARCH_CLASS) {
			Class* cl = Scope_searchClass(scope, scopeType, name, args);
			if (cl) {
				if (args) {
					args->resultType = SCOPESEARCH_CLASS;
				}
				return cl;
			}
		}

		if (searchFlags & SCOPESEARCH_FUNCTION) {
			Function* fn = Scope_searchFunction(scope, scopeType, name, args);
			if (fn) {
				if (args) {
					args->resultType = SCOPESEARCH_FUNCTION;
				}
				return fn;
			}
		}

		scope = scope->parent;
	}

	return NULL;
}




Module* Scope_reachModule(Scope* scope) {
	Scope* parent = scope->parent;
	while (parent) {
		scope = parent;
		parent = scope->parent;
	}

	if (scope->type != SCOPE_MODULE) {
		raiseError("[Architecture] Root scope should be a module");
		return NULL;
	}

	return (Module*)scope;
}


void Scope_defineOnFly(Scope* scope, label_t name) {
	/// TODO: complete this function
	ScopeFile* file = Module_findModuleFile(Scope_reachModule(scope), name);
	if (!file) {
		raiseError("[Architecture] Missing definition file\n");
		return;
	}

	definitionState_t s = file->state_th;
	if (s == DEFINITIONSTATE_READING) {
		raiseError("[Architecture] Trying to read on fly a file being read");
		return;
	}

	if (s == DEFINITIONSTATE_DONE) {
		raiseError("[Architecture] Trying to read on fly a file already read");
		return;
	}

	printf("Define on fly\n");

	Syntax_thFile(file);
}






Variable* Scope_searchVariable(Scope* scope, int scopeType, label_t name, ScopeSearchArgs* args) {
	switch (scopeType) {
	case SCOPE_MODULE:
		return Module_searchVariable((Module*)scope, name, args);
	
	case SCOPE_FILE:
		return ScopeFile_searchVariable((ScopeFile*)scope, name, args);

	case SCOPE_CLASS:
		return ScopeClass_searchVariable((ScopeClass*)scope, name, args);

	case SCOPE_FUNCTION:
		return ScopeFunction_searchVariable((ScopeFunction*)scope, name, args);

	}

	return NULL;
}

Class* Scope_searchClass(Scope* scope, int scopeType, label_t name, ScopeSearchArgs* args) {
	switch (scopeType) {
	case SCOPE_MODULE:
		return Module_searchClass((Module*)scope, name, args);
	
	case SCOPE_FILE:
		return ScopeFile_searchClass((ScopeFile*)scope, name, args);

	case SCOPE_CLASS:
		return ScopeClass_searchClass((ScopeClass*)scope, name, args);

	case SCOPE_FUNCTION:
		return ScopeFunction_searchClass((ScopeFunction*)scope, name, args);

	}

	return NULL;
}

Function* Scope_searchFunction(Scope* scope, int scopeType, label_t name, ScopeSearchArgs* args) {
	switch (scopeType) {
	case SCOPE_MODULE:
		return Module_searchFunction((Module*)scope, name, args);
	
	case SCOPE_FILE:
		return ScopeFile_searchFunction((ScopeFile*)scope, name, args);

	case SCOPE_CLASS:
		return ScopeClass_searchFunction((ScopeClass*)scope, name, args);

	case SCOPE_FUNCTION:
		return ScopeFunction_searchFunction((ScopeFunction*)scope, name, args);

	}

	return NULL;
}


void Scope_addVariable(Scope* scope, int scopeType, Variable* v) {
	switch (scopeType) {
	case SCOPE_MODULE:
		Module_addVariable((Module*)scope, v);
		break;
	
	case SCOPE_FILE:
		ScopeFile_addVariable((ScopeFile*)scope, v);
		break;

	case SCOPE_CLASS:
		ScopeClass_addVariable((ScopeClass*)scope, v);
		break;

	}
}

void Scope_addClass(Scope* scope, int scopeType, Class* cl) {
	switch (scopeType) {
	case SCOPE_MODULE:
		Module_addClass((Module*)scope, cl);
		break;
	
	case SCOPE_FILE:
		ScopeFile_addClass((ScopeFile*)scope, cl);
		break;

	case SCOPE_CLASS:
		ScopeClass_addClass((ScopeClass*)scope, cl);
		break;

	}
}

void Scope_addFunction(Scope* scope, int scopeType, Function* fn) {
	switch (scopeType) {
	case SCOPE_MODULE:
		Module_addFunction((Module*)scope, fn);
		break;
	
	case SCOPE_FILE:
		ScopeFile_addFunction((ScopeFile*)scope, fn);
		break;

	case SCOPE_CLASS:
		ScopeClass_addFunction((ScopeClass*)scope, fn);
		break;

	}
}







void ScopeFile_create(ScopeFile* file) {
	file->scope.type = SCOPE_FILE;
	Array_create(&file->variables, sizeof(Variable*));
	Array_create(&file->classes, sizeof(Class*));
	Array_create(&file->functions, sizeof(Function*));
}

void ScopeFile_free(ScopeFile* file) {
	free(file->filepath);
	
	Array_loopPtr(Variable, file->variables, v)
		Variable_delete(*v);
	
	Array_free(file->variables);
	

	Array_loopPtr(Class, file->classes, c)
		Class_delete(*c);
	
	Array_free(file->classes);
	
	Array_loopPtr(Function, file->functions, f)
		Function_delete(*f);
	
	Array_free(file->functions);

}




Variable* ScopeFile_searchVariable(ScopeFile* file, label_t name, ScopeSearchArgs* args) {
	Array_loopPtr(Variable, file->variables, ptr) {
		Variable* e = *ptr;
		if (e->name == name) {
			return e;
		}
	}

	return NULL;
}

Class* ScopeFile_searchClass(ScopeFile* file, label_t name, ScopeSearchArgs* args) {
	Array_loopPtr(Class, file->classes, ptr) {
		Class* e = *ptr;
		if (e->name == name) {
			return e;
		}
	}

	return NULL;

}

Function* ScopeFile_searchFunction(ScopeFile* file, label_t name, ScopeSearchArgs* args) {
	Array_loopPtr(Function, file->functions, ptr) {
		Function* e = *ptr;
		if (e->name == name) {
			return e;
		}
	}

	return NULL;
}



void ScopeFile_addVariable(ScopeFile* file, Variable* v) {
	raiseError("[TODO]");
}

void ScopeFile_addClass(ScopeFile* file, Class* cl) {
	raiseError("[TODO]");
}

void ScopeFile_addFunction(ScopeFile* file, Function* fn) {
	raiseError("[TODO]");
}





