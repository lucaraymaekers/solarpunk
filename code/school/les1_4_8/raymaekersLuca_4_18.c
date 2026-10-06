#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS

int main(void)
{
	char Name[256], FirstName[256], TownName[256];
	int Street, Code;
	
	printf("Enter first name: ");
	scanf("%s", FirstName);
	
	printf("Enter name: ");
	scanf("%s", Name);
	
	printf("Enter street number: ");
	scanf("%d%*c", &Street);
	
	printf("Enter zip code: ");
	scanf("%d%*c", &Code);
	
	printf("Enter town name: ");
	scanf("%s%*c", TownName);
	
	printf("%16s %s\n", FirstName, Name);
	printf("%d\n", Street);
	printf("%-16d %s", Code, TownName);
	
	return 0;
}
