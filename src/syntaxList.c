#include "syntaxList.h"

#include "globalLabelPool.h"

#define k_let "let"
#define k_const "const"
#define k_class "class"
#define k_function "function"
#define k_if "if"
#define k_else "else"
#define k_for "for"
#define k_while "while"
#define k_switch "switch"
#define k_module "module"
#define k_main "main"



Token SYNTAXLIST_FILE_DECLARATION[] = {
	{.type = TOKEN_CTYPE_KEYWORD, .string = k_function},
	{.type = TOKEN_CTYPE_KEYWORD, .string = k_class},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_ORDER},
};

Token SYNTAXLIST_ORDER[] = {
	{.type = TOKEN_CTYPE_KEYWORD, .string = k_module},
};

Token SYNTAXLIST_REQUIRE_BRACE[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_LBRACE}
};

Token SYNTAXLIST_DECLARATION_LIST[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_RBRACE},
	{.type = TOKEN_CTYPE_KEYWORD, .string = k_function},
	{.type = TOKEN_CTYPE_KEYWORD, .string = k_class},
};

Token SYNTAX_FREE_LABEL[] = {
	{.type = TOKEN_CTYPE_LABEL}
};

Token SYNTAXLIST_FUNCTION_DECLARATION[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_LANGLE},
	{.type = TOKEN_CTYPE_LABEL},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_LPAREN},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_COLON},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_LBRACE},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_END},
};

Token SYNTAXLIST_CLASS_DECLARATION[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_LANGLE},
	{.type = TOKEN_CTYPE_LABEL},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_LBRACE},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_END},	
};

Token SYNTAXLIST_CLASS_DEFINITION[] = {
	{.type = TOKEN_CTYPE_KEYWORD, .string = k_function},
	{.type = TOKEN_CTYPE_KEYWORD, .string = k_class},
	{.type = TOKEN_CTYPE_LABEL},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_ORDER},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_RBRACE},
};

Token SYNTAXLIST_MODULE_FILE[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_ORDER},
};

Token SYNTAXLIST_MODULE_MODULE[] = {
	{.type = TOKEN_CTYPE_KEYWORD, .string = k_module},
};

Token SYNTAXLIST_CLASS_ABSTRACT_MEMBER[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_COLON},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_LPAREN},
};



static void initSyntaxList(Token tokens[], int length) {
	Array_for(Token, tokens, length, token) {
		if (token->type == TOKEN_CTYPE_KEYWORD) {
			token->label = LabelPool_push(&_labelPool, token->string);
		}
	}
}

#define init(list) initSyntaxList(list, sizeof(list)/sizeof(Token))
void syntaxList_init() {
	init(SYNTAXLIST_FILE_DECLARATION);
	init(SYNTAXLIST_ORDER);
	init(SYNTAXLIST_REQUIRE_BRACE);
	init(SYNTAXLIST_DECLARATION_LIST);
	init(SYNTAX_FREE_LABEL);
	init(SYNTAXLIST_FUNCTION_DECLARATION);
	init(SYNTAXLIST_CLASS_DECLARATION);
	init(SYNTAXLIST_CLASS_DEFINITION);
	init(SYNTAXLIST_MODULE_FILE);
	init(SYNTAXLIST_MODULE_MODULE);
}



#undef init

#undef k_let
#undef k_const
#undef k_class
#undef k_function
#undef k_if
#undef k_else
#undef k_for
#undef k_while
#undef k_switch
#undef k_module
#undef k_main


