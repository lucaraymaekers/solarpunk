#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int main(void)
{
	printf("Enter 6 integer numbers: ");
	
	float X1, X2, X3, X4, X5, X6;
	scanf("%f%*c%f%*c%f%*c%f%*c%f%*c%f%*c", &X1, &X2, &X3, &X4, &X5, &X6);
	
	printf("-----------------\n");
	printf("| %3.2f | %3.2f |\n", X1, X2);
	printf("-----------------\n");
	printf("| %3.2f | %3.2f |\n", X3, X4);
	printf("-----------------\n");
	printf("| %3.2f | %3.2f |\n", X5, X6);
	printf("-----------------\n");
	
	return 0;
}
