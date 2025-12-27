#ifndef TOOLS_ERROR_H
#define TOOLS_ERROR_H

#include "tools.h"
#include <setjmp.h>


uint Error_try(jmp_buf handler, uint(*run)());
uint Error_try_catch(jmp_buf handler, uint(*run)(), uint(*error)(uint));
void Error_throw(jmp_buf handler, uint error);


#endif