#include "Type.h"

#include "Class.h"
#include "Prototype.h"
#include "Variable.h"
#include "langstd.h"

#include "helper.h"

#include <string.h>



void Type_free(Type* type) {
	if (type->primitiveSizeCode)
		return;

	int refCount = type->refCount;
	printf("left: %d\n", refCount);
	if (refCount > 0) {
		type->refCount = refCount - 1;
		return;
	}

	printf("FTYPE %p\n", type);

	Type* ref = type->reference;
	Type* meta = type->meta;
	if (ref) {
		Type_free(ref);
		if (ref != type) {
			free(type);
		}

		if (meta) {
			Type_free(meta);
		}

		return;
	}


	void* data = type->data;
	if (data) {
		Type_defaultDestructors(data, meta);
		free(data);
	}

	Prototype_free(type->proto, true);
	free(type);

	if (meta) {
		Type_free(meta);
	}
}


void Type_defaultConstructors(
	void* data,
	Class* cl,
	ProtoSetting* settings,
	int settingLength,
	Type meta,
	Scope* scope
) {
	typedef Variable* var_t;

	Type* backup = malloc(sizeof(Type));
	*backup = meta;

	Variable** const metaVariables = meta.proto->direct.cl->variables.data;
	const int metaVariables_len = meta.proto->direct.cl->variables.length;

	Array_for(ProtoSetting, settings, settingLength, setting) {
		if (setting->useProto) {
			Prototype* proto = setting->proto;
			Variable* variable = ProtoSetting_getVariable(setting, cl);
			Type* stype = Prototype_generateType(proto, scope);
			*((size_t*) data + variable->offset) = (size_t)stype;
			printf("gave at %d\n", variable->offset);
		}
	}
}


void Type_defaultDestructors(void* data, Type* meta) {
	typedef Variable* var_t;
	Variable** const metaVariables = meta->proto->direct.cl->variables.data;
	const int metaVariables_len = meta->proto->direct.cl->variables.length;

	Class* cl = Prototype_getClass(meta->proto);
	typedef Variable* var_t;
	Array_loop(var_t, cl->variables, vptr) {
		Variable* variable = *vptr;
		Class* vcl = Prototype_getClass(variable->proto);

		if (vcl == _langstd.type) {
			printf("typeat %p of %p\n", *(Type**)data, data);
			Type_free(*(Type**)data);
		}		
	}
}

