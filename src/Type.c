#include "Type.h"

#include "Interpreter.h"
#include "Expression.h"
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
		if (!meta) {free(type);free(type);}
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
	int way,
	Scope* scope
) {
	typedef Function* fn_t;

	// Fill settings
	Array_for(ProtoSetting, settings, settingLength, setting) {
		if (setting->useProto) {
			Prototype* proto = setting->proto;
			Variable* variable = ProtoSetting_getVariable(setting, meta);
			Type* stype = Prototype_generateType(proto, scope, way);
			*((size_t*) (data + variable->offset)) = (size_t)stype;
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

		switch (way) {
		case TYPE_CWAY_DEFAULT:
			Type_defaultConstructors(data + variable->offset, Prototype_getClass(proto),
				subSettings, subSettingLength, TYPE_CWAY_DEFAULT, &subScope.scope);
			break;

		case TYPE_CWAY_ARGUMENT:
			Type_defaultConstructors(data + variable->offset, Prototype_getClass(proto),
				subSettings, subSettingLength, TYPE_CWAY_ARGUMENT, &subScope.scope);
			break;
		}
	}

	// Handle way to construct
	switch (way) {
	case TYPE_CWAY_DEFAULT:
		break;

	case TYPE_CWAY_ARGUMENT:
		Array_loop(fn_t, meta->constructors, cptr) {
			// Call constructor
			Function* fn = *cptr;
			if ((fn->flags & FUNCTIONFLAGS_ARGUMENT_CONSTRUCTOR) == 0)
				continue;

			switch (fn->definitionState) {
			case DEFINITIONSTATE_UNDEFINED:
				printf("which %s\n", meta->name);
				raiseError("[Undefined] Interpret status is undefined for this function");
				return;

			case DEFINITIONSTATE_READING:
				raiseError("[Intern] Interpret status is reading for this function");
				return;

			case DEFINITIONSTATE_NOEXIST:
				goto finishArgumentConstruction;
			}

			printf("construct %s\n", meta->name);

			Expression expr = {.type = EXPRESSION_MBLOCK, .data = {
				.mblock = data
			}};
			Expression* args[] = {&expr};

			/// TODO: check argsStartIndex
			Intrepret_interpret(
				fn->interpreter,
				scope,
				args,
				1,
				0,
				true
			);
			break;
		}

		finishArgumentConstruction:
		break;

	}
}


void Type_defaultDestructors(mblock_t data, Class* cl) {
	typedef Variable* var_t;
	Array_loop(var_t, cl->variables, vptr) {
		Variable* variable = *vptr;
		Class* vcl = Prototype_getClass(variable->proto);

		mblock_t dataPtr = data+variable->offset;
		if (vcl == _langstd.type) {
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
		Prototype* proto = variable->proto;
		Class* cl = Prototype_getClass(proto);
		if (cl == _langstd.type) {
			Type* newType = Type_deepCopy(*(Type**)src, proto, vptr, 1);
			*(Type**)dst = newType;
			continue;
		}

		// default behavior
		Type_defaultCopy(dst + offset, src + offset, cl);
	}
}


Type* Type_deepCopy(Type* root, Prototype* proto, Variable** varr, int varr_len) {
	typedef Variable* var_ptr_t;

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
		
		Prototype* newProto = Prototype_copyWithoutSettings(proto);
		type->proto = newProto;
		type->reference = NULL;
		type->refCount = 0;
		type->primitiveSizeCode = 0;

		// Define the prototype
		if (proto->direct.settingLength > 0) {
			ProtoSetting* srcSettings = proto->direct.settings;
			int settingLength = proto->direct.settingLength;
			int settingsToDefineCount = settingLength;

			ProtoSetting* settings = malloc(settingLength * sizeof(ProtoSetting));

			// Copy settings
			for (int i = 0; i < settingLength; i++) {
				if (srcSettings[i].useVariable) {
					settings[i].useVariable = true;
					settings[i].variable = srcSettings[i].variable;
				} else {
					settings[i].useVariable = false;
					settings[i].name = srcSettings[i].name;
				}

				if (srcSettings[i].useProto) {
					Prototype* p = srcSettings[i].proto;
					Prototype_addUsage(*p);
					settings[i].useProto = true;
					settings[i].proto = p;
				} else {
					settings[i].useProto = false;
					// settings[i].expr = srcSettings[i].expr;
					raiseError("[TODO] copy expressions");
				}
			}

			ProtoSetting* settings_end = settings + settingLength;
			Array_for(var_ptr_t, &varr[1], varr_len - 1, vptr) {
				for (ProtoSetting* s = settings; s < settings_end; s++) {
					if (s->useVariable && s->useProto) {
						Prototype* origin = s->proto;
						Prototype* p = origin;
						printf("def %p\n", p);
						while (true) {
							Prototype* res = Prototype_reachProto(p, (*(vptr-1))->proto);
							if (res == NULL || p == res) {
								break;
							}

							p = res;
						}

						if (p != origin) {
							Prototype_addUsage(*p);
							Prototype_free(origin, false);
						}

						s->proto = p;
						
					} else {
						raiseError("[TODO] different protoSetting case");
					}
				}
			}
			
			
			newProto->direct.settings = settings;
			newProto->direct.settingLength = settingLength;
			newProto->direct.settingsMustBeFreed = true;
		} else {
			newProto->direct.settings = NULL;
			newProto->direct.settingLength = 0;
			newProto->direct.settingsMustBeFreed = false;
		}

		
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


		Prototype* metaProto = proto->direct.meta;
		Type* metaType = Type_deepCopy(rootMetaType, metaProto, varr, varr_len);
		type->meta = metaType;

		ExtendedPrototypeSize sizes = Prototype_getSizes(metaProto);
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

