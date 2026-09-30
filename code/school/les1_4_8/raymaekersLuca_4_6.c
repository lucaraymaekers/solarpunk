#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS

int main(void)
{
 printf("Enter 2 real numbers: ");
 
 float X, Y;
 scanf("%f%*c%f%*c", &X, &Y);
 
 float Q = X/Y;
 
 printf("Input: %.20f\n", Q);
 return 0;
}
