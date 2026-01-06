#include "Class.h"

#include "Scope.h"
#include "Function.h"
#include "Variable.h"

#include "helper.h"


#include <string.h>

void Class_create(Class* cl) {
	Array_create(&cl->variables, sizeof(Variable*));
	Array_create(&cl->functions, sizeof(Function*));
	cl->definitionState = DEFINITIONSTATE_UNDEFINED;
	cl->metaDefinitionState = DEFINITIONSTATE_UNDEFINED;
	cl->size = CLASSSIZE_UNDEFINED; // size is undefined
	cl->primitiveSizeCode = PSC_UNKNOWN;

	cl->std_methods.fastAccess = NULL;
}


void Class_delete(Class* cl) {
	if (cl->metaDefinitionState == DEFINITIONSTATE_DONE) {
		Class_delete(cl->meta);
		free(cl->meta);
	}

	// Delete variables
	Array_loopPtr(Variable, cl->variables, ptr) {
		Variable* i = *ptr;
		Variable_delete(i);
		free(i);
	}



	Array_free(cl->variables);

	// Delete functions
	Array_loopPtr(Function, cl->functions, ptr) {
		Function* i = *ptr;
		Function_delete(i);
		free(i);
	}

	Array_free(cl->functions);
}



bool Class_getCompleteDefinition(Class* cl, Scope* scope, bool throwError) {
	switch (cl->definitionState) {
	case DEFINITIONSTATE_UNDEFINED:
	{
		if (!Scope_defineOnFly(scope, cl->name)) {
			if (throwError) {raiseError("[Architecture] Cannot define class on fly");}
			return false;
		}
		return true;
	}

	case DEFINITIONSTATE_READING:
	{
		if (throwError) {raiseError("[Architecture] Cannot complete a class already being read");}
		return false;
	}

	case DEFINITIONSTATE_DONE:
	{
		return true;
	}

	case DEFINITIONSTATE_NOEXIST:
	{
		if (throwError) {raiseError("[Architecture] Class may not exist");}
		return false;
	}


	}
}



char Class_getPrimitiveSizeCode(const Class* cl) {
	if (cl->variables.length == 1) {
		Variable* v = *Array_get(Variable*, cl->variables, 0);
		return Prototype_getPrimitiveSizeCode(v->proto);
	}

	return 0;
}

label_t Class_generateMetaName(label_t name, char addChar) {
	size_t length = strlen(name);
	char* dest = malloc(length+2);
	memcpy(dest, name, length);
	dest[length] = addChar;
	dest[length+1] = 0;
	label_t result = LabelPool_push(&_labelPool, dest);
	free(dest);

	return result;
}



void Class_appendMetas(Class* cl) {
	MemoryCursor cursor;
	typedef Variable* var_ptr_t;


	Class* meta;
	switch (cl->metaDefinitionState) {
	case DEFINITIONSTATE_DONE:
	{
		raiseError("[Architecture] Cannot append meta to a DONE class");
		break;
	}

	case DEFINITIONSTATE_NOEXIST:
	{
		raiseError("[Architecture] Cannot append meta to a NOEXIST class");
		break;
	}
	
	case DEFINITIONSTATE_READING:
	{
		meta = cl->meta;
		cursor.offset = meta->size;
		cursor.maxMinimalSize = meta->maxMinimalSize;
		break;
	}
	
	case DEFINITIONSTATE_UNDEFINED:
	{
		// Search for meta arguments
		Array_loop(var_ptr_t, cl->variables, vptr) {
			Variable* source = *vptr;
			char hasMeta = Prototype_hasMeta(source->proto);
			switch (hasMeta) {
			case 1:
				goto containsMetaArgumentsLabel;

			case 0:
				break;

			case -1:
				raiseError("[TODO] does not know if there is a meta");
				return;
			}
		}

		
		// Here, no meta argument to append found
		return;


		containsMetaArgumentsLabel:
		cursor.offset = 0;
		cursor.maxMinimalSize = 0;
		
		meta = malloc(sizeof(Class));
		Class_create(meta);
		cl->meta = meta;

		cl->metaDefinitionState = DEFINITIONSTATE_READING;
		meta->definitionState = DEFINITIONSTATE_READING;
		meta->name = Class_generateMetaName(cl->name, '~');
		break;
	}

	}


	Array_loop(var_ptr_t, cl->variables, vptr) {
		Variable* source = *vptr;

		char hasMeta = Prototype_hasMeta(source->proto);

		if (hasMeta == 0)
			continue;

		if (hasMeta == -1) {
			raiseError("[TODO] handle unknown meta");
			continue;
		}

		Prototype* mp = Prototype_reachMeta(source->proto);
		/// TODO: check direct usage
		Class* mcl = mp->direct.cl;
		int offset = MemoryCursor_give(&cursor, mcl->size, mcl->maxMinimalSize);
		
		Variable* variable = malloc(sizeof(Variable));
		*Array_push(Variable*, &meta->variables) = variable;

		Variable_create(variable);
		variable->name = source->name;
		variable->proto = mp;
		variable->offset = offset;

		source->meta = variable;
	}


	meta->maxMinimalSize = cursor.maxMinimalSize;
	meta->size = MemoryCursor_align(cursor);
	meta->primitiveSizeCode = Class_getPrimitiveSizeCode(meta);

	Class_appendMetas(meta);
	return;
}



void Class_acheiveDefinition(Class* cl) {
	switch (cl->metaDefinitionState) {
	case DEFINITIONSTATE_UNDEFINED:
		// raiseError("[Architecture] Cannot acheive a class with unknown meta");
		cl->metaDefinitionState = DEFINITIONSTATE_NOEXIST;
		break;

	case DEFINITIONSTATE_READING:
		Class_acheiveDefinition(cl->meta);
		cl->metaDefinitionState = DEFINITIONSTATE_DONE;
		break;
		
	case DEFINITIONSTATE_NOEXIST:
	case DEFINITIONSTATE_DONE:
		raiseError("[Intern] Cannot acheive a class with NOEXIST/DONE state");
		break;
	}
	
	cl->definitionState = DEFINITIONSTATE_DONE;
}





void ScopeClass_delete(ScopeClass* scope) {

}





Variable* ScopeClass_searchVariable(ScopeClass* scope, label_t name, ScopeSearchArgs* args) {
	if (!scope->cl)
		return NULL;

	Array_loopPtr(Variable, scope->cl->variables, ptr) {
		Variable* v = *ptr;
		if (v->name == name)
			return v;
	}
	
	return NULL;
}

Class* ScopeClass_searchClass(ScopeClass* scope, label_t name, ScopeSearchArgs* args) {
	if (!scope->cl)
		return NULL;

	return NULL;
}

Function* ScopeClass_searchFunction(ScopeClass* scope, label_t name, ScopeSearchArgs* args) {
	if (!scope->cl)
		return NULL;

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

