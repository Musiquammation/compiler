#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "TraceStackHandler.h"
#include "helper.h"




const int TRACE_STACK_HANDLER_SIZE = sizeof(TraceStackHandler);

void TraceStackHandler_create(TraceStackHandler* handler, int labelLength) {
	handler->totalSize = 0;
	handler->holes = NULL;
	TraceStackHandlerPosition* positions =	
		malloc(labelLength * sizeof(TraceStackHandlerPosition));

	for (int i = 0; i < labelLength; i++)
		positions[i].position = -1;

	handler->positions = positions;
}

void TraceStackHandler_free(TraceStackHandler* handler) {
	TraceStackHandlerHole* current = handler->holes;
	while (current) {
		TraceStackHandlerHole* next = current->next;
		free(current);
		current = next;
	}
	free(handler->positions);

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
	if (anchor <= 0) {
		anchor = 1;
	}
	
	// Init freeze
	handler->positions[label].freeze = 0;

	
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
		
		handler->positions[label].position = alignedPos;
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
		handler->positions[label].position = alignedPos;
		return alignedPos;
	}
}


int TraceStackHandler_reach(TraceStackHandler* handler, int label) {
	return handler->positions[label].position;
}

int TraceStackHandler_guarantee(TraceStackHandler* handler, int size, int anchor, int label) {
	if (handler->positions[label].position >= 0) {
		return handler->positions[label].position;
	}

	return TraceStackHandler_add(handler, size, anchor, label);
}




void TraceStackHandler_remove(TraceStackHandler* handler, int label, int size) {
	int address = handler->positions[label].position;
	handler->positions[label].position = -1; // remove
	if (address < 0) {
		raiseError("[Intern] Stack tried to remove a variable absent from the stack");
	}

	// Resist
	if (handler->positions[label].freeze > 0) {
		return;
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
}


void TraceStackHandler_freeze(TraceStackHandler* handler, int label) {
	handler->positions[label].freeze++;
}

void TraceStackHandler_unfreeze(TraceStackHandler* handler, int label) {
	handler->positions[label].freeze--;
}

