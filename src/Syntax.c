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
	Array lineArr; // type: Expression
	Array_create(&lineArr, sizeof(Expression));

	// Collect expressions
	while (true) {
		Token token = Parser_read(parser, &_labelPool);

		const int syntaxIndex = TokenCompare(SYNTAXLIST_EXPRESSION, 0);
		switch (syntaxIndex) {

		// variable
		case 0:
		{
			Array exprPath;
			Array_create(&exprPath, sizeof(Expression));

			char currentScopeType = 0;
			ScopeClass scopeClass;

			while (true) {
				// Create expression
				Expression* expr = Array_push(Expression, &exprPath);
				expr->type = EXPRESSION_PROPERTY;

				Scope* subScope;
				switch (currentScopeType) {
				case 0:
					subScope = scope;
					break;

				case 1:
					subScope = &scopeClass.scope;
					break;
				}
				
				// Search variable
				Variable* variable = Scope_search(subScope, token.label, NULL, SCOPESEARCH_VARIABLE);
				expr->data.property.variable = variable;

				if (!variable) {
					raiseError("[UNKOWN] Variable not defined");
					return NULL;
				}

				token = Parser_read(parser, &_labelPool);
				
				
				switch (TokenCompare(SYNTAXLIST_PROPERTY, SYNTAXFLAG_UNFOUND)) {
				// get
				case 0:
				{
					// Set scope
					scopeClass.scope.type = SCOPE_CLASS;
					scopeClass.scope.parent = NULL;
					scopeClass.cl = variable->proto.cl;
					scopeClass.allowThis = false;
					currentScopeType = 1;

					// Get next label
					token = Parser_read(parser, &_labelPool);
					if (TokenCompare(SYNTAXLIST_FREE_LABEL, 0)) {
						raiseError("[Syntax] Property was expected");
						return NULL;
					}


					break; // continue reading variable
				}

				default:
					goto finishVariable;
				}

			}


			finishVariable:
			Parser_saveToken(parser, &token); // save back token

			// Fill expressions
			Expression* finalExpr = Array_push(Expression, &lineArr);

			if (exprPath.length == 1) {
				/// TODO: handle for function calls
				Expression* origin = Array_get(Expression, exprPath, 0);
				finalExpr->type = EXPRESSION_PROPERTY;
				finalExpr->data.property.source = NULL;
				finalExpr->data.property.variable = origin->data.property.variable;
				break;
			}

			const int subLength = exprPath.length-1;
			Expression* const buffer = malloc(subLength * sizeof(Expression));

			Expression* e = Array_get(Expression, exprPath,  subLength);
			
			finalExpr->type = EXPRESSION_PROPERTY;
			finalExpr->data.property.source = buffer;
			finalExpr->data.property.variable = e->data.property.variable;

			for (int i = 0; i < subLength; i++) {
				e = Array_get(Expression, exprPath,  subLength - i);

				buffer[i].type = EXPRESSION_PROPERTY;
				buffer[i].data.property.source = &buffer[i+1];
				buffer[i].data.property.variable = e->data.property.variable;
			}

			buffer[subLength-1].data.property.source = NULL;


			Array_free(exprPath);


			break;
		}

		// number
		case 1:
		{
			Expression* e = Array_push(Expression, &lineArr);
			switch (token.type) {
			case TOKEN_TYPE_I8:
				e->type = EXPRESSION_I8;
				e->data.i8 = token.i8;
				break;

			case TOKEN_TYPE_U8:
				e->type = EXPRESSION_U8;
				e->data.u8 = token.u8;
				break;

			case TOKEN_TYPE_I16:
				e->type = EXPRESSION_I16;
				e->data.i16 = token.i16;
				break;

			case TOKEN_TYPE_U16:
				e->type = EXPRESSION_U16;
				e->data.u16 = token.u16;
				break;

			case TOKEN_TYPE_I32:
				e->type = EXPRESSION_I32;
				e->data.i32 = token.i32;
				break;

			case TOKEN_TYPE_U32:
				e->type = EXPRESSION_U32;
				e->data.u32 = token.u32;
				break;

			case TOKEN_TYPE_F32:
				e->type = EXPRESSION_F32;
				e->data.f32 = token.f32;
				break;

			case TOKEN_TYPE_F64:
				e->type = EXPRESSION_F64;
				e->data.f64 = token.f64;
				break;

			case TOKEN_TYPE_I64:
				e->type = EXPRESSION_I64;
				e->data.i64 = token.i64;
				break;

			case TOKEN_TYPE_U64:
				e->type = EXPRESSION_U64;
				e->data.u64 = token.u64;
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
				raiseError("[Syntax] Closing parenthesis forbidden here");
				return NULL;
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





Class* Syntax_type(Parser* parser, Scope* scope, Prototype* proto, bool finishByComma) {
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

		// Finish by end operator (or equal)
		case 4:
		case 5:
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
	Class* cl = Scope_search(scope, name, NULL, SCOPESEARCH_CLASS);
	if (!cl) {
		raiseError("[UNKOWN] Class not found");
	}

	proto->cl = cl;
	proto->isPrimitive = canBePrimitive ? cl->isPrimitive : false;
	return cl;
}







void Syntax_classDeclaration(Scope* scope, Parser* parser, int flags, const Syntax_ClassDeclaration* defaultData) {
	label_t name = LABEL_NULL;
	int syntaxIndex = -1;
	bool definitionToRead;
	
	// Handle annotations
	/// TODO: complete annotations 
	Array_loop(Annotation, parser->annotations, annotation) {
		label_t annotName = annotation->name;

		raiseError("[Syntax] Illegal annotation");
		return;


		finishAnnotation:
	}


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
		/// TODO: handle if class is primitive or not 
		cl = malloc(sizeof(Class));
		Class_create(cl);
		cl->name = name;
		cl->definitionState = definitionToRead ? DEFINITIONSTATE_READING : DEFINITIONSTATE_NOT;
		cl->isPrimitive = false;
		Scope_addClass(scope, scope->type, cl);
	}


	// Read definition
	if (definitionToRead) {
		Syntax_classDefinition(scope, parser, cl);
	}


}


void Syntax_classDefinition(Scope* parentScope, Parser* parser, Class* cl) {
	cl->definitionState = DEFINITIONSTATE_READING;

	ScopeClass classScope = {
		.scope = {.parent = parentScope, .type = SCOPE_CLASS},
		.cl = cl,
		.allowThis = true
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
				
				Class* tcl = Syntax_type(parser, &classScope.scope, &variable->proto, false);
	
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
	Array arguments = {.length = 0xffff}; // no args read
	Prototype returnType = {.cl = NULL};
	
	// Handle annotations
	/// TODO: complete annotations 
	Array_loop(Annotation, parser->annotations, annotation) {
		label_t annotName = annotation->name;

		// Function is described as name function
		if (annotName == _commonLabels._main) {
			/// TODO: check main declaration/definition validity
			name = _commonLabels._main;
			goto finishAnnotation;
		}


		raiseError("[Syntax] Illegal annotation\n");
		return;


		finishAnnotation:
	}


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
			name = token.label;
			break;
		
		// Arguments
		case 2:
			arguments = Syntax_functionArguments(scope, parser);
			break;

		// Type
		case 3:
			Syntax_type(parser, scope, &returnType, false);
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
			variable->offset = -1;

			Syntax_type(parser, scope, &variable->proto, true);
			
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



void Syntax_functionScope_varDecl(ScopeFunction* scope, Parser* parser) {
	Token token = Parser_read(parser, &_labelPool);
	if (TokenCompare(SYNTAXLIST_FREE_LABEL, 0))
		return;

	Variable* variable = malloc(sizeof(Variable));
	variable->name = token.label;
	variable->offset = -1;
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
			Syntax_type(parser, &scope->scope, &variable->proto, false);
			break;

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
	
	TypeNode* tnode = ScopeFunction_pushVariable(scope, variable);
	if (variable->proto.isPrimitive) {
		/// TODO: define length using TYPENODE_LENGTH_I8
		tnode->length = TYPENODE_LENGTH_UNDEF_I8;
	} else {
		Type* type = malloc(sizeof(Type));
		Prototype_generateType(&variable->proto, type);
		tnode->value.type = type;
	}
}


void Syntax_functionScope(ScopeFunction* scope, Parser* parser) {
	while (true) {
		Token token = Parser_read(parser, &_labelPool);
		switch (TokenCompare(SYNTAXLIST_FUNCTION, 0)) {
		// let
		case 0:
		{
			Syntax_functionScope_varDecl(scope, parser);
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
			raiseError("[TODO] in Syntax_functionScope: if");
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
			raiseError("[TODO] in Syntax_functionScope: while");
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
			raiseError("[TODO] in Syntax_functionScope: free label");
			break;
		}

		// free label
		case 7:
		{
			goto finishScope;
		}
		}
	}


	finishScope:
	return;
}


bool Syntax_functionDefinition(Scope* scope, Parser* parser, Function* fn) {
	fn->definitionState = DEFINITIONSTATE_READING;


	ScopeFunction fnScope = {
		{scope, SCOPE_FUNCTION},
		fn
	};
	ScopeFunction_create(&fnScope);
	
	Syntax_functionScope(&fnScope, parser);

	ScopeFunction_delete(&fnScope);


	fn->definitionState = DEFINITIONSTATE_DONE;
}



void Syntax_annotation(Annotation* annotation, Parser* parser, LabelPool* labelPool) {
	Token token = Parser_read(parser, labelPool);

	if (TokenCompare(SYNTAXLIST_FREE_LABEL, 0))
		return;

	annotation->name = token.label;
}


#undef TokenCompare
