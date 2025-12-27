#include "Stream.h"
#include "Array.h"

#include <memory.h>

void Stream_create(Stream* stream) {
	Array_create((Array*)stream, sizeof(char*));
	stream->completion = 0;
}

void Stream_free(Stream* stream) {
	Array_loopPtr(char, *stream, i)
		free(*i);
	
	Array_free(*(Array*)stream);
}

void Stream_pushData(Stream* stream, const void* data, ushort length) {
	ushort completion = stream->completion;

	if (completion == 0)
		*Array_push(char*, (Array*)stream) = malloc(TOOLS_STREAM_SIZE); // allow
	

	ushort left = length;
	ushort index = stream->length - 1;
	

	// Push 512 blocks
	while (left + completion > TOOLS_STREAM_SIZE) {
		const ushort diff = TOOLS_STREAM_SIZE - completion;
		left -= diff;
		
		memcpy(*Array_get(char*, *(Array*)stream, index) + completion, data, diff);
		*Array_push(char*, (Array*)stream) = malloc(TOOLS_STREAM_SIZE);

		index++;
		completion = 0;
		data += diff;
	}

	// Push left blocks
	memcpy(*Array_get(char*, *(Array*)stream, index) + completion, data, left);
	if (left == TOOLS_STREAM_SIZE) {
		stream->completion = 0;
	} else {
		stream->completion = completion + left;
	}
}

ushort Stream_getLength(const Stream* stream) {
	const ushort length = stream->length;
	return length == 0 ? 0 : (length - 1) * TOOLS_STREAM_SIZE + stream->completion;
}

void* Stream_get(const Stream* stream) {
	ushort size = Stream_getLength(stream);
	if (size == 0)
		return NULL;
	
	void* const result = malloc(size + sizeof(int));
	void* dest = result;
	
	*((int*)dest) = size;
	dest += sizeof(int);

	const ushort l = stream->length - 1;

	for (char** i = stream->data, **const end = i + l; i != end; i++) {
		memcpy(dest, *i, TOOLS_STREAM_SIZE);
		dest += TOOLS_STREAM_SIZE;
	}

	memcpy(dest, *Array_get(char*, *(Array*)stream, l), size % TOOLS_STREAM_SIZE);
	
	return result;
}

