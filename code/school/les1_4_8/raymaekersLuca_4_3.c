#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int main(void)
{
	printf("Enter 2 integers: ");
	
	int Number1, Number2;
	scanf("%d%*c%d%*c", &Number1, &Number2);
	
	printf("Input: %d %d\n", Number1, Number2);
	return 0;
}