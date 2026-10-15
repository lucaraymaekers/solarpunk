#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int main(void)
{
	int a, b, c, d, e, f, g, h, i, j, k, l, m;
	
	printf("Enter a year: ");
	scanf("%d%*c", &j);
	
	a = j%19;
	b = j/100;
	c = j%100;
	d = b/4;
	e = b%4;
	f = (b + 8)/25;
	g = (b - f + 1)/3;
	h = (19*a + b - d - g + 15)%30;
	i = c/4;
	k = c%4;
	l = (32 + 2*e + 2*i - h - k)%7;
	m = (a + 11*h + 22*l)/451;
	
	int Month, Day;
	Month = (h + l - 7*m + 114)/31;
	Day = 1 + (h + l - 7*m + 114)%31;
	
	printf("Easter: %02d/%02d\n", Day, Month);
	return 0;
}
