#include "Class.h"

#include "Scope.h"
#include "Function.h"
#include "Variable.h"

#include "helper.h"

void Class_create(Class* cl) {
	Array_create(&cl->variables, sizeof(Variable*));
	Array_create(&cl->functions, sizeof(Function*));
	cl->definitionState = DEFINITIONSTATE_UNDEFINED;
	cl->metaDefinitionState = DEFINITIONSTATE_UNDEFINED;
	cl->size = CLASSSIZE_UNDEFINED; // size is undefined
	cl->isRegistrable = REGISTRABLE_UNKNOWN;
	cl->primitiveSizeCode = 0;
}


void Class_delete(Class* cl) {
	printf("for {%p %d} %s (%d) [%d]\n", cl, cl->definitionState, cl->name, cl->metaDefinitionState, cl->size);
	if (cl->metaDefinitionState == DEFINITIONSTATE_DONE) {
		Class_delete(cl->meta);
		free(cl->meta);
	}


	// Delete variables
	Array_loopPtr(Variable, cl->variables, ptr) {
		Variable* i = *ptr;
		printf("\t%s\n", i->name);
		Variable_delete(i);
		free(i);
	}
	printf("end %s\n", cl->name);


	Array_free(cl->variables);

	// Delete functions
	Array_loopPtr(Function, cl->functions, ptr) {
		Function* i = *ptr;
		Function_delete(i);
		free(i);
	}

	Array_free(cl->functions);

}




Class* Class_appendMeta(Class* cl, Class* meta, bool containsMetaArguments) {
	MemoryCursor cursor;
	typedef Variable* var_ptr_t;

	printf("append %d %p to %p\n", cl->metaDefinitionState, meta, cl);

	switch (cl->metaDefinitionState) {
	case DEFINITIONSTATE_DONE:
	{
		printf("done\n");
		return meta;
	}

	case DEFINITIONSTATE_READING:
	{
		cursor.offset = meta->size;
		cursor.maxMinimalSize = meta->maxMinimalSize;
		break;
	}
	
	case DEFINITIONSTATE_UNDEFINED:
	case DEFINITIONSTATE_NOEXIST:
	{
		// Search for meta arguments
		if (!containsMetaArguments) {
			Array_loop(var_ptr_t, cl->variables, vptr) {
				Variable* source = *vptr;
				Prototype* meta = Prototype_getMeta(source->proto);
				if (meta) {
					goto containsMetaArgumentsLabel;
				}
			}
			
			printf("cut\n");
			cl->metaDefinitionState = DEFINITIONSTATE_NOEXIST;
			cl->meta = NULL;
			cl->definitionState = DEFINITIONSTATE_DONE;
			return NULL;
		}
		
		containsMetaArgumentsLabel:
		cursor.offset = 0;
		cursor.maxMinimalSize = 0;
		meta = malloc(sizeof(Class));
		printf("new %p of %p\n", meta, cl);
		meta->name = NULL;
		Class_create(meta);
		break;
	}

	}



	Array_loop(var_ptr_t, cl->variables, vptr) {
		Variable* source = *vptr;
		Prototype* mp = Prototype_getMeta(source->proto);
		if (!mp)
			continue;

		/// TODO: check direct usage
		Class* mcl = mp->direct.cl;
		printf("add %s[%p] to %p\n", source->name, mcl, cl);
		int offset = MemoryCursor_give(&cursor, mcl->size, mcl->maxMinimalSize);
		
		Variable* variable = malloc(sizeof(Variable));
		*Array_push(Variable*, &meta->variables) = variable;

		Variable_create(variable);
		variable->name = source->name;
		variable->proto = mp;
		variable->offset = offset;
	}

	meta->maxMinimalSize = cursor.maxMinimalSize;
	meta->size = MemoryCursor_align(cursor);

	cl->meta = meta;
	cl->metaDefinitionState = DEFINITIONSTATE_DONE;
	
	/// TODO: warning to this line
	Class_appendMeta(meta, meta->metaDefinitionState == DEFINITIONSTATE_NOEXIST ? NULL : meta->meta, false);
	printf("erase %p of %p\n", cl->meta, cl);
	
	cl->definitionState = DEFINITIONSTATE_DONE;
	return meta;
}


void ScopeClass_delete(ScopeClass* scope) {

}



Variable* ScopeClass_searchVariable(ScopeClass* scope, label_t name, ScopeSearchArgs* args) {
	Array_loopPtr(Variable, scope->cl->variables, ptr) {
		Variable* v = *ptr;
		if (v->name == name)
			return v;
	}
	
	return NULL;
}

Class* ScopeClass_searchClass(ScopeClass* scope, label_t name, ScopeSearchArgs* args) {
	return NULL;
}

Function* ScopeClass_searchFunction(ScopeClass* scope, label_t name, ScopeSearchArgs* args) {
	Array_loopPtr(Function, scope->cl->functions, ptr) {
		Function* f = *ptr;
		if (f->name == name)
			return f;
	}
	
	return NULL;
}


void ScopeClass_addVariable(ScopeClass* scope, Variable* v) {
	raiseError("[TODO]");

}

void ScopeClass_addClass(ScopeClass* scope, Class* cl) {
	raiseError("[TODO]");

}

void ScopeClass_addFunction(ScopeClass* scope, Function* fn) {
	*Array_push(Function*, &scope->cl->functions) = fn;
}

