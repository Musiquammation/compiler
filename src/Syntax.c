#include "Syntax.h"

#include "Scope.h"
#include "Parser.h"
#include "globalLabelPool.h"

#include "syntaxList.c" // !do not include twice!

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
			const Syntax_FunctionDeclaration defaultData = {
				.initialized = true,
				.defaultName = scope->name
			};

			Function* fn = Syntax_functionDeclaration(&scope->scope, &parser, &defaultData);

			break;
		}

		// class
		case 1:
		{
			printf("TODO: class\n");
			break;
		}

		// order
		case 2:
		{
			Syntax_order(&scope->scope, &parser);
			break;
		}

		default:
			goto breakWhileLoop;
		}
	}

	breakWhileLoop:
	Parser_close(&parser);
}



void Syntax_order(Scope* scope, Parser* parser) {
	Token token;
	token = Parser_read(parser, &_labelPool);

	switch (TokenCompare(SYNTAXLIST_ORDER, 0)) {
		// module
		case 0:
			token = Parser_read(parser, &_labelPool);

			if (TokenCompare(SYNTAXLIST_REQUIRE_BRACE, 0) != 0)
				return;
			
			Syntax_declarationList(scope, parser);
			return;

		default:
			return;
	}
	
}

void Syntax_declarationList(Scope* scope, Parser* parser) {
	
}


Function* Syntax_functionDeclaration(Scope* scope, Parser* parser, const Syntax_FunctionDeclaration* defaultData) {
	printf("fn\n");
}



#undef TokenCompare