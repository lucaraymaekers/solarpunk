#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#define StringCount 10
#define StringMaxSize 100

int  ReadNames(char Strings[][StringMaxSize]);
void PrintNames(char Strings[][StringMaxSize], int Size);

int main(void)
{
	char Strings[StringCount][StringMaxSize] = {0};
	int Size = 0;
	
	Size = ReadNames(Strings);
	PrintNames(Strings, Size);
	
	return 0;
}

int ReadNames(char Strings[][StringMaxSize])
{
	int Size = 0;
	
	while(Size < StringCount)
	{
		printf("Name[%d]: ", Size);
		scanf("%s%*c", Strings[Size]);
		
		if(strcmp(Strings[Size], "end"))
		{
			Size += 1;
		}
		else
		{
			break;
		}
	}
	
	return Size;
}

void PrintNames(char Strings[][StringMaxSize], int Size)
{
	printf("Names: \n");
	for(int Idx = 0; Idx < Size; Idx += 1)
	{
		printf("- %s\n", Strings[Idx]);
	}
}
