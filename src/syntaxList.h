#ifndef COMPILER_SYNTAXLIST_H_
#define COMPILER_SYNTAXLIST_H_

#include "Parser.h"


enum {
    SYNTAXFLAG_EOF = 1,
    SYNTAXFLAG_UNFOUND = 2,
};

void syntaxList_init();


#endif