#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS

int main(void)
{
 printf("Enter money: ");
 
 int Money;
 
 scanf("%d%*c", &Money);
 
 int H5, H2, H1, D5, D2, D1, F, T, O;
 
 H5 = Money/(500);
 Money -= H5*(500);
 H2 = Money/(200);
 Money -= H2*(200);
 H1 = Money/(100);
 Money -= H1*(100);
 D5 = Money/(50);
 Money -= D5*(50);
 D2 = Money/(20);
 Money -= D2*(20);
 D1 = Money/(10);
 Money -= D1*(10);
 F = Money/(5);
 Money -= F*(5);
 T = Money/(2);
 Money -= T*(2);
 O = Money/(1);
 Money -= O*(1);
 
 printf("%dx500, %dx200, %dx100, %dx50, %dx20, %dx10, %dx5, %dx2, %dx1\n",
        H5, H2, H1, D5, D2, D1, F, T, 1);
 
 return 0;
}
