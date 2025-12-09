#include "Prototype.h"


#include "ScopeBuffer.h"
#include "Type.h"
#include "Variable.h"
#include "Expression.h"
#include "Function.h"

#include "primitives.h"
#include "helper.h"
#include "langstd.h"

#include <string.h>


#define setMode(m) {proto->state = m;}

static ExtendedPrototypeSize Prototype_defineSize_direct(Prototype* proto, Class* cl) {
	if (cl->definitionState == DEFINITIONSTATE_UNDEFINED || cl->definitionState == DEFINITIONSTATE_READING) {
		proto->direct.sizes.size = CLASSSIZE_LATER;
		proto->direct.sizes.maxMinimalSize = CLASSSIZE_LATER;
		proto->direct.primitiveSizeCode = PSC_UNKNOWN; // unknown
		return (ExtendedPrototypeSize){CLASSSIZE_LATER, CLASSSIZE_LATER, PSC_UNKNOWN};
	}

	printf("direct %s %d\n", cl->name, cl->size);

	if (cl->size >= 0) {
		ExtendedPrototypeSize sizes = {cl->size, cl->maxMinimalSize, cl->primitiveSizeCode};
		proto->direct.sizes.size = sizes.size;
		proto->direct.sizes.maxMinimalSize = sizes.maxMinimalSize;
		proto->direct.primitiveSizeCode = cl->primitiveSizeCode;
		return sizes;
	}
	// proto->direct.sizes = sizes;
	raiseError("[TODO] do here right now!");
}

Prototype* Prototype_create_direct(
	Class* cl,
	char primitiveSizeCode,
	ProtoSetting* settings,
	int settingLength
) {
	Prototype* proto = malloc(sizeof(Prototype));
	setMode(PROTO_MODE_DIRECT);
	printf("new %p to %s\n", proto, cl->name);
	proto->direct.cl = cl;
	proto->direct.primitiveSizeCode = cl->primitiveSizeCode;
	if (settingLength) {
		proto->direct.settings = settings;
		proto->direct.settingLength = settingLength;
		proto->direct.settingsMustBeFreed = true;
	} else {
		proto->direct.settingLength = 0;
		proto->direct.settingsMustBeFreed = false;
	}
	
	proto->direct.hasMeta = cl->metaDefinitionState == DEFINITIONSTATE_NOEXIST ? 0 : -1;
	Prototype_defineSize_direct(proto, cl);
	
	return proto;
}


Prototype* Prototype_create_meta(Prototype* origin, Class* meta) {
	Prototype* mp = Prototype_create_direct(
		meta, meta->primitiveSizeCode, NULL, -1
	);
	mp->direct.origin = origin;

	return mp;
}

Prototype* Prototype_create_reference(Variable** varr, int varrLength) {
	Prototype* proto = malloc(sizeof(Prototype));
	setMode(PROTO_MODE_REFERENCE);
	proto->ref.varr = varr;
	proto->ref.varrLength = varrLength;

	Prototype* rp = varr[varrLength-1]->proto;
	proto->ref.proto = rp;

	Prototype_addUsage(*rp);
	return proto;
}
	
Prototype* Prototype_create_variadic(Variable* ref) {
	Prototype* proto = malloc(sizeof(Prototype));
	setMode(PROTO_MODE_VARIADIC);
	proto->variadic.ref = ref;

	return proto;

}


void Prototype_free(Prototype* proto, bool deep) {
	int state = proto->state;
	printf("fre %p / %d, %d\n", proto, state >> 8, state & 0xff);

	if (state >> 8) {
		proto->state = (((state >> 8) - 1) << 8) | (state & 0xff);
		return;
	}
	
	switch (state & 0xff) {
	case PROTO_MODE_REFERENCE:
		printf("from %p\n", proto);
		Prototype_free(proto->ref.proto, deep);
		free(proto->ref.varr);
		free(proto);
		break;
	
	case PROTO_MODE_DIRECT:
		if (proto->direct.settingsMustBeFreed) {
			ProtoSetting* settings = proto->direct.settings;
			int slength = proto->direct.settingLength;
			Array_for(ProtoSetting, settings, slength, setting) {
				if (setting->useProto) {
					Prototype_free(setting->proto, true);
				} else {
					Expression_free(setting->expr->type, setting->expr);
				}
			}

			if (slength > 0)
				free(settings);
		}
		
	
		if (deep) {
			char hasMeta = proto->direct.hasMeta;

			if (hasMeta == 1) {
				Prototype_free(proto->direct.meta, deep);
			}
		}

		free(proto);
		break;

	case PROTO_MODE_VARIADIC:
		free(proto);
		break;

	case PROTO_MODE_PRIMITIVE:
		break;
	}
}


