#include <asm-generic/errno-base.h>
#include <stdio.h>
#include "shuntingyard_stack.h"
#include <string.h>
#include <stdlib.h>
typedef enum {
	TOKEN_OPERATOR,
	TOKEN_OPERAND,
	TOKEN_PAREN,
	TOKEN_UNARY_MINUS,
	TOKEN_UNARY_PLUS
} token_t;

typedef struct _TOKEN {
	int num;
	char op;
	token_t type;
} Token;

// #define DEBUG

DEFINE_STACK(Token, Token)
DEFINE_STACK(Value, long)

void print_tokens_debug(const Token *, const int);
int precedence(char, int);
int eval(Token *, int, int *);

Token *tokenize(const char *, int, int *, int *);
Token *shunting_yard(const Token *, int, int, int *);

int main()
{
	int token_len = 0;
	int postfix_len = 0;
	int result = 0;
	int paren_count = 0;
	int ret = -EINVAL;
	Token *tokens = NULL, *postfix = NULL;
	size_t bufsize = 0;
	char *expr = NULL;
	ssize_t nread = getline(&expr, &bufsize, stdin);

	if (nread == -1)
		goto err_inval;

	if (nread > 0 && expr[nread - 1] == '\n') {
		expr[nread - 1] = '\0';
		nread--;
	}
	tokens = tokenize(expr, nread, &token_len, &paren_count);
	if (!tokens)
		goto err_inval;
	print_tokens_debug(tokens, token_len);
	postfix = shunting_yard(tokens, token_len, paren_count, &postfix_len);

	if (!postfix)
		goto err_inval;
	print_tokens_debug(postfix, postfix_len);

	if (eval(postfix, postfix_len, &result) != 0)
		goto err_inval;

	printf("%d\n", result);
	ret = 0;

err_inval:
	free(expr);
	free(tokens);
	free(postfix);
	return ret;
}

void print_tokens_debug(const Token *token_arr, const int token_len)
{
#ifdef DEBUG
	Token t;
	printf("----\n");
	for (int i = 0; i < token_len; ++i) {
		t = token_arr[i];
		switch (t.type) {
		case TOKEN_OPERAND:
			printf("Token #%d: (type, val) = (operand, %d)", i,
			       t.num);
			break;
		case TOKEN_OPERATOR:
			printf("Token #%d: (type, val) = (opeator, '%c')", i,
			       t.op);
			break;
		case TOKEN_PAREN:
			printf("Token #%d: (type, val) = (paraenthesis, '%c')",
			       i, t.op);
			break;
		case TOKEN_UNARY_MINUS:
			printf("Token #%d: (type, val) = (unary_minus, '%c')",
			       i, t.op);
			break;
		case TOKEN_UNARY_PLUS:
			printf("Token #%d: (type, val) = (unary_plus, '%c')", i,
			       t.op);
			break;
		}
		printf("\n");
	}
	printf("----\n");
#endif
}
Token *tokenize(const char *expr, int len, int *token_len, int *paren_count)
{
	Token *token_arr = (Token *)malloc(sizeof(Token) * len);
	int i = 0;
	int num = 0;
	int ptr = 0;
	int state = 0;
	int parens = 0;
	if (!token_arr)
		goto err_alloc;

	for (; i < len; ++i) {
		char ch = expr[i];
		if ('0' <= ch && ch <= '9') {
			num = num * 10 + ch - '0';
			state = 1;
			continue;
		}
		if (state) {
			token_arr[ptr].type = TOKEN_OPERAND;
			token_arr[ptr].num = num;
			token_arr[ptr].op = '\0';
			num = 0;
			state = 0;
			ptr++;
		}
		if (ch == ' ') {
			continue;
		} else if (ch == '+' || ch == '-' || ch == '*' || ch == '/' ||
			   ch == '%' || ch == '^') {
			token_arr[ptr].type = TOKEN_OPERATOR;
			token_arr[ptr].op = ch;
			ptr++;
		} else if (ch == '(' || ch == ')') {
			token_arr[ptr].type = TOKEN_PAREN;
			token_arr[ptr].op = ch;
			parens++;
			ptr++;
		} else {
			goto err_inval;
		}
	}
	if (state) {
		token_arr[ptr].type = TOKEN_OPERAND;
		token_arr[ptr].num = num;
		ptr++;
	}
	*token_len = ptr;
	*paren_count = parens;
	return token_arr;

err_inval:
err_alloc:
	free(token_arr);
	return NULL;
}
int precedence(char op, int is_unary)
{
	// If we ever need more operators, it will be easier to add it.
	if (is_unary)
		return 30;

	switch (op) {
	case '+':
	case '-':
		return 10;
	case '*':
	case '%':
	case '/':
		return 20;
	case '^':
		return 40;
	default:
		return -1;
	}
}

