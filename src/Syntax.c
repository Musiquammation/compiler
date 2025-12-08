#include "Syntax.h"

#include "ScopeBuffer.h"
#include "Scope.h"
#include "Parser.h"
#include "globalLabelPool.h"
#include "helper.h"

#include "MemoryCursor.h"

#include "syntaxList.c" // !do not include twice!

#include "Expression.h"
#include "definitionState_t.h"
#include "Module.h"
#include "Class.h"
#include "Variable.h"
#include "Function.h"
#include "Trace.h"

#include "primitives.h"
#include "langstd.h"

#include "Type.h"
#include "Prototype.h"

#include <string.h>




#define TokenCompare(list, flags) (Token_compare(token, list, sizeof(list)/sizeof(Token), flags))

static char* generateExtension(const char* filename, const char* extension) {
	if (!filename || !extension) return NULL;

	size_t len = strlen(filename) + strlen(extension) + 1; // +1 for '\0'
	char* result = (char*)malloc(len);
	if (!result) return NULL;

	strcpy(result, filename);
	strcat(result, extension);

	return result;
}




void Syntax_thFile(ScopeFile* scope) {
	char* const filepath = generateExtension(scope->filepath, ".th");
	Parser parser;
	Parser_open(&parser, filepath);
	free(filepath);

	scope->state_th = DEFINITIONSTATE_READING;

	while (true) {
		Token token = Parser_readAnnotated(&parser, &_labelPool);
		
		switch (TokenCompare(SYNTAXLIST_FILE, SYNTAXFLAG_EOF)) {
		// function
		case 0:
		{
			/// TODO: definition required ?
			const Syntax_FunctionDeclarationArg defaultData = {
				.defaultName = scope->name,
				.definedClass = NULL
			};

			Syntax_functionDeclaration(&scope->scope, &parser, 0, &defaultData);

			break;
		}

		// class
		case 1:
		{
			const Syntax_ClassDeclarationArg defaultData = {
				.defaultName = scope->name
			};
			Syntax_classDeclaration(&scope->scope, &parser, 0, &defaultData);
			break;
		}

		// order
		case 2:
		{
			raiseError("[TODO] File order\n");
			break;
		}

		default:
			goto close;
		}
	}

	close:
	scope->state_th = DEFINITIONSTATE_DONE;
	Parser_close(&parser);
}


void Syntax_tcFile(ScopeFile* scope) {
	char* const filepath = generateExtension(scope->filepath, ".tc");
	Parser parser;
	Parser_open(&parser, filepath);
	free(filepath);
	
	scope->state_tc = DEFINITIONSTATE_READING;

	Class* definedClass = Module_searchClass(Scope_reachModule(&scope->scope), scope->name, NULL);

	
	while (true) {
		Token token = Parser_readAnnotated(&parser, &_labelPool);
		Class* subDefinedClass = definedClass;

		Array_loop(Annotation, parser.annotations, annotation) {
			int annotType = annotation->type;

			switch (annotType) {
			case ANNOTATION_CONTROL:
			{
				subDefinedClass = definedClass->meta;
				
				if (!subDefinedClass) {
					raiseError("[Architecture] Cannot control a class without meta");
				}
				break;
			}

			default:
			{
				raiseError("[Syntax] Illegal annotation");
				return;
			}
			}
		}

		parser.annotations.length = 0;
		
		switch (TokenCompare(SYNTAXLIST_FILE, SYNTAXFLAG_EOF)) {
		// function
		case 0:
		{
			/// TODO: definition required ?
			const Syntax_FunctionDeclarationArg defaultData = {
				.defaultName = scope->name,
				.definedClass = subDefinedClass
			};

			Syntax_functionDeclaration(&scope->scope, &parser, 0, &defaultData);

			break;
		}

		// class
		case 1:
		{
			const Syntax_ClassDeclarationArg defaultData = {
				.defaultName = scope->name
			};
			Syntax_classDeclaration(&scope->scope, &parser, 0, &defaultData);
			break;
		}

		// order
		case 2:
		{
			raiseError("[TODO] File order\n");
			break;
		}

		default:
			goto close;
		}
	}


	close:
	scope->state_tc = DEFINITIONSTATE_DONE;
	Parser_close(&parser);

}





Expression* Syntax_expression(Parser* parser, Scope* scope, char isExpressionRoot) {
	Token token = Parser_read(parser, &_labelPool);
	if (
		isExpressionRoot &&
		TokenCompare(SYNTAXLIST_SINGLETON_RPAREN, SYNTAXFLAG_UNFOUND) == 0
	) {
		if (isExpressionRoot == 2) {
			Parser_saveToken(parser, &token);
		}
		return NULL;
	}	
	
	
	Array lineArr; // type: Expression
	Array_create(&lineArr, sizeof(Expression));

	// Collect expressions
	while (true) {

		const int syntaxIndex = TokenCompare(SYNTAXLIST_EXPRESSION, 0);
		switch (syntaxIndex) {

		// variable
		case 0:
		{
			Expression* path = Syntax_readPath(token.label, parser, scope);
			Expression* expr = Array_push(Expression, &lineArr);
			expr->type = EXPRESSION_PATH;
			expr->data.target = path;
			break;
		}

		// number
		case 1:
		{
			Expression* e = Array_push(Expression, &lineArr);
			switch (token.type) {
			case TOKEN_TYPE_I8:
				e->type = EXPRESSION_I8;
				e->data.num.i8 = token.num.i8;
				break;

			case TOKEN_TYPE_U8:
				e->type = EXPRESSION_U8;
				e->data.num.u8 = token.num.u8;
				break;

			case TOKEN_TYPE_I16:
				e->type = EXPRESSION_I16;
				e->data.num.i16 = token.num.i16;
				break;

			case TOKEN_TYPE_U16:
				e->type = EXPRESSION_U16;
				e->data.num.u16 = token.num.u16;
				break;

			case TOKEN_TYPE_I32:
				e->type = EXPRESSION_I32;
				e->data.num.i32 = token.num.i32;
				break;

			case TOKEN_TYPE_U32:
				e->type = EXPRESSION_U32;
				e->data.num.u32 = token.num.u32;
				break;

			case TOKEN_TYPE_F32:
				e->type = EXPRESSION_F32;
				e->data.num.f32 = token.num.f32;
				break;

			case TOKEN_TYPE_F64:
				e->type = EXPRESSION_F64;
				e->data.num.f64 = token.num.f64;
				break;

			case TOKEN_TYPE_I64:
				e->type = EXPRESSION_I64;
				e->data.num.i64 = token.num.i64;
				break;

			case TOKEN_TYPE_U64:
				e->type = EXPRESSION_U64;
				e->data.num.u64 = token.num.u64;
				break;

			default:
				raiseError("[INTERN] Token should be a number");
				break;
			}

			break;
		}

		// get
		case 2:
		{
			Expression* e = Array_push(Expression, &lineArr);
			raiseError("Syntax_expression : get");
			break;
		}

		// open parenthesis
		case 3:
		{
			Expression* target = Syntax_expression(parser, scope, 0);
			Expression* e = Array_push(Expression, &lineArr);

			int targetType = target->type;
			if (targetType >= EXPRESSION_U8 && targetType <= EXPRESSION_F64) {
				*e = *target;
				free(target);

			} else {
				e->type = EXPRESSION_GROUP;
				e->data.target = target;
			}

			break;
		}

		// close parenthesis
		case 4:
		{
			if (isExpressionRoot) {
				Parser_saveToken(parser, &token);
			}

			goto processExpressions;
			break;
		}

		// comma, end
		case 5:
		case 6:
		{
			if (!isExpressionRoot) {
				raiseError("[Syntax] Expression is not finished (missing closing parenthesis)");
			}
			
			Parser_saveToken(parser, &token);
			goto processExpressions;
		}

		// two operand operators
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:

		case 12:
		case 13:
		case 14:
		case 15:
		case 16:

		case 17:
		case 18:

		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		{
			Expression* e = Array_push(Expression, &lineArr);
			e->type = syntaxIndex + (EXPRESSION_ADDITION-7);
			break;
		}

		// bit not
		case 25:
		{
			Expression* e = Array_push(Expression, &lineArr);
			e->type = EXPRESSION_BITWISE_NOT;
			break;
		}

		// logical not
		case 26:
		{
			Expression* e = Array_push(Expression, &lineArr);
			e->type = EXPRESSION_LOGICAL_NOT;
			break;
		}

		// increment
		case 27:
		{
			Expression* e = Array_push(Expression, &lineArr);
			e->type = EXPRESSION_A_INCREMENT;
			break;
		}

		// decrement
		case 28:
		{
			Expression* e = Array_push(Expression, &lineArr);
			e->type = EXPRESSION_A_INCREMENT;
			break;
		}


		default:
			raiseError("[TODO] This expression is not handled");
			break;
		}
	
		token = Parser_read(parser, &_labelPool);
	}

	processExpressions:
	if (lineArr.length == 0) {
		return NULL; // no expression
	}



	Expression* result = Expression_processLine(lineArr.data, lineArr.length);
	free(lineArr.data);
	return result;
}





