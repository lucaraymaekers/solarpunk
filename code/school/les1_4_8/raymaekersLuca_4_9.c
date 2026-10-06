#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS

int main(void)
{
	printf("Enter circle radius: ");
	
	float R;
	scanf("%f%*c", &R);
	
	const float Pi = 3.141592653589793f;
	float S = R*R*Pi;
	
	printf("Surface area: %fcm^2\n", S);
	return 0;
}
