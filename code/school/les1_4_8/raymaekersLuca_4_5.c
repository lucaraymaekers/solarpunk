#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int main(void)
{
	printf("Enter 2 integers: ");
	
	int X, Y;
	scanf("%d%*c%d%*c", &X, &Y);
	
	int IntQuotient = X/Y;
	int IntRemainder = X%Y;
	float Quotient = (float)X/(float)Y;
	
	printf("integer quotient = %d\n", IntQuotient);
	printf("remainder = %d\n", IntRemainder);
	printf("quotient = %.2f\n", Quotient);
	
	return 0;
}
