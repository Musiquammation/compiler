#include "Expression.h"
#include "Interpreter.h"

#include "chooseSign.h"
#include "declarations.h"
#include "helper.h"
#include "Function.h"
#include "Variable.h"

#include "primitives.h"

#include <stddef.h>
#include "tools/Array.h"

#include <string.h>

void Expression_free(int type, Expression* e) {
	switch (type) {
	case EXPRESSION_NONE:
	case EXPRESSION_INVALID:
		break;

	case EXPRESSION_VALUE:
	{
		free(e->data.value.value);
		break;
	}

	case EXPRESSION_PROPERTY:
	{
		if (e->data.property.freeVariableArr) {
			free(e->data.property.varr);
		}
		
		Expression* o = e->data.property.origin;
		if (o) {
			Expression_free(o->type, o);
			free(o);
		}

		break;
	}

	case EXPRESSION_FNCALL:
	{
		Function* fn = e->data.fncall.fn;
		Expression** args = e->data.fncall.args;
		if (args) {
			int totalLength = fn->projections_len +
				fn->settings_len + fn->args_len;

			typedef Expression* ptr_t;
			Array_for(ptr_t, e->data.fncall.args, totalLength, ptr) {
				Expression* i = *ptr;
				Expression_free(i->type, i);
				free(i);
			}

			free(e->data.fncall.args);
		}


		break;
	}

	case EXPRESSION_FAST_ACCESS:
	{
		Expression* o = e->data.fastAccess.origin;
		if (o) {
			Expression_free(o->type, o);
			free(o);
		}

		break;
	}

	case EXPRESSION_LINK:
	{
		Expression* l = e->data.linked;
		Expression_free(l->type, l);
		free(l);
		break;
	}



	case EXPRESSION_I8:
	case EXPRESSION_I16:
	case EXPRESSION_I32:
	case EXPRESSION_I64:

	case EXPRESSION_U8:
	case EXPRESSION_U16:
	case EXPRESSION_U32:
	case EXPRESSION_U64:

	case EXPRESSION_F32:
	case EXPRESSION_F64:

	case EXPRESSION_INTEGER:
	case EXPRESSION_FLOATING:
		
	case EXPRESSION_STRING:
		break;



	case EXPRESSION_ADDITION:
	case EXPRESSION_SUBSTRACTION:
	case EXPRESSION_MULTIPLICATION:
	case EXPRESSION_DIVISION:
	case EXPRESSION_MODULO:
	case EXPRESSION_BITWISE_AND:
	case EXPRESSION_BITWISE_OR:
	case EXPRESSION_BITWISE_XOR:
	case EXPRESSION_LEFT_SHIFT:
	case EXPRESSION_RIGHT_SHIFT:
	
	case EXPRESSION_LOGICAL_AND:
	case EXPRESSION_LOGICAL_OR:
	
	case EXPRESSION_EQUAL:
	case EXPRESSION_NOT_EQUAL:
	case EXPRESSION_LESS:
	case EXPRESSION_LESS_EQUAL:
	case EXPRESSION_GREATER:
	case EXPRESSION_GREATER_EQUAL:
		Expression_free(e->data.operands.left->type, e->data.operands.left);
		Expression_free(e->data.operands.right->type, e->data.operands.right);
		break;


	case EXPRESSION_POSITIVE:
	case EXPRESSION_NEGATION:
	case EXPRESSION_BITWISE_NOT:
	case EXPRESSION_ADDR_OF:
	case EXPRESSION_VALUE_AT:
	case EXPRESSION_LOGICAL_NOT:
		Expression_free(e->data.operand.op->type, e->data.operand.op);
		if (e->data.operand.toFree) {free(e->data.operand.op);}
		break;

	case EXPRESSION_R_INCREMENT:
	case EXPRESSION_R_DECREMENT:
		Expression_free(e->data.operand.op->type, e->data.operand.op);
		if (e->data.operand.toFree) {free(e->data.operand.op);}
		break;

	case EXPRESSION_L_INCREMENT:
	case EXPRESSION_L_DECREMENT:
		Expression_free(e->data.operand.op->type, e->data.operand.op);
		if (e->data.operand.toFree) {free(e->data.operand.op);}
		break;


	case EXPRESSION_GROUP:
	{
		Expression* target = e->data.target;
		Expression_free(target->type, target);
		free(target);
		break;
	}

	case EXPRESSION_CONSTRUCTOR:
		for (constructorDefinition_t* m = e->data.constructor.members; m->variable; m++) {
			Expression_free(m->expr->type, m->expr);
			free(m->expr);
		}

		Function* fn = e->data.constructor.origin->fn;
		int length = fn->projections_len + fn->settings_len + fn->args_len;

		typedef Expression* expr_t;
		Array_for(expr_t, e->data.constructor.args, length, ptr) {
			Expression* arg = *ptr;
			Expression_free(arg->type, arg);
			free(arg);
		}

		free(e->data.constructor.members);
		free(e->data.constructor.args);
		Prototype_free(e->data.constructor.origin->proto, true);
		free(e->data.constructor.origin);
		break;
	}
}




