#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS

int main(void)
{
	printf("Enter an integer number with 3 digits: ");
	
	int X, Y;
	scanf("%d%*c", &X);
	
	Y = (((X/100)) +
						(((X%100)/10)*10) +
						((X%10)*100));
	
	printf("The number printed backwards is: %d\n", Y);
	return 0;
}
