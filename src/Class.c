#include "Class.h"


void Class_create(Class* class) {
    Array_create(&class->variables, sizeof(Variable*));
}


void Class_delete(Class* class) {
	Array_loopPtr(void, class->variables, i) {free(*i);}

    Array_free(class->variables);
}

