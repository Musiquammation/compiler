// clear && gcc src/main.c -fsanitize=address -g -o bin/main -ltools && bin/main programm
// clear && gdb bin/main

#include <stdio.h>

#include "Module.h"
#include "LabelPool.h"

#include "Variable.h"
#include "Class.h"

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
int main() {
	Scope scope;
	Scope_create(&scope);


	Class classInt;
	classInt.name = "int";
	Class_create(&classInt);

	Class class;
	class.name = "Point";
	Class_create(&class);

	Variable* point_x_var = malloc(sizeof(Variable));
	Variable* point_y_var = malloc(sizeof(Variable));

	TypeCall typeInt = { .class = &classInt };

	point_x_var->name = "x";
	point_x_var->typeCall = typeInt;

	point_y_var->name = "y";
	point_y_var->typeCall = typeInt;


	*Array_push(Variable*, &class.variables) = point_x_var;
	*Array_push(Variable*, &class.variables) = point_y_var;


	TypeCall type = { .class = &class };
	Scope_pushVariable(&scope, "point", &type);



	Scope_delete(&scope);

	Class_delete(&class);
	Class_delete(&classInt);

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