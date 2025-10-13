#include "Type.h"
#include "Class.h"

#include <string.h>

Type* Type_newCopy(Type* src) {
	if (src->primitiveSizeCode)
		return src;

	Type* dest = malloc(sizeof(Type));
	Class* cl = src->cl;
	dest->cl = cl;
	dest->primitiveSizeCode = 0;


	void* srcData = src->data;
	if (srcData) {
		Class* meta = cl->meta;
		int size = meta->size;
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
