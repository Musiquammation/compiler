#ifndef TOOLS_QUEUE_H_
#define TOOLS_QUEUE_H_


#include "Array.h"

structdef(Queue);

// Can be cast into an Array*
struct Queue {
	void* data;
	ushort size;
	ushort length;
	ushort read;
    ushort write;
};


void Queue_create(Queue* queue, ushort size);
void* Queue_push(Queue* queue);
void* Queue_get(Queue* queue);
void* Queue_seek(const Queue* queue);
#define Queue_isEmpty(queue) ((queue).read == NULL_USHORT)
#define Queue_free(array) {if ((array).length) {free((array).data);};}


#endif