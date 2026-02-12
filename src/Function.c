#include "Function.h"

#include "Expression.h"
#include "Type.h"
#include "Variable.h"
#include "Scope.h"
#include "Class.h"
#include "Interpreter.h"

#include "langstd.h"
#include "helper.h"

#include <string.h>

long functionNextId = 0;

void Function_create(Function* fn) {
	fn->stdBehavior = -1;
	fn->traceId = functionNextId;
	fn->interpreter = NULL;
	functionNextId++;
	fn->requires_len = 0;
	fn->projections_len = 0;
}

void Function_delete(Function* fn) {
	if (fn->requires_len) {
		free(fn->labelRequires); // or frees realRequires since they are in a same union
	}

	if (fn->projections_len) {
		Array_for(FunctionArgProjection, fn->projections, fn->projections_len, p) {
			if (p->name) {
				Prototype_free(p->proto, true);
			}
		}

		free(fn->projections);
	}
	
	typedef Variable* vptr_t;
	if (fn->metaDefinitionState == DEFINITIONSTATE_DONE) {
		Function_delete(fn->meta);
		free(fn->meta);
	}

	if (fn->arguments) {
		Array_for(vptr_t, fn->arguments, fn->args_len, vptr) {
			Variable* v = *vptr;
			Variable_destroy(v);
			free(v);
		}
		free(fn->arguments);
	}

	if (fn->returnPrototype) {
		Prototype_free(fn->returnPrototype, true);
	}

	if (fn->settings) {
		Array_for(vptr_t, fn->settings, fn->settings_len, vptr) {
			Variable* v = *vptr;
			Variable_destroy(v);
			free(v);
		}
		free(fn->settings);
	}

	if (fn->interpreter) {
		Interpreter_delete(fn->interpreter);
		free(fn->interpreter);
	}
}


label_t Function_generateMetaName(label_t name, char addChar) {
	if (name == NULL)
		return NULL;

	size_t length = strlen(name);
	char* dest = malloc(length+2);
	memcpy(dest, name, length);
	dest[length] = addChar;
	dest[length+1] = 0;
	label_t result = LabelPool_push(&_labelPool, dest);
	free(dest);

	return result;
}


Function* Function_produceMeta(Function* origin) {
	int originArgLen = origin->args_len;
	int originSettingLength = origin->settings_len;
	Array arguments;
	Array_create(&arguments, sizeof(Variable*));

	// Fill projections
	typedef Variable* vptr_t;
	Array_for(FunctionArgProjection, origin->projections, origin->projections_len, proj) {
		if (proj->name) {
			ProtoSetting* setting = malloc(sizeof(ProtoSetting));
			setting->useVariable = true;
			setting->useProto = true;
			setting->variable = *Array_get(Variable*, _langstd.pointer->meta->variables, 0);

			Prototype* metaProto = Prototype_reachMeta(proj->proto);
			setting->proto = metaProto;
			Prototype_addUsage(*metaProto);
			
			Prototype* ptrProto = Prototype_create_direct(
				_langstd.pointer, 8, setting, 1
			);

			Variable* dst = malloc(sizeof(Variable));
			Variable_create(dst);
			dst->name = proj->name;
			dst->proto = ptrProto;
			dst->id = arguments.length;
			*Array_push(Variable*, &arguments) = dst;
		}
	}

	// Fill settings
	Array_for(vptr_t, origin->settings, origin->settings_len, vptr) {
		Variable* src = *vptr;
		Prototype* proto = src->proto;
		Prototype_addUsage(*proto);

		Variable* dst = malloc(sizeof(Variable));
		Variable_create(dst);
		dst->name = src->name;
		dst->proto = proto;
		dst->id = arguments.length;
		*Array_push(Variable*, &arguments) = dst;
	}


	Function* fn = malloc(sizeof(Function));
	Function_create(fn);
	fn->name = Function_generateMetaName(origin->name, '^');
	fn->definitionState = DEFINITIONSTATE_READING;
	fn->metaDefinitionState = DEFINITIONSTATE_UNDEFINED;

	fn->arguments = realloc(arguments.data, sizeof(Variable*) * arguments.length);
	fn->args_len = arguments.length;
	fn->settings = NULL;
	fn->settings_len = 0;
	fn->flags = FUNCTIONFLAGS_THIS | FUNCTIONFLAGS_INTERPRET;
	if (origin->flags & FUNCTIONFLAGS_ARGUMENT_CONSTRUCTOR) {
		fn->flags |= FUNCTIONFLAGS_ARGUMENT_CONSTRUCTOR;
	}

	// Fill return type
	Prototype* returnType = origin->returnPrototype;
	if (returnType) {
		returnType = Prototype_reachMeta(returnType);
		fn->returnPrototype = returnType;
		if (returnType) {
			Prototype_addUsage(*returnType);
		}
	} else {
		fn->returnPrototype = NULL;
	}

	return fn;
}