void Expression_exchangeReferences(
	Expression* expr,
	const Expression* base,
	Expression* results,
	int length
) {
	#define exchange(ptr) {                   \
		int diff = ptr - base;                \
		if (diff <= 0) {diff += length;}      \
		ptr = &results[diff];                 \
	};

	switch (expr->type) {
		case EXPRESSION_NONE:
		case EXPRESSION_INVALID:

		case EXPRESSION_I8:
		case EXPRESSION_I16:
		case EXPRESSION_I32:
		case EXPRESSION_I64:

		case EXPRESSION_U8:
		case EXPRESSION_U16:
		case EXPRESSION_U32:
		case EXPRESSION_U64:

		case EXPRESSION_F32:
		case EXPRESSION_F64:

		case EXPRESSION_INTEGER:
		case EXPRESSION_FLOATING:

			break;
	

		case EXPRESSION_PROPERTY:
			break;

		
		/// TODO: there
		case EXPRESSION_STRING:
			break;
	

		case EXPRESSION_ADDITION:
		case EXPRESSION_SUBSTRACTION:
		case EXPRESSION_MULTIPLICATION:
		case EXPRESSION_DIVISION:
		case EXPRESSION_MODULO:

		case EXPRESSION_BITWISE_AND:
		case EXPRESSION_BITWISE_OR:
		case EXPRESSION_BITWISE_XOR:
		case EXPRESSION_LEFT_SHIFT:
		case EXPRESSION_RIGHT_SHIFT:
	
		case EXPRESSION_LOGICAL_AND:
		case EXPRESSION_LOGICAL_OR:
	
		case EXPRESSION_EQUAL:
		case EXPRESSION_NOT_EQUAL:
		case EXPRESSION_LESS:
		case EXPRESSION_LESS_EQUAL:
		case EXPRESSION_GREATER:
		case EXPRESSION_GREATER_EQUAL:
			exchange(expr->data.operands.left);
			exchange(expr->data.operands.right);
			break;

		
		case EXPRESSION_BITWISE_NOT:
		case EXPRESSION_LOGICAL_NOT:
		case EXPRESSION_POSITIVE:
		case EXPRESSION_NEGATION:
		case EXPRESSION_R_INCREMENT:
		case EXPRESSION_R_DECREMENT:
		case EXPRESSION_L_INCREMENT:
		case EXPRESSION_L_DECREMENT:
		case EXPRESSION_A_INCREMENT:
		case EXPRESSION_A_DECREMENT:
		case EXPRESSION_ADDR_OF:
		case EXPRESSION_VALUE_AT:
			exchange(expr->data.operand.op);
			break;

	}


	#undef exchange
}




bool Expression_followsOperatorPlace(int operatorPlace, int type) {
	switch (operatorPlace) {
		// 0: multiplication, division, modulo
		case 0:
			switch (type) {
				case EXPRESSION_MULTIPLICATION:
				case EXPRESSION_DIVISION:
				case EXPRESSION_MODULO:
					return true;
				default:
					return false;
			}

		// 1: addition, subtraction
		case 1:
			switch (type) {
				case EXPRESSION_ADDITION:
				case EXPRESSION_SUBSTRACTION:
					return true;
				default:
					return false;
			}

		// 2: bit shifts
		case 2:
			switch (type) {
				case EXPRESSION_LEFT_SHIFT:
				case EXPRESSION_RIGHT_SHIFT:
					return true;
				default:
					return false;
			}

		// 3: relational (< <= > >=)
		case 3:
			switch (type) {
				case EXPRESSION_LESS:
				case EXPRESSION_LESS_EQUAL:
				case EXPRESSION_GREATER:
				case EXPRESSION_GREATER_EQUAL:
					return true;
				default:
					return false;
			}

		// 4: equality (== !=)
		case 4:
			switch (type) {
				case EXPRESSION_EQUAL:
				case EXPRESSION_NOT_EQUAL:
					return true;
				default:
					return false;
			}

		// 5: bitwise AND
		case 5:
			return type == EXPRESSION_BITWISE_AND;

		// 6: bitwise XOR
		case 6:
			return type == EXPRESSION_BITWISE_XOR;

		// 7: bitwise OR
		case 7:
			return type == EXPRESSION_BITWISE_OR;

		// 8: logical AND
		case 8:
			return type == EXPRESSION_LOGICAL_AND;

		// 9: logical OR
		case 9:
			return type == EXPRESSION_LOGICAL_OR;

		default:
			return false;
	}
}




