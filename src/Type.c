#include "Type.h"
#include "Class.h"

Type* Type_newCopy(Type* src) {
    printf("cpy type %s\n", src->cl->name);
    if (src->isPrimitive)
        return src;

    Type* dest = malloc(sizeof(Type));
    dest->cl = src->cl;
    dest->isPrimitive = 0;
}

void Type_free(Type* type) {
    if (type->isPrimitive)
        return;
    
    free(type);
}