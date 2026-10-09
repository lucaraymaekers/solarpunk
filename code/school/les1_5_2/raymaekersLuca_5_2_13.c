#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS


#define OperatorCase(Result, Op1, Op2, Operator) case (#Operator)[0]: Result = Op1 Operator Op2; break; 

int main(int ArgCount, char **Args)
{
	float Op1, Op2;
	char Operator;
	float Result = 0.f;
	
	printf("Enter an expression (without spaces!): ");
	scanf("%f%c%f%*c", &Op1, &Operator, &Op2);
	
	switch(Operator)
	{
		default: 
		{
			printf("ERROR: Operator '%c' not supported.\n", Operator);
		} break;
		
		case '+': 
		{
			Result = Op1 + Op2;
		} break;
		case '*': 
		{
			Result = Op1*Op2; 
		} break;
		case '-': 
		{
			Result = Op1 - Op2; 
		} break;
		case '/': 
		{
			Result = Op1/Op2; 
		} break;
	}
	
	printf("%.0f %c %.0f  = %.2f\n", Op1, Operator, Op2, Result);
	
	return 0;
}
