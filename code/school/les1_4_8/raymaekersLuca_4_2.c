#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int main(void)
{
	printf("Enter an integer: ");
	
	int Number = 0;
	scanf("%d%*c", &Number);
	
	printf("Input: %d\n", Number);
	return 0;
}