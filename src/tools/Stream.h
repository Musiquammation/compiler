#ifndef TOOLS_STREAM_H_
#define TOOLS_STREAM_H_

#include "tools.h"

enum {TOOLS_STREAM_SIZE = 512};

structdef(Stream);

struct Stream {
    void* data;
	ushort size;
	ushort length;
	ushort reserved;
	
    ushort completion;
};

void Stream_create(Stream* stream);
void Stream_free(Stream* stream);

void Stream_pushData(Stream* stream, const void* data, ushort length);

ushort Stream_getLength(const Stream* stream);
void* Stream_get(const Stream* stream); // starts by an int giving size

#define Stream_push(streamPtr, var) Stream_pushData(streamPtr, &(var), sizeof(var));


#endif

