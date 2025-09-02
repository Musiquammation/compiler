// label_pool.c
#include <stdlib.h>
#include <string.h>
#include "LabelPool.h"


void LabelPool_create(LabelPool* pool) {
	pool->data = NULL;
	pool->size = 0;
	pool->capacity = 0;
}

void LabelPool_delete(LabelPool* pool) {
    for (int i = 0; i < pool->size; i++) {
        free(pool->data[i]);
    }
	free(pool->data);
	pool->data = NULL;
	pool->size = 0;
	pool->capacity = 0;
}

static void LabelPool_grow(LabelPool* pool) {
	int new_capacity = (pool->capacity == 0) ? 8 : pool->capacity * 2;
	pool->data = realloc(pool->data, new_capacity * sizeof(char*));
	pool->capacity = new_capacity;
}

label_t LabelPool_push(LabelPool* pool, const char* text) {
    if (pool->size == pool->capacity) {
        LabelPool_grow(pool);
    }

    int left = 0, right = pool->size;
    while (left < right) {
        int mid = (left + right) / 2;
        int cmp = strcmp(pool->data[mid], text);
        if (cmp == 0) {
            // Déjà présent
            return pool->data[mid];
        } else if (cmp < 0) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }

    memmove(&pool->data[left + 1], &pool->data[left],
            (pool->size - left) * sizeof(char*));

	size_t length = strlen(text) + 1;
    char* copy = malloc(length);
	memcpy(copy, text, length);
    pool->data[left] = copy;
    pool->size++;

    return copy;
}

label_t LabelPool_search(const LabelPool* pool, const char* text) {
	int left = 0, right = pool->size;
	while (left < right) {
		int mid = (left + right) / 2;
		int cmp = strcmp(pool->data[mid], text);
		if (cmp == 0) return pool->data[mid];
		else if (cmp < 0) left = mid + 1;
		else right = mid;
	}
	return LABEL_NULL;
}



void CommonLabels_generate(CommonLabels* labels, LabelPool* pool) {
    labels->_let      = LabelPool_push(pool, "let");
    labels->_const    = LabelPool_push(pool, "const");
    labels->_function = LabelPool_push(pool, "function");
    labels->_class    = LabelPool_push(pool, "class");
    labels->_module   = LabelPool_push(pool, "module");
    labels->_main     = LabelPool_push(pool, "main");
}