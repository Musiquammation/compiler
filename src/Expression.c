#include "Expression.h"

#include "declarations.h"
#include "helper.h"


void Expression_create(expression_t type, Expression* e) {
	printf("CREATE %p\n", e);
	e->type = type;
	e->followerCount = 0;
	Array_create(&e->followers, sizeof(Expression*));
}


void Expression_delete(expression_t type, Expression* e) {
	printf("DELETE %p\n", e);
	
	switch (type) {

	}
}

void Expression_free(expression_t type, Expression* e) {
	int followingNumber = Expression_getFollowingExpressionNumber(type, e);

	// Unfollow followed expressions
	if (followingNumber > 0) {
		expressionPtr_t* const buffer = malloc(followingNumber * sizeof(expressionPtr_t));

		Expression_collectFollowedExpressions(type, e, buffer);

		Array_for(expressionPtr_t, buffer, followingNumber, ptr) {
			expressionPtr_t followed = *ptr;
			if (followed) {
				Expression_unfollow(e, followed);
			}
		}

		free(buffer);
	}

	// Free expression
	Array_free(e->followers);
	Expression_delete(type, e);
	free(e);
}


void Expression_follow(Expression* follower, Expression* followed) {
	printf("FOLLOW %p %d (%p)\n", followed, followed->followerCount + 1, follower);
	*(Expression**)Array_pushInEmpty(&followed->followers, isNullPointerRef) = follower;
	followed->followerCount++;
}

void Expression_unfollow(Expression* follower, Expression* followed) {
	printf("UNFOLW %p %d\n", followed, followed->followerCount - 1);	
	
	Array_loop(expressionPtr_t, followed->followers, ptr) {
		if (*ptr == follower) {
			*ptr = NULL;
			

			if ((--followed->followerCount) > 0)
				return;

			// Followed is followed by nobody
			Expression_free(followed->type, followed);
		}
	}

	printf("WARN: Unfollow failed\n");
}

void Expression_followAsNull(Expression* followed) {
	printf("XOLLOW %p %d\n", followed, followed->followerCount + 1);

	followed->followerCount++;
}

void Expression_unfollowAsNull(expression_t type, Expression* followed) {
	printf("XNFOLW %p %d\n", followed, followed->followerCount - 1);

	if ((--followed->followerCount) <= 0) {
		Expression_free(type, followed);
	}

}





int Expression_getFollowingExpressionNumber(expression_t type, Expression* e) {
	switch (type) {
	case EXPRESSION_NONE:
	case EXPRESSION_INVALID:
		return 0;


	/// TODO: followingNumber
	case EXPRESSION_VARIABLE:
		return -1;
	
	/// TODO: followingNumber
	case EXPRESSION_PROPERTY:
		return 2;
	
	/// TODO: followingNumber
	case EXPRESSION_CLASS:
		return -1;
	
	/// TODO: followingNumber
	case EXPRESSION_FUNCTION:
		return -1;

	case EXPRESSION_ARRAY:
		return ((ExpressionArray*)e)->expressions.length;

	/// TODO: followingNumber
	case EXPRESSION_TYPE:
		return -1;

	case EXPRESSION_CHAR:
	case EXPRESSION_BOOL:
	case EXPRESSION_SHORT:
	case EXPRESSION_INT:
	case EXPRESSION_LONG:
	case EXPRESSION_FLOAT:
	case EXPRESSION_DOUBLE:
	case EXPRESSION_STRING:
	case EXPRESSION_LABEL:
		return 0;

	case EXPRESSION_NEGATION:        // -a
	case EXPRESSION_INCREMENT:       // a + 1
	case EXPRESSION_DECREMENT:       // a - 1
	case EXPRESSION_BITWISE_NOT:     // ~a
	case EXPRESSION_LOGICAL_NOT:     // !a
		return 1;

	case EXPRESSION_ADDITION:        // a + b
	case EXPRESSION_SUBSTRACTION:    // a - b
	case EXPRESSION_MULTIPLICATION:  // a * b
	case EXPRESSION_DIVISION:        // a / b
	case EXPRESSION_MODULO:          // a % b

	case EXPRESSION_BITWISE_AND:     // a & b
	case EXPRESSION_BITWISE_OR:      // a | b
	case EXPRESSION_BITWISE_XOR:     // a ^ b
	case EXPRESSION_LEFT_SHIFT:      // a << b
	case EXPRESSION_RIGHT_SHIFT:     // a >> b

	case EXPRESSION_LOGICAL_AND:     // a && b
	case EXPRESSION_LOGICAL_OR:      // a || b

	case EXPRESSION_EQUAL:           // a == b
	case EXPRESSION_NOT_EQUAL:       // a != b
	case EXPRESSION_LESS:            // a < b
	case EXPRESSION_LESS_EQUAL:      // a <= b
	case EXPRESSION_GREATER:         // a > b
	case EXPRESSION_GREATER_EQUAL:   // a >= b
		return 2;
	}

	return 0;
}


