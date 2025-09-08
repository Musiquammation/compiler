#include "helper.h"

#include <stddef.h>

bool isNullPointerRef(const void* ptr) {
	return (*((void**)ptr)) == NULL;
}


void raiseError(const char* reason) {
	printf("Error raised: %s\n", reason);
	int* ptr = NULL;
	int value = *ptr;
}