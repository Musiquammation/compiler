#ifndef COMPILER_PARSER_H_
#define COMPILER_PARSER_H_

#include "label_t.h"
#include "declarations.h"

#include <stdio.h>
#include <tools/Array.h>

#include "castable_t.h"

typedef char token_t;
typedef char operator_t;


enum {PARSER_LINE_BUFFER_LENGTH = 256};

typedef struct {
	token_t type;

	union {
		castable_t num;

		operator_t operator;
		label_t label;
		char* string;
	};
} Token;

struct Annotation {
	int type;

	union {
		label_t meta;
	};
};


struct Parser {
	FILE* file;
	char buffer[PARSER_LINE_BUFFER_LENGTH];
	Array annotations; // type: Annotation

	bool alive;
	char current;
	bool hasSavedToken;
	
	int cursor;
	int lineLength;

	int scopeLevel;

	int file_line;
	int file_column;

	Token token;
};



enum {
	TOKEN_TYPE_I8,
	TOKEN_TYPE_U8,
	TOKEN_TYPE_I16,
	TOKEN_TYPE_U16,
	TOKEN_TYPE_I32,
	TOKEN_TYPE_U32,
	TOKEN_TYPE_F32,
	TOKEN_TYPE_F64,
	TOKEN_TYPE_I64,
	TOKEN_TYPE_U64,
	
	TOKEN_TYPE_OPERATOR,
	TOKEN_TYPE_LABEL,
	TOKEN_TYPE_STRING,

	TOKEN_TYPE_NONE,
	TOKEN_TYPE_EOF,
	TOKEN_TYPE_ERROR,


	TOKEN_CTYPE_OPERATOR,
	TOKEN_CTYPE_KEYWORD,
	TOKEN_CTYPE_LABEL,
	TOKEN_CTYPE_INTEGER,
	TOKEN_CTYPE_NUMBER,
};



/**
 * Indexes of TOKEN_OPERATORS are generated here
 * https://chatgpt.com/c/68b6f263-c698-8322-a557-8b1bba6b48f7
 */

enum {
	TOKEN_OPERATOR_PLUS = 26,           // +
	TOKEN_OPERATOR_MINUS = 27,          // -
	TOKEN_OPERATOR_MUL = 28,            // *
	TOKEN_OPERATOR_DIV = 29,            // /
	TOKEN_OPERATOR_MOD = 30,            // %

	TOKEN_OPERATOR_INC = 2,             // ++
	TOKEN_OPERATOR_DEC = 3,             // --

	TOKEN_OPERATOR_EQ = 4,              // ==
	TOKEN_OPERATOR_NEQ = 5,             // !=
	TOKEN_OPERATOR_LT = 31,             // <
	TOKEN_OPERATOR_GT = 32,             // >
	TOKEN_OPERATOR_LE = 6,              // <=
	TOKEN_OPERATOR_GE = 7,              // >=

	TOKEN_OPERATOR_LOGICAL_NOT = 33,    // !
	TOKEN_OPERATOR_LOGICAL_AND = 8,     // &&
	TOKEN_OPERATOR_LOGICAL_OR = 9,      // ||

	TOKEN_OPERATOR_BIT_NOT = 34,        // ~
	TOKEN_OPERATOR_BIT_AND = 35,        // &
	TOKEN_OPERATOR_BIT_OR = 36,         // |
	TOKEN_OPERATOR_BIT_XOR = 37,        // ^
	TOKEN_OPERATOR_SHL = 10,            // <<
	TOKEN_OPERATOR_SHR = 11,            // >>

	TOKEN_OPERATOR_ASSIGN = 38,         // =

	TOKEN_OPERATOR_ADD_ASSIGN = 12,     // +=
	TOKEN_OPERATOR_SUB_ASSIGN = 13,     // -=
	TOKEN_OPERATOR_MUL_ASSIGN = 14,     // *=
	TOKEN_OPERATOR_DIV_ASSIGN = 15,     // /=
	TOKEN_OPERATOR_MOD_ASSIGN = 16,     // %=
	TOKEN_OPERATOR_SHL_ASSIGN = 0,      // <<=
	TOKEN_OPERATOR_SHR_ASSIGN = 1,      // >>=
	TOKEN_OPERATOR_AND_ASSIGN = 17,     // &=
	TOKEN_OPERATOR_XOR_ASSIGN = 18,     // ^=
	TOKEN_OPERATOR_OR_ASSIGN = 19,      // |=

	TOKEN_OPERATOR_TERNARY = 39,        // ?
	TOKEN_OPERATOR_COLON = 40,          // :
	TOKEN_OPERATOR_COMMA = 41,          // ,
	TOKEN_OPERATOR_MEMBER = 42,         // .
	TOKEN_OPERATOR_SCOPE = 20,          // ::
	TOKEN_OPERATOR_END = 25,            // ;

	TOKEN_OPERATOR_AT = 43,             // @
	TOKEN_OPERATOR_ORDER = 24,          // @!
	TOKEN_OPERATOR_HASH = 44,           // #

	TOKEN_OPERATOR_SINGLE_QUOTE = 45,   // '
	TOKEN_OPERATOR_DOUBLE_QUOTE = 46,   // "
	TOKEN_OPERATOR_BACKTICK = 47,       // `

	TOKEN_OPERATOR_LPAREN = 48,         // (
	TOKEN_OPERATOR_RPAREN = 49,         // )
	TOKEN_OPERATOR_LBRACKET = 50,       // [
	TOKEN_OPERATOR_RBRACKET = 51,       // ]
	TOKEN_OPERATOR_LBRACE = 52,         // {
	TOKEN_OPERATOR_RBRACE = 53,         // }
	TOKEN_OPERATOR_LANGLE = 54,         // <
	TOKEN_OPERATOR_RANGLE = 55,         // >

	TOKEN_OPERATOR_COMMENT_LINE = 21,   // //
	TOKEN_OPERATOR_COMMENT_START = 22,  // /*
	TOKEN_OPERATOR_COMMENT_END = 23     // */
};




enum {
	TOKEN_OPERATORMAXLENGTH = 3,
	TOKEN_OPERATORINDEX_3 = 0,
	TOKEN_OPERATORINDEX_2 = 2,
	TOKEN_OPERATORINDEX_1 = 25,
};

enum {
	ANNOTATION_META,
	ANNOTATION_MAIN,
};


extern const char PARSER_OPERATOR_CHARS[];
extern const char PARSER_OPERATORS[][TOKEN_OPERATORMAXLENGTH];

void Parser_open(Parser* parser, const char* filepath);
void Parser_close(Parser* parser);

Token Parser_read(Parser* parser, LabelPool* labelPool);
Token Parser_readAnnotated(Parser* parser, LabelPool* labelPool);

void Parser_saveToken(Parser* parser, const Token* token);

void Token_println(const Token* token);

int Token_compare(Token token, const Token comparators[], int length, char raise);


#endif