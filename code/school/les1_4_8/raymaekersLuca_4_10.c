#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS

int main(void)
{
	int D, H, M, S;
	
	printf("Enter seconds: ");
	scanf("%d%*c", &S);
	
	D = S/(60*60*24);
	S -= D*(60*60*24);
	
	H = S/(60*60);
	S -= H*(60*60);
	
	M = S/60;
	S -= M*60;
	
	printf("sec = %d day %d hour %d minute and %d second\n", 
								D, H, M, S);
	return 0;
}
