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

/*
int main() {
	LabelPool labelPool;
	LabelPool_create(&labelPool);
	
	Module module;
	label_t moduleName = LabelPool_push(&labelPool, "moduleName");
	Module_create(&module, moduleName);


	Module_delete(&module);
	LabelPool_delete(&labelPool);

	printf("Hello World!\n");
	return 0;
}
*/



// Test
int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Fatal error: missing module path\n");
		return 1;
	}


	Module module;

	LabelPool_create(&_labelPool);
	
	Module_create(&module);
	CommonLabels_generate(&_commonLabels, &_labelPool);
	syntaxList_init();

	module.name = LabelPool_push(&_labelPool, argv[1]);


	Module_generateFilesScopes(&module);
	Module_readDeclarations(&module);
	Module_generateDefinitions(&module);

	Module_delete(&module);
	LabelPool_delete(&_labelPool);
	printf("Sucessuly exited!\n");

	return 0;
}





// Include C files
#include "Scope.c"
#include "Branch.c"

#include "LabelPool.c"
#include "Module.c"
#include "helper.c"

#include "TypeCall.c"

#include "Expression.c"
#include "Property.c"
#include "Type.c"

#include "Function.c"
#include "Variable.c"
#include "Class.c"

#include "Parser.c"

#include "Syntax.c"
#include "globalLabelPool.c"
