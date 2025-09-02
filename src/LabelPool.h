#ifndef COMPILER_LABEL_POOL_H_
#define COMPILER_LABEL_POOL_H_

#include "declarations.h"
#include "label_t.h"

#include <tools/Array.h>



struct LabelPool {
	char** data;
	int size;
	int capacity;
};

struct CommonLabels {
	label_t _let;
	label_t _const;
	label_t _function;
	label_t _class;
	label_t _module;
	label_t _main;
};

void LabelPool_create(LabelPool* pool);
void LabelPool_delete(LabelPool* pool);

label_t LabelPool_push(LabelPool* pool, const char* text);
label_t LabelPool_search(const LabelPool* pool, const char* text);


void CommonLabels_generate(CommonLabels* labels, LabelPool* pool);

#endif
