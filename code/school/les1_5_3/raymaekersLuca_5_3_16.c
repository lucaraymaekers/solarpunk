#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS

int main(int ArgCount, char **Args)
{
	int Width = 0;
	int ShouldQuit = 0;
	char TowerChar = 219;
	char Answer = 0;
	int Offset = 0;
	int FloorCount = 0;
	
	printf("This program builds a tower upside down!\n");
	while(!ShouldQuit)
	{
		printf("How wide should the tower be?\n"
									"(enter an odd number between 3 and 15)\n");
		scanf("%d%*c", &Width);
		
		int IsValid;
		{
			int IsOdd = (Width & 1);
			int IsInBounds = (Width > 3 && Width < 15);
			IsValid = (IsOdd && IsInBounds);
		}
		
		if(IsValid)
		{
			Offset = 0;
			
			while(Width > 0)
			{
				FloorCount = Width/2 + 1;
				
				for(int FloorIdx = 0; FloorIdx < FloorCount; FloorIdx += 1)
				{     
					for(int Idx = 0; Idx < Offset; Idx += 1)
					{
						printf(" ");
					}
					for(int Idx = 0; Idx < Width; Idx += 1)
					{
						printf("%c", 219);
					}
					printf("\n");
				}
				
				Width -= 2;
				Offset += 1;
			}
		}
		else
		{
			printf("ERROR: Invalid width.\n");
		}
		
		printf("Do you want to build another tower (y/n) ");
		scanf("%c%*c", &Answer);
		ShouldQuit = (Answer != 'y');
	}
	
	return 0;
}
