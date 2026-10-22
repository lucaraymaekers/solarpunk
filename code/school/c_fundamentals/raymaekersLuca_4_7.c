#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int main(void)
{
	printf("Enter hours, minutes and seconds: ");
	
	int H, M;
	float  S;
	scanf("%d%*c%d%*c%f%*c", &H, &M, &S);
	
	printf("The entered time is: %02d hours %02d minutes and %02.3f seconds\n", H, M, S);
	return 0;
}
