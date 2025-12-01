#include "Parser.h"

#include "LabelPool.h"
#include "globalLabelPool.h"

#include "Syntax.h"

#include "syntaxList.h"
#include "helper.h"

#include <string.h>

const char PARSER_OPERATOR_CHARS[] = ".=;()+-,&|^!~()[]{}<>?:@#*/%";
const char PARSER_OPERATORS[][TOKEN_OPERATORMAXLENGTH] = {
	/* Length 3 */
	"<<=",  // 0 TOKEN_OPERATOR_SHL_ASSIGN
	">>=",  // 1 TOKEN_OPERATOR_SHR_ASSIGN

	/* Length 2 */
	"++",   // 2 TOKEN_OPERATOR_INC
	"--",   // 3 TOKEN_OPERATOR_DEC
	"==",   // 4 TOKEN_OPERATOR_EQ
	"!=",   // 5 TOKEN_OPERATOR_NEQ
	"<=",   // 6 TOKEN_OPERATOR_LE
	">=",   // 7 TOKEN_OPERATOR_GE
	"&&",   // 8 TOKEN_OPERATOR_LOGICAL_AND
	"||",   // 9 TOKEN_OPERATOR_LOGICAL_OR
	"<<",   // 10 TOKEN_OPERATOR_SHL
	">>",   // 11 TOKEN_OPERATOR_SHR
	"+=",   // 12 TOKEN_OPERATOR_ADD_ASSIGN
	"-=",   // 13 TOKEN_OPERATOR_SUB_ASSIGN
	"*=",   // 14 TOKEN_OPERATOR_MUL_ASSIGN
	"/=",   // 15 TOKEN_OPERATOR_DIV_ASSIGN
	"%=",   // 16 TOKEN_OPERATOR_MOD_ASSIGN
	"&=",   // 17 TOKEN_OPERATOR_AND_ASSIGN
	"^=",   // 18 TOKEN_OPERATOR_XOR_ASSIGN
	"|=",   // 19 TOKEN_OPERATOR_OR_ASSIGN
	"::",   // 20 TOKEN_OPERATOR_SCOPE
	"//",   // 21 TOKEN_OPERATOR_COMMENT_LINE
	"/*",   // 22 TOKEN_OPERATOR_COMMENT_START
	"*/",   // 23 TOKEN_OPERATOR_COMMENT_END
	"@!",   // 23 TOKEN_OPERATOR_COMMENT_END

	/* Length 1 */
	";",    // 25 TOKEN_OPERATOR_END
	"+",    // 26 TOKEN_OPERATOR_PLUS
	"-",    // 27 TOKEN_OPERATOR_MINUS
	"*",    // 28 TOKEN_OPERATOR_MUL
	"/",    // 29 TOKEN_OPERATOR_DIV
	"%",    // 30 TOKEN_OPERATOR_MOD
	"<",    // 31 TOKEN_OPERATOR_LANGLE
	">",    // 32 TOKEN_OPERATOR_RANGLE
	"!",    // 33 TOKEN_OPERATOR_LOGICAL_NOT
	"~",    // 34 TOKEN_OPERATOR_BIT_NOT
	"&",    // 35 TOKEN_OPERATOR_BIT_AND
	"|",    // 36 TOKEN_OPERATOR_BIT_OR
	"^",    // 37 TOKEN_OPERATOR_BIT_XOR
	"=",    // 38 TOKEN_OPERATOR_ASSIGN
	"?",    // 39 TOKEN_OPERATOR_TERNARY
	":",    // 40 TOKEN_OPERATOR_COLON
	",",    // 41 TOKEN_OPERATOR_COMMA
	".",    // 42 TOKEN_OPERATOR_MEMBER
	"@",    // 43 TOKEN_OPERATOR_AT
	"#",    // 44 TOKEN_OPERATOR_HASH
	"'",    // 45 TOKEN_OPERATOR_SINGLE_QUOTE
	"\"",   // 46 TOKEN_OPERATOR_DOUBLE_QUOTE
	"`",    // 47 TOKEN_OPERATOR_BACKTICK
	"(",    // 48 TOKEN_OPERATOR_LPAREN
	")",    // 49 TOKEN_OPERATOR_RPAREN
	"[",    // 50 TOKEN_OPERATOR_LBRACKET
	"]",    // 51 TOKEN_OPERATOR_RBRACKET
	"{",    // 52 TOKEN_OPERATOR_LBRACE
	"}",    // 53 TOKEN_OPERATOR_RBRACE

};







static char moveCursor(int cursor, int lineLength, Parser* parser);

