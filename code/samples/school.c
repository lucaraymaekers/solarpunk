//-
// School sample for doing exercises
//-

//~ Libraries
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>

//~ Constants
#define MaxHeight 8

//~ API
void FillMatrix(int *Matrix, int Width, int Height);
void PrintMatrix(int *Matrix, int Width, int Height);

//~ Entrypoint
int main(void)
{
 int Matrix[MaxHeight*MaxHeight] = {0};
 
 for(int Y = 0; Y < MaxHeight; Y += 1)
 {
  
  for(int X = 0; X < Y; X += 1)
  {
   int Value = 1;
   if(Y == 0)
   {
    
   }
   Matrix[Y * MaxHeight + X]  = Value;
   
  }
  
 }
 
 return 0;
}
