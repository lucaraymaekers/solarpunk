
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>

void CalcPrint(int *EvenSum, int *OddSum, int Number, int Limit);
void Read(int *Number, int *Limit);

int main(int ArgCount, char **Args)
{
	int Number = 0;
	int Limit = 0;
	int EvenSum = 0;
	int OddSum = 0;
	
	Read(&Number, &Limit);
	while(Number != 0 && Limit != 0)
	{  
		CalcPrint(&EvenSum, &OddSum, Number, Limit);
		
		printf("the sum of the even numbers is %d\n", EvenSum);
		printf("the sum of the odd numbers is %d\n\n", OddSum);
		
		Read(&Number, &Limit);
	}
	
	printf("Thanks!\n");
	
	return 0;
}

void Read(int *Number, int *Limit)
{
	printf("enter the number you want to use for the table of multiplication:\n");
	scanf("%d%*c", Number);
	printf("enter the limit:\n");
	scanf("%d%*c", Limit);
}

void CalcPrint(int *EvenSum, int *OddSum, int Number, int Limit)
{
	int Value = Number;
	
	while(Value < Limit)
	{
		printf("%d\n", Value);
		
		if(Value & 1)
		{
			*OddSum += Value;
		}
		else
		{
			*EvenSum += Value;
		}
		
		Value += Number;
	}
}