void Expression_collectFollowedExpressions(expression_t type, Expression* e, expressionPtr_t buffer[]) {
	switch (type) {
	case EXPRESSION_ARRAY:
	{
		memcpy(
			buffer,
			((ExpressionArray*)e)->expressions.data,
			((ExpressionArray*)e)->expressions.length * sizeof(Expression*)
		);
		break;
	}

	case EXPRESSION_VARIABLE:
	{
		/// TODO: collectExpressiosn
		
		break;
	}
	
	case EXPRESSION_PROPERTY:
	{
		buffer[0] = Expression_cast(((ExpressionProperty*)e)->parent);
		buffer[1] = ((ExpressionProperty*)e)->value;
		break;
	}
	
	case EXPRESSION_CLASS:
	{
		/// TODO: collectExpressiosn
		
		break;
	}
	
	case EXPRESSION_FUNCTION:
	{
		/// TODO: collectExpressiosn

		break;
	}

	case EXPRESSION_TYPE:
	{

		break;
	}
	
	case EXPRESSION_NEGATION:
		buffer[0] = ((ExpressionNegation*)e)->expr;
		break;
	case EXPRESSION_INCREMENT:
		buffer[0] = ((ExpressionIncrement*)e)->expr;
		break;
	case EXPRESSION_DECREMENT:
		buffer[0] = ((ExpressionDecrement*)e)->expr;
		break;
	case EXPRESSION_BITWISE_NOT:
		buffer[0] = ((ExpressionBitwiseNot*)e)->expr;
		break;
	case EXPRESSION_LOGICAL_NOT:
		buffer[0] = ((ExpressionLogicalNot*)e)->expr;
		break;

	case EXPRESSION_ADDITION:
		buffer[0] = ((ExpressionAddition*)e)->left;
		buffer[1] = ((ExpressionAddition*)e)->right;
		break;
	case EXPRESSION_SUBSTRACTION:
		buffer[0] = ((ExpressionSubstraction*)e)->left;
		buffer[1] = ((ExpressionSubstraction*)e)->right;
		break;
	case EXPRESSION_MULTIPLICATION:
		buffer[0] = ((ExpressionMultiplication*)e)->left;
		buffer[1] = ((ExpressionMultiplication*)e)->right;
		break;
	case EXPRESSION_DIVISION:
		buffer[0] = ((ExpressionDivision*)e)->left;
		buffer[1] = ((ExpressionDivision*)e)->right;
		break;
	case EXPRESSION_MODULO:
		buffer[0] = ((ExpressionModulo*)e)->left;
		buffer[1] = ((ExpressionModulo*)e)->right;
		break;

	case EXPRESSION_BITWISE_AND:
		buffer[0] = ((ExpressionBitwiseAnd*)e)->left;
		buffer[1] = ((ExpressionBitwiseAnd*)e)->right;
		break;
	case EXPRESSION_BITWISE_OR:
		buffer[0] = ((ExpressionBitwiseOr*)e)->left;
		buffer[1] = ((ExpressionBitwiseOr*)e)->right;
		break;
	case EXPRESSION_BITWISE_XOR:
		buffer[0] = ((ExpressionBitwiseXor*)e)->left;
		buffer[1] = ((ExpressionBitwiseXor*)e)->right;
		break;
	case EXPRESSION_LEFT_SHIFT:
		buffer[0] = ((ExpressionLeftShift*)e)->left;
		buffer[1] = ((ExpressionLeftShift*)e)->right;
		break;
	case EXPRESSION_RIGHT_SHIFT:
		buffer[0] = ((ExpressionRightShift*)e)->left;
		buffer[1] = ((ExpressionRightShift*)e)->right;
		break;

	case EXPRESSION_LOGICAL_AND:
		buffer[0] = ((ExpressionLogicalAnd*)e)->left;
		buffer[1] = ((ExpressionLogicalAnd*)e)->right;
		break;
	case EXPRESSION_LOGICAL_OR:
		buffer[0] = ((ExpressionLogicalOr*)e)->left;
		buffer[1] = ((ExpressionLogicalOr*)e)->right;
		break;

	case EXPRESSION_EQUAL:
		buffer[0] = ((ExpressionEqual*)e)->left;
		buffer[1] = ((ExpressionEqual*)e)->right;
		break;
	case EXPRESSION_NOT_EQUAL:
		buffer[0] = ((ExpressionNotEqual*)e)->left;
		buffer[1] = ((ExpressionNotEqual*)e)->right;
		break;
	case EXPRESSION_LESS:
		buffer[0] = ((ExpressionLess*)e)->left;
		buffer[1] = ((ExpressionLess*)e)->right;
		break;
	case EXPRESSION_LESS_EQUAL:
		buffer[0] = ((ExpressionLessEqual*)e)->left;
		buffer[1] = ((ExpressionLessEqual*)e)->right;
		break;
	case EXPRESSION_GREATER:
		buffer[0] = ((ExpressionGreater*)e)->left;
		buffer[1] = ((ExpressionGreater*)e)->right;
		break;
	case EXPRESSION_GREATER_EQUAL:
		buffer[0] = ((ExpressionGreaterEqual*)e)->left;
		buffer[1] = ((ExpressionGreaterEqual*)e)->right;
		break;

	default:
		break;
	}
}
