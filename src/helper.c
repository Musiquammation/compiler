#include "helper.h"

#include <stddef.h>

bool isNullPointerRef(const void* ptr) {
	return (*((void**)ptr)) == NULL;
}


void raiseError() {
	printf("Error raised.\n");
	*(int*)NULL; // generate error
}