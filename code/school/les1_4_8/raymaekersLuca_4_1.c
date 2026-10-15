#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int main(void)
{
	
	printf("Enter an integer: ");
	
	short int Number = 0;
	scanf("%hd%*c", &Number);
	
	printf("Input: %hd\n", Number);
	return 0;
}