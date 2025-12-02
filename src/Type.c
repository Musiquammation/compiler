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

	void* data = type->data;
	Type* meta = type->meta;
	if (data) {
		Type_defaultDestructors(data, meta);
		free(data);
	}

	free(type);

	if (meta) {
		Type_free(meta);
	}
}


void Type_defaultConstructors(void* data, Class* cl, ProtoSetting* settings, int settingLength, Type meta) {
	typedef Variable* var_t;
	Variable** const metaVariables = meta.proto->direct.cl->variables.data;
	const int metaVariables_len = meta.proto->direct.cl->variables.length;

	Array_for(ProtoSetting, settings, settingLength, setting) {
		if (setting->useProto) {
			Variable* variable = setting->variable;
			Type* stype = Prototype_generateType(setting->proto);
			*((size_t*) data + variable->offset) = (size_t)stype;
			
			// Define owner
			label_t name = variable->name;
			Array_for(var_t, metaVariables, metaVariables_len, vptr) {
				Variable* v = *vptr;
				if (v->name == name) {
					// Variable found (owner is at +0)
					*(bool*)(meta.data + v->offset + 0) = true;
					break;
				}
			}
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
			/// TODO: check owner
			label_t name = variable->name;
			Array_for(var_t, metaVariables, metaVariables_len, vptr) {
				Variable* v = *vptr;
				if (v->name == name) {
					// Variable found (owner is at +0)
					if (*(bool*)(meta->data + v->offset + 0)) {
						Type_free(*(Type**)data);
					}
					goto typeOwner_found;
				}
			}

			raiseError("[Intern] Cannot find type meta-class");

			typeOwner_found:
		}
		
	}
}

