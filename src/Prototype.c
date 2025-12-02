#include "Prototype.h"


#include "ScopeBuffer.h"
#include "Type.h"
#include "Variable.h"
#include "primitives.h"
#include "Expression.h"
#include "helper.h"
#include "langstd.h"

#include <string.h>


static ExtendedPrototypeSize Prototype_defineSize_direct(Prototype* proto, Class* cl) {
	if (cl->definitionState == DEFINITIONSTATE_UNDEFINED || cl->definitionState == DEFINITIONSTATE_READING) {
		proto->direct.sizes.size = CLASSSIZE_LATER;
		proto->direct.sizes.maxMinimalSize = CLASSSIZE_LATER;
		proto->direct.isRegistrable = REGISTRABLE_UNKNOWN;
		return (ExtendedPrototypeSize){CLASSSIZE_LATER, CLASSSIZE_LATER, REGISTRABLE_UNKNOWN};
	}

	printf("direct %s %d\n", cl->name, cl->size);

	if (cl->size >= 0) {
		ExtendedPrototypeSize sizes = {cl->size, cl->maxMinimalSize, cl->isRegistrable};
		proto->direct.sizes.size = sizes.size;
		proto->direct.sizes.maxMinimalSize = sizes.maxMinimalSize;
		proto->direct.isRegistrable = sizes.isRegistrable;
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
	proto->mode = PROTO_MODE_DIRECT;
	proto->direct.cl = cl;
	proto->direct.primitiveSizeCode = primitiveSizeCode;
	proto->direct.isRegistrable = cl->isRegistrable;
	if (settingLength) {
		proto->direct.settings = settings;
		proto->direct.settingLength = settingLength;
		proto->direct.settingsMustBeFreed = true;
	} else {
		proto->direct.settingLength = 0;
		proto->direct.settingsMustBeFreed = false;
	}
	Prototype_defineSize_direct(proto, cl);


	definitionState_t mds = cl->metaDefinitionState;
	
	switch (mds) {
	case DEFINITIONSTATE_UNDEFINED:
		proto->direct.metaDefintionState = DEFINITIONSTATE_UNDEFINED;
		break;

	case DEFINITIONSTATE_DONE:
	{
		Prototype* pm = Prototype_create_meta(pm, cl->meta);
		proto->direct.meta = pm;
		proto->direct.metaDefintionState = DEFINITIONSTATE_DONE;
		break;
	}

	case DEFINITIONSTATE_READING:
		proto->direct.metaDefintionState = DEFINITIONSTATE_UNDEFINED;
		break;

	case DEFINITIONSTATE_NOEXIST:
		proto->direct.metaDefintionState = DEFINITIONSTATE_NOEXIST;
		break;
	}

	return proto;
}


Prototype* Prototype_create_meta(Prototype* origin, Class* meta) {
	Prototype* mp = Prototype_create_direct(
		meta, meta->primitiveSizeCode, NULL, -1
	);
	mp->direct.origin = origin;

	return mp;
}

Prototype* Prototype_create_expression(Expression* expr) {
	Prototype* proto = malloc(sizeof(Prototype));
	proto->mode = PROTO_MODE_EXPRESSION;
	proto->expr.ptr = expr;

	return proto;
	
}
Prototype* Prototype_create_variadic() {
	Prototype* proto = malloc(sizeof(Prototype));
	proto->mode = PROTO_MODE_VARIADIC;

	return proto;

}


void Prototype_free(Prototype* proto, bool deep) {
	switch (proto->mode) {
	case PROTO_MODE_EXPRESSION:
		Expression_free(proto->expr.ptr->type, proto->expr.ptr);
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
	
		if (deep && proto->direct.metaDefintionState == DEFINITIONSTATE_DONE) {
			Prototype_free(proto->direct.meta, deep);
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





Type* Prototype_generateType(Prototype* proto) {
	switch (proto->mode) {
	case PROTO_MODE_EXPRESSION:
	{
		raiseError("[TODO] Prototype_generateType");
		return NULL;
	}
	
	case PROTO_MODE_DIRECT:
	{
		Type* type = malloc(sizeof(Type));
		Class* cl = proto->direct.cl;
		type->proto = proto;
		type->primitiveSizeCode = 0;

		// Handle meta
		switch (proto->direct.metaDefintionState) {
	    case DEFINITIONSTATE_UNDEFINED:
			raiseError("[TODO] define type on flee");
			break;

		case DEFINITIONSTATE_READING:
			raiseError("[Architecture] Cannot create a type with meta-class in reading state");
			break;

		case DEFINITIONSTATE_DONE:
		{
			Prototype* meta = proto->direct.meta;
			Type* metaType = Prototype_generateType(meta);
			type->meta = metaType;

			ExtendedPrototypeSize sizes = Prototype_getSizes(meta);
			if (sizes.size > 0) {
				void* data = malloc(sizes.size);
				memset(data, 0, sizes.size);
				type->data = data;

				// Append data if necessary
				Type_defaultConstructors(
					data,
					meta->direct.cl,
					proto->direct.settings,
					proto->direct.settingLength,
					*metaType
				);

			} else {
				type->data = NULL;
			}

			break;
		}

		case DEFINITIONSTATE_NOEXIST:
			type->data = NULL;
			type->meta = NULL;
			break;

		default:
			raiseError("[Intern] Invalid metaDefintionState");
			break;
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





definitionState_t Prototype_reachMetaDefinition(Prototype* proto, Scope* scope, bool throwError) {
	switch (proto->mode) {
	case PROTO_MODE_EXPRESSION:
		raiseError("[TODO] Prototype_reachMetaDefinition");
		return DEFINITIONSTATE_UNDEFINED;
		
	case PROTO_MODE_DIRECT:
	{
		Class* cl = proto->direct.cl;

		definitionState_t state = cl->definitionState;
		if (state == DEFINITIONSTATE_UNDEFINED || state == DEFINITIONSTATE_READING) {
			Scope_defineOnFly(scope, cl->name);
			state = cl->definitionState;
			if (state != DEFINITIONSTATE_DONE) {
				if (throwError) {
					raiseError("[Architecture] Cannot define on fly current class");
				} else {
					return DEFINITIONSTATE_UNDEFINED;
				}
			}
		}

		// Enshure meta size first
		state = proto->direct.metaDefintionState;
		if (state != DEFINITIONSTATE_UNDEFINED)
			return state;

		if (cl->metaDefinitionState == DEFINITIONSTATE_DONE) {
			Prototype* pm = Prototype_create_meta(proto, cl->meta);
			proto->direct.meta = pm;
			proto->direct.metaDefintionState = DEFINITIONSTATE_DONE;
			return DEFINITIONSTATE_DONE;
		}

		proto->direct.metaDefintionState = DEFINITIONSTATE_NOEXIST;
		return DEFINITIONSTATE_NOEXIST;
	}
		
	case PROTO_MODE_VARIADIC:
		return DEFINITIONSTATE_NOEXIST;

	case PROTO_MODE_PRIMITIVE:
		return DEFINITIONSTATE_NOEXIST;

	case PROTO_MODE_VOID:
		return DEFINITIONSTATE_NOEXIST;

	}
}

ExtendedPrototypeSize Prototype_getSizes(Prototype* proto) {
	switch (proto->mode) {
	case PROTO_MODE_EXPRESSION:
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
				proto->direct.isRegistrable
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
		return (ExtendedPrototypeSize){CLASSSIZE_NOEXIST, CLASSSIZE_NOEXIST, REGISTRABLE_UNKNOWN};
	

	definitionState_t state = Prototype_reachMetaDefinition(proto, scope, throwError);
	switch (state) {
	case DEFINITIONSTATE_UNDEFINED:
	case DEFINITIONSTATE_READING:
		return (ExtendedPrototypeSize){CLASSSIZE_LATER, CLASSSIZE_LATER, REGISTRABLE_UNKNOWN};

	case DEFINITIONSTATE_DONE:
		Prototype_reachSizes(proto->direct.meta, scope, throwError);
		break;

	case DEFINITIONSTATE_NOEXIST:
		break;

	}

	switch (proto->mode) {
	case PROTO_MODE_EXPRESSION:
	{
		raiseError("[TODO] reachSize");
		break;
	}

	case PROTO_MODE_DIRECT:
	{
		int size = proto->direct.sizes.size;
		if (size >= 0)
			return (ExtendedPrototypeSize){
				proto->direct.sizes.size,
				proto->direct.sizes.maxMinimalSize,
				proto->direct.isRegistrable
			};


		ExtendedPrototypeSize sizes = Prototype_defineSize_direct(proto, proto->direct.cl);

		if (sizes.size < 0) {
			if (throwError) {
				raiseError("[Architecture] Cannot get the size of an uncomplete type");
			}
			return (ExtendedPrototypeSize){CLASSSIZE_LATER, CLASSSIZE_LATER, REGISTRABLE_UNKNOWN};
		}
		
		proto->direct.sizes.size = sizes.size;
		proto->direct.sizes.maxMinimalSize = sizes.maxMinimalSize;
		proto->direct.isRegistrable = sizes.isRegistrable;
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
}


ExtendedPrototypeSize Prototype_getMetaSizes(Prototype* proto) {
	switch (proto->mode) {
	case PROTO_MODE_EXPRESSION:
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
		return (ExtendedPrototypeSize){CLASSSIZE_NOEXIST, CLASSSIZE_NOEXIST, REGISTRABLE_UNKNOWN};
	}

	}
}

ExtendedPrototypeSize Prototype_reachMetaSizes(Prototype* proto, Scope* scope, bool throwError) {
	switch (proto->mode) {
	case PROTO_MODE_EXPRESSION:
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
		return (ExtendedPrototypeSize){CLASSSIZE_NOEXIST, CLASSSIZE_NOEXIST, REGISTRABLE_UNKNOWN};
	}

	}
}


Prototype* Prototype_getMeta(Prototype* proto) {
	switch (proto->mode) {
	case PROTO_MODE_EXPRESSION:
	{
		raiseError("[TODO] Prototype_getMeta");
		break;
	}

	case PROTO_MODE_DIRECT:
	{
		if (proto->direct.metaDefintionState == DEFINITIONSTATE_DONE)
			return proto->direct.meta;

		return NULL;
	}

	case PROTO_MODE_VARIADIC:
	{
		raiseError("[TODO] Prototype_getMeta");	
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
	Prototype* p = Prototype_getMeta(proto);
	return p ? p->direct.cl : NULL;
}

Class* Prototype_getClass(Prototype* proto) {
	switch (proto->mode) {
	case PROTO_MODE_EXPRESSION:
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
	if (proto->mode == PROTO_MODE_PRIMITIVE) {
		return (int)proto->primitive.sizeCode;
	}

	if (proto->mode == PROTO_MODE_VOID) {
		return 0;
	}

	return Prototype_getSizes(proto).size;
}

char Prototype_isRegistrable(Prototype* proto, bool throwError) {
	switch (proto->mode) {
	case PROTO_MODE_EXPRESSION:
	{
		raiseError("[TODO] Prototype_isRegistrable");
		break;
	}

	case PROTO_MODE_DIRECT:
	{
		return proto->direct.isRegistrable;
	}

	case PROTO_MODE_VARIADIC:
	{
		raiseError("[TODO] Prototype_isRegistrable");
		break;
	}

	case PROTO_MODE_PRIMITIVE:
	case PROTO_MODE_VOID:
	{
		return REGISTRABLE_TRUE;
	}

	}

}

char Prototype_getPrimitiveSizeCode(Prototype* proto) {
	switch (proto->mode) {
	case PROTO_MODE_EXPRESSION:
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
		raiseError("[TODO] Prototype_getPrimitiveSizeCOde");
		break;
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
	/// TODO: handle not registrable issue
	return Prototype_getGlobalVariableOffset(path[0]->proto, &path[1], length-1);
}

Scope* Prototype_reachSubScope(Prototype* proto, ScopeBuffer* buffer) {
	switch (proto->mode) {
	case PROTO_MODE_EXPRESSION:
		raiseError("[TODO] subscoppe of an expression");
	
	case PROTO_MODE_DIRECT:
		buffer->cl.scope.parent = NULL;
		buffer->cl.scope.type = SCOPE_CLASS;
		buffer->cl.cl = proto->direct.cl;
		buffer->cl.allowThis = false;
		return &buffer->cl.scope;
		break;

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

void Prototype_copy(Prototype* dest, const Prototype* src) {
	*dest = *src;
	
	switch (src->mode) {
	case PROTO_MODE_EXPRESSION:
		break;
	
	case PROTO_MODE_DIRECT:
		dest->direct.settingsMustBeFreed = false;
		break;
		
	case PROTO_MODE_VARIADIC:
		break;
	
	case PROTO_MODE_PRIMITIVE:
		break;
	}
}


bool Prototype_isType(Prototype* proto) {
	return proto->mode == PROTO_MODE_DIRECT && proto->direct.cl == _langstd.type;
}