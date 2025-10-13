#include "Variable.h"

#include "Class.h"

#include <tools/Array.h>

void Variable_create(Variable* variable) {

}

void Variable_delete(Variable* variable) {

}

int Variable_getPathOffset(Variable** path, int length) {
    int offset = 0;
    for (int i = 0; i < length; i++) {
        offset += path[i]->offset;
    }
    return offset;
}


