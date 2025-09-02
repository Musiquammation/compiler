#include "syntaxList.h"

#include "globalLabelPool.h"

#define k_let "let"
#define k_const "const"
#define k_function "function"
#define k_class "class"
#define k_module "module"
#define k_main "main"



Token SYNTAXLIST_FILE_DECLARATION[] = {
	{.type = TOKEN_CTYPE_LABEL, .string = k_function},
	{.type = TOKEN_CTYPE_LABEL, .string = k_class},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_ORDER},
};

Token SYNTAXLIST_ORDER[] = {
	{.type = TOKEN_CTYPE_LABEL, .string = k_module},
};

Token SYNTAXLIST_REQUIRE_BRACE[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_LBRACE}
};




static void initSyntaxList(Token tokens[], int length) {
	Array_for(Token, tokens, length, token) {
		if (token->type == TOKEN_CTYPE_LABEL) {
			token->label = LabelPool_push(&_labelPool, token->string);
		}
	}
}

#define init(list) initSyntaxList(list, sizeof(list)/sizeof(Token))
void syntaxList_init() {
	init(SYNTAXLIST_FILE_DECLARATION);
	init(SYNTAXLIST_ORDER);
	init(SYNTAXLIST_REQUIRE_BRACE);
}



#undef k_let
#undef k_const
#undef k_function
#undef k_class
#undef k_module
#undef k_main



#undef init
