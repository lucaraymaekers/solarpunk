#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS

int main(void)
{
 printf("Enter 6 integer numbers: ");
 
 int X1, X2, X3, X4, X5, X6;
 scanf("%d%*c%d%*c%d%*c%d%*c%d%*c%d%*c", &X1, &X2, &X3, &X4, &X5, &X6);
 
 printf("%2d %5d\n%2d %5d\n%2d %5d\n", X1, X2, X3, X4, X5, X6);
 
 return 0;
}
