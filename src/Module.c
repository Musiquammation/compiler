#include "Module.h"

#include "Syntax.h"
#include "helper.h"

#include <string.h>
#include <dirent.h>
#include <ctype.h>

void Module_create(Module* module) {
	module->scope.type = SCOPE_MODULE;
	Scope_create(&module->scope);
}

void Module_delete(Module* module) {
	Scope_delete(&module->scope);

	ScopeFile* files = module->files;
	Array_for(ScopeFile, files, module->fileLength, i) {
		ScopeFile_free(i);
	}

	free(files);
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
		raiseError();
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
		sf->name = label;
		Scope_create(&sf->scope);
		sf->scope.type = SCOPE_FILE;

		size_t pathLen = strlen(path) + 1 + nameLen + 1;
		sf->filepath = malloc(pathLen);
		snprintf(sf->filepath, pathLen, "%s/%s", path, nameBuf);

		sf->definitionMode = definitionMode;
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
	// Search module.dh
	ScopeFile* sf = Module_findModuleFile(module, _commonLabels._module);
	if (!sf || sf->definitionMode != 1) {
		raiseError();
		return;
	}

	
	Syntax_file(sf);
}