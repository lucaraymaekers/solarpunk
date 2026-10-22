//-
// School sample for doing exercises
//-

//~ Libraries
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <ctype.h>

//~ Prototypes
int IsPalindrome(char *);

//~ Entrypoint
int main(void)
{
 char *String = "Was it a car or a cat I saw?";
 
 printf("\"%s\" is ", String);
 if(!IsPalindrome(String))
 {
  printf("not ");
 }
 
 printf("a palindrome.\n");
}

//~ Functions
int IsPalindrome(char *String)
{
 int Result = 1;
 int Size = 0;
 int StartIdx = 0;
 int EndIdx = 0;
 
 Size = (int)strlen(String);
 if(Size > 0)
 {
  EndIdx = Size - 1;
  
  while(StartIdx < Size && EndIdx > 0)
  {
   while(EndIdx > 0 && 
         (ispunct(String[EndIdx]) || 
          isspace(String[EndIdx])))
   {
    EndIdx -= 1;
   }
   
   while(StartIdx < Size &&
         (ispunct(String[StartIdx]) ||
          isspace(String[StartIdx])))
   {
    StartIdx += 1;
   }
   
   if(toupper(String[StartIdx]) != toupper(String[EndIdx]))
   {
    Result = 0;
    break;
   }
   
   StartIdx += 1;
   EndIdx -= 1;
  }
 }
 
 return Result;
}
