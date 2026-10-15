#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int main(void)
{
	printf("Enter 2 real numbers: ");
	
	float Number1, Number2;
	
	scanf("%f%*c%f%*c", &Number1, &Number2);
	
	float Sum = Number1 + Number2;
	float Product = Number1*Number2;
	
	printf("Real sum: %f\nReal product: %f\n", Sum, Product);
	return 0;
}