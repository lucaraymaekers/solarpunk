#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS

int main(void)
{
	int Invoice, ProductCount, UnitPrice;
	
	printf("Enter invoice number: ");
	scanf("%d%*c", &Invoice);
	
	printf("Enter the number of products ordered: ");
	scanf("%d%*c", &ProductCount);
	
	printf("Enter the price per unit: ");
	scanf("%d%*c", &UnitPrice);
	
	printf("\n");
	printf("%-12s%-12s%-12s%-12s\n", "INVOICE", "NUMBER", "PRICE/UNIT", "TOTAL");
	printf("%-12d%-12d%-12d%-12d\n",
								Invoice, ProductCount, UnitPrice, ProductCount*UnitPrice);
	
	return 0;
}
