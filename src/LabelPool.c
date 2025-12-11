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
	#define def(n) labels->_ ##n = LabelPool_push(pool, #n);

	def(let);
	def(const);
	def(class);
	def(function);
	def(this);
	def(if);
	def(else);
	def(for);
	def(while);
	def(switch);

	def(i8);
	def(u8);
	def(i16);
	def(u16);
	def(i32);
	def(u32);
	def(i64);
	def(u64);
	def(f32);
	def(f64);

	def(char);
	def(bool);
	def(short);
	def(int);
	def(long);
	def(float);
	def(double);
	def(size_t);

	def(Pointer);
	def(Type);
	def(Variadic);
	def(Token);

	def(module);
	def(main);
	def(control);
	def(langstd);
	def(token);
	def(fastAccess);
	def(stdFastAccess);




	#undef def
}