Token *shunting_yard(const Token *infix, int tokens_len, int paren_count,
		     int *postfix_len)
{
	int i;
	Token *token_arr;
	Token *postfix;
	TokenStack st;
	int postfix_ptr = 0;
	int expect_opnd = 1;
	Token t;

	token_arr = (Token *)malloc(sizeof(Token) * tokens_len);
	postfix = (Token *)malloc(sizeof(Token) * (tokens_len - paren_count));

	if (!token_arr || !postfix)
		goto err_alloc;

	Token_stack_init(&st, token_arr, tokens_len);

	for (i = 0; i < tokens_len; ++i) {
		t = infix[i];
		switch (t.type) {
		case TOKEN_OPERAND: {
			postfix[postfix_ptr++] = t;
			expect_opnd = 0;
			break;
		}
		case TOKEN_OPERATOR: {
			int cur_uny = expect_opnd &&
				      (t.op == '+' || t.op == '-');
			Token *top;
			int is_right_assoc = t.op == '^' || cur_uny;

			if (cur_uny)
				t.type = ((t.op == '+') ? TOKEN_UNARY_PLUS :
							  TOKEN_UNARY_MINUS);

			// if is_right_assoc == 1 then we check precedence(...) < precedence(...)
			// if is_right_assoc == 0 then we check precedence(...) <= precedence(...)
			// We don't want to use tranary operators because that would require branching
			// Therefore we just add the is_right_assoc to the LHS, and only compare <=.

			while ((top = Token_stack_top(&st)) &&
			       top->type != TOKEN_PAREN &&
			       precedence(t.op, cur_uny) + is_right_assoc <=
				       precedence(
					       top->op,
					       top->type == TOKEN_UNARY_MINUS ||
						       top->type ==
							       TOKEN_UNARY_PLUS)) {
				Token popped;
				// Already checked if top is NULL, so this will always return 0
				(void)Token_stack_pop(&st, &popped);
				postfix[postfix_ptr++] = popped;
			}

			if (Token_stack_push(&st, t) == -1)
				goto err_inval;

			expect_opnd = 1;
			break;
		}
		case TOKEN_PAREN: {
			if (t.op == '(') {
				if (Token_stack_push(&st, t) == -1)
					goto err_inval;
				expect_opnd = 1;
			} else {
				Token popped;
				int found_match = 0;
				while (Token_stack_pop(&st, &popped) == 0) {
					if (popped.type == TOKEN_PAREN &&
					    popped.op == '(') {
						found_match = 1;
						break;
					}
					postfix[postfix_ptr++] = popped;
				}
				if (!found_match)
					goto err_inval;

				expect_opnd = 0;
			}
			break;
		}
		case TOKEN_UNARY_MINUS:
		case TOKEN_UNARY_PLUS:
			break;
		default:
			goto err_inval;
		}
	}
	Token remaining;
	while (Token_stack_pop(&st, &remaining) == 0) {
		if (remaining.type == TOKEN_PAREN && remaining.op == '(')
			// There is unmatched paraenthesis
			goto err_inval;
		postfix[postfix_ptr++] = remaining;
	}

	*postfix_len = postfix_ptr;
	free(token_arr);
	return postfix;

err_inval:
err_alloc:
	free(postfix);
	free(token_arr);
	return NULL;
}

int eval(Token *postfix, int len, int *result)
{
	TokenStack st;
	Token *token_arr = (Token *)malloc(sizeof(Token) * len);
	int i = 0;
	Token t;

	if (!token_arr)
		goto err_alloc;

	Token_stack_init(&st, token_arr, len);

	for (; i < len; ++i) {
		t = postfix[i];
		if (t.type == TOKEN_OPERAND) {
			Token_stack_push(&st, t);
		} else if (t.type == TOKEN_OPERATOR) {
			Token x, y;
			int rvx = Token_stack_pop(&st, &x);
			int rvy = Token_stack_pop(&st, &y);
			if (rvx == -1 || rvy == -1)
				goto err_inval;

			switch (t.op) {
			case '+': {
				y.num = y.num + x.num;
				break;
			}

			case '-': {
				y.num = y.num - x.num;
				break;
			}
			case '*': {
				y.num = y.num * x.num;
				break;
			}
			case '/':
			case '%': {
				if (x.num == 0)
					goto err_inval;

				if (t.op == '/')
					y.num = y.num / x.num;
				else
					y.num = y.num % x.num;

				break;
			}
			case '^':
				if (x.num < 0)
					goto err_inval;

				// 0^0 = 1;
				{
					long result = 1;
					int a = 0;
					for (; a < x.num; ++a) {
						result *= y.num;
					}
					y.num = result;
				}
				break;
			default:
				goto err_inval;
			}
			if (Token_stack_push(&st, y) == -1)
				goto err_inval;
		} else if (t.type == TOKEN_UNARY_MINUS ||
			   t.type == TOKEN_UNARY_PLUS) {
			Token x;
			if (Token_stack_pop(&st, &x) == -1)
				goto err_inval;

			if (t.type == TOKEN_UNARY_MINUS)
				x.num = -x.num;

			if (Token_stack_push(&st, x) == -1)
				goto err_inval;
		}
	}
	if (Token_stack_empty(&st) || Token_stack_size(&st) > 1)
		goto err_inval;

	*result = Token_stack_top(&st)->num;
	Token_stack_free(&st);
	return 0;

err_inval:
err_alloc:
	*result = 0;
	free(token_arr);
	return -EINVAL;
}
