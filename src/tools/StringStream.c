#include "StringStream.h"

#include <string.h>
#include <stdio.h>

void StringStream_create(StringStream* stream, ushort bufferInitialSize) {
	stream->data = malloc(bufferInitialSize);
	stream->read = 0;
	stream->write = 0;
	stream->reserved = bufferInitialSize;
}

void StringStream_push(StringStream* stream, const char* data, ushort length) {
	const ushort read = stream->read;
	const ushort write = stream->write;
	ushort reserved = stream->reserved;
	
	if (read <= write) {
		if (write + length <= reserved) {
			memcpy(stream->data + write, data, length);

			if (write + length == reserved) {
				stream->write = 0;
			} else {
				stream->write += length;
			}

			return;
		}

		const ushort rightPart = reserved - write;
		const ushort leftPart = length - rightPart;
		char* const streamData = stream->data;

		if (leftPart > read) {
			// Add memory
			reserved += length;
			char* newData = malloc(reserved);
			char* const oldData = stream->data;
			memcpy(newData + read, oldData + read, write - read);
			free(oldData);

			memcpy(newData + write, data, length);

			stream->data = newData;
			stream->write += length;
			stream->reserved = reserved;
			return;
		}

		// Share it two parts
		memcpy(streamData + write, data, rightPart);
		memcpy(streamData, data + rightPart, leftPart);
		stream->write = leftPart;
		return;
	}

	if (write + length <= read) {
		memcpy(stream->data + write, data, length);
		stream->write += length;
		return;
	}

	// Move data
	const ushort newReserved = reserved + length;
	char* const oldData = stream->data;
	char* newData = malloc(newReserved);

	const ushort rightPart = reserved - read;
	memcpy(newData, oldData + read, rightPart); // copy read part
	memcpy(newData + rightPart, oldData, write); // copy write part
	memcpy(newData + rightPart + write, data, length); // copy data

	stream->data = newData;
	stream->reserved = newReserved;
	stream->read = 0;
	stream->write = rightPart + write + length;
}

char StringStream_readChar(StringStream* stream, char notValue) {
	const ushort read = stream->read;

	if (read == stream->write)
		return notValue;
	
	if (read == stream->reserved - 1) {
		stream->read = 0;
		return stream->data[read];
	}

	stream->read++;
	return stream->data[read];
}

char* StringStream_read(StringStream* stream, ushort length, bool* hasGeneratedACopy) {
	const ushort read = stream->read;
	const ushort write = stream->write;

	if (read == write)
		return NULL;
	
	if (read < write) {
		if (read + length > write)
			return NULL; // No enought content to read

		*hasGeneratedACopy = false;
		stream->read += length;
		return stream->data + read;
	}
	
	
	const ushort reserved = stream->reserved;
	const ushort rightPart = reserved - read;
	
	if (rightPart + write < length)
		return NULL; // No enought content to read
	
	if (read + length <= reserved) {
		*hasGeneratedACopy = false;
		stream->read += length;
		return stream->data + read;
	}

	const ushort leftPart = length - rightPart;

	*hasGeneratedACopy = true;
	char* msg = malloc(length);
	char* const data = stream->data;
	memcpy(msg, data + read, rightPart);
	memcpy(msg + rightPart, data, leftPart);

	stream->read = leftPart;
	return msg;
}

char* StringStream_readCopy(StringStream* stream, ushort length) {
	bool hasGeneratedACopy;
	char* str = StringStream_read(stream, length, &hasGeneratedACopy);

	if (str == NULL)
		return NULL;

	if (hasGeneratedACopy)
		return str;
	
	// Generate a copy
	char* ret = malloc(length);
	memcpy(ret, str, length);
	return ret;

}
