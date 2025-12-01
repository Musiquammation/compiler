#include "Module.h"

#include "helper.h"
#include "globalLabelPool.h"

#include "Syntax.h"

#include "Function.h"
#include "Class.h"

#include <string.h>
#include <dirent.h>
#include <ctype.h>

void Module_create(Module* module) {
	module->scope.type = SCOPE_MODULE;
	module->scope.parent = NULL;
	module->mainFunction = NULL;

	Array_create(&module->classes, sizeof(Class*));
	Array_create(&module->functions, sizeof(Function*));
}

void Module_delete(Module* module) {
	ScopeFile* files = module->files;
	Array_for(ScopeFile, files, module->fileLength, i) {
		ScopeFile_free(i);
	}

	free(files);

	// Delete classes
	Array_loopPtr(Class, module->classes, clPtr) {
		Class* cl = *clPtr;
		Class_delete(cl);
		free(cl);
	}

	Array_free(module->classes);

	// Delete functions
	Array_loopPtr(Function, module->functions, fnPtr) {
		Function* fn = *fnPtr;
		Function_delete(fn);
		free(fn);
	}

	Array_free(module->functions);
}


static bool isValidVariableName(const char* name) {
	if (!name || !name[0]) return false;
	if (isdigit(name[0])) return false;
	for (const char* p = name; *p; p++) {
		if (!isalnum(*p)) return false;
	}
	return true;
}



void Module_generateFilesScopes(Module* module) {
	DIR *d;
	struct dirent *dir;
	const char *path = module->name;

	d = opendir(path);
	if (!d) {
		raiseError("Cannot open directory");
		return;
	}

	Array filesArray;
	Array_create(&filesArray, sizeof(ScopeFile));

	while ((dir = readdir(d)) != NULL) {
		char* filename = dir->d_name;

		if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) continue;

		char* ext = strrchr(filename, '.');
		if (!ext) continue;

		char definitionMode;
		if (strcmp(ext, ".th") == 0) {
			definitionMode = 1;
		} else if (strcmp(ext, ".tc") == 0) {
			definitionMode = -1;
		} else {
			continue;
		}

		size_t nameLen = ext - filename;
		char nameBuf[256];
		if (nameLen >= sizeof(nameBuf)) continue;
		strncpy(nameBuf, filename, nameLen);
		nameBuf[nameLen] = '\0';

		if (!isValidVariableName(nameBuf)) continue;

		// Récupérer le label
		label_t label = LabelPool_push(&_labelPool, nameBuf);
		if (label == _commonLabels._module)
			continue; // skip module

		// Vérifier si on a déjà un ScopeFile pour ce label
		bool found = false;
		Array_loop(ScopeFile, filesArray, sf) {
			if (sf->name == label) {
				// Fusionner : .th + .tc => 0
				sf->definitionMode = 0;
				found = true;
				break;
			}
		}
		if (found) continue;

		// Créer un nouveau ScopeFile
		ScopeFile* sf = Array_push(ScopeFile, &filesArray);
		ScopeFile_create(sf);
		sf->name = label;
		sf->scope.parent = &module->scope;

		size_t pathLen = strlen(path) + 1 + nameLen + 1;
		sf->filepath = malloc(pathLen);
		snprintf(sf->filepath, pathLen, "%s/%s", path, nameBuf);

		sf->definitionMode = definitionMode;
		sf->state_tc = DEFINITIONSTATE_UNDEFINED;
		sf->state_th = DEFINITIONSTATE_UNDEFINED;
	}

	closedir(d);

	// Copier dans module
	module->fileLength = filesArray.length;
	module->files = malloc(sizeof(ScopeFile) * filesArray.length);
	memcpy(module->files, filesArray.data, sizeof(ScopeFile) * filesArray.length);

	Array_free(filesArray);
}



ScopeFile* Module_findModuleFile(Module* module, label_t target) {
	for (int i = 0; i < module->fileLength; i++) {
		ScopeFile* sf = &module->files[i];
		if (sf->name == target) {
			return sf;
		}
	}
	return NULL;
}





void Module_readDeclarations(Module* module) {
	// Search module.th
	int moduleNameLength = strlen(module->name);
	char* const filepath = malloc(moduleNameLength + sizeof("/module.th"));
	memcpy(filepath, module->name, moduleNameLength);
	memcpy(&filepath[moduleNameLength], "/module.th", sizeof("/module.th"));

	Syntax_module(module, filepath);
	free(filepath);
}

void Module_generateDefinitions(Module* module) {
	// Read th classes
	Array_loopPtr(Class, module->classes, clPtr) {
		Class* cl = *clPtr;
		ScopeFile* file = Module_findModuleFile(module, cl->name);

		/// TODO: should we raise an error?
		if (!file) {
			raiseError("[Architecture] Missing class definition file\n");
		}

		if (file->definitionMode >= 0 && file->state_th == DEFINITIONSTATE_UNDEFINED)
			Syntax_thFile(file);
	}

	// Read tc classes
	Array_loopPtr(Class, module->classes, clPtr) {
		Class* cl = *clPtr;
		ScopeFile* file = Module_findModuleFile(module, cl->name);

		/// TODO: should we raise an error?
		if (!file) {
			raiseError("[Architecture] Missing module file\n");
		}


		if (file->definitionMode <= 0 && file->state_tc == DEFINITIONSTATE_UNDEFINED)
			Syntax_tcFile(file);
	}

	// Read th functions
	Array_loopPtr(Function, module->functions, fnPtr) {
		Function* fn = *fnPtr;
		ScopeFile* file = Module_findModuleFile(module, fn->name);

		/// TODO: should we raise an error?
		if (!file) {
			raiseError("[Architecture] Missing module file\n");
		}

		if (file->definitionMode <= 0 && file->state_tc == DEFINITIONSTATE_UNDEFINED)
			Syntax_tcFile(file);
	}
}



Variable* Module_searchVariable(Module* module, label_t name, ScopeSearchArgs* args) {
	// raiseError("[TODO] Module_searchVariable");
	return NULL;
}

Class* Module_searchClass(Module* module, label_t name, ScopeSearchArgs* args) {
	Array_loopPtr(Class, module->classes, ptr) {
		Class* e = *ptr;
		if (e->name == name) {
			if (args) {
				args->resultType = 0;
			}
			return e;
		}
	}

	return NULL;
}

Function* Module_searchFunction(Module* module, label_t name, ScopeSearchArgs* args) {
	Array_loopPtr(Function, module->functions, ptr) {
		Function* e = *ptr;
		if (e->name == name) {
			if (args) {
				args->resultType = 1;
			}
			return e;
		}
	}

	return NULL;
}


void Module_addVariable(Module* module, Variable* v) {
	raiseError("[TODO]");
}

void Module_addClass(Module* module, Class* cl) {
	*Array_push(Class*, &module->classes) = cl;
}

void Module_addFunction(Module* module, Function* fn) {
	*Array_push(Function*, &module->functions) = fn;
}

