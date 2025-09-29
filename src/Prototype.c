#include "Prototype.h"

#include "Type.h"
#include "primitives.h"


Type* Prototype_generateType(Prototype* proto) {
    if (proto->isPrimitive)
        return primitives_getType(proto->cl);

    Type* type = malloc(sizeof(Type));
    type->cl = proto->cl;
    type->isPrimitive = 0;
    return type;
}


bool Prototype_accepts(const Prototype* proto, const Type* type) {
    /// TODO: Improve this test
    return proto->cl == type->cl;
}

