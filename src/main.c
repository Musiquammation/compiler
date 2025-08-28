// clear && gcc src/main.c -o bin/main -ltools && bin/main programm

#include <stdio.h>

#include "Module.h"
#include "LabelPool.h"

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






// Include C files
#include "Expression.c"
#include "Scope.c"
#include "LabelPool.c"
#include "Module.c"
#include "helper.c"

#include "Function.c"
#include "Variable.c"
#include "Class.c"
