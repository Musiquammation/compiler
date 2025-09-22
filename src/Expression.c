#include "Expression.h"


#include "declarations.h"
#include "helper.h"

void Expression_free(int type, Expression* e) {
	switch (type) {
	case EXPRESSION_NONE:
	case EXPRESSION_INVALID:
		break;

	case EXPRESSION_PROPERTY:
	{
		Expression* source = e->data.property.source;
		if (!source)
			break;
			
		Expression_free(-EXPRESSION_PROPERTY, e);

		
		free(source);
		break;
	}

	// Special case without free
	case -EXPRESSION_PROPERTY:
	{
		Expression* source = e->data.property.source;
		if (!source) 
		break;
		
		Expression_free(-EXPRESSION_PROPERTY, source);

		/// TODO: free proto 
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
		Expression_free(e->data.operands.right->type, e->data.operands.right);
		break;

	case EXPRESSION_R_INCREMENT:
	case EXPRESSION_R_DECREMENT:
		Expression_free(e->data.operands.left->type, e->data.operands.left);
		break;

	case EXPRESSION_L_INCREMENT:
	case EXPRESSION_L_DECREMENT:
		Expression_free(e->data.operands.right->type, e->data.operands.right);
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
			printf("op %p\n", expr->data.operand);
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