static char loadNextLine(Parser* parser) {
	while (fgets(parser->buffer, PARSER_LINE_BUFFER_LENGTH, parser->file)) {
		char c = parser->buffer[0];

		// Get line length
		char* ptr = &parser->buffer[1];
		while (*ptr != '\n' && *ptr != 0)
			ptr++;
		
		int length = ptr - parser->buffer;
		parser->lineLength = length;
		return moveCursor(0, length, parser);
	}


	parser->current = -1;
	return feof(parser->file) ? -1 : -2;
}





static char moveCursor(int cursor, int lineLength, Parser* parser) {
	while (cursor < lineLength) {
		char c = parser->buffer[cursor];
		if (c == ' ' || c == '\t') {
			cursor++;
			parser->file_column++;
			continue;
		}

		if (c == '\n') {
			parser->file_line++;
			parser->file_column = 0;
			break;
		}

		parser->current = c;
		parser->cursor = cursor;
		return c;
	}

	loadNextLine(parser);
}



static Token handleNegativeChar(Parser* parser, char c) {
	if (c == -2) {
		raiseError("[Parser] Error from parser");
		return (Token){.type = TOKEN_TYPE_ERROR};
	}
	
	if (c == -1) {
		if (parser->scopeLevel != 0) {
			raiseError("[Parser] Scope level error");
			return (Token){.type = TOKEN_TYPE_ERROR};
		}
		
		return (Token){.type = TOKEN_TYPE_EOF};
	}
	
	raiseError("[Parser] Invalid char");
	return (Token){.type = TOKEN_TYPE_ERROR};
}

static bool isOperatorChar(char c) {
	const char *p = PARSER_OPERATOR_CHARS;
	while (*p) {
		if (*p == c)
			return true;
		
		p++;
	}

	return false;
}

static operator_t getOperator(const char* str, int length) {
	if (length >= 3) {
		for (operator_t i = TOKEN_OPERATORINDEX_3; i < TOKEN_OPERATORINDEX_2; ++i) {
			if (str[0] == PARSER_OPERATORS[i][0] &&
				str[1] == PARSER_OPERATORS[i][1] &&
				str[2] == PARSER_OPERATORS[i][2]) {
				return i;
			}
		}
		
		goto l2;
	}

	if (length == 2) {
		l2:
		for (operator_t i = TOKEN_OPERATORINDEX_2; i < TOKEN_OPERATORINDEX_1; ++i) {
			if (str[0] == PARSER_OPERATORS[i][0] &&
				str[1] == PARSER_OPERATORS[i][1]) {
				return i;
			}
		}
		
		goto l1;
	}

	if (length == 1) {
		l1:
		for (operator_t i = TOKEN_OPERATORINDEX_1; i < (operator_t)(sizeof(PARSER_OPERATORS)/sizeof(PARSER_OPERATORS[0])); ++i) {
			if (str[0] == PARSER_OPERATORS[i][0]) {
				return i;
			}
		}
	}

	return -1;
}

static int getOperatorLength(operator_t operator) {    
	if (operator < TOKEN_OPERATORINDEX_3)
		return 0;

	if (operator < TOKEN_OPERATORINDEX_2)
		return 3;

	if (operator < TOKEN_OPERATORINDEX_1)
		return 2;

	return 1;
}




static int collectNumber(const char* str, Token* token) {
	const char* start = str;
	const char* end = str;
	int base = 10;
	int is_float = 0;

	// Detect base prefix
	if (str[0] == '0') {
		if (str[1] == 'b' || str[1] == 'B') {
			base = 2;
			end += 2;
		} else if (str[1] == 'x' || str[1] == 'X') {
			base = 16;
			end += 2;
		}
	}

	// Scan digits
	while (*end) {
		char ch = *end;
		int valid = 0;

		if (base == 2) valid = (ch == '0' || ch == '1');
		else if (base == 10) valid = (ch >= '0' && ch <= '9');
		else if (base == 16) valid = ((ch >= '0' && ch <= '9') ||
									  (ch >= 'a' && ch <= 'f') ||
									  (ch >= 'A' && ch <= 'F'));

		if (base == 10 && ch == '.') {
			valid = 1;
			is_float = 1;
		}

		if (!valid) break;
		end++;
	}

	// Check valid suffix
	char suffix = *end;
	if (suffix == 'c' || suffix == 'C' || suffix == 's' || suffix == 'S' ||
		suffix == 'u' || suffix == 'l' || suffix == 'L' || suffix == 'd' || suffix == 'f') {
		end++; // include suffix
	} else {
		suffix = 0; // no valid suffix
	}

	// Convert to value
	if (is_float) {
		double val = strtod(start, NULL);
		switch (suffix) {
			case 'f': token->type = TOKEN_TYPE_F32; token->num.f32 = (float)val; break;
			case 'd': token->type = TOKEN_TYPE_F64; token->num.f64 = val; break;
			default:  token->type = TOKEN_TYPE_F32; token->num.f32 = (float)val; break;
		}
	} else {
		unsigned long val = strtoul(start, NULL, base);
		switch (suffix) {
			case 'c': token->type = TOKEN_TYPE_I8; token->num.i8 = (char)val; break;
			case 'C': token->type = TOKEN_TYPE_U8; token->num.u8 = (unsigned char)val; break;
			case 's': token->type = TOKEN_TYPE_I16; token->num.i16 = (short)val; break;
			case 'S': token->type = TOKEN_TYPE_U16; token->num.u16 = (unsigned short)val; break;
			case 'u': token->type = TOKEN_TYPE_U32; token->num.u32 = (unsigned int)val; break;
			case 'l': token->type = TOKEN_TYPE_I64; token->num.i64 = (long)val; break;
			case 'L': token->type = TOKEN_TYPE_U64; token->num.u64 = (unsigned long)val; break;
			case 'd': token->type = TOKEN_TYPE_I32; token->num.i32 = (int)val; break;
			default:  token->type = TOKEN_TYPE_I32; token->num.i32 = (int)val; break;
		}
	}

	return (int)(end - start);
}