void Syntax_module(Module* module, const char* filepath) {
	Parser parser;
	Parser_open(&parser, filepath);

	
	bool moduleDefined = false;
	
	while (true) {
		Token token = Parser_read(&parser, &_labelPool);

		switch (TokenCompare(SYNTAXLIST_MODULE_FILE, SYNTAXFLAG_EOF)) {
		// order
		case 0:
		{
			if (moduleDefined) {
				raiseError("Module already defined");
				goto close;
			}

			moduleDefined = true;

			// Get type of order
			while (true) {
				token = Parser_read(&parser, &_labelPool);

				switch (TokenCompare(SYNTAXLIST_MODULE_MODULE, 0)) {
				
				// module
				case 0:
				{
					token = Parser_read(&parser, &_labelPool);
					if (TokenCompare(SYNTAXLIST_REQUIRE_BRACE, 0)) {
						goto close;
					}

					Syntax_declarationList(&module->scope, &parser);
				}

				default:
					goto close;
				}
			}

			break;
		}
	
		// EOF
		case -2:
		{
			if (!moduleDefined) {
				raiseError("Missing @!module declaration");
			}
			
			goto close;
		}
		}	
	}


	close:
	Parser_close(&parser);
}


void Syntax_declarationList(Scope* scope, Parser* parser) {
	while (true) {
		Token token;
		token = Parser_readAnnotated(parser, &_labelPool);
		switch (TokenCompare(SYNTAXLIST_DECLARATION_LIST, 0)) {
			// end
			case 0:
				return;

			// function
			case 1:
			{
				const Syntax_FunctionDeclarationArg defaultData = {
					.defaultName = LABEL_NULL,
					.definedClass = NULL
				};

				Syntax_functionDeclaration(
					scope,
					parser,
					SYNTAXDATAFLAG_FN_DCL_REQUIRES_DECLARATION,
					&defaultData
				);
				break;
			}

			// class
			case 2:
			{
				const Syntax_ClassDeclarationArg defaultData = {
					.defaultName = LABEL_NULL,
				};
				Syntax_classDeclaration(
					scope,
					parser,
					SYNTAXDATAFLAG_CL_DCL_REQUIRES_DECLARATION,
					&defaultData
				);
				break;
			}

			default:
				return;
		}
	}
}






