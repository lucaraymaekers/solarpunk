#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

void ReadInteger(int *X);
int  VerifyIntegers(int *List, int Size);
void InsertSortedInteger(int *List, int Size, int Search);
void PrintIntegers(int *List, int Size);

int main(void)
{
	int List[10] = {0};
	int Size = (sizeof(List)/sizeof(*List));
	int Value = 0;
	
	for(int Idx = 0; Idx < Size - 1; Idx += 1)
	{
		printf("Number[%d]: ", Idx);
		ReadInteger(List + Idx);
	}
	
	if(VerifyIntegers(List, Size - 1))
	{
		printf("Insert this number: ");
		ReadInteger(&Value);
		
		InsertSortedInteger(List, Size, Value);
		
		PrintIntegers(List, Size);
	}
	else
	{
		printf("Error, numbers were not entered in ascending order.\n");
	}
	
	return 0;
}

void ReadInteger(int *X)
{
	scanf("%d%*c", X);
}

int VerifyIntegers(int *List, int Size)
{
	int Valid = 1;
	
	for(int Idx = 0; Idx < Size - 1; Idx += 1)
	{
		if(List[Idx + 1] < List[Idx])
		{
			Valid = 0;
			break;
		}
	}
	
	return Valid;
}

void InsertSortedInteger(int *List, int Size, int Value)
{
	int Pos = 0;
	
	for(int Idx = 0; Idx < Size; Idx += 1)
	{
		Pos = Idx;
		if(Value < List[Idx])
		{
			break;
		}
	}
	
	// NOTE(luca): Iterate backwards from the end until pos and move every value
	// one position towards the end. => Make space for the new value
	for(int Idx = Size - 1; Idx > Pos; Idx -= 1)
	{
		List[Idx] = List[Idx - 1];
	}
	
	// NOTE(luca): Insert the new value.
	List[Pos] = Value;
}

void PrintIntegers(int *List, int Size)
{
	printf("List:\n");
	for(int Idx = 0; Idx < Size; Idx += 1)
	{
		printf("- %d\n", List[Idx]);
	}
}