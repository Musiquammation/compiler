#ifndef COMPILER_UTIL_AVL_H_
#define COMPILER_UTIL_AVL_H_

#include <stdbool.h>

typedef struct AVLNode {
	void* data;
	struct AVLNode* left;
	struct AVLNode* right;
} AVLNode;


typedef bool(AVL_compare_t)(const void* a, const void* b);

AVLNode* AVL_insert(AVLNode* node, void* data, AVL_compare_t compare);
void AVL_delete(AVLNode* node);
void* AVL_search(AVLNode* node, const void* data);


#endif