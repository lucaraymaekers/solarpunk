#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int main(void)
{
	printf("Enter 5 integers: ");
	
	int A, B, C, D, E;
	scanf("%d%*c%d%*c%d%*c%d%*c%d%*c", &A, &B, &C, &D, &E);
	
	float Mean = (float)(A+B+C+D+E)/5.f;
	
	printf("Mean: %.2f\n", Mean);
	return 0;
}
