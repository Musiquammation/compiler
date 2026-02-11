#include "FunctionArgProjection.h"

Prototype* ScopeFunctionArgProjection_globalSearchPrototype(Scope* scope, label_t name) {
	while (scope) {
		if (scope->type == SCOPE_PROJECTIONS) {
			ScopeFunctionArgProjection* s = (ScopeFunctionArgProjection*)scope;
			Array_for(FunctionArgProjection, s->projections, s->length, p) {
				if (p->name == name)
					return p->proto;
			}
		}

		scope = scope->parent;
	}
}