Expression* Expression_processLine(Expression* line, int length) {
	char* const lineUsed = malloc(length);
	memset(lineUsed, 0, length);


	// Handle increments/decrements
	int prev_type = -1;
	int prev_index = -1;
	for (int i = 0; i < length; i++) {
		if (lineUsed[i])
			continue;

		int i_type = line[i].type;

		switch (i_type) {
		case EXPRESSION_A_INCREMENT:
		case EXPRESSION_A_DECREMENT:
			// Operand is at left (syntaxically)
			if (prev_type == EXPRESSION_PROPERTY) {
				lineUsed[prev_index] = 1; // consume expression
				prev_type = i_type - (EXPRESSION_A_INCREMENT - EXPRESSION_R_INCREMENT); // change type
				line[i].type = prev_type;
				line[i].data.operand.op = &line[prev_index];
				line[i].data.operand.toFree = false;
				goto nextIncrement;
			}
			
			
			// Operand is at right (syntaxically)
			prev_index = i;
			
			// Search operand
			while (true) {
				i++;

				if (i >= length) {
					raiseError("Increment (or decrement) operator used but operand is missing");
					return NULL;
				}

				if (lineUsed[i] == 0)
					break;
			}

			// Check operand type
			/// TODO: type validity 
			switch (line[i].type) {
			case EXPRESSION_PROPERTY:
				break;

			default:
				raiseError("Invalid operand for increment");
				return NULL;
				
			}
			
			
			// Change type
			lineUsed[i] = 1; // consume expression
			prev_type = i_type - (EXPRESSION_A_INCREMENT - EXPRESSION_L_INCREMENT);
			line[prev_index].type = prev_type;
			line[prev_index].data.operand.op = &line[i];
			line[prev_index].data.operand.toFree = false;
			goto nextIncrement;
		}

		prev_type = i_type;

		nextIncrement:
		prev_index = i;
	}


	// Handle one-operand expressions
	prev_type = -1;
	prev_index = -1;

	for (int i = 0; i < length; i++) {
		if (lineUsed[i])
			continue;

		int i_type = line[i].type;
		int nextTypeApplied;

		switch (i_type) {
			case EXPRESSION_ADDITION:
				nextTypeApplied = EXPRESSION_POSITIVE;
				goto applyOneOp;

			case EXPRESSION_SUBSTRACTION:
				nextTypeApplied = EXPRESSION_NEGATION;
				goto applyOneOp;
				
			case EXPRESSION_MULTIPLICATION:
				nextTypeApplied = EXPRESSION_VALUE_AT;
				goto applyOneOp;
				
			case EXPRESSION_BITWISE_AND:
				nextTypeApplied = EXPRESSION_ADDR_OF;
				goto applyOneOp;
			
			
			applyOneOp:
			// check if operand is one-operand
			switch (prev_type) {
				case -1:
				case EXPRESSION_ADDITION:
				case EXPRESSION_SUBSTRACTION:
				case EXPRESSION_MULTIPLICATION:
				case EXPRESSION_DIVISION:
				case EXPRESSION_MODULO:
					i_type = nextTypeApplied;
					break;


				default:
					goto nextOneOperand;
				}

				break;

			case EXPRESSION_BITWISE_NOT:
			case EXPRESSION_LOGICAL_NOT:
				break;

			default:
				goto nextOneOperand;
		}


		prev_index = i;

		// Search operand
		while (true) {
			i++;

			if (i >= length) {
				raiseError("Cannot finish expression with an operator");
				return NULL;
			}

			if (lineUsed[i] == 0)
				break;
		}

		// Consume operand
		lineUsed[i] = 1;

		// Set
		int operandType = line[i].type;
		if (Expression_canSimplify(i_type, operandType, -1)) {
			prev_type = Expression_simplify(
				i_type,
				operandType,
				-1,
				&line[prev_index],
				&line[i],
				NULL
			);
			
			line[prev_index].type = prev_type;

		} else {
			changeType:
			// Change type
			line[prev_index].type = i_type;
			line[prev_index].data.operand.op = &line[i];
			line[prev_index].data.operand.toFree = false;
			prev_type = i_type;
		}

		

		continue; // skip prev_type = i_type (maybe over-optimisation)

		nextOneOperand:
		prev_type = i_type;
	}



	// Handle two-operand operators
	for (int operatorPlace = 0; operatorPlace < 10; operatorPlace++) {
		prev_index = -1;
		prev_type = -1;

		for (int i = 0; i < length; i++) {
			if (lineUsed[i])
				continue;

			// Check if it is a type
			int type = line[i].type;
			if (
				type < EXPRESSION_ADDITION ||
				type > EXPRESSION_GREATER_EQUAL ||
				!Expression_followsOperatorPlace(operatorPlace, type)
			) {
				prev_type = type;
				prev_index = i;
				continue;
			}

			// Search operand
			int operationIndex = i;
			while (true) {
				i++;

				if (i >= length) {
					raiseError("Cannot finish expression with an operator");
					return NULL;
				}

				if (lineUsed[i] == 0)
					break;
			}


			int rightType = line[i].type;
			if (Expression_canSimplify(type, prev_type, rightType)) {
				// Simplify
				line[operationIndex].type = Expression_simplify(
					type,
					prev_type,
					rightType,
					&line[operationIndex],
					&line[prev_index],
					&line[i]
				);


			} else {
				// Add operands
				line[operationIndex].data.operands.left = &line[prev_index];
				line[operationIndex].data.operands.right = &line[i];

			}



			// Consume operands
			lineUsed[prev_index] = 1;
			lineUsed[i] = 1;

			// For next iteration
			prev_type = type;
			prev_index = operationIndex;
			continue;
		}

		

	}



	// Get expression to return
	int decalage = -1;
	for (int i = 0; i < length; i++) {
		if (lineUsed[i] == 0) {
			if (decalage >= 0) {
				raiseError("[INTERN] Two main expression found");
				return NULL;
			}

			decalage = i;
		}
	}

	if (decalage < 0) {
		raiseError("[INTERN] No main expression found");
		return NULL;
	}

	
	// Generate result
	Expression* result = malloc(length * sizeof(Expression));
	int i, j;

	Expression* base = &line[decalage];
	for (i = 0, j = decalage; j < length; i++) {
		result[i] = line[j];
		Expression_exchangeReferences(&result[i], base, result, length);
		j++;
	}
	
	for (j = 0; i < length; i++) {
		result[i] = line[j];
		Expression_exchangeReferences(&result[i], base, result, length);
		j++;
	}



	

	free(lineUsed);
	
	return result;

}




