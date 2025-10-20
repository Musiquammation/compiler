#include "Scope.h"


#include "Expression.h"
#include "Module.h"
#include "Variable.h"
#include "Class.h"
#include "primitives.h"
#include "Function.h"

#include "Syntax.h"
#include "helper.h"


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>


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

ScopeFile* Scope_reachFile(Scope* scope) {
	int type = SCOPE_NONE;
	while (true) {
		type = scope->type;
		if (type == SCOPE_FILE)
			return (ScopeFile*)scope;
		
		scope = scope->parent;
		if (scope == NULL) {
			raiseError("[Architecture] Cannot reach file");
			return NULL;
		}
	}
}


bool Scope_defineOnFly(Scope* scope, label_t name) {
	/// TODO: complete this function
	ScopeFile* file = Module_findModuleFile(Scope_reachModule(scope), name);
	if (!file) {
		return false;
	}

	definitionState_t s = file->state_th;
	if (s == DEFINITIONSTATE_READING) {
		raiseError("[Architecture] Trying to read on fly a file being read");
		return false;
	}

	if (s == DEFINITIONSTATE_DONE) {
		raiseError("[Architecture] Trying to read on fly a file already read");
		return false;
	}

	Syntax_thFile(file);
	return true;
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
	file->generationState = SCOPEFILE_GENERATION_NONE;

	Array_create(&file->variables, sizeof(Variable*));
	Array_create(&file->classes, sizeof(Class*));
	Array_create(&file->functions, sizeof(Function*));
}

void ScopeFile_free(ScopeFile* file) {
	free(file->filepath);
	
	switch (file->generationState) {
	case SCOPEFILE_GENERATION_ID:
		free(file->id);
		break;
	}

	Array_loopPtr(Variable, file->variables, v) {
		Variable* o = *v;
		Variable_delete(o);
		free(o);

	}
	
	Array_free(file->variables);
	
	Array_loopPtr(Class, file->classes, c) {
		Class* o = *c;
		Class_delete(o);
		free(o);

	}
	
	Array_free(file->classes);
	
	Array_loopPtr(Function, file->functions, f) {
		Function* o = *f;
		Function_delete(o);
		free(o);

	}
	
	Array_free(file->functions);

}



char* ScopeFile_generateId(ScopeFile* file) {
	const char* fname = file->name;
	const char* moduleName = Scope_reachModule(&file->scope)->name;
	size_t len = strlen(moduleName) + 1   // '_'
			+ strlen(fname) + 1;  // '\0'
	
	char* id = malloc(len);
	sprintf(id, "%s_%s", moduleName, fname);
	return id;
}

char* ScopeFile_requireId(ScopeFile* file, char generationState) {
	if (generationState == SCOPEFILE_GENERATION_NONE) {
		char* id = ScopeFile_generateId(file);
		file->generationState = SCOPEFILE_GENERATION_ID;
		file->id = id;
		return id;
	}

	return file->id;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>


int mkdir_p(const char *path) {
    if (!path || !*path) return -1;

    size_t len = strlen(path);
    char *tmp = malloc(len + 1);
    if (!tmp) return -1;
    strcpy(tmp, path);

    if (len > 0 && tmp[len - 1] == '/')
        tmp[len - 1] = '\0';

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) && errno != EEXIST) {
                free(tmp);
                return -1;
            }
            *p = '/';
        }
    }

    free(tmp);
    return 0;
}



FILE* ScopeFile_requireAssembly(ScopeFile* file, char generationState) {
	switch (generationState) {
	case SCOPEFILE_GENERATION_NONE:
	case SCOPEFILE_GENERATION_ID:
	{
		ScopeFile_requireId(file, generationState);

		// generate filename
		char* binFolder = Scope_reachModule(&file->scope)->binFolder;
		char* filepath = file->filepath;

		size_t len = strlen(binFolder) + strlen(filepath) + 6;

		char* path = malloc(len);

		sprintf(path, "%s/%s.asm", binFolder, filepath);
		mkdir_p(path);

		FILE* f = fopen(path, "w");
		printf("path: %s\n", path);
		free(path);
		
		if (f == NULL) {
			raiseError("[File] Cannot create assembly file");
			return NULL;
		}
		

		// Write text section
		fprintf(f, "section .text\n\n\n");

		file->output = f;
		return f;
	}

	case SCOPEFILE_GENERATION_ASM:
	{
		return file->output;
	}

	}

	raiseError("[Architecture] Cannot require assembly");
	return NULL;
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
	*Array_push(Class*, &file->classes) = cl;
}

void ScopeFile_addFunction(ScopeFile* file, Function* fn) {
	*Array_push(Function*, &file->functions) = fn;
}