void Function_makeRequiresReal(Function* fn, Class* thisclass) {
	if (fn->flags & FUNCTIONFLAGS_REAL_REQUIRES)
		return;
	
	fn->flags |= FUNCTIONFLAGS_REAL_REQUIRES;

	int requires_len = fn->requires_len;
	if (requires_len == 0)
		return;
	
	labelRequireCouple_t* const labelRqs = fn->labelRequires;
	realRequireCouple_t* const requires = requires_len ?
		malloc(sizeof(realRequireCouple_t) * requires_len) : NULL;

	if (thisclass) {
		switch (thisclass->metaDefinitionState) {
		case DEFINITIONSTATE_UNDEFINED:
			raiseError("[Undefined] Missing definition state for @require");
			return;

		case DEFINITIONSTATE_READING:
			raiseError("[Undefined] Reading definition state for @require");
			return;

		case DEFINITIONSTATE_DONE:
			thisclass = thisclass->meta;
			break;
			
		case DEFINITIONSTATE_NOEXIST:
			thisclass = NULL;
			break;
		}
	} else {
		thisclass = NULL;
	}

	ScopeSearchArgs searchArgs[] = {
		{.resultType = SCOPESEARCH_FUNCTION}
	};

	ScopeClass sc = {
		.scope = {NULL, SCOPE_CLASS},
		.allowThis = false
	};

	typedef Variable* var_t;
	var_t* fnArguments = fn->arguments;
	int fnArgsLen = fn->args_len;

	for (int i = 0; i < requires_len; i++) {
		// Search object in args
		label_t objectName = labelRqs[i].object;
		int position;
		if (objectName == _commonLabels._this) {			
			if (thisclass == NULL) {
				raiseError("[Syntax] 'this' is not defined here");
				return;
			}

			position = 0;
			sc.cl = thisclass;

		} else {
			// Search variable in function arguments
			Array_for(var_t, fnArguments, fnArgsLen, vptr) {
				Variable* variable = *vptr;
				if (variable->name == objectName) {
					sc.cl = Prototype_getMetaClass(variable->proto);
					goto found;
				}
			}

			raiseError("[Unfound] Variable used in @require not found");

			found:
			if (thisclass) {
				position++; // decalage for this
			}
		}

		Function* sub = Scope_search(&sc.scope, labelRqs[i].fn, searchArgs, SCOPESEARCH_FUNCTION);
		if (!sub) {
			raiseError("[Unfound] Cannot find method from @require");
			return;
		}

		if ((sub->flags & FUNCTIONFLAGS_CONDITION) == 0) {
			raiseError("[TODO] Missing FUNCTIONFLAGS_CONDITION flag");
		}

		requires[i].position = position;
		requires[i].fn = sub;
		printf("got %s at %d\n", sub->name, position);

	}
		

	fn->realRequires = requires;
	free(labelRqs);
}




void FunctionAssembly_create(FunctionAssembly* fa, ScopeFunction* sf) {
	ScopeFile* file = Scope_reachFile(&sf->scope);

	fa->output = ScopeFile_requireAssembly(file, file->generationState);
	fa->fn = sf->fn;
}

void FunctionAssembly_delete(FunctionAssembly* fa) {

}




static void fillArgument(ScopeFunction* scope, Variable* variable, int way) {
	TypeDefinition* def = Array_push(TypeDefinition, &scope->types);
	Prototype* proto = variable->proto;
	Prototype_reachMetaSizes(proto, &scope->scope, true);

	Type* type = Prototype_generateType(proto, &scope->scope, way);
	def->variable = variable;
	def->type = type;

	// call constructor on the type
	typedef Function* fn_t;
	Class* cl = Prototype_getClass(proto);

	Array_loop(fn_t, cl->constructors, cptr) {
		// Call constructor
		Function* fn = *cptr;
		if ((fn->flags & FUNCTIONFLAGS_ARGUMENT_CONSTRUCTOR))
			continue;

		Expression arg = {
			.type = EXPRESSION_TYPE,
			.data = {.type = type}
		};

		Expression* argsList[] = {&arg};

		Expression fncallExpr = { .type = EXPRESSION_FNCALL, .data = {
			.fncall = {
				.args = argsList,
				.fn = fn
			}
		}};

		
		Interpret_call(&fncallExpr, &scope->scope);
		break;
	}
}