bool Expression_canSimplify(int type, int op1, int op2) {
	#define checkLeft()  {if (op1 < EXPRESSION_I8 || op1 > EXPRESSION_F64) return false;}
	#define checkRight() {if (op2 < EXPRESSION_I8 || op2 > EXPRESSION_F64) return false;}

	if (type >= EXPRESSION_ADDITION && type <= EXPRESSION_GREATER_EQUAL) {
		checkLeft();
		checkRight();
		return true;
	}

	if (type >= EXPRESSION_BITWISE_NOT && type <= EXPRESSION_NEGATION) {
		checkLeft();
		return true;
	}


	return false;
	
	#undef checkLeft
	#undef checkRight
	#undef scale

}




int Expression_getSignedSize(int exprType) {
	switch (exprType) {
		case EXPRESSION_U8:  return 1;
		case EXPRESSION_I8:  return -1;
		case EXPRESSION_U16: return -2;
		case EXPRESSION_I16: return 2;
		case EXPRESSION_U32: return 4;
		case EXPRESSION_I32: return -4;
		case EXPRESSION_F32: return 5;
		case EXPRESSION_U64: return 8;
		case EXPRESSION_I64: return -8;
		case EXPRESSION_F64: return 9;
		case EXPRESSION_INTEGER: return -4;
		case EXPRESSION_FLOATING: return 5;

		default: return 0;
	}
}