void Syntax_classDeclaration(Scope* scope, Parser* parser, int flags, const Syntax_ClassDeclarationArg* defaultData) {
	label_t name = LABEL_NULL;
	int syntaxIndex = -1;
	bool definitionToRead;
	bool langstd = false;
	
	// Handle annotations
	/// TODO: complete annotations 
	Array_loop(Annotation, parser->annotations, annotation) {
		int annotType = annotation->type;

		switch (annotType) {
		case ANNOTATION_LANGSTD:
		{
			langstd = true;
			break;
		}
		
		default:
		{
			raiseError("[Syntax] Illegal annotation");
			return;
		}
		}
	}

	parser->annotations.length = 0; // clear annotations


	// Read content
	while (true) {
		Token token;
		// Collect next syntaxIndex
		{
			token = Parser_read(parser, &_labelPool);
			int next = TokenCompare(SYNTAXLIST_CLASS_DECLARATION, 0);
			if (next <= syntaxIndex) {
				raiseError("[Syntax] Illegal order in class declaration");
				return;
			}

			syntaxIndex = next;
		}


		switch (syntaxIndex) {
		// Settings
		case 0:
			raiseError("[TODO] read settings");
			break;
		
		// Name
		case 1:
			name = token.label;
			break;
		
		// Definition
		case 2:
			if (flags & SYNTAXDATAFLAG_CL_DCL_REQUIRES_DECLARATION) {
				raiseError("[Syntax] A class DECLARATION was expected");
				break;
			}

			definitionToRead = true;
			goto finishWhile;
		
		// End
		case 3:
			if (flags & SYNTAXDATAFLAG_CL_DCL_REQUIRES_DEFINITION) {
				raiseError("[Syntax] A class DEFINITION was expected");
				return;
			}

			definitionToRead = false;
			goto finishWhile;

		default:
			raiseError("[Syntax] Invalid class syntax");
			return;
			
		}


	}


	finishWhile:

	// Check name
	if (name == LABEL_NULL) {
		if (flags & SYNTAXDATAFLAG_FNDCL_REQUIRES_NAME) {
			raiseError("[Syntax] Class name required");
		} else {
			name = defaultData->defaultName;
		}
	}

	// Search class in scope
	Class* cl = Scope_search(scope, name, NULL, SCOPESEARCH_CLASS);
	if (cl) {
		switch (cl->definitionState) {
		case DEFINITIONSTATE_READING:
			raiseError("[Architecture] This class is already being defined");
			return;

		case DEFINITIONSTATE_DONE:
			raiseError("[Architecture] This class has already been defined");
			return;
		}


	} else {
		cl = malloc(sizeof(Class));
		Class_create(cl);
		cl->name = name;
		cl->definitionState = definitionToRead ? DEFINITIONSTATE_READING : DEFINITIONSTATE_UNDEFINED;
		cl->primitiveSizeCode = 0;
		Scope_addClass(scope, scope->type, cl);
		
		if (langstd) {
			#define check(label, Label) if (name == _commonLabels._##Label) {\
				if (_langstd.label) {raiseError("[Architecture] langstd" #Label "already defined"); return;}\
				_langstd.label = cl; goto langstdDone;\
			}
			
			check(pointer, Pointer);
			check(type, Type);

			#undef check


			raiseError("[Unknown] Cannot find langstd object to define");
			langstdDone:
		}
	}



	// Read definition
	if (definitionToRead) {
		Syntax_ClassDefinitionArg arg = {.controlFunctions.size = 0};
		printf("DEFINE %s\n", cl->name);
		Syntax_classDefinition(scope, parser, cl, &arg);
		
		// Define metas
		Class_appendMetas(cl);
		Class_acheiveDefinition(cl);
	}
}






void Syntax_classDefinition(Scope* parentScope, Parser* parser, Class* cl, Syntax_ClassDefinitionArg* cdefs) {

	cl->definitionState = DEFINITIONSTATE_READING;

	ScopeClass insideScope = {
		.scope = {.parent = NULL, .type = SCOPE_CLASS},
		.cl = cl,
		.allowThis = true
	};

	ScopeClass outsideScope = {
		.scope = {.parent = parentScope, .type = SCOPE_CLASS},
		.cl = cl,
		.allowThis = false
	};

	ScopeClass variadicScope = {
		.scope = {.parent = parentScope, .type = SCOPE_CLASS},
		.cl = NULL,
		.allowThis = false
	};
	
	
	Class* meta = NULL;
	MemoryCursor cursor = {0, 0};
	Array metaControlFunctions;
	metaControlFunctions.reserved = 0; // to prevent Array_free

	// Read content
	while (true) {

		Token token = Parser_readAnnotated(parser, &_labelPool);

		Array_loop(Annotation, parser->annotations, annotation) {
			int annotType = annotation->type;

			switch (annotType) {
			case ANNOTATION_CONTROL:
			{
				if (TokenCompare(SYNTAXLIST_FREE_LABEL, 0) != 0)
					return; 
				
				label_t name = token.label;

				token = Parser_read(parser, &_labelPool);
				if (TokenCompare(SYNTAXLIST_SINGLETON_END, 0) != 0)
					return; 

				if (cdefs->controlFunctions.size == 0) {
					raiseError("[Architecture] Adapt functions are forbidden");
					return; 
				}

				*Array_push(label_t, &cdefs->controlFunctions) = name;

				goto nextSequence;
			}

			default:
			{
				raiseError("[Syntax] Illegal annotation");
				return; 
			}
			}
		}

		parser->annotations.length = 0; // clear annotations

		
		switch (TokenCompare(SYNTAXLIST_CLASS_DEFINITION, 0)) {
		// function
		case 0:
		{
			raiseError("[TODO] method of a class definition");
			break;
		}

		// class
		case 1:
		{
			raiseError("[TODO] subclass of a class definition");
			break;
		}

		// meta class
		case 2:
		{
			meta = malloc(sizeof(Class));

			// Temp name
			Class_create(meta);
			meta->name = Class_generateMetaName(cl->name, '^');

			token = Parser_read(parser, &_labelPool);
			if (TokenCompare(SYNTAXLIST_SINGLETON_LBRACE, 0) != 0)
				return; 

			Syntax_ClassDefinitionArg cdefs;
			Array_create(&cdefs.controlFunctions, sizeof(label_t));
			Syntax_classDefinition(&outsideScope.scope, parser, meta, &cdefs);
			metaControlFunctions = cdefs.controlFunctions;

			variadicScope.cl = meta;
						
			break;
		}

		// label
		case 3:
		{
			const label_t name = token.label;
			token = Parser_read(parser, &_labelPool);
			switch (TokenCompare(SYNTAXLIST_CLASS_ABSTRACT_MEMBER, 0)) {
			// variable
			case 0:
			{
				Variable* variable = malloc(sizeof(Variable));
				*Array_push(Variable*, &cl->variables) = variable;
				
				variable->name = name;

				variable->proto = Syntax_proto(parser, &variadicScope.scope);

				// Finish by end operation
				token = Parser_read(parser, &_labelPool);
				if (TokenCompare(SYNTAXLIST_SINGLETON_END, 0) != 0)
					return; 
	

				// Get object size
				ExtendedPrototypeSize vsize = Prototype_reachSizes(variable->proto, parentScope, false);
				if (vsize.size >= 0) {
					variable->offset = MemoryCursor_give(
						&cursor,
						vsize.size,
						vsize.maxMinimalSize
					);
				} else {
					raiseError("[TODO] handle unknown sizes in class declaration for MemoryCursor");
				}



				break;
			}
			
			// function
			case 1:
			{
				Syntax_FunctionDeclarationArg declData = {
					.defaultName = name,
					.definedClass = NULL
				};

				Parser_saveToken(parser, &token);

				Syntax_functionDeclaration(
					&outsideScope.scope,
					parser,
					SYNTAXDATAFLAG_FNDCL_FORBID_NAME,
					&declData
				);

				break;
			}
			}

			break;
		}

		// order
		case 4:
		{
			raiseError("[TODO] orders in class definition\n");
			break;
		}

		// end
		case 5:
			goto close;

		default:
			goto close;
		}

		nextSequence:
	}

	

	close:

	// Align offset
	cl->size = MemoryCursor_align(cursor);
	cl->maxMinimalSize = cursor.maxMinimalSize;
	cl->primitiveSizeCode = Class_getPrimitiveSizeCode(cl);


	

	// Get meta
	if (meta) {
		cl->meta = meta;
		cl->metaDefinitionState = DEFINITIONSTATE_READING;
		
		/// TODO: handle metaControlFunctions
		Array_free(metaControlFunctions);
	}
}








void Syntax_proto_readSettings(Parser* parser, Scope* scope, Class* meta, Array* settings) {
	label_t label = NULL;
	enum {NODEFINE = -1, NEEDS_COMMA = -2};

	int offsetToDefine = NODEFINE;
	Variable* variableToDefine;
	bool typeToDefine = false;
	Array_create(settings, sizeof(ProtoSetting));
	
	while (true) {
		Token token = Parser_read(parser, &_labelPool);
		int syntaxResult = TokenCompare(SYNTAXLIST_SETTING, 0);

		printf("at %d: ", offsetToDefine);
		Token_println(&token);
		switch (syntaxResult) {
		// label
		case 0:
		case 1:
		{
			if (syntaxResult == 0 && !typeToDefine) {
				raiseError("[Syntax] No type to define");
				return;
			}

			if (typeToDefine) {
				Parser_saveToken(parser, &token);
				Prototype* p = Syntax_proto(parser, scope);
				ProtoSetting* ps = Array_push(ProtoSetting, settings);
				ps->useProto = true;
				ps->variable = variableToDefine;
				ps->proto = p;
				
				typeToDefine = false;
				offsetToDefine = NEEDS_COMMA;
				break;
			}


			if (offsetToDefine == NEEDS_COMMA) {goto raiseComma;}


			if (label) {
				raiseError("[Syntax] Previous label not defined in setting list");
				return;
			}

			label = token.label;
			break;
		}


		// assign value
		case 2:
		{
			if (offsetToDefine == NEEDS_COMMA) {goto raiseComma;}

			raiseError("[TODO] assign value in settings");
			break;
		}

		// assign type
		case 3:
		{
			if (offsetToDefine == NEEDS_COMMA) {goto raiseComma;}

			// Search type to define
			typedef Variable* v_ptr;
			Array_loop(v_ptr, meta->variables, vptr) {
				Variable* v = *vptr;
				if (v->name != label)
					continue;

				if (!Prototype_isType(v->proto)) {
					raiseError("[Type] Variadic type was expected");
					return;
				}

				offsetToDefine = v->offset;
				typeToDefine = true;
				label = NULL;
				variableToDefine = v;
				goto varFound_type;
			}

			raiseError("[Unknown] Cannot find type to set\n");

			varFound_type:

			break;
		}

		// finish
		case 4:
		case 5:
		case 6:
		{
			if (label) {
				// Label is a type that we have to define
				raiseError("[TODO] define in order in settings\n");
			}

			// Closing angle
			if (syntaxResult == 5) {
				return;
			}

			// Save end character
			if (syntaxResult == 6) {
				Parser_saveToken(parser, &token);
				return;
			}
			break;
		}

		}
	}

	return;
	
	
	raiseComma:
	raiseError("[Syntax] A comma was expected");
}

Prototype* Syntax_proto(Parser* parser, Scope* scope) {
	Token token = Parser_read(parser, &_labelPool);

	// Start with class keyword
	if (TokenCompare(SYNTAXLIST_START_TYPE, 0) == 0) {
		// Opening bracket or something else
		token = Parser_read(parser, &_labelPool);
		if (TokenCompare(SYNTAXLIST_SINGLETON_LBRACKET, SYNTAXFLAG_UNFOUND) < 0) {
			// Variadic prototype
			raiseError("[TODO] check type existence");
			Prototype* proto = Prototype_create_direct(
				_langstd.type,
				_langstd.type->primitiveSizeCode,
				NULL, 0
			);
			Parser_saveToken(parser, &token);
			return proto;
		}
			

		// Here, it's type of a variable

		// Variable
		token = Parser_read(parser, &_labelPool);
		if (TokenCompare(SYNTAXLIST_FREE_LABEL, 0))
			return false;
		
		// Expression
		Expression* expression = Syntax_readPath(token.label, parser, scope);
		if (expression->type != EXPRESSION_PROPERTY || expression->data.property.next) {
			raiseError("[Syntax] class[var] syntax works only with a variable");
		}

		Prototype* proto = Prototype_create_reference(
			expression->data.property.variableArr, expression->data.property.length);

		free(expression);
		

		// Closing bracket
		token = Parser_read(parser, &_labelPool);
		if (TokenCompare(SYNTAXLIST_SINGLETON_RBRACKET, 0))
			return false;


		return proto;
	}

	
	
	
	// Search variadic
	Variable* vdic =  Scope_search(scope, token.label, NULL, SCOPESEARCH_VARIABLE);
	if (vdic) {
		if (!Prototype_isType(vdic->proto)) {
			raiseError("[Type] Variadic type must be typed as Type");
			return NULL;
		}

		return Prototype_create_variadic(vdic);
	}
	
	// Default behavior
	Class* cl = Scope_search(scope, token.label, NULL, SCOPESEARCH_CLASS);
	if (cl) {
		Prototype* proto = primitives_getPrototype(cl);
		if (proto)
			return proto;
	} else {		
		classNotFound:
		raiseError("[Unknown] Class not found");
		return false;
	}

	int syntaxIndex = -1;
	bool canBePrimitive = true;
	Array settings = {.length = 0, .reserved = 0};

	// Collect settings data
	while (true) {
		// Collect next syntaxIndex
		{
			token = Parser_read(parser, &_labelPool);
			int next = TokenCompare(SYNTAXLIST_TYPE, SYNTAXFLAG_UNFOUND);
			if (next < 0) {
				Parser_saveToken(parser, &token);
				goto generate;
			}

			if (next <= syntaxIndex) {
				raiseError("[Syntax] Illegal order in type declaration");
				return false;
			}
			
			syntaxIndex = next;
		}


		switch (syntaxIndex) {
		// Settings
		case 0:
			canBePrimitive = false;
			if (!cl->meta) {
				raiseError("[Architecture] Cannot append settings to a class within meta-class");
			}

			Syntax_proto_readSettings(parser, scope, cl->meta, &settings);
			break;
			
		// Read verified functions
		case 1:
			canBePrimitive = false;
			raiseError("[TODO] verified functions");
			break;
		
		}
	}

	// Generate prototype
	generate:

	/// TODO: Array_shrinkToFit
	// Array_shrinkToFit()
	Prototype* ret = Prototype_create_direct(cl, canBePrimitive ? cl->primitiveSizeCode : 0,
		settings.data, settings.length);


	return ret;
}






void Syntax_functionDeclaration(Scope* scope, Parser* parser, int flags, const Syntax_FunctionDeclarationArg* defaultData) {
	label_t name = LABEL_NULL;
	int syntaxIndex = -1;
	bool definitionToRead;
	bool returnTypeAlreadyRead = false;
	Array arguments = {.length = 0xffff}; // no args read ; type: Variable*
	Prototype* returnType = NULL;
	
	// Handle annotations
	/// TODO: complete annotations 
	Array_loop(Annotation, parser->annotations, annotation) {
		switch (annotation->type) {
		case ANNOTATION_MAIN:
		{
			// Set name
			name = _commonLabels._main;

			// Return type
			returnType = &_primitives.proto_i32;
			returnTypeAlreadyRead = true;

			// Arguments
			Variable** argList = malloc(1 * sizeof(Variable*));
			Variable* argc = malloc(sizeof(Variable));
			Variable_create(argc);
			argc->id  -1;
			argc->proto = &_primitives.proto_i32;
			
			/// TODO: edit this prototype (add char** argv)
			argList[0] = argc;
			arguments.data = argList;
			arguments.size = sizeof(Variable*);
			arguments.reserved = 1;
			arguments.length = 1;

			break;
		}		
		default:
		{
			raiseError("[Syntax] Illegal annotation\n");
			return;
		}	
		}
	}

	parser->annotations.length = 0; // clear annotations


	// Read content
	while (true) {
		Token token;
		// Collect next syntaxIndex
		{
			token = Parser_read(parser, &_labelPool);
			int next = TokenCompare(SYNTAXLIST_FUNCTION_DECLARATION, 0);
			if (next <= syntaxIndex) {
				raiseError("[Syntax] Illegal order in function declaration");
				return;
			}

			syntaxIndex = next;
		}


		switch (syntaxIndex) {
		// Settings
		case 0:
			raiseError("[TODO] read settings");
			break;
		
		// Name
		case 1:
			if (flags & SYNTAXDATAFLAG_FNDCL_FORBID_NAME) {
				raiseError("[Syntax] Name already given");
				break;
			}
			name = token.label;
			break;
		
		// Arguments
		case 2:
			if (arguments.length != 0xffff) {
				raiseError("[Architecture] argument list already defined");
				break;
			}
			arguments = Syntax_functionArgumentsDecl(scope, parser);
			break;

		// Type
		case 3:
			if (returnTypeAlreadyRead) {
				raiseError("[Architecture] return type defined");
				break;
			}
			returnTypeAlreadyRead = true;
			returnType = Syntax_proto(parser, scope);
			break;
		
		// Definition
		case 4:
			if (flags & SYNTAXDATAFLAG_FN_DCL_REQUIRES_DECLARATION) {
				raiseError("[Syntax] A function DECLARATION was expected");
				break;
			}

			definitionToRead = true;
			goto finishWhile;
			
		// End
		case 5:
			if (flags & SYNTAXDATAFLAG_FN_DCL_REQUIRES_DEFINITION) {
				raiseError("[Syntax] A function DEFINITION was expected");
				return;
			}

			definitionToRead = false;
			goto finishWhile;

		default:
			raiseError("[Syntax] Invalid function syntax");
			return;

		}

	}
	

	finishWhile:
	
	// Check name
	if (name == LABEL_NULL) {
		if (flags & SYNTAXDATAFLAG_FNDCL_REQUIRES_NAME) {
			raiseError("[Syntax] Function name required");
		} else {
			name = defaultData->defaultName;
		}
	}

	// Search function in scope
	/// TODO: this search
	Function* fn;
	Class* definedClass = defaultData->definedClass;
	if (defaultData->definedClass) {
		ScopeClass sc = {
			.scope = {.type=SCOPE_CLASS, .parent=NULL},
			.allowThis = false,
			.cl = definedClass,
		};

		fn = Scope_search(&sc.scope, name, NULL, SCOPESEARCH_FUNCTION);

		if (!fn) {
			fn = Scope_search(scope, name, NULL, SCOPESEARCH_FUNCTION);
		}

	} else {
		fn = Scope_search(scope, name, NULL, SCOPESEARCH_FUNCTION);
	}

	if (fn) {
		switch (fn->definitionState) {
		case DEFINITIONSTATE_READING:
			raiseError("[Architecture] This function is already being defined");
			return;

		case DEFINITIONSTATE_DONE:
			raiseError("[Architecture] This function has already been defined");
			return;
		}

		// Check if extra data have not been written
		if (arguments.length != 0xffff) {
			raiseError("[Architecture] A definition of arguments already exists for this function");
			return;
		}

		if (returnType) {
			raiseError("[Architecture] A definition of return type already exists for this function");
			return;
		}

		/// TODO: check for settings 

	} else {
		// Check if extra data have been given
		if (arguments.length == 0xffff) {
			raiseError("[Architecture] Arguments are missing for this function");
			return;
		}

		

		/// TODO: check for settings



		// Generate function
		fn = malloc(sizeof(Function));
		Function_create(fn);
		fn->name = name;
		fn->definitionState = definitionToRead ? DEFINITIONSTATE_READING : DEFINITIONSTATE_UNDEFINED;
		fn->arguments = arguments;
		fn->returnType = returnType;

		Scope_addFunction(scope, scope->type, fn);
	}


	if (definitionToRead) {
		Syntax_functionDefinition(scope, parser, fn, definedClass);
	}

	/// TODO: define module.mainFunction

}


Array Syntax_functionArgumentsDecl(Scope* scope, Parser* parser) {
	Array arguments;
	Array_create(&arguments, sizeof(Variable*));

	Token token = Parser_read(parser, &_labelPool);
	bool cannotFinish = false;
	while (true) {
		switch (TokenCompare(SYNTAXLIST_ARGS, 0)) {
		// variable
		case 0:
		{
			cannotFinish = true;

			const label_t name = token.label;

			// Get COLON operator for type
			token = Parser_read(parser, &_labelPool);
			if (TokenCompare(SYNTAXLIST_SINGLETON_COLON, 0))
				goto finish;

			Variable* variable = malloc(sizeof(Variable));
			int position = arguments.length;
			*Array_push(Variable*, &arguments) = variable;

			variable->name = name;
			variable->id = position;

			variable->proto = Syntax_proto(parser, scope);

			token = Parser_read(parser, &_labelPool);
			switch (TokenCompare(SYNTAXLIST_ARGS_ENDING, 0)) {
			case 0:
				token = Parser_read(parser, &_labelPool);
				break;

			case 1: // end
				goto finish;
			}
			
			break;
		}

		// end
		case 1:
			if (cannotFinish) {
				raiseError("[Syntax] Closing in function declaration already marked");
			}
			goto finish;
		}
	}

	finish:
	return arguments;
}

void Syntax_functionArgumentsCall(Scope* scopePtr, Parser* parser, Function* fn, Expression* expr) {
	ScopeSearchArgs searchArg;

	union {
		ScopeClass cl;
	} subScope;



	/// TODO: create a subscope to handle words (of enums)
	Array args; // type: Expression*
	Array_create(&args, sizeof(Expression*));
	while (true) {
		Expression* expr = Syntax_expression(parser, scopePtr, 2);
		if (expr) {
			*Array_push(Expression*, &args) = expr;
		}

		Token token = Parser_read(parser, &_labelPool);
		if (TokenCompare(SYNTAXLIST_EXPRESSION_SEPARATOR, 0) == 1) {
			break;
		}
	}

	Expression_FnCall* call = malloc(sizeof(Expression_FnCall));
	call->fn = fn;
	if (args.length) {
		size_t size = sizeof(Expression*) * args.length;
		Expression** ae = malloc(size);
		memcpy(ae, args.data, size);
		call->argsLength = args.length;
		call->args = ae;
		free(args.data);
		
	} else {
		call->argsLength = 0;
	}

	expr->type = EXPRESSION_FNCALL;
	expr->data.fncall.object = call;
}




Expression* Syntax_readPath(label_t label, Parser* parser, Scope* scope) {
	Array expressions; // type: Expression
	Array_create(&expressions, sizeof(Expression)); 

	ScopeBuffer subScope;
	

	ScopeFunction* baseScope = scope->type == SCOPE_FUNCTION ? (ScopeFunction*)scope : NULL;

	
	Scope* subScopePtr = scope;
	Token token;
	int syntaxIndex;
	
	mblock_t metaPtr = NULL;
	Prototype* metaProto = NULL;


	while (true) {
		ScopeSearchArgs searchArg;
		void* object = Scope_search(subScopePtr, label, &searchArg, SCOPESEARCH_ANY);


		if (!object) {
			raiseError("[Unknown] Object not defined");
			return NULL;
		}

		if (searchArg.resultType == SCOPESEARCH_VARIABLE) {
			bool hasNext;

			Array currentVarPath; // type: Variable*
			Array_createAllowed(&currentVarPath, sizeof(Variable*), 4);

			currentVarPath.length = 1;
			*(Variable**)currentVarPath.data = (Variable*)object;

			while (true) {
				token = Parser_read(parser, &_labelPool);
				syntaxIndex = TokenCompare(SYNTAXLIST_PATH, SYNTAXFLAG_UNFOUND);

				if (syntaxIndex < 0) {
					hasNext = false;
					goto generatePathExpr;
				}

				// Search sub scope
				subScopePtr = Prototype_reachSubScope(((Variable*)object)->proto, &subScope);

				switch (syntaxIndex) {
				case 0:
				{
					token = Parser_read(parser, &_labelPool);
					if (TokenCompare(SYNTAXLIST_FREE_LABEL, SYNTAXFLAG_UNFOUND) != 0)
						return NULL;

					label = token.label;
					object = Scope_search(subScopePtr, label, &searchArg, SCOPESEARCH_ANY);

					if (!object) {
						raiseError("[Unknown] Property not found");
						return NULL;
					}

					if (searchArg.resultType == SCOPESEARCH_VARIABLE) {
						*Array_push(Variable*, &currentVarPath) = (Variable*)object;
						break;
					}

					// Finish current path
					hasNext = true;
					goto generatePathExpr;
				}
					
				// fncall
				case 1: 
				{
					raiseError("[TODO] Handle fncall from a variable");
					hasNext = true;
					goto generatePathExpr;
				}

				// scope
				case 2:
				{
					Variable* first = *Array_get(Variable*, currentVarPath, 0);

					char hasMeta = Prototype_hasMeta(first->proto);
					if (hasMeta == 0) {
						raiseError("[Unknown] This variable does not have meta class");
						return NULL;
					}
					
					if (hasMeta == -1) {
						raiseError("[Unknown] This variable does not have meta class (it is unkown)");
						return NULL;
					}
					
					if (!baseScope) {
						raiseError("[Structure] Meta content is not accessible");
						return NULL;
					}

					Type* type = ScopeFunction_quickSearchMetaBlock(baseScope, first);
					if (!type) {
						raiseError("[Structure] Type of the variable cannot be found");
						return NULL;
					}
					
					/// TODO: handle multiple :: scope operators
					metaPtr = type->data;
					if (!metaPtr) {
						raiseError("[Structure] Type has not meta content");
						return NULL;
					}

					metaPtr += Prototype_getVariableOffset(
						currentVarPath.data,
						currentVarPath.length
					);

					metaProto = first->proto;

					// Read next
					token = Parser_read(parser, &_labelPool);
					if (TokenCompare(SYNTAXLIST_FREE_LABEL, SYNTAXFLAG_UNFOUND) != 0)
						return NULL;

					label = token.label;
					break;
				}
				}

			}

			generatePathExpr:
			if (metaPtr) {
				
				Variable* first = *Array_get(Variable*, currentVarPath, 0);
				// Class* meta = Prototype_getMetaClass(first->proto);
				
				/// TODO: test this line
				int offset = Prototype_getGlobalVariableOffset(
					first->proto,
					currentVarPath.data,
					currentVarPath.length
				);

				metaPtr += offset;
				
				Expression_Value* ev = malloc(sizeof(Expression_Value));
				ev->value = metaPtr;
				ev->proto = (*Array_get(Variable*, currentVarPath, currentVarPath.length-1))->proto;

				Array_free(currentVarPath);

				Expression* expr = Array_push(Expression, &expressions);
				expr->type = EXPRESSION_VALUE;
				expr->data.value.value = ev;
				expr->data.value.next = hasNext;


				break;

			} else {
				Expression* expr = Array_push(Expression, &expressions);
				expr->type = EXPRESSION_PROPERTY;
				// Array_shrinkToFit(&currentVarPath);
				expr->data.property.variableArr = currentVarPath.data;
				expr->data.property.length = currentVarPath.length;
				expr->data.property.next = hasNext;
				expr->data.property.freeVariableArr = true;
	
				if (!hasNext)
					break;
			}

			skipPathExpr:
			continue;
		}

		if (searchArg.resultType == SCOPESEARCH_FUNCTION) {
			token = Parser_read(parser, &_labelPool);

			if (TokenCompare(SYNTAXLIST_SINGLETON_LPAREN, SYNTAXFLAG_UNFOUND) != 0) {
				raiseError("[TODO] Handle function references");
				return NULL;
			}

			// Fn call: let's read args
			Expression* expr = Array_push(Expression, &expressions);
			Syntax_functionArgumentsCall(subScopePtr, parser, (Function*)object, expr);

			token = Parser_read(parser, &_labelPool);

			syntaxIndex = TokenCompare(SYNTAXLIST_PATH, SYNTAXFLAG_UNFOUND);
			switch (syntaxIndex) {
			// member
			case 0:
			{
				token = Parser_read(parser, &_labelPool);
				label = token.label;
				expr->data.fncall.next = true;
				break;
			}

			// fncall
			case 1:
			{
				expr->data.fncall.next = true;
				raiseError("[TODO] handle an other function call");
				return NULL;
				break;
			}
			
			// scope
			case 2:
			{
				expr->data.fncall.next = true;
				raiseError("[TODO] handle scope from a fncall-result");
				return NULL;
				break;
			}
			
			// finish
			case -3:
			{
				expr->data.fncall.next = false;
				goto realFinish;
			}

			}

			continue;
		}

		raiseError("[Syntax] Invalid object in path");
		return NULL;
	}

	realFinish:
	// Give last token
	Parser_saveToken(parser, &token);

	// Array_shrinkToFit(&expressions);
	return expressions.data;
}





static int giveIdToVariable(Variable* variable, Trace* trace, int size, bool isRegistrable) {
	int id = variable->id;
	if (id >= 0)
		return id;

	variable->id = (int)Trace_ins_create(trace, variable, size, 0, isRegistrable);
}

static void placeExpression(
	Parser* parser,
	Trace* trace,
	Expression* valueExpr,
	Expression* targetExpr,
	int expressionType
) {
	// Read targetExpr
	Expression* const sourceExpr = valueExpr;
	int sourceExprType = valueExpr->type;


	if (sourceExprType == EXPRESSION_PATH) {
		valueExpr = valueExpr->data.target;
		switch (valueExpr->type) {
		case EXPRESSION_PROPERTY:
		{
			/// TODO: handle this case
			if (valueExpr->data.property.next) {
				raiseError("[TODO]: valueExpr such that next==true not handled for TypeNode");
				return;
			}

			Variable** varrDest = targetExpr->data.property.variableArr;
			int subLength = targetExpr->data.property.length - 1;

			Variable* last = varrDest[subLength];
			int id;
			int signedSize;
			if (last->proto) {
				signedSize = Prototype_getSignedSize(last->proto);
				id = last->id;
			} else {
				Variable* lv = valueExpr->data.property.variableArr[valueExpr->data.property.length-1];
				Prototype* lvp = lv->proto;

				switch (Prototype_mode(*lvp)) {
				case PROTO_MODE_REFERENCE:
				{

					break;
				}

				case PROTO_MODE_DIRECT:
				{
					signedSize = Prototype_getSignedSize(lvp);
					id = Trace_ins_create(trace, lv, lvp->direct.primitiveSizeCode, 0, true);
					last->proto = Prototype_copy(lvp);
					last->id = id;

					break;
				}

				case PROTO_MODE_VARIADIC:
				{

					break;
				}

				case PROTO_MODE_PRIMITIVE:
				{
					signedSize = lvp->primitive.sizeCode;
					id = Trace_ins_create(trace, lv, signedSize < 0 ? -signedSize : signedSize, 0, true);
					last->proto = lvp;
					last->id = id;
					break;
				}

				case PROTO_MODE_VOID:
				{
					raiseError("[Intern] Cannot set a variable to void");
					return;
				}


				}
			}
			// int firstId = giveIdToVariable(varrDest);

			Trace_set(
				trace,
				valueExpr,
				id,
				Prototype_getVariableOffset(varrDest, subLength),
				signedSize,	
				EXPRESSION_PROPERTY
			);

			break;
		}

		
		case EXPRESSION_FNCALL:
		{
			/// TODO: handle null id
			Variable** varrDest = targetExpr->data.property.variableArr;
			int subLength = targetExpr->data.property.length - 1;

			Trace_set(
				trace,
				valueExpr,
				varrDest[0]->id,
				Prototype_getVariableOffset(varrDest, subLength),
				Prototype_getSignedSize(varrDest[subLength]->proto),
				EXPRESSION_FNCALL
			);


			break;
		}

		case EXPRESSION_VALUE:
		{
			raiseError("[TODO]: handle value read/edits");
			break;
		}


		}

	
	
	} else if (sourceExprType >= EXPRESSION_ADDITION && sourceExprType <= EXPRESSION_L_DECREMENT) {
		Variable** varrDest = targetExpr->data.property.variableArr;
		int subLength = targetExpr->data.property.length - 1;

		Prototype* vproto = varrDest[subLength]->proto;

		if (vproto) {
			Trace_set(
				trace,
				valueExpr,
				varrDest[0]->id,
				Prototype_getVariableOffset(varrDest, subLength),
				Prototype_getSignedSize(varrDest[subLength]->proto),
				sourceExprType
			);
		
		} else if (subLength == 0) {
			Variable* v = varrDest[0];
			int signedSize = Expression_reachSignedSize(sourceExprType, sourceExpr);
			Prototype* p = Expression_getPrimitiveProtoFromSize(signedSize);
			v->proto = p;

			int id = v->id;
			if (v->id < 0) {
				id = Trace_ins_create(trace, v, signedSize < 0 ? -signedSize : signedSize, 0, true);
				v->id = id;
			}


			Trace_set(
				trace,
				valueExpr,
				id,
				0,
				signedSize,
				sourceExprType
			);


		} else {
			raiseError("[Intern] No prototype to place expression");
		}



	} else if (sourceExprType >= EXPRESSION_U8 && sourceExprType <= EXPRESSION_F64) {
		Variable** variableArr = targetExpr->data.property.variableArr;
		int length = targetExpr->data.property.length;
		int subLength = length - 1;
		
		Variable** varrDest = targetExpr->data.property.variableArr;
		/// TODO: check sizes

		Prototype* vproto = varrDest[subLength]->proto;

		if (vproto) {
			int signedSize = Prototype_getSignedSize(vproto);
	
			Trace_ins_def(
				trace,
				varrDest[0]->id,
				Prototype_getVariableOffset(varrDest, length),
				signedSize,
				castable_cast(
					Expression_getSignedSize(sourceExprType),
					signedSize,
					valueExpr->data.num
				)
			);

		} else if (subLength == 0) {
			Variable* v = varrDest[0];
			int signedSize = Expression_getSignedSize(sourceExprType);
			v->proto = Expression_getPrimitiveProtoFromType(sourceExprType);

			int id = v->id;
			if (v->id < 0) {
				id = Trace_ins_create(trace, v, signedSize < 0 ? -signedSize : signedSize, 0, true);
				v->id = id;
			}

			Trace_ins_def(
				trace,
				id,
				0,
				signedSize,
				castable_cast(
					Expression_getSignedSize(sourceExprType),
					signedSize,
					valueExpr->data.num
				)
			);
		} else {
			raiseError("[Intern] No prototype to place expression");
		}


	} else if (sourceExprType == EXPRESSION_ADDR_OF) {
		Expression* operand = sourceExpr->data.operand;
		if (operand->type != EXPRESSION_PATH) {
			raiseError("[Syntax] Can only get the address of a variable");
			return;
		}
		
		Expression* reference = operand->data.target;
		if (reference->type != EXPRESSION_PROPERTY || reference->data.property.next) {
			raiseError("[Syntax] Can only get the address of a variable (property was expected)");
			return;
		}
		
		int refArrLength = reference->data.property.length;
		Variable** refVarArr = reference->data.property.variableArr;
		int srcOffset = Prototype_getVariableOffset(
			&refVarArr[1], refArrLength-1);

		int srcVar = refVarArr[0]->id;

		int targetSubLength = targetExpr->data.property.length - 1;
		Variable** varrDest = targetExpr->data.property.variableArr;
		int destVar = varrDest[0]->id;
		int destOffset;
		if (destVar < 0) {
			destVar = Trace_ins_create(trace, varrDest[0], 8, 0, true);
			varrDest[0]->id = destVar;
			destOffset = -1;
		} else {
			destOffset = Prototype_getVariableOffset(&varrDest[1], targetSubLength);

		}


		if (varrDest[targetSubLength]->proto == NULL) {
			if (targetSubLength != 0) {
				raiseError("[Intern] No prototype to place expression");
			}
			
			varrDest[targetSubLength]->proto = Prototype_generateStackPointer(refVarArr, refArrLength);
			reference->data.property.freeVariableArr = false;
		}
		

		Trace_ins_getStackPtr(trace, destVar, srcVar, destOffset, srcOffset);
		
		



	} else {
		raiseError("[TODO]: this expression is not handled");
	}

	// Free expression
	Expression_free(sourceExprType, sourceExpr);
	free(sourceExpr);
}




void Syntax_functionScope_varDecl(ScopeFunction* scope, Trace* trace, Parser* parser) {
	Token token = Parser_read(parser, &_labelPool);
	if (TokenCompare(SYNTAXLIST_FREE_LABEL, 0))
		return;

	Variable* variable = malloc(sizeof(Variable));
	variable->name = token.label;
	variable->id = -1;
	variable->proto = NULL;
	Variable_create(variable);
	
	int syntaxIndex = -1;
	Prototype* proto = NULL;
	Expression* valueExpr = NULL;

	
	// Type
	while (true) {
		token = Parser_read(parser, &_labelPool);
		int nextSyntaxIndex = TokenCompare(SYNTAXLIST_FUNCTION_VARDECL, 0);

		
		if (nextSyntaxIndex <= syntaxIndex) {
			raiseError("[Syntax] Illegal order in type declaration");
			return;
		}
		
		syntaxIndex = nextSyntaxIndex;

		switch (syntaxIndex) {
		// Type
		case 0:
		{
			proto = Syntax_proto(parser, &scope->scope);
			variable->proto = proto;
			ExtendedPrototypeSize eps = Prototype_reachSizes(variable->proto, &scope->scope, true);
			variable->id = (int)Trace_ins_create(trace, variable, eps.size, 0, eps.primitiveSizeCode);
			break;
		}

		// Value
		case 1:
			/// TODO: handle this expression
			valueExpr = Syntax_expression(parser, &scope->scope, 1);
			break;

		// end
		case 2:
			goto finish;
		
		}
	}

	// Add variable
	finish:
	if (valueExpr) {
		Expression exprToSet = {
			.type = EXPRESSION_PATH,
			.data = {
				.property = {&variable, 1, 0}
			}
		};

		placeExpression(parser, trace, valueExpr, &exprToSet, EXPRESSION_PATH);
	}
	
	
	Type* type = ScopeFunction_pushVariable(scope, variable, valueExpr);
}








void Syntax_functionScope_freeLabel(
	ScopeFunction* scope,
	Parser* parser,
	label_t firstLabel,
	Trace* trace
) {
	Expression* expression = Syntax_readPath(firstLabel, parser, &scope->scope);
	int expressionType = expression->type;

	// Analyze 
	bool containsEqual;
	{
		Token token = Parser_read(parser, &_labelPool);
		if (TokenCompare(SYNTAXLIST_SINGLETON_ASSIGN, SYNTAXFLAG_UNFOUND) == 0) {
			containsEqual = true;
		} else {
			containsEqual = false;
			Parser_saveToken(parser, &token);
		}
	}

	int leftSize = Expression_evalNextLength(expression);

	/// TODO: improve this case
	if (leftSize == 2) {
		if (containsEqual) {
			raiseError("[TODO] handle this case");
		} else {
			// Finish token
			Token token = Parser_read(parser, &_labelPool);
			if (TokenCompare(SYNTAXLIST_SINGLETON_END, 0))
				return;
		}

		if (expression[0].type != EXPRESSION_PROPERTY || expression[1].type != EXPRESSION_FNCALL) {
			raiseError("[TODO] just path.fn(...) handled");
			return;
		}

		

		goto freeData;


	}


	// fill branches
	switch (expressionType) {
	case EXPRESSION_PROPERTY:
	{
		if (expression->data.property.next) {
			raiseError("[TODO]: next==true not handled");
			break;
		}

		if (!containsEqual) {
			raiseError("[TODO]: must contain equal");
		}

		
		Expression* valueExpr = Syntax_expression(parser, &scope->scope, 1);

		// End operand
		Token token = Parser_read(parser, &_labelPool);
		if (TokenCompare(SYNTAXLIST_SINGLETON_END, 0) != 0)
			return;

		placeExpression(parser, trace, valueExpr, expression, expressionType);



		break;
	}

	case EXPRESSION_FNCALL:
	{
		Trace_set(
			trace,
			expression,
			TRACE_VARIABLE_NONE,
			0,
			0,
			EXPRESSION_FNCALL
		);


		Token token = Parser_read(parser, &_labelPool);
		if (TokenCompare(SYNTAXLIST_SINGLETON_END, 0))
			return;
			
		break;
	}


	case EXPRESSION_VALUE:
	{
		raiseError("[TODO]: Handle value edits");
		break;
	}

	default:
		raiseError("[Syntax] invalid expression type in free label");
		break;
	}


	freeData:
	Expression_free(expressionType, expression);
	free(expression);
}




static void Syntax_functionScope_if(
	Parser* parser,
	ScopeFunction* scope,
	Trace* trace
) {
	// Require left parethesis
	Token token = Parser_read(parser, &_labelPool);
	if (TokenCompare(SYNTAXLIST_SINGLETON_LPAREN, 0) != 0)
		return;

	Expression* expr = Syntax_expression(parser, &scope->scope, 1);
	// Require right parenthesis
	token = Parser_read(parser, &_labelPool);
	if (TokenCompare(SYNTAXLIST_SINGLETON_RPAREN, 0) != 0)
		return;

	// Require left brace
	token = Parser_read(parser, &_labelPool);
	if (TokenCompare(SYNTAXLIST_SINGLETON_LBRACE, 0) != 0)
		return;

	/// TODO: follow each pair

	ScopeFunction subScope = {
		.scope = {.parent = &scope->scope, type: SCOPE_FUNCTION},
		.fn = scope->fn
	};
	ScopeFunction_create(&subScope);


	int exprType = expr->type;
	uint dest = Trace_ins_create(trace, NULL, 4, 0, 4);
	
	/// TODO: handle size
	Trace_set(trace, expr, dest, TRACE_OFFSET_NONE, -4, exprType);

	Expression_free(exprType, expr);
	free(expr);

	
	trline_t* ifLine = Trace_ins_if(trace, dest);
	Trace_removeVariable(trace, dest);

	Trace_ins_savePlacement(trace);

	Syntax_functionScope(&subScope, trace, parser);
	ScopeFunction_delete(&subScope);
	
	Trace_ins_openPlacement(trace);

	/// TODO: compare new TypeNode with the else case => track similarities


	
	// No else
	token = Parser_read(parser, &_labelPool);
	if (token.type != TOKEN_TYPE_LABEL|| token.label != _commonLabels._else) {
		// Mark position
		*Trace_push(trace, 1) = TRACECODE_STAR | (6<<10);
		Parser_saveToken(parser, &token);
		*ifLine |= (trace->instruction << 10);
		return;
	}

	/// Here, else 
	raiseError("[TODO] else");

	// Require left brace
	token = Parser_read(parser, &_labelPool);
	if (TokenCompare(SYNTAXLIST_SINGLETON_LBRACE, 0) != 0)
		return;

	ScopeFunction_create(&subScope);
	Syntax_functionScope(&subScope, trace, parser);
	ScopeFunction_delete(&subScope);


	*ifLine |= (trace->instruction << 10);
}


static void Syntax_functionScope_while(
	Parser* parser,
	ScopeFunction* scope,
	Trace* trace
) {
	int* usageBackup = Trace_prepareWhileUsages(trace);
	Trace_ins_savePlacement(trace);

	// Mark start position
	*Trace_push(trace, 1) = TRACECODE_STAR | (6<<10);

	// Require left parethesis
	Token token = Parser_read(parser, &_labelPool);
	if (TokenCompare(SYNTAXLIST_SINGLETON_LPAREN, 0) != 0)
		return;

	Expression* expr = Syntax_expression(parser, &scope->scope, 1);
	// Require right parenthesis
	token = Parser_read(parser, &_labelPool);
	if (TokenCompare(SYNTAXLIST_SINGLETON_RPAREN, 0) != 0)
		return;

	// Require left brace
	token = Parser_read(parser, &_labelPool);
	if (TokenCompare(SYNTAXLIST_SINGLETON_LBRACE, 0) != 0)
		return;

	/// TODO: follow each pair

	ScopeFunction subScope = {
		.scope = {.parent = &scope->scope, type: SCOPE_FUNCTION},
		.fn = scope->fn,
		.thisvar = scope->thisvar
	};
	ScopeFunction_create(&subScope);

	
	
	int startInstruction = trace->instruction;

	/// TODO: handle size
	// Collect test value
	int exprType = expr->type;
	uint dest = Trace_ins_create(trace, NULL, 4, 0, 4);
	Trace_set(trace, expr, dest, TRACE_OFFSET_NONE, -4, exprType);
	Expression_free(exprType, expr);
	free(expr);

	// Add ifline
	trline_t* ifLine = Trace_ins_if(trace, dest);
	Trace_removeVariable(trace, dest);

	Trace_ins_saveShadowPlacement(trace);

	// While content
	int scopeId = Syntax_functionScope(&subScope, trace, parser);
	
	ScopeFunction_delete(&subScope);

	// Jump
	Trace_ins_openPlacement(trace);	
	Trace_ins_jmp(trace, startInstruction);
	
	
	Trace_ins_openShadowPlacement(trace);
	
	// Mark end position
	*Trace_push(trace, 1) = TRACECODE_STAR | (6<<10);
	int endInstruction = trace->instruction;
	*ifLine |= (endInstruction << 10);
	Trace_addWhileUsages(trace, scopeId, startInstruction, endInstruction, usageBackup);
}

int Syntax_functionScope(ScopeFunction* scope, Trace* trace, Parser* parser) {
	trace->deep++;
	int previousScopeId = trace->scopeId;
	int currentScopeId = trace->nextScopeId;
	trace->scopeId = currentScopeId;
	trace->nextScopeId = currentScopeId + 1;
	*Stack_push(int, &trace->scopeIdStack) = currentScopeId;

	while (true) {
		Token token = Parser_read(parser, &_labelPool);
		
		// Search
		switch (TokenCompare(SYNTAXLIST_FUNCTION, 0)) {
		// let
		case 0:
		{
			Syntax_functionScope_varDecl(scope, trace, parser);
			break;
		}

		// const
		case 1:
		{
			raiseError("[TODO] in Syntax_functionScope: const");
			break;
		}

		// if
		case 2:
		{
			Syntax_functionScope_if(parser, scope, trace);
			break;
		}

		// for
		case 3:
		{
			raiseError("[TODO] in Syntax_functionScope: for");
			break;
		}

		// while
		case 4:
		{
			Syntax_functionScope_while(parser, scope, trace);
			break;
		}

		// return
		case 5:
		{
			Expression* expr = Syntax_expression(parser, &scope->scope, 1);
			int exprType = expr->type;
			int signedSize = Prototype_getSignedSize(scope->fn->returnType);
			int size = signedSize >= 0 ? signedSize : -signedSize;

			if (!scope->fn->returnType) {
				raiseError("[Architecture] Tried to return void");
				return -1;
			}
			char isRegistrable = Prototype_getPrimitiveSizeCode(scope->fn->returnType);
			uint variable = Trace_ins_create(trace, NULL, size, 0, isRegistrable);

			
			// Set value
			/// TODO: check offset
			Trace_set(trace, expr, variable, TRACE_OFFSET_NONE, signedSize, exprType);
			Expression_free(exprType, expr);
			free(expr);
			
			// Place value in rax
			uint regVar = Trace_ins_create(trace, NULL, size, 0, isRegistrable);
			Trace_ins_placeReg(trace, variable, regVar, TRACE_REG_RAX, Trace_packSize(size));
			Trace_removeVariable(trace, variable);
			
			*Trace_push(trace, 1) = TRACECODE_STAR | (2<<10);
			Trace_removeVariable(trace, regVar);


			token = Parser_read(parser, &_labelPool);
			if (TokenCompare(SYNTAXLIST_SINGLETON_END, 0) != 0)
				return -1;

			break;
		}

		// switch
		case 6:
		{
			raiseError("[TODO] in Syntax_functionScope: switch");
			break;
		}

		// free label
		case 7:
		{
			Syntax_functionScope_freeLabel(scope, parser, token.label, trace);
			break;
		}

		// finish scope
		case 8:
		{
			goto finishScope;
		}
		}
	}


	// Remove variable traces
	finishScope:
	Array_loop(TypeDefinition, scope->types, td) {
		printf("id[%d]{%s}\n", td->variable->id, td->variable->name);
		Trace_removeVariable(trace, td->variable->id);
	}

	trace->deep--;
	Stack_pop(int, &trace->scopeIdStack);
	trace->scopeId = previousScopeId;
	return currentScopeId;
}


bool Syntax_functionDefinition(Scope* scope, Parser* parser, Function* fn, Class* thisclass) {
	fn->definitionState = DEFINITIONSTATE_READING;

	Variable thisvar;
	ScopeFunction fnScope;
	Trace trace;
	Trace_create(&trace);


	if (thisclass) {
		thisvar.name = _commonLabels._this;

		thisvar.id = Trace_pushVariable(&trace);
		/// TODO: warning with this declaration (no settings)
		thisvar.proto = Prototype_create_direct(thisclass, 0, NULL, 0);
		Variable_create(&thisvar);

		fnScope = (ScopeFunction){
			{scope, SCOPE_FUNCTION},
			fn,
			.thisvar = &thisvar,
			.thisclass = thisclass
		};


	} else {
		fnScope = (ScopeFunction){
			{scope, SCOPE_FUNCTION},
			fn,
			.thisvar = NULL,
			.thisclass = NULL
		};
	}

	// Add arguments
	Trace_pushArgs(&trace, fn->arguments.data, fn->arguments.length);
	

	// Function procedure
	ScopeFunction_create(&fnScope);	

	Syntax_functionScope(&fnScope, &trace, parser);
	
	ScopeFunction_delete(&fnScope);
	
	*Trace_push(&trace, 1) = TRACECODE_STAR;
	

	TracePack* pack = trace.first;
	int position = 0;
	while (pack) {
		TracePack_print(pack, position);
		position += pack->completion+1;
		pack = pack->next;
	}


	Trace_placeRegisters(&trace);

	
	
	FunctionAssembly fnAsm;
	FunctionAssembly_create(&fnAsm, &fnScope);
	
	/// TODO: enable this line
	Trace_generateAssembly(&trace, &fnAsm);
	FunctionAssembly_delete(&fnAsm);

	

	Trace_delete(&trace);

	if (fnScope.thisvar) {
		Variable_delete(&thisvar);
	}

	fn->definitionState = DEFINITIONSTATE_DONE;
	return true;
}



void Syntax_annotation(Annotation* annotation, Parser* parser, LabelPool* labelPool) {
	Token token = Parser_read(parser, labelPool);

	if (TokenCompare(SYNTAXLIST_FREE_LABEL, 0))
		return;

	if (token.label == _commonLabels._main) {
		annotation->type = ANNOTATION_MAIN;
		return;
	}

	if (token.label == _commonLabels._control) {
		annotation->type = ANNOTATION_CONTROL;
		return;
	}

	if (token.label == _commonLabels._langstd) {
		annotation->type = ANNOTATION_LANGSTD;
		return;
	}

	raiseError("[Syntax] Unkown annotation");
}


#undef TokenCompare