void Parser_open(Parser* parser, const char* filepath) {
	parser->file_line = 0;
	parser->file_column = 0;
	parser->cursor = 0;
	parser->current = 0;
	parser->hasSavedToken = false;

	Array_create(&parser->annotations, sizeof(Annotation));
	
	FILE* file = fopen(filepath, "r");
	if (!file) {
		printf("%s\n", filepath);
		parser->file = NULL;
		parser->alive = false;
		raiseError("[Parser] Cannot open file");
		return;
	}

	parser->file = file;
	parser->scopeLevel = 0;

	loadNextLine(parser);
}

void Parser_close(Parser* parser) {
	if (parser->file)
		fclose(parser->file);

	Array_free(parser->annotations);
}




Token Parser_read(Parser* parser, LabelPool* labelPool) {
	if (parser->hasSavedToken) {
		parser->hasSavedToken = false;
		return parser->token;
	}


	char firstChar = parser->current;
	
	#define move(n) {parser->file_column += n; moveCursor(cursor + n, lineLength, parser);}


	if (firstChar < 0) {
		return handleNegativeChar(parser, firstChar);
	}

	int cursor = parser->cursor;
	int lineLength = parser->lineLength;

	// Number
	if (firstChar >= '0' && firstChar <= '9') {
		Token token;
		int length = collectNumber(&parser->buffer[cursor], &token);
		move(length);
		return token;
	}

	// Operator
	if (isOperatorChar(firstChar)) {
		operator_t operator = getOperator(&parser->buffer[cursor], lineLength - cursor);
		if (operator < 0) {
			raiseError("[Parser] Operator not found");
			return (Token){.type = TOKEN_TYPE_ERROR};
		}

		/// TODO: #1 handle strings and comments
		/// TODO: #2 scopeLevel (with {}, <>, (), [])
		int length = getOperatorLength(operator);
		move(length);
		return (Token){
			.type = TOKEN_TYPE_OPERATOR,
			.operator = operator
		};
	}

	// Label
	{
		/// TODO: when label is beetween to buffers, label is shrink

		// Collect length
		char* start = &parser->buffer[cursor];
		char* ptr = start;
		while (*ptr != ' ' && *ptr != '\n' && !isOperatorChar(*ptr)) {
			ptr++;
		}
	
		int length = ptr - start;
		if (length == 0) {
			return (Token){.type = TOKEN_TYPE_ERROR};
		}
		
		// Push text
		char* copy = malloc(length+1);
		memcpy(copy, start, length);
		copy[length] = 0;
		const label_t label = LabelPool_push(labelPool, copy);
		free(copy);

		// Move cursor
		move(length);


		return (Token){
			.type = TOKEN_TYPE_LABEL,
			.label = label
		};
	}	

	return (Token){.type = TOKEN_TYPE_NONE};

	#undef move
}


Token Parser_readAnnotated(Parser* parser, LabelPool* labelPool) {
	parser->annotations.length = 0;

	Token token = Parser_read(parser, labelPool);
	while (token.type == TOKEN_TYPE_OPERATOR && token.operator == TOKEN_OPERATOR_AT) {
		Syntax_annotation(Array_push(Annotation, &parser->annotations), parser, labelPool);
		token = Parser_read(parser, labelPool);
	}

	return token;
}



void Parser_saveToken(Parser* parser, const Token* token) {
	if (parser->hasSavedToken) {
		raiseError("[Parser] A token is already saved");
	}
	parser->hasSavedToken = true;
	parser->token = *token;
}


