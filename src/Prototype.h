#ifndef COMPILER_PROTOTYPE_H_
#define COMPILER_PROTOTYPE_H_

#include "declarations.h"
#include "label_t.h"
#include "definitionState_t.h"
#include <tools/Array.h>

enum {
	PROTO_MODE_REFERENCE,
	PROTO_MODE_DIRECT,
	PROTO_MODE_VARIADIC,
	PROTO_MODE_PRIMITIVE,
	PROTO_MODE_VOID,
};

typedef struct {
	int size;
	int maxMinimalSize;
} PrototypeSize;

typedef struct {
	int size;
	int maxMinimalSize;
	char primitiveSizeCode;
} ExtendedPrototypeSize;


struct ProtoSetting {
	char useProto;
	char useVariable;

	union {
		Variable* variable;
		label_t name;
	};

	union {
		Prototype* proto;
		Expression* expr;
	};
};

#define Prototype_mode(p) ((p).state & 0xff)
#define Prototype_addUsage(p)\
{int protoState = (p).state;\
	(p).state = (((protoState >> 8) + 1) << 8) | (protoState & 0xff);}

struct Prototype {
	int state;

	union {
		struct {
			Class* cl;
			Prototype* meta;
			union {
				ProtoSetting* settings;
				Prototype* origin;
			};
			PrototypeSize sizes;
			int settingLength; // -1 means origin
			char primitiveSizeCode;
			bool settingsMustBeFreed;
			char hasMeta; // -1 means check cl
		} direct;
		
		struct {
			Class* cl;
			int size;
			char sizeCode;
		} primitive;
		
		struct {
			Prototype* proto;
			Variable** varr;
			int varrLength;
		} ref;

		struct {
			Variable* ref;
		} variadic;
	};
};


Prototype* Prototype_create_direct(Class* cl, char primitiveSizeCode, ProtoSetting* settings, int settingLength);
Prototype* Prototype_create_meta(Prototype* origin, Class* meta);
Prototype* Prototype_create_reference(Variable** varr, int varrLength);
Prototype* Prototype_create_variadic(Variable* ref);
void Prototype_free(Prototype* proto, bool deep);


Type* Prototype_generateType(Prototype* proto, Scope* scope);

bool Prototype_accepts(const Prototype* proto, const Type* type);



ExtendedPrototypeSize Prototype_getSizes(Prototype* proto);
ExtendedPrototypeSize Prototype_reachSizes(Prototype* proto, Scope* scope, bool throwError);

ExtendedPrototypeSize Prototype_getMetaSizes(Prototype* proto);
ExtendedPrototypeSize Prototype_reachMetaSizes(Prototype* proto, Scope* scope, bool throwError);

char Prototype_hasMeta(Prototype* proto);
Prototype* Prototype_reachMeta(Prototype* proto);
Class* Prototype_getMetaClass(Prototype* proto);

Class* Prototype_getClass(Prototype* proto);

int Prototype_getSignedSize(Prototype* proto);

char Prototype_getPrimitiveSizeCode(Prototype* proto);

int Prototype_getGlobalVariableOffset(Prototype* proto, Variable* path[], int length);
int Prototype_getVariableOffset(Variable* path[], int length);

Scope* Prototype_reachSubScope(Prototype* proto, ScopeBuffer* buffer);

Prototype* Prototype_copy(Prototype* src);

bool Prototype_isType(Prototype* proto);

Prototype* Prototype_generateStackPointer(Variable **varr, int varLength);



Variable* ProtoSetting_getVariable(ProtoSetting* setting, Class* meta);


#endif
