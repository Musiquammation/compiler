#ifndef COMPILER_PARSER_H_
#define COMPILER_PARSER_H_

#include "label_t.h"
#include "declarations.h"

#include <stdio.h>

typedef char token_t;
typedef char operator_t;


typedef struct {
	token_t type;

	union {
		char i8;
		unsigned char u8;

		short i16;
		unsigned u16;

		int i32;
		unsigned u32;

		float f32;

		operator_t operator;
		label_t label;
		char* string;
	};

} Token;

struct Parser {
	FILE* file;
	char carry;
	
	int position;
	int line;
	int column;
};



enum {
	TOKEN_TYPE_I8,
	TOKEN_TYPE_U8,
	TOKEN_TYPE_I16,
	TOKEN_TYPE_U16,
	TOKEN_TYPE_I32,
	TOKEN_TYPE_U32,
	TOKEN_TYPE_F32,
	
	TOKEN_TYPE_OPERATOR,
	TOKEN_TYPE_LABEL,
	TOKEN_TYPE_STRING,
};

enum {
	TOKEN_OPERATOR_PLUS,        // +
	TOKEN_OPERATOR_MINUS,       // -
	TOKEN_OPERATOR_MUL,         // *
	TOKEN_OPERATOR_DIV,         // /
	TOKEN_OPERATOR_MOD,         // %

	TOKEN_OPERATOR_INC,         // ++
	TOKEN_OPERATOR_DEC,         // --

	TOKEN_OPERATOR_EQ,          // ==
	TOKEN_OPERATOR_NEQ,         // !=
	TOKEN_OPERATOR_LT,          // <
	TOKEN_OPERATOR_GT,          // >
	TOKEN_OPERATOR_LE,          // <=
	TOKEN_OPERATOR_GE,          // >=

	TOKEN_OPERATOR_LOGICAL_NOT, // !
	TOKEN_OPERATOR_LOGICAL_AND, // &&
	TOKEN_OPERATOR_LOGICAL_OR,  // ||

	TOKEN_OPERATOR_BIT_NOT,     // ~
	TOKEN_OPERATOR_BIT_AND,     // &
	TOKEN_OPERATOR_BIT_OR,      // |
	TOKEN_OPERATOR_BIT_XOR,     // ^
	TOKEN_OPERATOR_SHL,         // <<
	TOKEN_OPERATOR_SHR,         // >>

	TOKEN_OPERATOR_ASSIGN,      // =
	
	TOKEN_OPERATOR_ADD_ASSIGN,  // +=
	TOKEN_OPERATOR_SUB_ASSIGN,  // -=
	TOKEN_OPERATOR_MUL_ASSIGN,  // *=
	TOKEN_OPERATOR_DIV_ASSIGN,  // /=
	TOKEN_OPERATOR_MOD_ASSIGN,  // %=
	TOKEN_OPERATOR_SHL_ASSIGN,  // <<=
	TOKEN_OPERATOR_SHR_ASSIGN,  // >>=
	TOKEN_OPERATOR_AND_ASSIGN,  // &=
	TOKEN_OPERATOR_XOR_ASSIGN,  // ^=
	TOKEN_OPERATOR_OR_ASSIGN,   // |=

	TOKEN_OPERATOR_TERNARY,     // ?
	TOKEN_OPERATOR_COLON,       // :
	TOKEN_OPERATOR_COMMA,       // ,
	TOKEN_OPERATOR_MEMBER,      // .
	TOKEN_OPERATOR_PTR_MEMBER,  // ->
};



void Parser_open(Parser* parser, char* filename);
void Parser_close(Parser* parser);

Token Parser_read(Parser* parser);



#endif