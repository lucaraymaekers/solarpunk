#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS
// TODO(luca): Flowchart

void draw_bar();

int main(int ArgCount, char **Args)
{
	char Symbol = 0;
	int Score = 0;
	
	printf("What symbol would you like to use? ");
	scanf("%c%*c", &Symbol);
	
	printf("Enter scores: ");
	
	scanf("%d*c", &Score);
	while(Score >= 0)
	{
		draw_bar(Score, Symbol);
		scanf("%d*c", &Score);
	}
	
	return 0;
}


void draw_bar(int Score, char Symbol)
{
	printf("score = %-20d", Score);
	for(int Idx = 0; Idx < Score; Idx += 1)
	{
		printf("%c", Symbol);
	}
	printf("\n");
}