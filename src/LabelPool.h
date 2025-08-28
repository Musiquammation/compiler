#ifndef COMPILER_LABEL_POOL_H_
#define COMPILER_LABEL_POOL_H_

#include "label_t.h"

#include <tools/Array.h>



typedef struct {
    char** data;
    int size;
    int capacity;
} LabelPool;


void LabelPool_create(LabelPool* pool);
void LabelPool_delete(LabelPool* pool);

label_t LabelPool_push(LabelPool* pool, const char* text);
label_t LabelPool_search(const LabelPool* pool, const char* text);

#endif
