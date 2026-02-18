#include <string.h>

#include "Variable.h"

#include "Class.h"

#include "tools/Array.h"

void Variable_create(Variable* variable) {
	variable->meta = NULL;
}


void Variable_delete(Variable* variable) {
	Prototype_free(variable->proto);
}