static char direct_hasMeta(Prototype* proto) {
	char hasMeta = proto->direct.hasMeta;
	if (hasMeta == -1) {
		switch (proto->direct.cl->metaDefinitionState) {
		case DEFINITIONSTATE_UNDEFINED:
			return -1;
		
		case DEFINITIONSTATE_READING:
			return -1;
		
		case DEFINITIONSTATE_DONE:
		{
			proto->direct.meta = Prototype_create_meta(
				proto, proto->direct.cl->meta);

			proto->direct.hasMeta = 1;
			return 1;
		}
		
		case DEFINITIONSTATE_NOEXIST:
			proto->direct.hasMeta = 0;
			return 0;

		}
	}

	return hasMeta;
}




Type* Prototype_generateType(Prototype* proto, Scope* scope) {
	switch (Prototype_mode(*proto)) {
	case PROTO_MODE_REFERENCE:
	{
		if (!scope) {
			raiseError("[Architecture] Prototype reference requires a scope");
			return NULL;
		}

		Variable** varr = proto->ref.varr;
		Type* origin = ScopeFunction_globalSearchType(scope, varr[0]);
		if (!origin) {
			raiseError("[Architecture] Refered type not found");
		}

		int varr_len = proto->ref.varrLength;
		if (varr_len == 1) {
			origin->refCount++;
			return origin;
		}

		// Generate another type with data decaled
		int varr_sublen = varr_len - 1;
		Type* newType = malloc(sizeof(Type));
		Variable* lastVariable = varr[varr_sublen];
		Type* meta = origin->meta;

		printf("lastwas %p %s\n", newType, lastVariable->name);
		newType->proto = lastVariable->proto;
		newType->meta = meta;
		newType->data = origin->data + Prototype_getVariableOffset(&varr[1], varr_sublen);
		newType->reference = origin;
		newType->refCount = 0;
		newType->primitiveSizeCode = 0;

		origin->refCount++;
		if (meta) {
			// meta->refCount++;
		}

		return newType;
	}
	
	case PROTO_MODE_DIRECT:
	{
		// Handle meta
		char hasMeta = direct_hasMeta(proto);
		if (hasMeta == -1) {
			raiseError("[Architecture] Cannot create a type with unknown meta");
			return NULL;
		}

		Type* type = malloc(sizeof(Type));
		printf("NTYPE %p %s\n", type, proto->direct.cl->name);
		Class* cl = proto->direct.cl;
		type->proto = proto;
		type->reference = 0;
		type->refCount = 0;
		type->primitiveSizeCode = 0;

		Prototype_addUsage(*proto);
		
		if (hasMeta == 0) {
			type->data = NULL;
			type->meta = NULL;
			return type;
		}


		Prototype* meta = proto->direct.meta;
		Type* metaType = Prototype_generateType(meta, scope);
		type->meta = metaType;

		ExtendedPrototypeSize sizes = Prototype_getSizes(meta);
		if (sizes.size > 0) {
			printf("newfor %p slen=%d\n", type, proto->direct.settingLength);
			void* data = calloc(1, sizes.size);
			memset(data, 0, sizes.size);
			type->data = data;

			// Append data if necessary
			Type_defaultConstructors(
				data,
				meta->direct.cl,
				proto->direct.settings,
				proto->direct.settingLength,
				*metaType,
				scope
			);

		} else {
			type->data = NULL;
		}

		return type;
	}

	case PROTO_MODE_VARIADIC:
	{
		raiseError("[TODO] Prototype_generateType");
		return NULL;
	}

	case PROTO_MODE_PRIMITIVE:
	{
		return primitives_getType(proto->primitive.cl);
	}
	}

	raiseError("[Intern] Invalid proto mode");
	
}


bool Prototype_accepts(const Prototype* proto, const Type* type) {
	/// TODO: Improve this test
	// return proto->cl == type->proto->cl;
	return true;
}





ExtendedPrototypeSize Prototype_getSizes(Prototype* proto) {
	switch (Prototype_mode(*proto)) {
	case PROTO_MODE_REFERENCE:
	{
		raiseError("[TODO] get direct size");
		break;
	}
	case PROTO_MODE_DIRECT:
	{
		int size = proto->direct.sizes.size;
		if (size >= 0) {
			return (ExtendedPrototypeSize){
				proto->direct.sizes.size,
				proto->direct.sizes.maxMinimalSize,
				proto->direct.primitiveSizeCode
			};
		}


		switch (size) {
		case CLASSSIZE_LATER:
		{
			ExtendedPrototypeSize sizes = Prototype_defineSize_direct(proto, proto->direct.cl);
			if (sizes.size >= 0)
				return sizes;
			break;
		}
		}

		raiseError("[Architecture] Cannot get the size of an uncomplete type");
		break;
	}
	case PROTO_MODE_VARIADIC:
	{
		return (ExtendedPrototypeSize){0, 0, true};
	}
	case PROTO_MODE_PRIMITIVE:
	{
		int size = proto->primitive.size;
		return (ExtendedPrototypeSize){size, size, true};
	}
	case PROTO_MODE_VOID:
		return (ExtendedPrototypeSize){0, 0, true};

	}
}



