#include "Variable.h"

void Variable_create(Variable* variable) {

}

void Variable_delete(Variable* variable) {

}

int Variable_getOffsetPath(Variable** path, int length) {
    int offset = 0;
    for (int i = 0; i < length; i++) {
        offset += path[i]->offset;
    }
    return offset;
}
