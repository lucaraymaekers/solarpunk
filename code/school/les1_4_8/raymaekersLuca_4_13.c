#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS

int main(void)
{
 printf("Enter money: ");
 
 int Money;
 int Cents;
 
 scanf("%d,%d%*c", &Money, &Cents);
 
 int M500, M200, M100, M50, M20, M10, M5, M2, M1;
 
 M500 = Money/500;
 Money -= M500*500;
 M200 = Money/200;
 Money -= M200*200;
 M100 = Money/100;
 Money -= M100*100;
 M50 = Money/50;
 Money -= M50*50;
 M20 = Money/20;
 Money -= M20*20;
 M10 = Money/10;
 Money -= M10*10;
 M5 = Money/5;
 Money -= M5*5;
 M2 = Money/2;
 Money -= M2*2;
 M1 = Money/1;
 Money -= M1*1;
 
 int C50, C20, C10, C5, C2, C1;
 C50 = Cents/50;
 Cents -= C50*50;
 C20 = Cents/20;
 Cents -= C20*20;
 C10 = Cents/10;
 Cents -= C10*10;
 C5 = Cents/5;
 Cents -= C5*5;
 C2 = Cents/2;
 Cents -= C2*2;
 C1 = Cents/1;
 Cents -= C1*1;
 
 printf("%dx500, %dx200, %dx100, %dx50, %dx20, %dx10, %dx5, %dx2, %dx1, %dx.50, %dx.20, %dx.10, %dx.05, %dx.02, %dx.01 \n",
        M500, M200, M100, M50, M20, M10, M5, M2, M1, C50, C20, C10, C5, C2, C1);
 
 return 0;
}
