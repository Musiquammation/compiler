#include "Type.h"
#include "Class.h"
#include "Prototype.h"

#include <string.h>

Type* Type_newCopy(Type* src) {
	if (src->primitiveSizeCode)
		return src;

	Type* dest = malloc(sizeof(Type));
	Prototype* proto = src->proto;
	dest->proto = proto;
	dest->primitiveSizeCode = 0;


	void* srcData = src->data;
	if (srcData) {
		size_t size = Prototype_getMetaSizes(proto).size;
		void* data = malloc(size);
		memcpy(data, src->data, size);
		dest->data = data;
	} else {
		dest->data = NULL;
	}

}


void Type_free(Type* type) {
	if (type->primitiveSizeCode)
		return;
	
	if (type->data)
		free(type->data);
		
	free(type);
}
