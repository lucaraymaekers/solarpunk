#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int main(void)
{
	printf("Enter name and first name: ");
	
	char Name[256];
	fgets(Name, sizeof(Name), stdin);
	
	printf("%s\n", Name);
	return 0;
}