void Token_println(const Token* token) {
	switch(token->type) {
		// Signed integers
		case TOKEN_TYPE_I8:  printf("number<i8>(%d)\n", token->num.i8); break;
		case TOKEN_TYPE_I16: printf("number<i16>(%d)\n", token->num.i16); break;
		case TOKEN_TYPE_I32: printf("number<i32>(%d)\n", token->num.i32); break;
		case TOKEN_TYPE_I64: printf("number<i64>(%ld\n)", token->num.i64); break;

		// Unsigned integers
		case TOKEN_TYPE_U8:  printf("number<u8>(%u)\n", token->num.u8); break;
		case TOKEN_TYPE_U16: printf("number<u16>(%u)\n", token->num.u16); break;
		case TOKEN_TYPE_U32: printf("number<u32>(%u)\n", token->num.u32); break;
		case TOKEN_TYPE_U64: printf("number<u64>(%lu\n)", token->num.u64); break;

		// Floating-point numbers
		case TOKEN_TYPE_F32: printf("number<f32>(%f)\n", token->num.f32); break;
		case TOKEN_TYPE_F64: printf("number<f64>(%lf\n)", token->num.f64); break;

		// Operators
		case TOKEN_TYPE_OPERATOR:
			if(token->operator >= 0 && token->operator < (operator_t)(sizeof(PARSER_OPERATORS)/TOKEN_OPERATORMAXLENGTH)) {
				printf("operator");
				printf("%.*s\n", TOKEN_OPERATORMAXLENGTH, PARSER_OPERATORS[token->operator]);
			} else {
				printf("operator <invalid>\n");
			}
			break;

		// Labels or strings
		case TOKEN_TYPE_LABEL:
		case TOKEN_TYPE_STRING:
			if(token->string) printf("label[%s]\n", token->string);
			else printf("<null>\n");
			break;

		case TOKEN_TYPE_NONE:
			printf("<NONE>\n");
			break;

		case TOKEN_TYPE_EOF:
			printf("<EOF>\n");
			break;

		case TOKEN_TYPE_ERROR:
			printf("<ERROR>\n");
			break;
		
		default:
			printf("<UNKNOWN>\n");
	}
}



int Token_compare(Token token, const Token comparators[], int length, char flags) {
	#define success() {return comparator - comparators;}

	switch (token.type) {
	case TOKEN_TYPE_ERROR:
		raiseError("[Parser] Token to compare is has ERROR type");
		return -1;
	
	case TOKEN_TYPE_EOF:
		if ((flags & SYNTAXFLAG_EOF) == 0)
			raiseError("[Parser] Illegal EOF");

		return -2;
	}

	Array_for(const Token, comparators, length, comparator) {
		switch (comparator->type) {
		case TOKEN_CTYPE_OPERATOR:
			if (token.type == TOKEN_TYPE_OPERATOR && token.operator == comparator->operator)
				success();
			
			break;
		
		case TOKEN_CTYPE_KEYWORD:
			if (token.type == TOKEN_TYPE_LABEL && token.label == comparator->label)
				success();
			
			break;


		case TOKEN_CTYPE_LABEL:
			if (token.type == TOKEN_TYPE_LABEL) {
				// Forbidden keywords
				if (
					token.label == _commonLabels._let ||
					token.label == _commonLabels._const ||
					token.label == _commonLabels._class ||
					token.label == _commonLabels._function
				) {
					raiseError("[Parser] Forbidden keyword used");
				}
				
				success();
			}

			break;

		case TOKEN_CTYPE_INTEGER:
			switch (token.type) {
			case TOKEN_TYPE_I8:
			case TOKEN_TYPE_U8:
			case TOKEN_TYPE_I16:
			case TOKEN_TYPE_U16:
			case TOKEN_TYPE_I32:
			case TOKEN_TYPE_U32:
			case TOKEN_TYPE_I64:
			case TOKEN_TYPE_U64:
				success();
			}
			
			break;
		
		
		case TOKEN_CTYPE_NUMBER:
			switch (token.type) {
			case TOKEN_TYPE_I8:
			case TOKEN_TYPE_U8:
			case TOKEN_TYPE_I16:
			case TOKEN_TYPE_U16:
			case TOKEN_TYPE_I32:
			case TOKEN_TYPE_U32:
			case TOKEN_TYPE_F32:
			case TOKEN_TYPE_I64:
			case TOKEN_TYPE_U64:
			case TOKEN_TYPE_F64:
				success();
			}
			
			break;
		}

		

		fail:
		continue;
	}


	if ((flags & SYNTAXFLAG_UNFOUND) == 0)
		raiseError("[Parser] Token not found");
		

	return -3; // not found
}


