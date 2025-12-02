#ifndef COMPILER_PROTOTYPE_H_
#define COMPILER_PROTOTYPE_H_

#include "declarations.h"
#include "definitionState_t.h"
#include <tools/Array.h>

enum {
	PROTO_MODE_EXPRESSION,
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
	char isRegistrable;
} ExtendedPrototypeSize;



struct ProtoSetting {
	char useProto;

	Variable* variable;

	union {
		Prototype* proto;
		Expression* expr;
	};
};

struct Prototype {
	char mode;

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
			char isRegistrable;
			definitionState_t metaDefintionState;
		} direct;
		
		struct {
			Class* cl;
			int size;
			char sizeCode;
		} primitive;
		
		struct {
			Expression* ptr;
			// PrototypeSize sizes;
		} expr;

		struct {
			Variable* ptr;
		} variadic;
	};
};


Prototype* Prototype_create_direct(Class* cl, char primitiveSizeCode, ProtoSetting* settings, int settingLength);
Prototype* Prototype_create_meta(Prototype* origin, Class* meta);
Prototype* Prototype_create_expression(Expression* expr);
Prototype* Prototype_create_variadic();
void Prototype_free(Prototype* proto, bool deep);


Type* Prototype_generateType(Prototype* proto);

bool Prototype_accepts(const Prototype* proto, const Type* type);



/**
 * @return `DEFINITIONSTATE_NOEXIST` or `DEFINITIONSTATE_DONE` if `throwError`== `false`
 */
definitionState_t Prototype_reachMetaDefinition(Prototype* proto, Scope* scope, bool throwError);

ExtendedPrototypeSize Prototype_getSizes(Prototype* proto);
ExtendedPrototypeSize Prototype_reachSizes(Prototype* proto, Scope* scope, bool throwError);

ExtendedPrototypeSize Prototype_getMetaSizes(Prototype* proto);
ExtendedPrototypeSize Prototype_reachMetaSizes(Prototype* proto, Scope* scope, bool throwError);

Prototype* Prototype_getMeta(Prototype* proto);
Class* Prototype_getMetaClass(Prototype* proto);

Class* Prototype_getClass(Prototype* proto);

int Prototype_getSignedSize(Prototype* proto);

char Prototype_isRegistrable(Prototype* proto, bool throwError);
char Prototype_getPrimitiveSizeCode(Prototype* proto);

int Prototype_getGlobalVariableOffset(Prototype* proto, Variable* path[], int length);
int Prototype_getVariableOffset(Variable* path[], int length);

Scope* Prototype_reachSubScope(Prototype* proto, ScopeBuffer* buffer);

void Prototype_copy(Prototype* dest, const Prototype* src);

bool Prototype_isType(Prototype* proto);


#endif
