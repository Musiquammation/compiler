#ifndef COMPILER_UTIL_STACK_H_
#define COMPILER_UTIL_STACK_H_

#include <stddef.h>

typedef struct {
	char *data;
	int top;
	int capacity;
} Stack;

void Stack_create(Stack* stack, size_t size);
#define Stack_free(s) {free(s.data);}
#define Stack_isEmpty(s) (s.top < 0)
void* Stack_pushfn(Stack *s, size_t size);
void* Stack_popfn(Stack *s, size_t size);

#define Stack_push(T, s) ((T*)Stack_pushfn(s, sizeof(T)))
#define Stack_pop(T, s) ((T*)Stack_popfn(s, sizeof(T)))




#endif