int Expression_reachSignedSize(int type, const Expression* expr) {
	restart:
	switch (type) {
	case EXPRESSION_U8:  return 1;
	case EXPRESSION_I8:  return -1;
	case EXPRESSION_U16: return -2;
	case EXPRESSION_I16: return 2;
	case EXPRESSION_U32: return 4;
	case EXPRESSION_I32: return -4;
	case EXPRESSION_F32: return 5;
	case EXPRESSION_U64: return 8;
	case EXPRESSION_I64: return -8;
	case EXPRESSION_F64: return 9;
	case EXPRESSION_INTEGER: return EXPRSIZE_INTEGER;
	case EXPRESSION_FLOATING: return EXPRSIZE_FLOATING;

	case EXPRESSION_GROUP:
		expr = expr->data.target;
		type = expr->type;
		goto restart;

	case EXPRESSION_FNCALL:
		return Prototype_getSignedSize(expr->data.fncall.fn->returnPrototype);

	case EXPRESSION_PROPERTY:
	{
		int subLength = expr->data.property.varr_len - 1;
		return Prototype_getSignedSize(expr->data.property.varr[subLength]->proto);
	}

	case EXPRESSION_LINK:
		expr = expr->data.linked;
		type = expr->type;
		goto restart;

	case EXPRESSION_ADDITION:
	case EXPRESSION_SUBSTRACTION:
	case EXPRESSION_MULTIPLICATION:
	case EXPRESSION_DIVISION:
	{
		int signedLeft = Expression_reachSignedSize(
			expr->data.operands.left->type,
			expr->data.operands.left
		);

		int signedRight = Expression_reachSignedSize(
			expr->data.operands.right->type,
			expr->data.operands.right
		);

		return chooseSign(signedLeft, signedRight);
	}
	
	case EXPRESSION_MODULO:
	case EXPRESSION_BITWISE_AND:
	case EXPRESSION_BITWISE_OR:
	case EXPRESSION_BITWISE_XOR:
	case EXPRESSION_LEFT_SHIFT:
	case EXPRESSION_RIGHT_SHIFT:
	{
		int signedLeft = Expression_reachSignedSize(
			expr->data.operands.left->type,
			expr->data.operands.left
		);

		int signedRight = Expression_reachSignedSize(
			expr->data.operands.right->type,
			expr->data.operands.right
		);

		/// TODO: raise floatings

		return chooseSign(signedLeft, signedRight);
	}


	case EXPRESSION_LOGICAL_AND:
	case EXPRESSION_LOGICAL_OR:
	case EXPRESSION_EQUAL:
	case EXPRESSION_NOT_EQUAL:
	case EXPRESSION_LESS:
	case EXPRESSION_LESS_EQUAL:
	case EXPRESSION_GREATER:
	case EXPRESSION_GREATER_EQUAL:
		int signedLeft = Expression_reachSignedSize(
			expr->data.operands.left->type,
			expr->data.operands.left
		);

		int signedRight = Expression_reachSignedSize(
			expr->data.operands.right->type,
			expr->data.operands.right
		);

		/// TODO: raise floatings

		printf("choose cmp %d %d\n", signedLeft, signedRight);
		return chooseSign(signedLeft, signedRight);
	/// TODO: logical operations (max of operands)


	default:
		raiseError("[Intern] Cannot get size of this expression type");
		return 0;
	}
}








Expression* Expression_crossTyped(int type, Expression* expr) {
	if (type == EXPRESSION_GROUP) {
		raiseError("[TODO] Expression_cross");
		return NULL;
	}

	if (type == EXPRESSION_LINK) {
		return expr->data.linked;
	}

	return expr;
}


Expression* Expression_cross(Expression* expr) {
	return Expression_crossTyped(expr->type, expr);
}


Prototype* Expression_getPrimitiveProtoFromType(int type) {
	switch (type) {
	case EXPRESSION_U8:
		return &_primitives.proto_u8;
	case EXPRESSION_I8:
		return &_primitives.proto_i8;

	case EXPRESSION_U16:
		return &_primitives.proto_u16;
	case EXPRESSION_I16:
		return &_primitives.proto_i16;

	case EXPRESSION_U32:
		return &_primitives.proto_u32;
	case EXPRESSION_I32:
	case EXPRESSION_INTEGER:
		return &_primitives.proto_i32;
	case EXPRESSION_F32:
	case EXPRESSION_FLOATING:
		return &_primitives.proto_f32;

	case EXPRESSION_U64:
		return &_primitives.proto_u64;
	case EXPRESSION_I64:
		return &_primitives.proto_i64;
	case EXPRESSION_F64:
		return &_primitives.proto_f64;


	default:
		return NULL;
	}
}

Type* Expression_getPrimitiveTypeFromType(int type) {
	switch (type) {
	case EXPRESSION_U8:
		return &_primitives.type_u8;

	case EXPRESSION_I8:
		return &_primitives.type_i8;


	case EXPRESSION_U16:
		return &_primitives.type_u16;

	case EXPRESSION_I16:
		return &_primitives.type_i16;


	case EXPRESSION_U32:
		return &_primitives.type_u32;

	case EXPRESSION_I32:
	case EXPRESSION_INTEGER:
		return &_primitives.type_i32;

	case EXPRESSION_F32:
	case EXPRESSION_FLOATING:
		return &_primitives.type_f32;


	case EXPRESSION_U64:
		return &_primitives.type_u64;

	case EXPRESSION_I64:
		return &_primitives.type_i64;

	case EXPRESSION_F64:
		return &_primitives.type_f64;


	}

	return NULL;

}

