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
		case EXPRESSION_NONE:
			break;
		case EXPRESSION_INVALID:
			break;

		case EXPRESSION_ARRAY:
			break;
			
		case EXPRESSION_VARIABLE:
			break;
			
		case EXPRESSION_PROPERTY:
			break;
			
		case EXPRESSION_CLASS:
			break;
			
		case EXPRESSION_FUNCTION:
			break;
			
		case EXPRESSION_TYPE:
			break;
			
		


		case EXPRESSION_CHAR:
		case EXPRESSION_BOOL:
		case EXPRESSION_SHORT:
		case EXPRESSION_INT:
		case EXPRESSION_LONG:
		case EXPRESSION_FLOAT:
		case EXPRESSION_DOUBLE:
		case EXPRESSION_STRING:
		case EXPRESSION_LABEL:
			break;


		case EXPRESSION_ADDITION:
			Expression_unfollow(e, ((ExpressionAddition*)e)->left);
			Expression_unfollow(e, ((ExpressionAddition*)e)->right);
			break;

		case EXPRESSION_SUBSTRACTION:
			Expression_unfollow(e, ((ExpressionSubstraction*)e)->left);
			Expression_unfollow(e, ((ExpressionSubstraction*)e)->right);
			break;

		case EXPRESSION_MULTIPLICATION:
			Expression_unfollow(e, ((ExpressionMultiplication*)e)->left);
			Expression_unfollow(e, ((ExpressionMultiplication*)e)->right);
			break;

		case EXPRESSION_DIVISION:
			Expression_unfollow(e, ((ExpressionDivision*)e)->left);
			Expression_unfollow(e, ((ExpressionDivision*)e)->right);
			break;

		case EXPRESSION_MODULO:
			Expression_unfollow(e, ((ExpressionModulo*)e)->left);
			Expression_unfollow(e, ((ExpressionModulo*)e)->right);
			break;


		case EXPRESSION_NEGATION:
			Expression_unfollow(e, ((ExpressionNegation*)e)->expr);
			break;

		case EXPRESSION_INCREMENT:
			Expression_unfollow(e, ((ExpressionIncrement*)e)->expr);
			break;

		case EXPRESSION_DECREMENT:
			Expression_unfollow(e, ((ExpressionDecrement*)e)->expr);
			break;

		case EXPRESSION_BITWISE_NOT:
			Expression_unfollow(e, ((ExpressionBitwiseNot*)e)->expr);
			break;

		case EXPRESSION_LOGICAL_NOT:
			Expression_unfollow(e, ((ExpressionLogicalNot*)e)->expr);
			break;


		case EXPRESSION_BITWISE_AND:
			Expression_unfollow(e, ((ExpressionBitwiseAnd*)e)->left);
			Expression_unfollow(e, ((ExpressionBitwiseAnd*)e)->right);
			break;

		case EXPRESSION_BITWISE_OR:
			Expression_unfollow(e, ((ExpressionBitwiseOr*)e)->left);
			Expression_unfollow(e, ((ExpressionBitwiseOr*)e)->right);
			break;

		case EXPRESSION_BITWISE_XOR:
			Expression_unfollow(e, ((ExpressionBitwiseXor*)e)->left);
			Expression_unfollow(e, ((ExpressionBitwiseXor*)e)->right);
			break;

		case EXPRESSION_LEFT_SHIFT:
			Expression_unfollow(e, ((ExpressionLeftShift*)e)->left);
			Expression_unfollow(e, ((ExpressionLeftShift*)e)->right);
			break;

		case EXPRESSION_RIGHT_SHIFT:
			Expression_unfollow(e, ((ExpressionRightShift*)e)->left);
			Expression_unfollow(e, ((ExpressionRightShift*)e)->right);
			break;


		case EXPRESSION_LOGICAL_AND:
			Expression_unfollow(e, ((ExpressionLogicalAnd*)e)->left);
			Expression_unfollow(e, ((ExpressionLogicalAnd*)e)->right);
			break;

		case EXPRESSION_LOGICAL_OR:
			Expression_unfollow(e, ((ExpressionLogicalOr*)e)->left);
			Expression_unfollow(e, ((ExpressionLogicalOr*)e)->right);
			break;


		case EXPRESSION_EQUAL:
			Expression_unfollow(e, ((ExpressionEqual*)e)->left);
			Expression_unfollow(e, ((ExpressionEqual*)e)->right);
			break;

		case EXPRESSION_NOT_EQUAL:
			Expression_unfollow(e, ((ExpressionNotEqual*)e)->left);
			Expression_unfollow(e, ((ExpressionNotEqual*)e)->right);
			break;

		case EXPRESSION_LESS:
			Expression_unfollow(e, ((ExpressionLess*)e)->left);
			Expression_unfollow(e, ((ExpressionLess*)e)->right);
			break;

		case EXPRESSION_LESS_EQUAL:
			Expression_unfollow(e, ((ExpressionLessEqual*)e)->left);
			Expression_unfollow(e, ((ExpressionLessEqual*)e)->right);
			break;

		case EXPRESSION_GREATER:
			Expression_unfollow(e, ((ExpressionGreater*)e)->left);
			Expression_unfollow(e, ((ExpressionGreater*)e)->right);
			break;

		case EXPRESSION_GREATER_EQUAL:
			Expression_unfollow(e, ((ExpressionGreaterEqual*)e)->left);
			Expression_unfollow(e, ((ExpressionGreaterEqual*)e)->right);
			break;
	}
}

void Expression_free(expression_t type, Expression* e) {
	// Free expression
	Expression_delete(type, e);
	Array_free(e->followers);
	free(e);
}


void Expression_follow(Expression* follower, Expression* followed) {
	printf("+FOLLW %p %d (%p)\n", followed, followed->followerCount + 1, follower);
	*(Expression**)Array_pushInEmpty(&followed->followers, isNullPointerRef) = follower;
	followed->followerCount++;
}

void Expression_unfollow(Expression* follower, Expression* followed) {
	printf("-FOLLW %p %d (%p)\n", followed, followed->followerCount - 1, follower);	
	
	Array_loop(expressionPtr_t, followed->followers, ptr) {
		if (*ptr == follower) {
			*ptr = NULL;
			
			if ((--followed->followerCount) > 0)
				return;

			// Followed is followed by nobody
			Expression_free(followed->type, followed);
			return;
		}
	}

	printf("WARN: Unfollow failed\n");
}

void Expression_followAsNull(Expression* followed) {
	printf("+XOLLW %p %d\n", followed, followed->followerCount + 1);

	followed->followerCount++;
}

void Expression_unfollowAsNull(expression_t type, Expression* followed) {
	printf("-XOLLW %p %d\n", followed, followed->followerCount - 1);

	if ((--followed->followerCount) <= 0) {
		Expression_free(type, followed);
	}

}




