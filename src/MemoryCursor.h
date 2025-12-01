#ifndef COMPILER_MEMORYCURSOR_H_
#define COMPILER_MEMORYCURSOR_H_

typedef struct {
   	int offset;
	int maxMinimalSize;
} MemoryCursor;



int MemoryCursor_align(MemoryCursor cursor);
int MemoryCursor_give(MemoryCursor* cursor, int size, int maxMinimalSize);



#endif