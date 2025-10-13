#include "Prototype.h"

#include "Type.h"
#include "primitives.h"


Type* Prototype_generateType(Prototype* proto) {
	if (proto->primitiveSizeCode)
		return primitives_getType(proto->cl);

	Type* type = malloc(sizeof(Type));
	Class* cl = proto->cl;
	type->cl = cl;
	type->primitiveSizeCode = proto->primitiveSizeCode;

	Class* meta = cl->meta;
	if (meta) {
		int size = meta->size;
		if (size > 0) {
			void* data = malloc(size);
			type->data = data;
		} else {
			type->data = NULL;
		}
	} else {
		type->data = NULL;
	}
	return type;
}


bool Prototype_accepts(const Prototype* proto, const Type* type) {
	/// TODO: Improve this test
	return proto->cl == type->cl;
}


int Prototype_getSignedSize(const Prototype* proto) {
	int s = proto->primitiveSizeCode;
	return s ? s : proto->cl->size;
}
