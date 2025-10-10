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



Token SYNTAXLIST_FILE[] = {
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

Token SYNTAXLIST_FREE_LABEL[] = {
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

Token SYNTAXLIST_TYPE[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_LANGLE},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_LBRACKET},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_COMMA},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_RPAREN},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_END},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_ASSIGN},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_LBRACE},
};

Token SYNTAXLIST_ARGS[] = {
	{.type = TOKEN_CTYPE_LABEL},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_RPAREN}
};

Token SYNTAXLIST_SINGLETON_COLON[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_COLON},
};

Token SYNTAXLIST_SINGLETON_END[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_END},
};

Token SYNTAXLIST_SINGLETON_ASSIGN[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_ASSIGN},
};

Token SYNTAXLIST_SINGLETON_LPAREN[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_LPAREN},
};

Token SYNTAXLIST_SINGLETON_RPAREN[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_RPAREN},
};

Token SYNTAXLIST_SINGLETON_LBRACE[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_LBRACE},
};

Token SYNTAXLIST_SINGLETON_RBRACE[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_RBRACE},
};

Token SYNTAXLIST_EXPRESSION_SEPARATOR[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_COMMA},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_RPAREN},
};

Token SYNTAXLIST_CONTINUE_FNCALL[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_MEMBER},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_LPAREN},
};



Token SYNTAXLIST_FUNCTION[] = {
	{.type = TOKEN_CTYPE_KEYWORD, .string = k_let},
	{.type = TOKEN_CTYPE_KEYWORD, .string = k_const},
	{.type = TOKEN_CTYPE_KEYWORD, .string = k_if},
	{.type = TOKEN_CTYPE_KEYWORD, .string = k_for},
	{.type = TOKEN_CTYPE_KEYWORD, .string = k_while},
	{.type = TOKEN_CTYPE_KEYWORD, .string = k_switch},
	{.type = TOKEN_CTYPE_LABEL},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_RBRACE},
};

Token SYNTAXLIST_FUNCTION_VARDECL[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_COLON},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_ASSIGN},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_END},
};

Token SYNTAXLIST_PROPERTY[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_MEMBER},
};

Token SYNTAXLIST_PATH[] = {
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_MEMBER},
	{.type = TOKEN_CTYPE_OPERATOR, .operator = TOKEN_OPERATOR_LPAREN},
};



#define op(x) {.type = TOKEN_CTYPE_OPERATOR, .operator = x}

Token SYNTAXLIST_EXPRESSION[] = {
	{.type = TOKEN_CTYPE_LABEL},
	{.type = TOKEN_CTYPE_NUMBER},

	op(TOKEN_OPERATOR_MEMBER),

	op(TOKEN_OPERATOR_LPAREN),
	op(TOKEN_OPERATOR_RPAREN),
	op(TOKEN_OPERATOR_COMMA),
	op(TOKEN_OPERATOR_END),

	
	op(TOKEN_OPERATOR_PLUS),
	op(TOKEN_OPERATOR_MINUS),
	op(TOKEN_OPERATOR_MUL),
	op(TOKEN_OPERATOR_DIV),
	op(TOKEN_OPERATOR_MOD),

	op(TOKEN_OPERATOR_BIT_AND),
	op(TOKEN_OPERATOR_BIT_OR),
	op(TOKEN_OPERATOR_BIT_XOR),
	op(TOKEN_OPERATOR_SHL),
	op(TOKEN_OPERATOR_SHR),

	op(TOKEN_OPERATOR_LOGICAL_AND),
	op(TOKEN_OPERATOR_LOGICAL_OR),

	op(TOKEN_OPERATOR_EQ),
	op(TOKEN_OPERATOR_NEQ),
	op(TOKEN_OPERATOR_LT),
	op(TOKEN_OPERATOR_LE),
	op(TOKEN_OPERATOR_GT),
	op(TOKEN_OPERATOR_GE),
	
	op(TOKEN_OPERATOR_BIT_NOT),
	op(TOKEN_OPERATOR_LOGICAL_NOT),
	op(TOKEN_OPERATOR_INC),
	op(TOKEN_OPERATOR_DEC),



	op(TOKEN_OPERATOR_ASSIGN),


	op(TOKEN_OPERATOR_ADD_ASSIGN),
	op(TOKEN_OPERATOR_SUB_ASSIGN),
	op(TOKEN_OPERATOR_MUL_ASSIGN),
	op(TOKEN_OPERATOR_DIV_ASSIGN),
	op(TOKEN_OPERATOR_MOD_ASSIGN),
	op(TOKEN_OPERATOR_SHL_ASSIGN),
	op(TOKEN_OPERATOR_SHR_ASSIGN),
	op(TOKEN_OPERATOR_AND_ASSIGN),
	op(TOKEN_OPERATOR_XOR_ASSIGN),
	op(TOKEN_OPERATOR_OR_ASSIGN),
};

#undef op




static void initSyntaxList(Token tokens[], int length) {
	Array_for(Token, tokens, length, token) {
		if (token->type == TOKEN_CTYPE_KEYWORD) {
			token->label = LabelPool_push(&_labelPool, token->string);
		}
	}
}


#define init(list) initSyntaxList(list, sizeof(list)/sizeof(Token))
void syntaxList_init(void) {
	init(SYNTAXLIST_FILE);
	init(SYNTAXLIST_ORDER);
	init(SYNTAXLIST_REQUIRE_BRACE);
	init(SYNTAXLIST_DECLARATION_LIST);
	init(SYNTAXLIST_FREE_LABEL);
	init(SYNTAXLIST_FUNCTION_DECLARATION);
	init(SYNTAXLIST_CLASS_DECLARATION);
	init(SYNTAXLIST_CLASS_DEFINITION);
	init(SYNTAXLIST_MODULE_FILE);
	init(SYNTAXLIST_MODULE_MODULE);
	init(SYNTAXLIST_CLASS_ABSTRACT_MEMBER);
	init(SYNTAXLIST_TYPE);
	init(SYNTAXLIST_SINGLETON_COLON);
	init(SYNTAXLIST_SINGLETON_END);
	init(SYNTAXLIST_SINGLETON_ASSIGN);
	init(SYNTAXLIST_SINGLETON_LPAREN);
	init(SYNTAXLIST_SINGLETON_RPAREN);
	init(SYNTAXLIST_SINGLETON_LBRACE);
	init(SYNTAXLIST_SINGLETON_RBRACE);
	init(SYNTAXLIST_FUNCTION);
	init(SYNTAXLIST_FUNCTION_VARDECL);
	init(SYNTAXLIST_EXPRESSION);
	init(SYNTAXLIST_PROPERTY);
	init(SYNTAXLIST_PATH);
	init(SYNTAXLIST_EXPRESSION_SEPARATOR);
	init(SYNTAXLIST_CONTINUE_FNCALL);
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


