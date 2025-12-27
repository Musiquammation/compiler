#ifndef TOOLS_TARRAY_H_
#define TOOLS_TARRAY_H_

#include <stdbool.h>
#include <stdlib.h>

#include "tools.h"
#include "Array.h"


structdef(TArray);

#define TArray_loop(Type, array, i)\
for (Type *i = (array).data, *const i##_end = i + (array).length; i != i##_end; i++)

#define TArray_loopPtr(Type, array, i)\
for (Type **i = (array).data, **const i##_end = i + (array).length; i != i##_end; i++)

#define TArray_getPos(ptr, array) (((void*)(ptr) - (array).data) / (array).size)


struct TArray {
	void* data;
	ushort size;
	ushort length;
	ushort reserved;
    uchar readers;
};


void TArray_create(TArray* array, ushort size);
void TArray_createAllowed(TArray* array, ushort size, ushort allowed);
#define TArray_free(array) {if ((array).reserved) {free((array).data);};}

void TArray_copy(const TArray* src, TArray* dest);
void* TArray_pushFastArray(TArray* array);
void* TArray_pushSafeArray(TArray* array);
void* TArray_search(TArray* array, const void* data, Array_SortComparator_t* comparator);
void* TArray_binarySearch(const TArray* array, const void* data, Array_SortComparator_t* comparator); // TArray should be sorted
void* TArray_binaryCompare(TArray* array, const void* data, Array_SortComparator_t* comparator); // TArray should be sorted
void* TArray_pushSort(TArray* array, const void* data, Array_SortComparator_t* comparator); // TArray should be sorted
void* TArray_tryDirectPush(TArray* array);
void* TArray_pushInEmpty(TArray* array, bool(*isEmpty)(const void*));
void TArray_shrinkToFit(TArray* array);
void* TArray_reach(TArray* array, ushort index, const void* emptyValue);
#define TArray_push(Type, arrayPtr) ((Type*)TArray_pushSafeArray((arrayPtr)))

void TArray_access(TArray* array);
void* TArray_getAt(TArray* array, ushort index);
#define TArray_get(Type, array, index) ((array).readers ? (Type*)((array).data) + (index) : (Type*)TArray_getAt(&(array), (index)))

#define TArray_dispose(array) {(array).readers--;}

#endif