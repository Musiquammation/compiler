// clear && gcc src/main.c -fsanitize=address -g -o bin/main -ltools && bin/main programm
// clear && gdb bin/main

#include <stdio.h>

#include "Module.h"
#include "LabelPool.h"

#include "Variable.h"
#include "Class.h"

#include "Parser.h"

#include "syntaxList.h"
#include "globalLabelPool.h"

#include "primitives.h"


#define RUNNING_TEST false

#if RUNNING_TEST

int main() {
	return 0;
}

#else

// Test
int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Fatal error: missing module path and output folder\n");
		return 1;
	}

	if (argc < 3) {
		printf("Fatal error: missing output folder\n");
		return 1;
	}

	Module module;
	module.compile = false;
	module.binFolder = argv[2];

	LabelPool_create(&_labelPool);
	
	Module_create(&module);
	CommonLabels_generate(&_commonLabels, &_labelPool);
	syntaxList_init();

	primitives_init();


	module.name = LabelPool_push(&_labelPool, argv[1]);


	Module_generateFilesScopes(&module);
	Module_readDeclarations(&module);
	Module_generateDefinitions(&module);

	Module_delete(&module);
	LabelPool_delete(&_labelPool);
	printf("\nSucessfully exited!\n");

	return 0;
}

#endif


