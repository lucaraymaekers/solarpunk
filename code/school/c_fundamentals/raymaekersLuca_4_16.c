#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int main(void)
{
	printf("Enter 6 integer numbers: ");
	
	int X1, X2, X3, X4, X5, X6;
	scanf("%d%*c%d%*c%d%*c%d%*c%d%*c%d%*c", &X1, &X2, &X3, &X4, &X5, &X6);
	
	printf("-------------\n");
	printf("| %3d | %3d |\n", X1, X2);
	printf("-------------\n");
	printf("| %3d | %3d |\n", X3, X4);
	printf("-------------\n");
	printf("| %3d | %3d |\n", X5, X6);
	printf("-------------\n");
	
	return 0;
}
