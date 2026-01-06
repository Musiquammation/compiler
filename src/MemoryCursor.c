#include "MemoryCursor.h"

#include <stdio.h>


int MemoryCursor_align(MemoryCursor cursor) {
	if (cursor.maxMinimalSize == 0) {
		return 0;
	}

	int d = cursor.offset % cursor.maxMinimalSize;
	if (d) {
		return cursor.offset + cursor.maxMinimalSize - d;
	}

	return cursor.offset;
}

int MemoryCursor_give(MemoryCursor* cursor, int size, int alignment) {
	if (size == 0)
		return cursor->offset;

	if (alignment > cursor->maxMinimalSize)
		cursor->maxMinimalSize = alignment;

	int misalignment = cursor->offset % alignment;
	if (misalignment != 0) {
		cursor->offset += alignment - misalignment;
	}

	int offset = cursor->offset;
	cursor->offset += size;
	return offset;
}

