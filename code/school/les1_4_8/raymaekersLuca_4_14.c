#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int main(void)
{
	float U, R1, R2;
	float I, V2;
	
	printf("Enter U: ");
	scanf("%f%*c", &U);
	printf("Enter R1: ");
	scanf("%f%*c", &R1);
	printf("Enter R2: ");
	scanf("%f%*c", &R2);
	
	I = U/(R1+R2);
	V2 = I*R2;
	
	printf("I = %f V2 = %f\n", I, V2);
	
	return 0;
}