void ScopeFunction_create(ScopeFunction* scope) {
	Array_create(&scope->types, sizeof(TypeDefinition));

	int useConstructor = scope->fn->flags & FUNCTIONFLAGS_ARGUMENT_CONSTRUCTOR;

	Function* fn = scope->fn;
	if (!fn)
		return;

	/// TODO: rework this part

	int argLength = fn->args_len;

	// Fill variables
	Variable** arguments = fn->arguments;
	for (int i = 0; i < argLength; i++) {
		Variable* v = arguments[i];

		if (i == 0 && useConstructor) {
			fillArgument(scope, v, TYPE_CWAY_DEFAULT);
			continue;
		}

		fillArgument(scope, v, TYPE_CWAY_ARGUMENT);
	}

	// Call direct requires

	/*
	Function_makeRequiresReal(fn, NULL);
	printf("requires %s %d\n", fn->name, fn->requires_len);
	Array_for(realRequireCouple_t, fn->realRequires, fn->requires_len, r) {
		Function* rqfn = r->fn;
		if ((rqfn->flags & FUNCTIONFLAGS_CONDITION) == 0) {
			raiseError("[TODO] Missing condition");
		}

		label_t passLabel = rqfn->testOrCond.pass;
		if (passLabel == NULL)
			continue; // no pass to call

		// Search pass
		int position = r->position;
		Variable* arg;
		Class* metaClass;

		arg = arguments[r->position];
		metaClass = Prototype_getMetaClass(arg->proto);

		gotArg:

		ScopeClass sc = {
			.scope = {NULL, SCOPE_CLASS},
			.allowThis = false,
			.cl = metaClass
		};
		
		ScopeSearchArgs search = {};
		Function* cndfn = Scope_search(&sc.scope, passLabel, &search, SCOPESEARCH_FUNCTION);
		if (!cndfn) {
			raiseError("[Unfound] Cannot find pass function");
			return;
		}

		if (cndfn->definitionState != DEFINITIONSTATE_DONE) {
			raiseError("[Undefined] Interpret status is invalid for this function");
		}


		// Call fn
		/// TODO: write cleaner code
		mblock_t mblock;
		Type* type = Scope_searchType(&scope->scope, thisVar);
		type = *(Type**)type->data;
		mblock = type->data;

		Expression argExpr = {
			.type = EXPRESSION_MBLOCK,
			.data = {
				.mblock = mblock
			}
		};
		Expression* argPointer = &argExpr;


		/// TODO: check argsStartIndex
		Interpret_interpret(
			cndfn->interpreter,
			&scope->scope,
			&argPointer,
			1,
			0,
			true,
			false
		);
	}

	*/
}

void ScopeFunction_delete(ScopeFunction* scope) {
	int arglen = scope->fn->args_len;
	int typelen = scope->types.length;
	TypeDefinition* types = scope->types.data;
	for (int i = 0; i < typelen; i++) {
		TypeDefinition* td = &types[i];
		Type_free(td->type);
		
		if (i >= arglen) {
			Variable* v = td->variable;
			Variable_destroy(v);
			free(v);
		}
	}

	
	Array_free(scope->types);
	
}



Variable* ScopeFunction_searchVariable(ScopeFunction* scope, label_t name, ScopeSearchArgs* args) {
	Array_loop(TypeDefinition, scope->types, td) {
		if (td->variable->name == name)
			return td->variable;
	}

	return NULL;
}

Class* ScopeFunction_searchClass(ScopeFunction* scope, label_t name, ScopeSearchArgs* args) {
	return NULL;
}

Function* ScopeFunction_searchFunction(ScopeFunction* scope, label_t name, ScopeSearchArgs* args) {
	return NULL;
}


void ScopeFunction_addVariable(ScopeFunction* scope, Variable* v) {
	raiseError("[TODO]: addVariable");
	/// TODO: let this code ?
	// ScopeFunction_pushVariable(scope, v, v->proto, NULL);
}

void ScopeFunction_addClass(ScopeFunction* scope, Class* cl) {
	raiseError("[TODO]");

}

void ScopeFunction_addFunction(ScopeFunction* scope, Function* fn) {
	raiseError("[TODO]");
}


void ScopeFunction_pushVariable(ScopeFunction* scope, Variable* v, Prototype* proto, Type* type) {
	if (Prototype_mode(*proto) == PROTO_MODE_REFERENCE) {
		raiseError("[Syntax] Cannot create a variable whose type is a reference of an other type");
	}

	TypeDefinition* td = Array_push(TypeDefinition, &scope->types);
	td->type = type;
	td->variable = v;
}


Type* ScopeFunction_quickSearchMetaBlock(ScopeFunction* scope, Variable* variable) {
	Array_loop(TypeDefinition, scope->types, m)
		if (m->variable == variable)
			return m->type;

	return NULL;
}



Type* ScopeFunction_searchType(ScopeFunction* scope, Variable* variable) {
	Array_loop(TypeDefinition, scope->types, t)
		if (t->variable == variable)
			return t->type;
	
	return NULL;
}