Prototype* Expression_getPrimitiveProtoFromSize(int signedSize) {
	switch (signedSize) {
	case 1:
		return &_primitives.proto_u8;
	case -1:
		return &_primitives.proto_i8;

	case 2:
		return &_primitives.proto_u16;
	case -2:
		return &_primitives.proto_i16;

	case 4:
		return &_primitives.proto_u32;
	
	case -4:
	case EXPRSIZE_INTEGER:
		return &_primitives.proto_i32;

	case 5:
	case EXPRSIZE_FLOATING:
		return &_primitives.proto_f32;

	case 8:
		return &_primitives.proto_u64;
	case -8:
		return &_primitives.proto_i64;
	case 9:
		return &_primitives.proto_f64;


	default:
		return NULL;
	
	}
}



protoAndType_t Expression_generateExpressionType(Expression* value, Scope* scope) {
	value = Expression_cross(value);
	int exprType = value->type;

	if (exprType == EXPRESSION_PROPERTY) {
		Expression* origin = value->data.property.origin;
		Variable** varr = value->data.property.varr;
		int varr_len = value->data.property.varr_len;

		Type* rootType;
		if (origin) {
			protoAndType_t pat = Expression_generateExpressionType(origin, scope);
			rootType = pat.type;
			Prototype_free(pat.proto, false);
		} else {
			rootType = Scope_searchType(scope, varr[0]);
			if (!rootType) {
				raiseError("[Intern] Cannot find type of variable");
			}
		}


		Variable* subVarr[varr_len];
		memcpy(subVarr, varr, sizeof(subVarr));
		Type* type = Type_deepCopy(rootType, subVarr[varr_len-1]->proto, subVarr, varr_len);

		if (origin) {
			Type_free(rootType);
		}
		
		// Prototype* proto = varr[varr_len - 1]->proto;
		Prototype* proto = type->proto;
		Prototype_addUsage(*proto);
		return (protoAndType_t){proto, type};
	}
	
	if (exprType == EXPRESSION_FNCALL) {
		protoAndType_t pat = {.proto = value->data.fncall.fn->returnPrototype};
		/// TODO: define useThis as true or false
		pat.type = Interpret_call(value, scope);
		return pat;
	}
	
	if (exprType == EXPRESSION_VALUE) {
		raiseError("[TODO]: handle value read/edits");
	}

	if (exprType == EXPRESSION_FAST_ACCESS) {
		protoAndType_t origin = Expression_generateExpressionType(value->data.fastAccess.origin, scope);
		int stdBehavior = value->data.fastAccess.accessor->stdBehavior;
		if (stdBehavior < 0) {
			raiseError("[TODO]: perform real std behavior");
		}

		switch (stdBehavior) {
		// pointer
		case 0:
		{
			return origin;
		}

		default:
			raiseError("[BadId]: Invalid id for standard fast access");
		}
	}

	
	if (exprType >= EXPRESSION_ADDITION && exprType <= EXPRESSION_L_DECREMENT) {
		int signedSize = Expression_reachSignedSize(exprType, value);
		protoAndType_t pat;
		pat.proto = Expression_getPrimitiveProtoFromSize(signedSize);
		pat.type = primitives_getType(pat.proto->primitive.cl);
		return pat;
	}
	
	if (exprType >= EXPRESSION_U8 && exprType <= EXPRESSION_FLOATING) {
		protoAndType_t pat;
		pat.proto = Expression_getPrimitiveProtoFromType(exprType);
		pat.type = Expression_getPrimitiveTypeFromType(exprType);
		return pat;
	}
	
	if (exprType == EXPRESSION_ADDR_OF) {
		Expression* reference = Expression_cross(value->data.operand.op);
		if (reference->type != EXPRESSION_PROPERTY) {
			raiseError("[Syntax] Can only get the address of a variable");
			return (protoAndType_t){};
		}

		Variable** refVarArr = reference->data.property.varr;
		reference->data.property.freeVariableArr = false;

		protoAndType_t pat;
		pat.proto = Prototype_generateStackPointer(refVarArr, reference->data.property.varr_len);
		pat.type = Prototype_generateType(pat.proto, scope, TYPE_CWAY_DEFAULT);
		return pat;
	}

	if (exprType == EXPRESSION_CONSTRUCTOR) {
		protoAndType_t pat;
		Function* fn = value->data.constructor.origin->fn;
		pat.proto = value->data.constructor.origin->proto;
		pat.type = Prototype_generateType(pat.proto, scope, TYPE_CWAY_DEFAULT);

		Expression thisArg = {
			.type = EXPRESSION_TYPE,
			.data = {.type = pat.type}
		};

		int thisIndex = fn->projections_len + fn->settings_len;

		Expression** args = value->data.constructor.args;
		args[thisIndex] = &thisArg;

		/// TODO: choose argsStartIndex
		Expression fncall = {
			.type = EXPRESSION_FNCALL,
			.data = {.fncall = {
				.args = args,
				.fn = fn
			}}
		};

		Interpret_call(&fncall, scope);

		args[thisIndex] = NULL;
		return pat;
	}


	
	raiseError("[TODO]: this expression is not handled");
}



