//~ Libraries
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>

//~ Constants
#define MAX 100

//~ Prototypes
int ReadArray(int []);
void PrintArray(int [], int);
double CalcMean(int row[], int size);
int CalcMax(int row[], int size);

//~ Entrypoint
int main(void)
{
	int row[MAX];
	int size = 0;
	double mean = 0.0;
	int max = 0;
	
	size = ReadArray(row);
	PrintArray(row, size);
	mean = CalcMean(row, size);
	max = CalcMax(row, size);
	
	printf("Mean: %.2f\n", mean);
	printf("Max: %d\n", max);
	
	return 0;
}

//~ Functions
int ReadArray(int array[])
{
	int result = 0;
	int value = 0;
	
	for(int idx = 0; idx < MAX; idx += 1)
	{
		printf("enter number %d: ", idx);
		scanf("%d%*c", &value);
		
		if(value == 999)
		{
			result = idx;
			break;
		}
		else
		{
			array[idx] = value;
		}
		
	}
	
	return result;
}

void PrintArray(int array[], int size)
{
	for(int idx = 0; idx < size; idx += 1)
	{
		printf("number %d: %d\n", idx, array[idx]);
	}
}

double CalcMean(int row[], int size)
{
	double result = 0.0;
	
	for(int idx = 0; idx < size; idx += 1)
	{
		result += ((double)row[idx]/(double)size);
	}
	
	return result;
}

int CalcMax(int row[], int size)
{
	int result = 0;
	
	for(int idx = 0; idx < size; idx += 1)
	{
		if(row[idx] > result)
		{
			result = row[idx];
		}
	}
	
	return result;
}