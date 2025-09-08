#include "Class.h"

#include "Scope.h"

void Class_create(Class* class) {
    class->variables = NULL;
    class->vlength = 0;
}


void Class_delete(Class* class) {
    if (class->variables)
        free(class->variables);
}



