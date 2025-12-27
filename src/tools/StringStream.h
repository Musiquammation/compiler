#ifndef TOOLS_STRINGSTREAM_H_
#define TOOLS_STRINGSTREAM_H_

#include "tools.h"
#include "Array.h"
#include <pthread.h>


structdef(StringStream);

struct StringStream {
	char* data;
	ushort read;
	ushort write;
	ushort reserved;	
};

void StringStream_create(StringStream* stream, ushort bufferInitialSize);
#define StringStream_free(stream) {free((stream).data);}

void StringStream_push(StringStream* stream, const char* data, ushort length);
char StringStream_readChar(StringStream* stream, char notValue);

char* StringStream_read(StringStream* stream, ushort length, bool* hasGeneratedACopy);
char* StringStream_readCopy(StringStream* stream, ushort length);


#endif