ExtendedPrototypeSize Prototype_reachSizes(Prototype* proto, Scope* scope, bool throwError) {
	if (!proto)
		goto returnEmptySize;
	

	switch (Prototype_mode(*proto)) {
	case PROTO_MODE_REFERENCE:
	{
		return Prototype_reachSizes(proto->ref.proto, scope, throwError);
	}

	case PROTO_MODE_DIRECT:
	{
		
		
		int size = proto->direct.sizes.size;
		if (size >= 0) {
			return (ExtendedPrototypeSize){
				proto->direct.sizes.size,
				proto->direct.sizes.maxMinimalSize,
				proto->direct.primitiveSizeCode
			};
		}


		if (!Class_getCompleteDefinition(proto->direct.cl, scope, throwError))
			goto returnEmptySize;


		ExtendedPrototypeSize sizes = Prototype_defineSize_direct(proto, proto->direct.cl);

		if (sizes.size < 0) {
			goto returnEmptySize;
		}
		
		proto->direct.sizes.size = sizes.size;
		proto->direct.sizes.maxMinimalSize = sizes.maxMinimalSize;
		proto->direct.primitiveSizeCode = sizes.primitiveSizeCode;
		return sizes;
	}

	case PROTO_MODE_VARIADIC:
	{
		return (ExtendedPrototypeSize){0, 0, true};
	}

	case PROTO_MODE_PRIMITIVE:
	{
		int size = proto->primitive.size;
		return (ExtendedPrototypeSize){size, size, true};
	}

	case PROTO_MODE_VOID:
	{
		return (ExtendedPrototypeSize){0, 0, true};
	}

	}


	returnEmptySize:
	if (throwError) {
		raiseError("[Type] Cannot get size of an incomplete type");
	}

	return (ExtendedPrototypeSize){CLASSSIZE_NOEXIST, CLASSSIZE_NOEXIST, PSC_UNKNOWN};

}


ExtendedPrototypeSize Prototype_getMetaSizes(Prototype* proto) {
	switch (Prototype_mode(*proto)) {
	case PROTO_MODE_REFERENCE:
	{
		raiseError("[TODO] Prototype_getMetaSizes");
		break;
	}

	case PROTO_MODE_DIRECT:
	{
		return Prototype_getSizes(proto->direct.meta);
	}

	case PROTO_MODE_VARIADIC:
	{
		raiseError("[TODO] Prototype_getMetaSizes");
		break;
	}

	case PROTO_MODE_PRIMITIVE:
	case PROTO_MODE_VOID:
	{
		return (ExtendedPrototypeSize){CLASSSIZE_NOEXIST, CLASSSIZE_NOEXIST, PSC_UNKNOWN};
	}

	}
}

ExtendedPrototypeSize Prototype_reachMetaSizes(Prototype* proto, Scope* scope, bool throwError) {
	switch (Prototype_mode(*proto)) {
	case PROTO_MODE_REFERENCE:
	{
		raiseError("[TODO] Prototype_reachMetaSizes");
		break;
	}

	case PROTO_MODE_DIRECT:
	{
		return Prototype_reachSizes(proto->direct.meta, scope, throwError);
	}

	case PROTO_MODE_VARIADIC:
	{
		raiseError("[TODO] Prototype_reachMetaSizes");
		break;
	}

	case PROTO_MODE_PRIMITIVE:
	case PROTO_MODE_VOID:
	{
		return (ExtendedPrototypeSize){CLASSSIZE_NOEXIST, CLASSSIZE_NOEXIST, PSC_UNKNOWN};
	}

	}
}




char Prototype_hasMeta(Prototype* proto) {
	switch (Prototype_mode(*proto)) {
	case PROTO_MODE_REFERENCE:
		raiseError("[TODO] Prototype_reachMeta");
		return -1;

	case PROTO_MODE_DIRECT:
		return direct_hasMeta(proto);

	case PROTO_MODE_VARIADIC:
		return -1;

	case PROTO_MODE_PRIMITIVE:
	case PROTO_MODE_VOID:
		return 0;

	}	
}

Prototype* Prototype_reachMeta(Prototype* proto) {
	switch (Prototype_mode(*proto)) {
	case PROTO_MODE_REFERENCE:
	{
		raiseError("[TODO] Prototype_reachMeta");
		break;
	}

	case PROTO_MODE_DIRECT:
	{
		char hasMeta = direct_hasMeta(proto);
		if (hasMeta == -1) {
			raiseError("[Architecture] Meta cannot be reach");
		}

		return hasMeta ? proto->direct.meta : NULL;
	}

	case PROTO_MODE_VARIADIC:
	{
		raiseError("[TODO] Prototype_reachMeta");	
		break;
	}

	case PROTO_MODE_PRIMITIVE:
	case PROTO_MODE_VOID:
	{
		return NULL;
	}

	}

}

