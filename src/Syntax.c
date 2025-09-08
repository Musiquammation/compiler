#include "Syntax.h"

#include "Scope.h"
#include "Parser.h"
#include "globalLabelPool.h"
#include "helper.h"

#include "syntaxList.c" // !do not include twice!

#include "definitionState_t.h"
#include "Module.h"
#include "Class.h"
#include "Function.h"

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




void Syntax_file(ScopeFile* scope) {
	char* const filepath = generateExtension(scope->filepath, ".th");
	Parser parser;
	Parser_open(&parser, filepath);
	free(filepath);

	while (true) {
		Token token = Parser_read(&parser, &_labelPool);
		
		switch (TokenCompare(SYNTAXLIST_FILE_DECLARATION, SYNTAXFLAG_EOF)) {
		// function
		case 0:
		{
			/// TODO: defintion required ?
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
			printf("[TODO] File order\n");
			break;
		}

		default:
			goto close;
		}
	}

	close:
	Parser_close(&parser);
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
					if (TokenCompare(SYNTAXLIST_REQUIRE_BRACE, 0) != 0) {
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


void Syntax_continueVariableDeclaration(Array* variableArray, Scope* scope, Parser* parser) {
	raiseError("[TODO]: Syntax_continueVariableDeclaration");
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
		if (flags & SYNTAXDATAFLAG_FNçDCL_REQUIRES_NAME) {
			raiseError("[Syntax] Class name required");
		} else {
			name = defaultData->defaultName;
		}
	}

	// Search class in scope
	Class* cl = Scope_search(name, scope, NULL, SCOPESEARCH_CLASS);
	/// TODO: handle double declaration
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
		*Array_push(Class*, &scope->classes) = cl;
		cl->name = name;
		cl->definitionState = definitionToRead ? DEFINITIONSTATE_READING : DEFINITIONSTATE_NOT;
	}

	// Read definition
	if (definitionToRead) {
		Syntax_classDefinition(scope, parser, cl);
		cl->definitionState = DEFINITIONSTATE_DONE;
	}


}


void Syntax_classDefinition(Scope* parentScope, Parser* parser, Class* cl) {
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
				/// TODO: should we create a scope for class ? (while reading inside)
				Syntax_continueVariableDeclaration(NULL, NULL, parser);
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
	return;
}













void Syntax_functionDeclaration(Scope* scope, Parser* parser, int flags, const Syntax_FunctionDeclaration* defaultData) {
	label_t name = LABEL_NULL;
	int syntaxIndex = -1;
	
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
			raiseError("[TODO] read args");
			break;

		// Type
		case 3:
			raiseError("[TODO] read type");
			break;
		
		// Definition
		case 4:
			if (flags & SYNTAXDATAFLAG_FN_DCL_REQUIRES_DECLARATION) {
				raiseError("[Syntax] A function DECLARATION was expected");
				break;
			}

			goto finishWhile;
		
		// End
		case 5:
			if (flags & SYNTAXDATAFLAG_FN_DCL_REQUIRES_DEFINITION) {
				raiseError("[Syntax] A function DEFINITION was expected");
				return;
			}

			goto finishWhile;

		default:
			raiseError("[Syntax] Invalid function syntax");
			return;

		}

	}
	

	finishWhile:
	
	// Check name
	if (name == LABEL_NULL) {
		if (flags & SYNTAXDATAFLAG_FNçDCL_REQUIRES_NAME) {
			raiseError("[Syntax] Function name required");
		} else {
			name = defaultData->defaultName;
		}
	}

	/// TODO: define module.mainFunction

}


void Syntax_functionArguments(Parser* parser, Function* fn) {
	/// TODO: Syntax_functionArguments
}

bool Syntax_functionDefinition(Scope* scope, Parser* parser, Function* fn) {
	/// TODO: Syntax_functionDefinition

}


void Syntax_annotation(Annotation* annotation, Parser* parser, LabelPool* labelPool) {
	Token token = Parser_read(parser, labelPool);

	if (TokenCompare(SYNTAX_FREE_LABEL, 0) != 0)
		return;

	annotation->name = token.label;
}


#undef TokenCompare