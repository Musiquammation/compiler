#include "Prototype.h"

#include "Type.h"
#include "Variable.h"
#include "primitives.h"
#include "helper.h"

#include <string.h>


static int Prototype_defineSize(Prototype* proto, Class* cl) {
	printf("cl(%p) as %d\n", cl, cl->definitionState);
	if (cl->definitionState == DEFINITIONSTATE_READING) {
		proto->size = CLASSSIZE_LATER;
		proto->maxMinimalSize = CLASSSIZE_LATER;
		proto->meta_size = CLASSSIZE_LATER;
		proto->meta_maxMinimalSize = CLASSSIZE_LATER;
		return CLASSSIZE_LATER;
	}

	int csize = cl->size;
	proto->size = csize;
	proto->maxMinimalSize = cl->maxMinimalSize;

	if (cl->meta) {
		proto->meta_size = cl->meta->size;
		proto->meta_maxMinimalSize = cl->meta->maxMinimalSize;
	} else {
		proto->meta_size = CLASSSIZE_NOEXIST;
		proto->meta_maxMinimalSize = CLASSSIZE_NOEXIST;
	}
}

void Prototype_create(Prototype* proto, Class* cl) {
	proto->cl = cl;
	proto->primitiveSizeCode = cl->primitiveSizeCode;
	Prototype_defineSize(proto, cl);
}

void Prototype_createWithSizeCode(Prototype* proto, Class* cl, char primitiveSizeCode) {
	proto->cl = cl;
	proto->primitiveSizeCode = primitiveSizeCode;
	Prototype_defineSize(proto, cl);
}

void Prototype_delete(Prototype* proto) {

}




Type* Prototype_generateType(Prototype* proto) {
	if (proto->primitiveSizeCode)
		return primitives_getType(proto->cl);

	Type* type = malloc(sizeof(Type));
	Class* cl = proto->cl;
	type->proto = proto;
	type->primitiveSizeCode = proto->primitiveSizeCode;

	Class* meta = cl->meta;
	if (meta) {
		int size = Prototype_getMetaSize(proto);
		if (size > 0) {
			void* data = malloc(size);
			memset(data, 0, size);
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
	return proto->cl == type->proto->cl;
}


int Prototype_getSize(Prototype* proto) {
	int size = proto->size;
	if (size >= 0)
		return size;

	switch (size) {
	case -CLASSSIZE_LATER:
		size = Prototype_defineSize(proto, proto->cl);
		if (size >= 0)
			return size;
		break;
	}

	raiseError("[Architecture] Cannot get the size of an uncomplete type");
	return -1;
}

int Prototype_getMetaSize(Prototype* proto) {
	if (proto->meta_size >= 0)
		return proto->meta_size;
	
	raiseError("[Architecture] Cannot get the size of an uncomplete type");
	return -1;
}

int Prototype_reachSize(Prototype* proto, Scope* scope) {
	int size = proto->size;
	if (size >= 0)
		return size;

	Class* cl = proto->cl;
	printf("got %p %d\n", cl,cl->definitionState);
	definitionState_t state = cl->definitionState;
	if (state == DEFINITIONSTATE_NOT || state == DEFINITIONSTATE_READING) {
		Scope_defineOnFly(scope, cl->name);
		if (cl->definitionState != DEFINITIONSTATE_DONE) {
			raiseError("[Architecture] Cannot define on fly current class");
			return -1;
		}
	}
	
	size = cl->size;
	if (size < 0) {
		raiseError("[Architecture] Cannot get the size of an uncomplete type");
		return -1;
	}
	
	proto->size = size;
	proto->maxMinimalSize = cl->maxMinimalSize;

	Class* meta = cl->meta;
	if (meta) {
		proto->meta_size= meta->size;
		proto->meta_maxMinimalSize = meta->maxMinimalSize;
	} else {
		proto->meta_size = 0;
		proto->meta_maxMinimalSize = 0;
	}
	return size;
}

int Prototype_getSignedSize(Prototype* proto) {
	int s = proto->primitiveSizeCode;
	return s ? s : Prototype_getSize(proto);
}


int Prototype_getGlobalVariableOffset(Prototype* proto, Variable* path[], int length) {
	typedef Variable* var_ptr_t;
	int offset = 0;
	Array_for(var_ptr_t, path, length, vptr) {
		Variable* v = *vptr;
		offset += v->offset;
	}
	
	return offset;
}

int Prototype_getVariableOffset(Variable* path[], int length) {
	if (path[0]->proto.cl->isRegistrable)
		return -1;
	
	return Prototype_getGlobalVariableOffset(&path[0]->proto, &path[1], length-1);
	
}

void Prototype_generateMeta(Prototype* dest, const Prototype* src) {
	/// TODO: complete this method
	dest->cl = src->cl->meta;
	dest->primitiveSizeCode = 0;
}