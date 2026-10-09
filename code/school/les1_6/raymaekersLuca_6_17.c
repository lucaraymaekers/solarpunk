#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS

void square(int Filled);
void empty_square(void);
void filled_square(void);

int main(int ArgCount, char **Args)
{
	int Option = 0;
	int ShouldQuit = 0;
	
	while(!ShouldQuit)
	{
		printf("What do you choose?\n"
									"        1. Draw an empty square\n"
									"        2. Draw a filled square\n"
									"        3. Stop\n");
		
		scanf("%d%*c", &Option);
		
		switch(Option)
		{
			default:
			{
				printf("ERROR: Invalid option.\n");
			} break;
			
			case 3:
			{
				ShouldQuit = 1;
			} break;
			
			case 1:
			{
				filled_square();
			} break;
			case 2:
			{
				empty_square();
			};
			
		}
	}
	
	return 0;
}

void empty_square(void)
{
	square(0);
}

void filled_square(void)
{
	square(1);
}

#define SQUARE_SIZE 8

void square(int Filled)
{
	int OneMinusSquareSize = SQUARE_SIZE - 1;
	
	for(int Y = 0; Y < SQUARE_SIZE; Y += 1)
	{
		for(int X = 0; X < SQUARE_SIZE; X += 1)
		{
			int IsEdge = (X == 0 || X == OneMinusSquareSize ||
																	Y == 0 || Y == OneMinusSquareSize);
			int DrawSquare = (Filled || IsEdge);
			
			// Square ASCII character.
			char Char = (DrawSquare ? 219 : ' ');
			
			printf("%c", Char);
		}
		printf("\n");
	}
}