Class* Prototype_getMetaClass(Prototype* proto) {
	Prototype* p = Prototype_reachMeta(proto);
	return p ? p->direct.cl : NULL;
}

Class* Prototype_getClass(Prototype* proto) {
	switch (Prototype_mode(*proto)) {
	case PROTO_MODE_REFERENCE:
	{
		raiseError("[TODO] Prototype_getClass");
		break;
	}

	case PROTO_MODE_DIRECT:
		return proto->direct.cl;

	case PROTO_MODE_VARIADIC:
	{
		raiseError("[TODO] Prototype_getClass");	
		break;
	}

	case PROTO_MODE_PRIMITIVE:
		return proto->primitive.cl;

	case PROTO_MODE_VOID:
		return NULL;
	}
}





int Prototype_getSignedSize(Prototype* proto) {
	if (Prototype_mode(*proto) == PROTO_MODE_PRIMITIVE) {
		return (int)proto->primitive.sizeCode;
	}

	if (Prototype_mode(*proto) == PROTO_MODE_VOID) {
		return 0;
	}

	return Prototype_getSizes(proto).size;
}


char Prototype_getPrimitiveSizeCode(Prototype* proto) {
	switch (Prototype_mode(*proto)) {
	case PROTO_MODE_REFERENCE:
	{
		raiseError("[TODO] Prototype_getPrimitiveSizeCOde");
		break;
	}

	case PROTO_MODE_DIRECT:
	{
		return proto->direct.primitiveSizeCode;
	}

	case PROTO_MODE_VARIADIC:
	{
		return PSC_UNKNOWN;
	}

	case PROTO_MODE_PRIMITIVE:
	{
		return proto->primitive.sizeCode;
	}

	case PROTO_MODE_VOID:
	{
		return 0;
	}

	}
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
	if (length == 0)
		return 0;

	/// TODO: handle not registrable issue
	switch (Prototype_getPrimitiveSizeCode(path[0]->proto)) {
	case PSC_UNKNOWN:
		raiseError("[Architecture] Cannot get offset of an unsized variable");
		return -1;

	case 0:
		return Prototype_getGlobalVariableOffset(path[0]->proto, &path[1], length-1);

	default: // primitive
		return -1;
	}
	

}

Scope* Prototype_reachSubScope(Prototype* proto, ScopeBuffer* buffer) {
	switch (Prototype_mode(*proto)) {
	case PROTO_MODE_REFERENCE:
		raiseError("[TODO] subscoppe of an expression");
	
	case PROTO_MODE_DIRECT:
		buffer->cl.scope.parent = NULL;
		buffer->cl.scope.type = SCOPE_CLASS;
		buffer->cl.cl = proto->direct.cl;
		buffer->cl.allowThis = false;
		return &buffer->cl.scope;

	case PROTO_MODE_VARIADIC:
		raiseError("[TODO] subscoppe of a variable");
		break;

	case PROTO_MODE_PRIMITIVE:
		raiseError("[Architecture] Cannot get a property from a primitive object");
		break;
		
	case PROTO_MODE_VOID:
		raiseError("[Architecture] Cannot get a property from a void object");
		break;


	}

}

Prototype* Prototype_copy(Prototype* src) {
	/// TODO: complete the copy
	int state = src->state;
	src->state = (((state >> 8) + 1) << 8) | (state & 0xff);

	switch (state & 0xff) {
	case PROTO_MODE_REFERENCE:
		return NULL;
	
	case PROTO_MODE_DIRECT:
	{
		return src;
	}
		
	case PROTO_MODE_VARIADIC:
		return NULL;
	
	case PROTO_MODE_PRIMITIVE:
	case PROTO_MODE_VOID:
		return src;
	}
}


bool Prototype_isType(Prototype* proto) {
	return Prototype_mode(*proto) == PROTO_MODE_DIRECT && proto->direct.cl == _langstd.type;
}



Prototype* Prototype_generateStackPointer(Variable **varr, int varLength) {
	ProtoSetting* settings = malloc(sizeof(ProtoSetting)*1);
	settings[0].useProto = true;
	settings[0].proto = Prototype_create_reference(varr, varLength);
	settings[0].variable = *Array_get(Variable*, _langstd.pointer->meta->variables, 0);

	Prototype* p = Prototype_create_direct(_langstd.pointer, 8, settings, 1);
	printf("gosr %p\n", p);
	return p;
}

#undef setMode


