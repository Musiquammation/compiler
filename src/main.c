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

#include "Function.h"
#include "Variable.h"

int main() {
	#define add(n) Variable* n = malloc(sizeof(Variable)); n->name = #n; ScopeFunction_addVariable(&scope, n);
	#define decl(n) Variable real_ ##n = {.name = #n}; Variable* n = &real_ ##n;


	#define COUNT_ARGS(_0,_1,_2,_3, COUNT, ...) COUNT
	#define NUM_ARGS(...) COUNT_ARGS(_, ##__VA_ARGS__, 3, 2, 1, 0)

	// Définition de Path
	typedef struct {
		int length;
		Variable* items[3]; // tableau de pointeurs vers Variable
	} Path;

	// Macros d’expansion
	#define CAT(a,b) a##b
	#define EXPAND_CAT(a,b) CAT(a,b)

	// Macro principale
	#define MAKE_PATH(...) EXPAND_CAT(MAKE_PATH_, NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

	// Déclinaisons suivant le nombre d’arguments
	#define MAKE_PATH_0()        (Path){ .length = 0, .items = {0} }
	#define MAKE_PATH_1(a)       (Path){ .length = 1, .items = {a} }
	#define MAKE_PATH_2(a,b)     (Path){ .length = 2, .items = {a,b} }
	#define MAKE_PATH_3(a,b,c)   (Path){ .length = 3, .items = {a,b,c} }




	decl(numerator);
	decl(denominator);

	decl(left);
	decl(center);
	decl(right);


	Path path;
	TypeNode* bff;

	ScopeFunction scope;
	ScopeFunction_create(&scope);


	add(r1);
	add(r2);
	add(r3);

	add(num1);
	add(num2);

	
	add(a);

	// a.left = r2;
	path = MAKE_PATH(r2);
	bff = TypeNode_get(&scope.rootNode, path.items, path.length);
	path = MAKE_PATH(a, left);
	TypeNode_set(&scope.rootNode, path.items, bff, path.length);

	// print
	path = MAKE_PATH(a, left);
	printf("$A= %p\n", TypeNode_get(&scope.rootNode, path.items, path.length));

	// b = a;
	add(b);
	path = MAKE_PATH(a);
	bff = TypeNode_get(&scope.rootNode, path.items, path.length);
	path = MAKE_PATH(b);
	TypeNode_set(&scope.rootNode, path.items, bff, path.length);

	// a.left = r2
	path = MAKE_PATH(r2);
	bff = TypeNode_get(&scope.rootNode, path.items, path.length);
	path = MAKE_PATH(a, left);
	TypeNode_set(&scope.rootNode, path.items, bff, path.length);

	// print (should be equal)
	path = MAKE_PATH(a, left);
	printf("$B= %p\n", TypeNode_get(&scope.rootNode, path.items, path.length));
	path = MAKE_PATH(b, left);
	printf("$C= %p\n", TypeNode_get(&scope.rootNode, path.items, path.length));
	path = MAKE_PATH(r2);
	printf("$D= %p\n", TypeNode_get(&scope.rootNode, path.items, path.length));


	// b = a;
	path = MAKE_PATH(a);
	bff = TypeNode_get(&scope.rootNode, path.items, path.length);
	path = MAKE_PATH(b);
	TypeNode_set(&scope.rootNode, path.items, bff, path.length);
	
	// print
	path = MAKE_PATH(b, left);
	printf("$E= %p\n", TypeNode_get(&scope.rootNode, path.items, path.length));

	// a = b;
	path = MAKE_PATH(b);
	bff = TypeNode_get(&scope.rootNode, path.items, path.length);
	path = MAKE_PATH(a);
	TypeNode_set(&scope.rootNode, path.items, bff, path.length);
	
	// print
	path = MAKE_PATH(b, left);
	printf("$F= %p\n", TypeNode_get(&scope.rootNode, path.items, path.length));





	printf("===DONE===\n");

	ScopeFunction_delete(&scope);


	#undef add
	#undef decl


	printf("Sucessuly exited!\n");
	return 0;
}

#else

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

	primitives_init();


	module.name = LabelPool_push(&_labelPool, argv[1]);


	Module_generateFilesScopes(&module);
	Module_readDeclarations(&module);
	Module_generateDefinitions(&module);

	Module_delete(&module);
	LabelPool_delete(&_labelPool);
	printf("Sucessuly exited!\n");

	return 0;
}

#endif



// Include C files
#include "Scope.c"

#include "LabelPool.c"
#include "Module.c"
#include "helper.c"


#include "Expression.c"

#include "Function.c"
#include "Variable.c"
#include "Class.c"

#include "Parser.c"

#include "Syntax.c"
#include "globalLabelPool.c"

#include "TypeNode.c"

#include "Prototype.c"
#include "Type.c"

#include "primitives.c"