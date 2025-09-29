#include "Prototype.h"

#include "Type.h"
#include "primitives.h"


Type* Prototype_generateType(Prototype* proto) {
    if (proto->isPrimitive)
        return primitives_getType(proto->cl);

    printf("new type %s\n", proto->cl->name);
    Type* type = malloc(sizeof(Type));
    type->cl = proto->cl;
    type->isPrimitive = 0;
    return type;
}

