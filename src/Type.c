#include "Type.h"

#include "Class.h"
#include "Prototype.h"
#include "Function.h"
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
		Type_defaultDestructors(data, Prototype_getClass(meta->proto));
		free(data);
	}

	Prototype_free(type->proto, true);
	free(type);

	if (meta) {
		Type_free(meta);
	}
}


void Type_defaultConstructors(
	mblock_t data,
	Class* meta,
	ProtoSetting* settings,
	int settingLength,
	Scope* scope
) {
	// Fill settings
	Array_for(ProtoSetting, settings, settingLength, setting) {
		if (setting->useProto) {
			Prototype* proto = setting->proto;
			Variable* variable = ProtoSetting_getVariable(setting, meta);
			Type* stype = Prototype_generateType(proto, scope);
			*((size_t*) (data + variable->offset)) = (size_t)stype;
			printf("gave at %d\n", variable->offset);
		}
	}

	ScopeType subScope = {
		.scope = {scope, SCOPE_TYPE},
		.data = data
	};

	// Fill variables
	Variable** clVariables = meta->variables.data;
	for (int i = 0, i_end = meta->variables.length; i < i_end; i++) {
		Variable* variable = clVariables[i];
		Prototype* proto = variable->proto;

		ProtoSetting* subSettings;
		int subSettingLength;
		if (Prototype_mode(*proto) == PROTO_MODE_DIRECT) {
			subSettingLength = proto->direct.settingLength;
			Prototype* origin = proto;
			while (subSettingLength == -1) {
				/// TODO: check for direct
				origin = origin->direct.origin;
				subSettingLength = origin->direct.settingLength;
			}

			subSettings = origin->direct.settings;

			if (subSettingLength < 0) {
				raiseError("TODO: subSettingLength == -1 (means origin)");
			}
		} else {
			subSettingLength = 0;
		}

		Type_defaultConstructors(
			data + variable->offset,
			Prototype_getClass(proto),
			subSettings, subSettingLength,
			&subScope.scope
		);	
	}
}


void Type_defaultDestructors(mblock_t data, Class* cl) {
	typedef Variable* var_t;
	Array_loop(var_t, cl->variables, vptr) {
		Variable* variable = *vptr;
		Class* vcl = Prototype_getClass(variable->proto);

		mblock_t dataPtr = data+variable->offset;
		if (vcl == _langstd.type) {
			printf("typeat %p of %p [var: %s]\n", *(Type**)dataPtr, data, variable->name);
			Type_free(*(Type**)dataPtr);
			continue;
		}

		Type_defaultDestructors(dataPtr, vcl);
	}
}

Type* Type_newCopy(Type* source, Prototype* proto, int offset) {
	raiseError("[TODO] fix Type_newCopy => choisir la meta");

	if (!source)
		return NULL;

	Type* type = malloc(sizeof(Type));
	int size = Prototype_getMetaSizes(proto).size;

	if (offset > 0) {
		mblock_t* data = malloc(size);
		memcpy(data, source->data + offset, size);
		type->data = data;
	} else {
		type->data = NULL;
	}

	Type* srcMeta = source->meta;
	// type->meta = Type_newCopy(srcMeta, srcMeta->proto)
	type->meta = NULL;	
	type->proto = proto;
	type->reference = NULL;
	type->refCount = 0;
	type->primitiveSizeCode = 0;


	return type;
}




Variable* ScopeType_searchVariable(ScopeType* scope, label_t name, ScopeSearchArgs* args) {
	return NULL;
}

Class* ScopeType_searchClass(ScopeType* scope, label_t name, ScopeSearchArgs* args) {
	return NULL;
}

Function* ScopeType_searchFunction(ScopeType* scope, label_t name, ScopeSearchArgs* args) {
	return NULL;
}



void ScopeType_addVariable(ScopeType* scope, Variable* v) {
	raiseError("[Intern] Tried to add a variable to a type");
}

void ScopeType_addClass(ScopeType* scope, Class* cl) {
	raiseError("[Intern] Tried to add a class to a type");
}

void ScopeType_addFunction(ScopeType* scope, Function* fn) {
	raiseError("[Intern] Tried to add a function to a type");
}

Type* ScopeType_searchType(ScopeType* scope, Variable* variable) {
	return *(Type**)(scope->data + variable->offset);
}

