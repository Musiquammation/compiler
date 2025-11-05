#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "TraceStackHandler.h"
#include "helper.h"




const int TRACE_STACK_HANDLER_SIZE = sizeof(TraceStackHandler);

void TraceStackHandler_create(TraceStackHandler* handler, int labelLength) {
	handler->totalSize = 0;
	handler->holes = NULL;
	int* labels = malloc(labelLength * sizeof(int));

	for (int i = 0; i < labelLength; i++)
		labels[i] = -1;

	handler->labels = labels;
}

void TraceStackHandler_free(TraceStackHandler* handler) {
	TraceStackHandlerHole* current = handler->holes;
	while (current) {
		TraceStackHandlerHole* next = current->next;
		free(current);
		current = next;
	}
	free(handler->labels);

}

// Trouve le plus petit trou qui peut contenir 'size' éléments
// avec position % anchor == 0
static TraceStackHandlerHole** findBestHole(TraceStackHandler* handler, int size, int anchor) {
	TraceStackHandlerHole** current = &handler->holes;
	TraceStackHandlerHole** best = NULL;
	int bestSize = INT_MAX;
	
	while (*current) {
		int pos = (*current)->position;
		int holeSize = (*current)->size;
		
		// Calculer la position alignée dans ce trou
		int offset = (anchor - (pos % anchor)) % anchor;
		int alignedPos = pos + offset;
		int availableSize = holeSize - offset;
		
		if (availableSize >= size && holeSize < bestSize) {
			best = current;
			bestSize = holeSize;
		}
		
		current = &(*current)->next;
	}
	
	return best;
}

// Ajoute un trou dans la liste triée (par position)
static void insertHole(TraceStackHandler* handler, int position, int size) {
	TraceStackHandlerHole** current = &handler->holes;
	
	while (*current && (*current)->position < position) {
		current = &(*current)->next;
	}
	
	TraceStackHandlerHole* newHole = (TraceStackHandlerHole*)malloc(sizeof(TraceStackHandlerHole));
	newHole->position = position;
	newHole->size = size;
	newHole->next = *current;
	*current = newHole;
}

int TraceStackHandler_add(TraceStackHandler* handler, int size, int anchor, int label) {
	if (anchor <= 0) anchor = 1;
	
	TraceStackHandlerHole** bestHolePtr = findBestHole(handler, size, anchor);
	
	if (bestHolePtr) {
		// Utiliser un trou existant
		TraceStackHandlerHole* hole = *bestHolePtr;
		int pos = hole->position;
		int offset = (anchor - (pos % anchor)) % anchor;
		int alignedPos = pos + offset;
		
		// Créer un trou avant si nécessaire
		if (offset > 0) {
			insertHole(handler, pos, offset);
		}
		
		// Créer un trou après si nécessaire
		int remaining = hole->size - offset - size;
		if (remaining > 0) {
			insertHole(handler, alignedPos + size, remaining);
		}
		
		// Supprimer le trou utilisé
		*bestHolePtr = hole->next;
		free(hole);
		
		handler->labels[label] = alignedPos;
		return alignedPos;

	} else {
		// Allouer à la fin
		int pos = handler->totalSize;
		int offset = (anchor - (pos % anchor)) % anchor;
		int alignedPos = pos + offset;
		
		if (offset > 0) {
			insertHole(handler, pos, offset);
		}
		
		handler->totalSize = alignedPos + size;
		handler->labels[label] = alignedPos;
		return alignedPos;
	}
}


int TraceStackHandler_reach(TraceStackHandler* handler, int label) {
	return handler->labels[label];
}

int TraceStackHandler_guarantee(TraceStackHandler* handler, int size, int anchor, int label) {
	if (handler->labels[label] >= 0) {
		return handler->labels[label];
	}

	return TraceStackHandler_add(handler, size, anchor, label);
}


int TraceStackHandler_remove(TraceStackHandler* handler, int label, int size) {
	int address = handler->labels[label];
	handler->labels[label] = -1; // remove
	if (address < 0) {
		raiseError("[Intern] Stack tried to remove a variable absent from the stack");
	}

	// Trouver la position d'insertion et fusionner avec trous adjacents
	TraceStackHandlerHole** current = &handler->holes;
	TraceStackHandlerHole* prevHole = NULL;
	TraceStackHandlerHole* nextHole = NULL;
	
	int newPos = address;
	int newSize = size;
	
	// Chercher trou précédent et suivant
	while (*current) {
		if ((*current)->position + (*current)->size == address) {
			prevHole = *current;
		}
		if ((*current)->position == address + size) {
			nextHole = *current;
			break;
		}
		if ((*current)->position > address) {
			break;
		}
		current = &(*current)->next;
	}
	
	// Fusionner avec le trou précédent
	if (prevHole) {
		newPos = prevHole->position;
		newSize += prevHole->size;
		
		// Supprimer prevHole
		TraceStackHandlerHole** tmp = &handler->holes;
		while (*tmp != prevHole) tmp = &(*tmp)->next;
		*tmp = prevHole->next;
		free(prevHole);
	}
	
	// Fusionner avec le trou suivant
	if (nextHole) {
		newSize += nextHole->size;
		
		// Supprimer nextHole
		TraceStackHandlerHole** tmp = &handler->holes;
		while (*tmp != nextHole) tmp = &(*tmp)->next;
		*tmp = nextHole->next;
		free(nextHole);
	}
	
	// Insérer le nouveau trou fusionné
	insertHole(handler, newPos, newSize);
	
	return label;
}

