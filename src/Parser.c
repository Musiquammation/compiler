#include "Parser.h"

void Parser_open(Parser* parser, char* filename) {
    parser->file = fopen(filename, "r");
}

void Parser_close(Parser* parser) {
    fclose(parser->file);
}


Token Parser_read(Parser* parser) {

}

