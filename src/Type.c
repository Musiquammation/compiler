#include "Type.h"

#include "Class.h"
#include "Prototype.h"
#include "Function.h"
#include "Variable.h"
#include "primitives.h"
#include "langstd.h"

#include "helper.h"

#include <string.h>



void Type_free(Type* type) {
	if (type->primitiveSizeCode)
		return;

	int refCount = type->refCount;
	if (refCount > 0) {
		type->refCount = refCount - 1;
		return;
	}


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
			printf("typeat %p inside %p [var=%s]\n", *(Type**)dataPtr, data, variable->name);
			Type_free(*(Type**)dataPtr);
			continue;
		}

		Type_defaultDestructors(dataPtr, vcl);
	}
}

void Type_defaultCopy(mblock_t dst, const mblock_t src, Class* cl) {
	if (cl->isPrimitive) {
		memcpy(dst, src, cl->size);
		return;
	}

	typedef Variable* var_t;
	Array_loop(var_t, cl->variables, vptr) {
		Variable* variable = *vptr;
		int offset = variable->offset;
		Class* cl = Prototype_getClass(variable->proto);
		if (cl == _langstd.type) {
			Type* newType = Type_deepCopy(*(Type**)src, vptr, 1);
			*(Type**)dst = newType;
			continue;
		}

		// default behavior
		Type_defaultCopy(dst + offset, src + offset, cl);
	}
}


Type* Type_deepCopy(Type* root, Variable** varr, int varr_len) {
	typedef Variable* var_ptr_t;

	Prototype* proto = varr[varr_len - 1]->proto;
	switch (Prototype_mode(*proto)) {
	case PROTO_MODE_REFERENCE:
	{
		raiseError("[TODO] Type_newCopy");
	}
	
	case PROTO_MODE_DIRECT:
	{
		// Get meta
		char hasMeta = Prototype_directHasMeta(proto);
		if (hasMeta == -1) {
			raiseError("[Architecture] Cannot create a type with unknown meta");
			return NULL;
		}

		// Create type
		Type* type = malloc(sizeof(Type));
		Class* cl = proto->direct.cl;
		
		type->proto = proto;
		type->reference = NULL;
		type->refCount = 0;
		type->primitiveSizeCode = 0;

		Prototype_addUsage(*proto);
		
		// No meta
		if (hasMeta == 0) {
			type->data = NULL;
			type->meta = NULL;
			return type;
		}

		// Get meta offset
		int offset = 0;
		Type* rootMetaType = root->meta;
		if (!rootMetaType)
			return NULL;

		Array_for(var_ptr_t, &varr[1], varr_len - 1, vptr) {
			Variable* variable = *vptr;
			Variable* meta = variable->meta;
			if (!meta) {
				offset = -1;
				break;
			}

			*vptr = meta;
			offset += meta->offset;
		}

		if (offset < 0)
			return NULL;


		Prototype* meta = proto->direct.meta;
		Type* metaType = offset >= 0 ? Type_deepCopy(rootMetaType, varr, varr_len) : 0;
		type->meta = metaType;

		ExtendedPrototypeSize sizes = Prototype_getSizes(meta);
		if (sizes.size > 0) {
			void* data = malloc(sizes.size);
			memset(data, 0, sizes.size);
			type->data = data;

			// Append data if necessary
			Type_defaultCopy(
				data,
				root->data + offset,
				cl->meta
			);

		} else {
			type->data = NULL;
		}

		return type;
	}

	case PROTO_MODE_VARIADIC:
	{
		/// TODO: search definition (but WHERE?)
		raiseError("[TODO] Type_newCopy");
	}

	case PROTO_MODE_PRIMITIVE:
	{
		return primitives_getType(proto->primitive.cl);
	}
	}
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

