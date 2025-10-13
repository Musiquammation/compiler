#include "Syntax.h"

#include "Scope.h"
#include "Parser.h"
#include "globalLabelPool.h"
#include "helper.h"

#include "syntaxList.c" // !do not include twice!

#include "Expression.h"
#include "definitionState_t.h"
#include "Module.h"
#include "Class.h"
#include "Variable.h"
#include "Function.h"
#include "Trace.h"

#include "primitives.h"
#include "Type.h"
#include "TypeNode.h"
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
			const Syntax_FunctionDeclaration defaultData = {
				.defaultName = scope->name
			};

			Syntax_functionDeclaration(&scope->scope, &parser, 0, &defaultData);

			break;
		}

		// class
		case 1:
		{
			const Syntax_ClassDeclaration defaultData = {
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


	while (true) {
		Token token = Parser_read(&parser, &_labelPool);
		
		switch (TokenCompare(SYNTAXLIST_FILE, SYNTAXFLAG_EOF)) {
		// function
		case 0:
		{
			/// TODO: definition required ?
			const Syntax_FunctionDeclaration defaultData = {
				.defaultName = scope->name
			};

			Syntax_functionDeclaration(&scope->scope, &parser, 0, &defaultData);

			break;
		}

		// class
		case 1:
		{
			const Syntax_ClassDeclaration defaultData = {
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





Expression* Syntax_expression(Parser* parser, Scope* scope, bool isExpressionRoot) {
	Token token = Parser_read(parser, &_labelPool);
	if (
		isExpressionRoot &&
		TokenCompare(SYNTAXLIST_SINGLETON_RPAREN, SYNTAXFLAG_UNFOUND) == 0
	) {
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
			Expression* target = Syntax_expression(parser, scope, false);
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
				const Syntax_FunctionDeclaration defaultData = {
					.defaultName = LABEL_NULL,
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
				const Syntax_ClassDeclaration defaultData = {
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





Class* Syntax_proto(Parser* parser, Scope* scope, Prototype* proto, bool finishByComma) {
	Token token = Parser_read(parser, &_labelPool);

	// Get type name
	if (TokenCompare(SYNTAXLIST_FREE_LABEL, 0))
		return NULL;

	const label_t name = token.label;
	int syntaxIndex = -1;
	bool canBePrimitive = true;

	// Collect syntax data
	while (true) {
		// Collect next syntaxIndex
		{
			token = Parser_read(parser, &_labelPool);
			int next = TokenCompare(SYNTAXLIST_TYPE, 0);
			if (next <= syntaxIndex) {
				raiseError("[Syntax] Illegal order in type declaration");
				return NULL;
			}
			
			syntaxIndex = next;
		}


		switch (syntaxIndex) {
		// Settings
		case 0:
			canBePrimitive = false;
			raiseError("[TODO] read settings");
			break;
			
			// Read verified functions
		case 1:
			canBePrimitive = false;
			raiseError("[TODO] verified functions");
			break;

		// Finish by comma (or right parenthesis) operator
		case 2:
		case 3:
			if (!finishByComma)
				raiseError("[SYNTAX] type should finish by semicolon");

			// Save right parenthesis
			if (syntaxIndex == 3)
				Parser_saveToken(parser, &token);

			goto generate;

		// Finish by end operator (or equal / lbrace)
		case 4:
		case 5:
		case 6:
			if (finishByComma)
				raiseError("[SYNTAX] type should finish by a comma");

			Parser_saveToken(parser, &token);
			goto generate;
			
			
		default:
			return NULL;
		}
	}

	// Generate type
	generate:
	/// TODO: check this call
	Class* cl = Scope_search(scope, name, NULL, SCOPESEARCH_CLASS);
	if (!cl) {
		if (!Scope_defineOnFly(scope, name))
			goto classNotFound;
		
		cl = Scope_search(scope, name, NULL, SCOPESEARCH_CLASS);
		if (!cl) {
			classNotFound:
			raiseError("[UNKOWN] Class not found");
			return NULL;
		}
	}

	proto->cl = cl;
	proto->primitiveSizeCode = canBePrimitive ? cl->primitiveSizeCode : 0;
	return cl;
}







void Syntax_classDeclaration(Scope* scope, Parser* parser, int flags, const Syntax_ClassDeclaration* defaultData) {
	label_t name = LABEL_NULL;
	int syntaxIndex = -1;
	bool definitionToRead;
	Class* meta = NULL;
	
	// Handle annotations
	/// TODO: complete annotations 
	Array_loop(Annotation, parser->annotations, annotation) {
		int annotType = annotation->type;

		switch (annotType) {
		case ANNOTATION_META:
		{
			if (meta) {
				raiseError("[Syntax] Meta already defined for this class declaration");
				return;
			}
	
			meta = Scope_search(scope, annotation->meta, NULL, SCOPESEARCH_CLASS);
			if (meta == NULL) {
				raiseError("[Unknown] @meta class not found");
				return;
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

		cl->meta = meta;

	} else {
		cl = malloc(sizeof(Class));
		Class_create(cl);
		cl->name = name;
		cl->meta = meta;
		cl->definitionState = definitionToRead ? DEFINITIONSTATE_READING : DEFINITIONSTATE_NOT;
		cl->primitiveSizeCode = 0;
		Scope_addClass(scope, scope->type, cl);
	}


	// Read definition
	if (definitionToRead) {
		Syntax_classDefinition(scope, parser, cl);
	} else if (meta) {
		raiseError("[Syntax] @meta should be given in defintion");
		return;
	}
}




void Syntax_classDefinition(Scope* parentScope, Parser* parser, Class* cl) {
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


	int offset = 0;
	int maxMinimalSize = 0;

	while (true) {
		Token token = Parser_read(parser, &_labelPool);
		
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

		// label
		case 2:
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
				
				Class* tcl = Syntax_proto(parser, &outsideScope.scope, &variable->proto, false);
	
				// Finish by end operation
				token = Parser_read(parser, &_labelPool);
				if (TokenCompare(SYNTAXLIST_SINGLETON_END, 0))
					return;

				

				// Get object size
				int vsize = tcl->size;
				if (vsize < 0) {
					// Define class
					if (tcl->definitionState != DEFINITIONSTATE_NOT) {
						raiseError("[Architecture] Cannot get the size of an uncomplete type");
						return;
					}

					Scope_defineOnFly(parentScope, tcl->name);
					if (tcl->definitionState != DEFINITIONSTATE_DONE) {
						raiseError("[Architecture] Cannot define on fly current class");
						return;
					}

					vsize = tcl->size;
					if (vsize < 0) {
						raiseError("[Architecture] Cannot get the size of an uncomplete type");
					}
				}

				// Get offset
				int mms = tcl->maxMinimalSize;
				if (mms > maxMinimalSize)
					maxMinimalSize = mms;

				int d = offset % mms;
				if (d) {
					offset += mms - d;
				}
				variable->offset = offset;
				offset += vsize;

				break;
			}
			
			// function
			case 1:
			{
				Syntax_FunctionDeclaration declData = {
					.defaultName = name
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
		case 3:
		{
			raiseError("[TODO] orders in class definition\n");
			break;
		}

		// end
		case 4:
			goto close;

		default:
			goto close;
		}
	}

	

	close:

	// Align offset
	if (maxMinimalSize == 0) {
		cl->size = 0;
		cl->maxMinimalSize = 0;
		return;
	}

	int d = offset % maxMinimalSize;
	if (d) {
		offset += maxMinimalSize - d;
	} 
	cl->size = offset;
	cl->maxMinimalSize = maxMinimalSize;


	cl->definitionState = DEFINITIONSTATE_DONE;

	return;
}












void Syntax_functionDeclaration(Scope* scope, Parser* parser, int flags, const Syntax_FunctionDeclaration* defaultData) {
	label_t name = LABEL_NULL;
	int syntaxIndex = -1;
	bool definitionToRead;
	bool returnTypeAlreadyRead = false;
	Array arguments = {.length = 0xffff}; // no args read ; type: Variable*
	Prototype returnType = {.cl = NULL};
	
	// Handle annotations
	/// TODO: complete annotations 
	Array_loop(Annotation, parser->annotations, annotation) {
		switch (annotation->type) {
		case ANNOTATION_MAIN:
		{
			// Set name
			name = _commonLabels._main;

			// Return type
			returnType.cl = &_primitives.class_i32;
			returnType.primitiveSizeCode = true;
			returnTypeAlreadyRead = true;

			// Arguments
			Variable** argList = malloc(1 * sizeof(Variable*));
			Variable* argc = malloc(sizeof(Variable));
			Variable_create(argc);
			argc->id  -1;
			argc->proto.cl = &_primitives.class_i32;
			argc->proto.primitiveSizeCode = true;
			
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
			arguments = Syntax_functionArguments(scope, parser);
			break;

		// Type
		case 3:
			if (returnTypeAlreadyRead) {
				raiseError("[Architecture] return type defined");
				break;
			}
			returnTypeAlreadyRead = true;
			Syntax_proto(parser, scope, &returnType, false);
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
	Function* fn = Scope_search(scope, name, NULL, SCOPESEARCH_FUNCTION);
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

		if (returnType.cl) {
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
		fn->definitionState = definitionToRead ? DEFINITIONSTATE_READING : DEFINITIONSTATE_NOT;
		fn->arguments = arguments;
		fn->returnType = returnType;

		Scope_addFunction(scope, scope->type, fn);
	}


	if (definitionToRead) {
		Syntax_functionDefinition(scope, parser, fn);
	}

	/// TODO: define module.mainFunction

}


Array Syntax_functionArguments(Scope* scope, Parser* parser) {
	Array arguments;
	Array_create(&arguments, sizeof(Variable*));

	while (true) {
		Token token = Parser_read(parser, &_labelPool);
		switch (TokenCompare(SYNTAXLIST_ARGS, 0)) {
		// variable
		case 0:
		{
			const label_t name = token.label;

			// Get COLON operator for type
			token = Parser_read(parser, &_labelPool);
			if (TokenCompare(SYNTAXLIST_SINGLETON_COLON, 0))
				goto finish;

			Variable* variable = malloc(sizeof(Variable));
			*Array_push(Variable*, &arguments) = variable;

			variable->name = name;
			variable->id = -1;

			Syntax_proto(parser, scope, &variable->proto, true);
			
			break;
		}

		// end
		case 1:
			goto finish;
		}
	}

	finish:
	return arguments;
}



Expression* Syntax_readPath(label_t label, Parser* parser, Scope* scope) {
	Array expressions; // type: Expression
	Array currentVarPath; // type: Variable*
	Array_create(&expressions, sizeof(Expression)); 
	Array_create(&currentVarPath, sizeof(Variable*)); 


	union {
		ScopeClass cl;

	} subScope;

	char isFirstLabelState = 1;
	Scope* subScopePtr = scope;


	while (true) {
		// Get type
		Token token = Parser_read(parser, &_labelPool);
		int syntaxResult = TokenCompare(SYNTAXLIST_PATH, SYNTAXFLAG_UNFOUND);
		switch (syntaxResult) {
		// Object (or not found)
		case 0:
		case -3:
		{
			// Search property
			ScopeSearchArgs searchArg;
			void* object = Scope_search(subScopePtr, label, &searchArg, SCOPESEARCH_ANYTYPE);

			if (!object) {
				raiseError("[Unknown] Object not defined");
				return NULL;
			}

			if (searchArg.resultType == SCOPESEARCH_VARIABLE) {
				*Array_push(Variable*, &currentVarPath) = (Variable*)object;
				subScope.cl.scope.parent = NULL;
				subScope.cl.scope.type = SCOPE_CLASS;
				subScope.cl.cl = ((Variable*)object)->proto.cl;
				subScope.cl.allowThis = false;
				subScopePtr = &subScope.cl.scope;

				goto nextProperty;
			}
			
			// Generate var path
			if (currentVarPath.length) {
				generateVarExpr:
				size_t size = sizeof(Variable) * currentVarPath.length;
				Variable** list = malloc(size);
				memcpy(list, currentVarPath.data, size);

				Expression* expr = Array_push(Expression, &expressions);
				expr->type = EXPRESSION_PROPERTY;
				expr->data.property.length = currentVarPath.length;
				expr->data.property.variableArr = list;

				currentVarPath.length = 0;

				// exit loop
				if (syntaxResult == -3) {
					Parser_saveToken(parser, &token);
					goto exitWhile;
				}

			}

			// class
			if (searchArg.resultType == SCOPESEARCH_CLASS) {
				raiseError("[TODO]: SCOPESEARCH_CLASS");
			} else if (searchArg.resultType == SCOPESEARCH_FUNCTION) {
				raiseError("[TODO]: SCOPESEARCH_FUNCTION");
			}

			nextProperty:
			if (syntaxResult == -3) {
				goto generateVarExpr;
			}

			token = Parser_read(parser, &_labelPool);
			if (TokenCompare(SYNTAXLIST_FREE_LABEL, 0) != 0)
				return NULL;

			// Next label and next scope
			label = token.label;
			break;
		}

		// Function call
		case 1:
		{
			// Search function
			/// TODO: search unsing args (for multiple fn calls)
			ScopeSearchArgs searchArg;
			Function* fn = Scope_search(subScopePtr, label, &searchArg, SCOPESEARCH_FUNCTION);

			if (!fn) {
				raiseError("[Unknown] Function not defined");
				return NULL;
			}

			/// TODO: create a subscope to handle words (of enums)
			Array args; // type: Expression*
			Array_create(&args, sizeof(Expression*));
			while (true) {
				Expression* expr = Syntax_expression(parser, scope, true);
				if (expr) {
					*Array_push(Expression*, &args) = expr;
				}

				token = Parser_read(parser, &_labelPool);
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

			Expression* expr = Array_push(Expression, &expressions);
			expr->type = EXPRESSION_FNCALL;
			expr->data.fncall.object = call;


			// Handle next call
			Token token = Parser_read(parser, &_labelPool);
			switch (TokenCompare(SYNTAXLIST_CONTINUE_FNCALL, SYNTAXFLAG_UNFOUND)) {
			// member
			case 0:
			{
				token = Parser_read(parser, &_labelPool);
				if (TokenCompare(SYNTAXLIST_FREE_LABEL, 0) != 0)
					return NULL;

				Class* retCl = fn->returnType.cl;
				if (retCl == NULL) {
					raiseError("[Type] Cannot get result of a void function");
					return NULL;
				}

				label = token.label;
				subScope.cl.scope.parent = NULL;
				subScope.cl.scope.type = SCOPE_CLASS;
				subScope.cl.cl = retCl;
				subScope.cl.allowThis = false;
				subScopePtr = &subScope.cl.scope;
				break;
			}

			// next call
			case 1:
			{
				raiseError("[TODO] Fn call after a fn call is not handled");
				return NULL;
				break;
			}

			// not found
			default:
			{
				Parser_saveToken(parser, &token);
				goto exitWhile;
			}
			}


			break;
		}
		}

		isFirstLabelState = 0;
	}


	// Fill 'next' members
	exitWhile:
	Expression* const output = malloc(sizeof(Expression) * expressions.length);
	int subLength = expressions.length - 1;
	for (int i = 0; i <= subLength; i++) {
		Expression* val = Array_get(Expression, expressions, i);
		Expression* res = &output[i];
		switch (val->type) {
		case EXPRESSION_PROPERTY:
			res->type = EXPRESSION_PROPERTY;
			res->data.property.variableArr = val->data.property.variableArr;
			res->data.property.length = val->data.property.length;
			res->data.property.next = i < subLength;
			break;

		case EXPRESSION_FNCALL:
			res->type = EXPRESSION_FNCALL;
			res->data.fncall.object = val->data.fncall.object;
			res->data.fncall.next = i < subLength;
			break;
		}
		
	}

	/// TODO: generate expressions 
	Array_free(expressions);
	Array_free(currentVarPath);

	return output;
}


void Syntax_functionScope_varDecl(ScopeFunction* scope, Trace* trace, Parser* parser) {
	Token token = Parser_read(parser, &_labelPool);
	if (TokenCompare(SYNTAXLIST_FREE_LABEL, 0))
		return;

	Variable* variable = malloc(sizeof(Variable));
	variable->name = token.label;
	variable->id = -1;
	Variable_create(variable);
	
	int syntaxIndex = -1;
	Expression* exprValue = NULL;

	
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
			Class* cl = Syntax_proto(parser, &scope->scope, &variable->proto, false);
			variable->id = (int)Trace_ins_create(trace, variable, cl->size, 0);
			break;
		}

		// Value
		case 1:
			/// TODO: handle this expression
			exprValue = Syntax_expression(parser, &scope->scope, true);

			break;

		// end
		case 2:
			goto finish;
		
		}
	}

	// Add variable
	finish:
	if (exprValue) {
		/// TODO: handle this expression
		Expression_free(exprValue->type, exprValue);
		free(exprValue);
		raiseError("[TODO]: read value while creating a new variable");
		return;
	}
	
	ScopeFunction_pushVariable(scope, variable, exprValue);
}





void Syntax_functionScope_freeLabel(
	ScopeFunction* scope,
	Parser* parser,
	Token token,
	Trace* trace
) {
	Expression* expression = Syntax_readPath(token.label, parser, &scope->scope);
	int expressionType = expression->type;

	// fill branches
	switch (expressionType) {
	case EXPRESSION_PROPERTY:
	{
		if (expression->data.property.next) {
			raiseError("[TODO]: next==true not handled");
			break;
		}

		token = Parser_read(parser, &_labelPool);
		if (TokenCompare(SYNTAXLIST_SINGLETON_ASSIGN, 0) != 0)
			return;
		
		// Read expression
		Expression* expr = Syntax_expression(parser, &scope->scope, true);
		Expression* const exprSource = expr;
		int exprSourceType = expr->type;


		if (exprSourceType == EXPRESSION_PATH) {
			expr = expr->data.target;
			switch (expr->type) {
			case EXPRESSION_PROPERTY:
			{
				/// TODO: handle this case
				if (expr->data.property.next) {
					raiseError("[TODO]: expr such that next==true not handled for TypeNode");
					return;
				}

				TypeNode* operand = TypeNode_get(
					scope->rootNode,
					expr->data.property.variableArr,
					expr->data.property.length
				);
				TypeNode_set(
					scope->rootNode,
					expression->data.property.variableArr,
					operand,
					expression->data.property.length
				);

				Variable** varrDest = expression->data.property.variableArr;
				int subLength = expression->data.property.length - 1;
				int signedSize = Prototype_getSignedSize(&varrDest[subLength]->proto);

				Trace_set(
					trace,
					expr,
					varrDest[0]->id,
					Variable_getPathOffset(&varrDest[1], subLength),
					signedSize,	
					EXPRESSION_PROPERTY
				);

				break;
			}

			
			case EXPRESSION_FNCALL:
			{
				/// TODO: handle TypeNode
				Variable** varrDest = expression->data.property.variableArr;
				int subLength = expression->data.property.length - 1;
				Trace_set(
					trace,
					expr,
					varrDest[0]->id,
					Variable_getPathOffset(&varrDest[1], subLength),
					Prototype_getSignedSize(&varrDest[subLength]->proto),
					EXPRESSION_FNCALL
				);


				break;
			}


			}

		
		
		} else if (exprSourceType >= EXPRESSION_ADDITION && expressionType <= EXPRESSION_L_DECREMENT) {
			Variable** varrDest = expression->data.property.variableArr;
			int subLength = expression->data.property.length - 1;
			Trace_set(
				trace,
				expr,
				varrDest[0]->id,
				Variable_getPathOffset(&varrDest[1], subLength),
				Prototype_getSignedSize(&varrDest[subLength]->proto),
				exprSourceType
			);


		} else if (exprSourceType >= EXPRESSION_U8 && exprSourceType <= EXPRESSION_F64) {
			Variable** variableArr = expression->data.property.variableArr;
			int length = expression->data.property.length;
			TypeNode* valueNode = TypeNode_tryReach(
				scope->rootNode,
				variableArr,
				length
			);

			if (valueNode) {
				valueNode->value.num = expr->data.num;
				valueNode->length = TYPENODE_LENGTH_I32;
			} else {
				valueNode = malloc(sizeof(TypeNode));
				valueNode->length = TYPENODE_LENGTH_I32;
				valueNode->usage = 0;
				TypeNode_set(scope->rootNode, variableArr, valueNode, length);
			}
			

			Variable** varr = expression->data.property.variableArr;
			int subLength = expression->data.property.length - 1;
			/// TODO: check sizes
			int signedSize = Prototype_getSignedSize(&varr[subLength]->proto);

			Trace_ins_def(
				trace,
				varr[0]->id,
				Variable_getPathOffset(&varr[1], subLength),
				signedSize,
				castable_cast(
					Expression_getSignedSize(exprSourceType),
					signedSize,
					expr->data.num
				)
			);

		} else {
			raiseError("[TODO]: this expression is not handled");
		}

		// Free expression
		Expression_free(exprSourceType, exprSource);
		free(exprSource);


		// End operand
		token = Parser_read(parser, &_labelPool);
		if (TokenCompare(SYNTAXLIST_SINGLETON_END, 0) != 0)
			return;


		break;
	}

	case EXPRESSION_FNCALL:
	{
		raiseError("[TODO] handle direct fn call in freelabel");
		break;
	}

	default:
		raiseError("[Syntax] invalid expression type in free label");
		break;
	}


	Expression_free(expressionType, expression);
	free(expression);
}



void Syntax_functionScope(ScopeFunction* scope, Trace* trace, Parser* parser) {
	/// TODO: create trace (icounter)

	while (true) {
		Token token = Parser_read(parser, &_labelPool);
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
			// Require left parethesis
			token = Parser_read(parser, &_labelPool);
			if (TokenCompare(SYNTAXLIST_SINGLETON_LPAREN, 0) != 0)
				return;

			Expression* expr = Syntax_expression(parser, &scope->scope, true);
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
				.rootNode = scope->rootNode
			};
			ScopeFunction_create(&subScope);


			int exprType = expr->type;
			uint dest = Trace_ins_create(trace, NULL, 4, 0);
			Trace_set(trace, expr, dest, 0, 4, exprType);

			Expression_free(exprType, expr);
			free(expr);

			
			trline_t* ifLine = Trace_ins_if(trace, dest);
			Trace_removeVariable(trace, dest);

			Syntax_functionScope(&subScope, trace, parser);

			ScopeFunction_delete(&subScope);

			/// TODO: compare new TypeNode with the else case => track similarities

			
			// Check else
			token = Parser_read(parser, &_labelPool);
			if (token.type != TOKEN_TYPE_LABEL|| token.label != _commonLabels._else) {
				Parser_saveToken(parser, &token);
				*ifLine |= (trace->instruction << 10);
				break;
			}

			// Require left brace
			token = Parser_read(parser, &_labelPool);
			if (TokenCompare(SYNTAXLIST_SINGLETON_LBRACE, 0) != 0)
				return;

			ScopeFunction_create(&subScope);
			Syntax_functionScope(&subScope, trace, parser);
			ScopeFunction_delete(&subScope);

			*ifLine |= (trace->instruction << 10);
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
			// Require left parethesis
			token = Parser_read(parser, &_labelPool);
			if (TokenCompare(SYNTAXLIST_SINGLETON_LPAREN, 0) != 0)
				return;

			Expression* expr = Syntax_expression(parser, &scope->scope, true);
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
				.rootNode = scope->rootNode
			};
			ScopeFunction_create(&subScope);

			
			int startInstruction = trace->instruction;

			int exprType = expr->type;
			uint dest = Trace_ins_create(trace, NULL, 4, 0);
			Trace_set(trace, expr, dest, 0, 4, exprType);

			Expression_free(exprType, expr);
			free(expr);

			
			trline_t* ifLine = Trace_ins_if(trace, dest);
			Trace_removeVariable(trace, dest);

			Syntax_functionScope(&subScope, trace, parser);


			Trace_ins_jmp(trace, startInstruction);
			
			*ifLine |= (trace->instruction << 10);

			ScopeFunction_delete(&subScope);

			break;
		}

		// switch
		case 5:
		{
			raiseError("[TODO] in Syntax_functionScope: switch");
			break;
		}

		// free label
		case 6:
		{
			Syntax_functionScope_freeLabel(scope, parser, token, trace);
			break;
		}

		// finish scope
		case 7:
		{
			goto finishScope;
		}
		}
	}


	// Remove variable traces
	finishScope:
	typedef Variable* vptr_t;
	Array_loop(vptr_t, scope->variables, vptr) {
		Variable* v = *vptr;
		Trace_removeVariable(trace, v->id);
	}

	return;
}


bool Syntax_functionDefinition(Scope* scope, Parser* parser, Function* fn) {
	fn->definitionState = DEFINITIONSTATE_READING;

	ScopeFunction fnScope = {
		{scope, SCOPE_FUNCTION},
		fn
	};
	
	Trace trace;
	{
		ScopeFile* file = Scope_reachFile(scope);
		trace.icounter = file->icounter;
	}
	Trace_create(&trace);

	
	ScopeFunction_create(&fnScope);	
	fnScope.rootNode = malloc(sizeof(TypeNode));
	fnScope.rootNode->length = 0;

	Syntax_functionScope(&fnScope, &trace, parser);
	
	TypeNode_unfollow(fnScope.rootNode, 1);
	free(fnScope.rootNode);
	ScopeFunction_delete(&fnScope);


	TracePack_print(trace.last);


	Trace_delete(&trace);

	// FunctionAssembly fnAsm;
	// FunctionAssembly_create(&fnAsm, &fnScope);

	fn->definitionState = DEFINITIONSTATE_DONE;
}



void Syntax_annotation(Annotation* annotation, Parser* parser, LabelPool* labelPool) {
	Token token = Parser_read(parser, labelPool);

	if (TokenCompare(SYNTAXLIST_FREE_LABEL, 0))
		return;

	if (token.label == _commonLabels._meta) {
		annotation->type = ANNOTATION_META;

		Token token = Parser_read(parser, &_labelPool);
		if (TokenCompare(SYNTAXLIST_SINGLETON_LPAREN, 0) != 0)
			return;
		
		token = Parser_read(parser, &_labelPool);
		if (TokenCompare(SYNTAXLIST_FREE_LABEL, 0) != 0)
			return;

		annotation->meta = token.label;

		token = Parser_read(parser, &_labelPool);
		if (TokenCompare(SYNTAXLIST_SINGLETON_RPAREN, 0) != 0)
			return;

		return;
	}

	if (token.label == _commonLabels._main) {
		annotation->type = ANNOTATION_MAIN;
		return;
	}

	raiseError("[Syntax] Unkown annotation");
}


#undef TokenCompare
