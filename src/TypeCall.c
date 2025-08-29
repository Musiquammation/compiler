#include "TypeCall.h"

#include "Type.h"

void TypeCall_generateType(const TypeCall* tcall, Type* type) {
    Expression_create(EXPRESSION_TYPE, Expression_cast(type));
    type->class = tcall->class;

}