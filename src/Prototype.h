#ifndef COMPILER_PROTOTYPE_H_
#define COMPILER_PROTOTYPE_H_

#include "declarations.h"

struct Prototype {
	Class* cl;
	int size;
	int maxMinimalSize;
	int meta_size;
	int meta_maxMinimalSize;
	char primitiveSizeCode;
};


void Prototype_create(Prototype* proto, Class* cl);
void Prototype_createWithSizeCode(Prototype* proto, Class* cl, char primitiveSizeCode);
void Prototype_delete(Prototype* proto);


Type* Prototype_generateType(Prototype* proto);

bool Prototype_accepts(const Prototype* proto, const Type* type);

int Prototype_getSize(Prototype* proto);
int Prototype_getMetaSize(Prototype* proto);
int Prototype_reachSize(Prototype* proto, Scope* scope);
int Prototype_getSignedSize(Prototype* proto);

int Prototype_getGlobalVariableOffset(Prototype* proto, Variable* path[], int length);
int Prototype_getVariableOffset(Variable* path[], int length);

void Prototype_generateMeta(Prototype* dest, const Prototype* src);


#endif