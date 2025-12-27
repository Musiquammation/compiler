#include "TArray.h"
#include <stdlib.h>
#include <string.h>

void TArray_create(TArray* array, ushort size) {
	array->reserved = 0;
	array->length = 0;
	array->size = size;
	array->readers = 0;
}

void TArray_createAllowed(TArray* array, ushort size, ushort allowed) {
	array->reserved = allowed;
	array->length = 0;
	array->size = size;
	array->data = malloc(size * allowed);
	array->readers = 0;
}

void TArray_copy(const TArray* src, TArray* dest) {
	const ushort length = src->length;
	const ushort size = src->size;

	dest->reserved = length;
	dest->length = length;
	dest->size = size;

	const ushort p = length * size;
	void* const data = malloc(p);
	dest->data = data;
	memcpy(dest->data, data, p);
}


void* TArray_pushFastArray(TArray* array) {
	void* ret = array->data + array->length * array->size;
	array->length++;
	return ret;
}

void* TArray_pushSafeArray(TArray* array) {
	const ushort length = array->length;
	const ushort reserved = array->reserved;
	
	// printf("%d %d\n", length, reserved);

	// Enought reserved data
	if (length < reserved)
		return TArray_pushFastArray(array);
	
	while (array->readers); // wait no thread reads
	array->readers = 255;

	// Realloc
	const ushort size = array->size;
	void* const newData = malloc(size * (reserved + TOOLS_ARRAY_CURRENT_ALLOWED_BUFFER_SIZE));
	
	if (reserved) {
		void* const oldData = array->data;
		memcpy(newData, oldData, reserved * size);
		free(oldData);
	}

	array->data = newData;
	array->reserved += TOOLS_ARRAY_CURRENT_ALLOWED_BUFFER_SIZE;
	array->length++;

	array->readers = 0;
	return newData + length * size;
}

void* TArray_tryDirectPush(TArray* array) {
	if (array->length < array->reserved)
		return TArray_pushFastArray(array);

	return NULL;	
}


void* TArray_pushInEmpty(TArray* array, bool(*isEmpty)(const void*)) {
	const ushort size = array->size;
	for (void* i = array->data, *const end = i + size * array->length; i != end; i += size)
		if ((*isEmpty)(i))
			return i;
	
	return TArray_pushSafeArray(array);
}


void* TArray_search(TArray* array, const void* data, Array_SortComparator_t* comparator) {
	const ushort size = array->size;
	for (void* ptr = array->data, *end = ptr += array->length * size; ptr != end; ptr++) {
		if ((*comparator)(ptr, data) == 0)
			return ptr;
	}

	return NULL;
}


void* TArray_binarySearch(const TArray* array, const void* data, Array_SortComparator_t* comparator) {
	int max = array->length - 1;
	if (max == -1)
		return NULL; // array is empty

	int min = 0;

	const size_t size = array->size;
	void* const list = array->data;

	while (min < max) {
		const int i = ((max - min) >> 1) + min;

		int cmp = (*comparator)(list + size*i, data);
		if (cmp == 0)
			return list + size*i;
		
		if (cmp > 0) {
			max = i-1;
		} else {
			min = i+1;
		}
	}

	if ((*comparator)(list + size*min, data) == 0) {
		return list + size*min;
	}

	return NULL; // not found
}

void* TArray_binaryCompare(TArray* array, const void* data, Array_SortComparator_t* comparator) {
	int max = array->length - 1;
	if (max == -1)
		return NULL; // array is empty

	const size_t size = array->size;
	void* const list = array->data;

	// Only one element
	if (max == 0)
		return (*comparator)(list, data) >= 0 ? list : list+size;
	
	if ((*comparator)(list, data) >= 0)
		return list;


	if ((*comparator)(list + size*max, data) <= 0)
		return list + array->length * size; // last element


	int min = 0;
	while (true) {
		const int i = ((max - min) >> 1) + min;
		const int cmp = (*comparator)(list + size*i, data);

		if (cmp == 0)
			return list + size*i;
		
		if (cmp > 0) {
			max = i;
		} else {
			min = i;
		}

		if (max - min == 1) {
			return list + size*max;
		}
	}

}


void* TArray_pushSort(TArray* array, const void* data, Array_SortComparator_t* comparator) {
	const ushort length = array->length;

	const size_t size = array->size;
	
	// list is empty
	if (length == 0) {
		void* const ptr = TArray_pushSafeArray(array);
		memcpy(ptr, data, size);
		return ptr;
	}

	// allow space for the new element
	TArray_pushSafeArray(array);
	array->length--;
	

	void* const ptr = TArray_binaryCompare(array, data, comparator);
	void* const list = array->data;
	memmove(ptr+size, ptr, list + length * size - ptr);
	memcpy(ptr, data, size);
	array->length++;
	
	return ptr;
}


void TArray_shrinkToFit(TArray* array) {
	const ushort length = array->length;
	array->reserved = length;

	const ushort total = length * array->size;
	void* const newData = malloc(total);
	void* const oldData = array->data;
	memcpy(newData, oldData, total);
	free(oldData);
}

void* TArray_reach(TArray* array, ushort index, const void* emptyValue) {
	const ushort length = array->length;

	if (length == index)
		return TArray_pushSafeArray(array);
	
	if (index < length)
		return TArray_getAt(array, index);
	

	TArray_access(array);
	// Here, index > length
	const ushort reserved = array->reserved;
	const ushort size = array->size;

	if (reserved == 0) {
		array->reserved = index+1;
		array->length = index+1;

		void* ptr = malloc(size * (index+1));
		array->data = ptr;

		void* const end = ptr + index * size;
		for (; ptr != end; ptr += size)
			memcpy(ptr, emptyValue, size);

		return ptr;
	}


	void* const data = array->data;
	if (index < reserved) {
		void* const aim = data + index * size;

		
		for (void* ptr = data + length * size; ptr != aim; ptr += size)
			memcpy(ptr, emptyValue, size);
		
		array->length = index+1;
		return aim;
	}

	void* const newData = malloc(size * (index+1));
	array->data = newData;
	array->reserved = index+1;
	array->length = index+1;

	// Copy old data
	memcpy(newData, data, size * length);
	
	// Fill with emptyValue
	for (void* ptr = newData + size * length, *const end = newData + index * size; ptr!= end; ptr += size)
		memcpy(ptr, emptyValue, size);
	

	return newData + index * size;
}

void TArray_access(TArray* array) {
	while (array->readers == 255);
	array->readers++;
}

void* TArray_getAt(TArray* array, ushort index) {
	while (array->readers == 255);
	array->readers++;
	return array->data + array->size * index;
}

