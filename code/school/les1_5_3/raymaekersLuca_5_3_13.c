#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <limits.h>

int main(int ArgCount, char **Args)
{
	int Max = INT_MIN;
	int Min = INT_MAX;
	int Invoer;
	
	printf("Enter 10 numbers: ");
	
	for(int Count = 0; Count < 10; Count += 1)
	{
		scanf("%d", &Invoer);
		
		if(Invoer > Max)
		{
			Max = Invoer;
		}
		
		if(Invoer < Min)
		{
			Min = Invoer;
		}
	}
	
	scanf("%*c");
	
	printf("Min = %d Max = %d\n", Min, Max);
	
	return 0;
}
