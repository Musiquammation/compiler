#include "Queue.h"
#include "Array.h"

#include <string.h>

void Queue_create(Queue* queue, ushort size) {
	queue->size = size;
	queue->length = 0;
	queue->write = 0;
}


void* Queue_push(Queue* queue) {
	const ushort length = queue->length;

	// No data
	if (length == 0) {
		void* data = malloc(queue->size * TOOLS_ARRAY_CURRENT_ALLOWED_BUFFER_SIZE);

		queue->data = data;
		queue->length = TOOLS_ARRAY_CURRENT_ALLOWED_BUFFER_SIZE;
		queue->read = 0;
		queue->write = 1;

		return data;
	}

	const ushort read = queue->read;
	const ushort write = queue->write;
	
	// Empty (so write == 0)
	if (read == NULL_USHORT) {
		queue->read = 0;
		queue->write = 1;
		return queue->data;
	}

	// Queue is full
	if (write == read) {
		const ushort size = queue->size;
		const ushort newLength = length + TOOLS_ARRAY_CURRENT_ALLOWED_BUFFER_SIZE;
		
		void* newData = malloc(size * newLength);
		void* oldData = queue->data;
		memcpy(newData, oldData + write * size, (length - write) * size);
		memcpy(newData + (length - write) * size, oldData, write * size);

		queue->read = 0;
		queue->write = length + 1;
		queue->data = newData;
		queue->length = newLength;
		free(oldData);

		return newData + size * length;
	}

	if (write+1 == length) {
		queue->write = 0;
	} else {
		queue->write++;
	}

	return queue->data + queue->size * write;	
}


void* Queue_get(Queue* queue) {
	const ushort length = queue->length;

	if (length == 0)
		return NULL; // empty
	

	const ushort read = queue->read;

	if (read == NULL_USHORT)
		return NULL; // nothing to read
	
	const ushort write = queue->write;
	
	// Queue is full, but we just remove one element
	if (read == write) {
		if (read+1 == length) {
			queue->read = 0;
		} else {
			queue->read++;
		}

		return queue->data + queue->size * read;
	}

	if (read < write) {
		if (read+1 == write) {
			queue->read = NULL_USHORT;
			queue->write = 0;
			return queue->data + queue->size * read;
		}

		queue->read++;
		return queue->data + queue->size * read;
	}

	// here, read > write
	{
		if (read+1 == length) {
			queue->read = write == 0 ? NULL_USHORT : 0;
		} else if (read+1 == write) {
			queue->write = 0;
			queue->read = NULL_USHORT;
		} else {
			queue->read++;
		}

		return queue->data + queue->size * read;
	}
}


void* Queue_seek(const Queue* queue) {
	if (queue->length == 0)
		return NULL; // empty
	
	const ushort read = queue->read;
	if (read == NULL_USHORT)
		return NULL; // nothing to read
	
	return queue->data + queue->size * read;	
}
