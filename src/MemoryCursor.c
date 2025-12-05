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

int MemoryCursor_give(MemoryCursor* cursor, int size, int maxMinimalSize) {
	if (size == 0)
		return cursor->offset;

	int mms = maxMinimalSize;
	if (mms > cursor->maxMinimalSize)
		cursor->maxMinimalSize = mms;

	int d = cursor->offset % mms;
	if (d) {
		cursor->offset += mms - d;
	}

	int offset = cursor->offset;
	cursor->offset += maxMinimalSize;
	return offset;
}