void Expression_place(
	Trace* trace,
	Expression* valueExpr,
	Variable** varrDest,
	int varrDest_len
) {
	// Read targetExpr
	Expression* sourceExpr = Expression_cross(valueExpr);
	int sourceExprType = sourceExpr->type;

	if (sourceExprType == EXPRESSION_PROPERTY) {
		Variable* first = varrDest[0];
		Variable* last = varrDest[varrDest_len-1];
		int id;
		int signedSize;
		id = first->id;
		if (id >= 0) {
			signedSize = Prototype_getSignedSize(last->proto);

		} else {
			Variable* lv = sourceExpr->data.property.varr[sourceExpr->data.property.varr_len-1];
			Prototype* lvp = lv->proto;

			switch (Prototype_mode(*lvp)) {
			case PROTO_MODE_REFERENCE:
			{

				break;
			}

			case PROTO_MODE_DIRECT:
			{
				signedSize = Prototype_getSignedSize(lvp);

				char psc = lvp->direct.primitiveSizeCode;
				if (psc) {
					id = Trace_ins_create(trace, lv, psc < 0 ? -psc : psc, 0, psc);
				} else {
					id = Trace_ins_create(trace, lv, signedSize < 0 ? -signedSize : signedSize, 0, 0);
				}
				first->id = id;
				break;
			}

			case PROTO_MODE_VARIADIC:
			{

				break;
			}

			case PROTO_MODE_PRIMITIVE:
			{
				signedSize = lvp->primitive.sizeCode;
				id = Trace_ins_create(trace, lv, signedSize < 0 ? -signedSize : signedSize, 0, true);
				first->id = id;
				break;
			}

			case PROTO_MODE_VOID:
			{
				raiseError("[Intern] Cannot set a variable to void");
				return;
			}


			}
		}
		// int firstId = giveIdToVariable(varrDest);

		Trace_set(
			trace,
			sourceExpr,
			id,
			Prototype_getVariableOffset(varrDest, varrDest_len),
			signedSize,	
			EXPRESSION_PROPERTY
		);

	} else if (sourceExprType == EXPRESSION_FNCALL) {
		/// TODO: handle null id
		Trace_set(
			trace,
			sourceExpr,
			varrDest[0]->id,
			Prototype_getVariableOffset(varrDest, varrDest_len),
			Prototype_getSignedSize(varrDest[varrDest_len-1]->proto),
			EXPRESSION_FNCALL
		);


	} else if (sourceExprType == EXPRESSION_VALUE) {
		raiseError("[TODO]: handle value read/edits");
	
	
	} else if (sourceExprType >= EXPRESSION_ADDITION && sourceExprType <= EXPRESSION_L_DECREMENT) {
		Prototype* vproto = varrDest[varrDest_len-1]->proto;

		if (vproto) {
			Trace_set(
				trace,
				sourceExpr,
				varrDest[0]->id,
				Prototype_getVariableOffset(varrDest, varrDest_len),
				Prototype_getSignedSize(varrDest[varrDest_len-1]->proto),
				sourceExprType
			);
		
		} else if (varrDest_len == 1) {
			Variable* v = varrDest[0];
			int signedSize = Expression_reachSignedSize(sourceExprType, sourceExpr);
			Prototype* p = Expression_getPrimitiveProtoFromSize(signedSize);

			int id = v->id;
			if (id < 0) {
				id = Trace_ins_create(trace, v, signedSize < 0 ? -signedSize : signedSize, 0, true);
				v->id = id;
			}


			Trace_set(
				trace,
				sourceExpr,
				id,
				-1,
				signedSize,
				sourceExprType
			);


		} else {
			raiseError("[Intern] No prototype to place expression");
		}



	} else if (sourceExprType >= EXPRESSION_U8 && sourceExprType <= EXPRESSION_FLOATING) {
		int subLength = varrDest_len - 1;
		/// TODO: check sizes

		Prototype* vproto = varrDest[subLength]->proto;

		if (vproto) {
			int signedSize = Prototype_getSignedSize(vproto);
	
			Trace_ins_def(
				trace,
				varrDest[0]->id,
				Prototype_getVariableOffset(varrDest, varrDest_len),
				signedSize,
				castable_cast(
					Expression_getSignedSize(sourceExprType),
					signedSize,
					sourceExpr->data.num
				)
			);

		} else if (subLength == 0) {
			Variable* v = varrDest[0];
			int signedSize = Expression_getSignedSize(sourceExprType);

			int id = v->id;
			if (id < 0) {
				id = Trace_ins_create(trace, v, signedSize < 0 ? -signedSize : signedSize, 0, true);
				v->id = id;
			}

			Trace_ins_def(
				trace,
				id,
				-1,
				signedSize,
				castable_cast(
					Expression_getSignedSize(sourceExprType),
					signedSize,
					sourceExpr->data.num
				)
			);
		} else {
			raiseError("[Intern] No prototype to place expression");
		}


	} else if (sourceExprType == EXPRESSION_ADDR_OF) {
		Expression* reference = Expression_cross(sourceExpr->data.operand.op);
		if (reference->type != EXPRESSION_PROPERTY) {
			raiseError("[Syntax] Can only get the address of a variable");
			return;
		}
		
		int refArrLength = reference->data.property.varr_len;
		Variable** refVarArr = reference->data.property.varr;
		int srcOffset = Prototype_getVariableOffset(refVarArr, refArrLength);

		int srcVar = refVarArr[0]->id;

		int destVar = varrDest[0]->id;
		int destOffset;
		if (destVar < 0) {
			destVar = Trace_ins_create(trace, varrDest[0], 8, 0, true);
			varrDest[0]->id = destVar;
			destOffset = -1;
		} else {
			destOffset = Prototype_getVariableOffset(varrDest, varrDest_len);
		}


		if (varrDest[varrDest_len-1]->proto == NULL) {
			if (varrDest_len != 1) {
				raiseError("[Intern] No prototype to place expression");
			}

			reference->data.property.freeVariableArr = false;
		}
		

		Trace_ins_getStackPtr(trace, destVar, srcVar, destOffset, srcOffset);
		
		



	} else if (sourceExprType == EXPRESSION_CONSTRUCTOR) {
		int id = varrDest[0]->id;
			int offsetBase = Prototype_getPrimitiveSizeCode(varrDest[varrDest_len-1]->proto)
			? -1 : Prototype_getVariableOffset(varrDest, varrDest_len);

		// Define members
		for (constructorDefinition_t* d = sourceExpr->data.constructor.members; true; d++) {
			Variable* v = d->variable;
			if (!v)
				break;

			Expression* expr = d->expr;
			Trace_set(
				trace,
				expr,
				id,
				offsetBase == -1 ? -1 : offsetBase + v->offset,
				Prototype_getSignedSize(v->proto),
				expr->type
			);
		}

		Expression** args = sourceExpr->data.constructor.args;

		Function* fn = sourceExpr->data.constructor.origin->fn;
		if (fn->flags & FUNCTIONFLAGS_THIS) {
			// 'this' projection
			Expression* thisProj = malloc(sizeof(Expression));
			thisProj->type = EXPRESSION_NONE;
			args[0] = thisProj;


			// 'this' argument
			Expression* thisArg = malloc(sizeof(Expression));
			thisArg->type = EXPRESSION_PROPERTY,
			thisArg->data.property.origin = NULL;
			thisArg->data.property.varr = varrDest;
			thisArg->data.property.varr_len = varrDest_len;
			thisArg->data.property.freeVariableArr = false;
		
			Expression* thisAddrArg = malloc(sizeof(Expression));
			thisAddrArg->type = EXPRESSION_ADDR_OF;
			thisAddrArg->data.operand.op = thisArg;
			args[fn->settings_len + fn->projections_len] = thisAddrArg;
		}

		// Call constructor
		Expression tmpExpr = {
			.type = EXPRESSION_FNCALL,
			.data = {.fncall = {
				.args = args,
				.fn = sourceExpr->data.constructor.origin->fn,
			}}
		};

		Trace_set(
			trace,
			&tmpExpr,
			TRACE_VARIABLE_NONE,
			Prototype_getVariableOffset(varrDest, varrDest_len),
			Prototype_getSignedSize(varrDest[varrDest_len-1]->proto),
			EXPRESSION_FNCALL
		);


	} else {
		raiseError("[TODO]: this expression is not handled");
	}
}



