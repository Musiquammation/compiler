#include "Expression.h"

#include "declarations.h"
#include "helper.h"

#include <stddef.h>

void Expression_free(int type, Expression* e) {
	restartFree:
	switch (type) {
	case EXPRESSION_NONE:
	case EXPRESSION_INVALID:
		break;

	case EXPRESSION_PROPERTY:
	{
		free(e->data.property.variableArr);
		if (e->data.property.next) {
			e++;
			type = e->type;
			goto restartFree; // avoid recursion
		}

		break;
	}

	case EXPRESSION_PATH:
	{
		Expression* target = e->data.target;
		Expression_free(target->type, target);
		free(target);
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
	case EXPRESSION_LOGICAL_NOT:
		Expression_free(e->data.operand->type, e->data.operand);
		break;

	case EXPRESSION_R_INCREMENT:
	case EXPRESSION_R_DECREMENT:
		Expression_free(e->data.operand->type, e->data.operand);
		break;

	case EXPRESSION_L_INCREMENT:
	case EXPRESSION_L_DECREMENT:
		Expression_free(e->data.operand->type, e->data.operand);
		break;


	case EXPRESSION_GROUP:
	{
		Expression* target = e->data.target;
		Expression_free(target->type, target);
		free(target);
		break;
	}
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
			break;
	

		case EXPRESSION_PROPERTY:
		case EXPRESSION_PATH:
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
			exchange(expr->data.operand);
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
				line[i].data.operand = &line[prev_index];
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
			line[prev_index].data.operand = &line[i];
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

		switch (i_type) {
			case EXPRESSION_ADDITION:
			case EXPRESSION_SUBSTRACTION:
			
			// check if operand is one-operand
			switch (prev_type) {
				case -1:
				case EXPRESSION_ADDITION:
				case EXPRESSION_SUBSTRACTION:
				case EXPRESSION_MULTIPLICATION:
				case EXPRESSION_DIVISION:
				case EXPRESSION_MODULO:
					i_type += EXPRESSION_POSITIVE - EXPRESSION_ADDITION;
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
			line[prev_index].data.operand = &line[i];
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








