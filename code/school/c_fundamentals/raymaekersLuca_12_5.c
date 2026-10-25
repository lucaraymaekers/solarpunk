#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS

void PrintShort(short int Value);

int main(void)
{
	PrintShort(23043);
	PrintShort(-24523);
	
	return 0;
}

void PrintShort(short int Value)
{
	int BitCount = sizeof(Value)*8;
	int Bit = 0;
	
	printf("input:       %hd\n", Value);
	printf("output:      ");
	
	for(int Idx = (BitCount - 1); Idx >= 0; Idx -= 1)
	{
		Bit = ((Value >> Idx) & 1);
		printf("%d", Bit);
		if((Idx % 4) == 0)
		{
			printf(" ");
		}
	}
	
	printf("\n");
}
