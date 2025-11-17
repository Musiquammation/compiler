#include "Stack.h"

#include <stddef.h>
#include <stdlib.h>

void Stack_createfn(Stack* stack, size_t size) {
	stack->data = malloc(16 * size);
	stack->top = -1;
	stack->capacity = 16;
}

void* Stack_pushfn(Stack *s, size_t size) {
	s->top++;
	if (s->top >= s->capacity) {
		s->capacity *= 2;
		s->data = realloc(s->data, s->capacity * size);
	}

	return &s->data[s->top * size];
}

void* Stack_popfn(Stack *s, size_t size) {
    void *ptr = &s->data[s->top * size];
    s->top--;
    return ptr;
}

void* Stack_seekfn(Stack* s, size_t size) {
	return &s->data[s->top * size];
}

