

enum type_kind
{
Type_asm,
 Type_c,
 Type_Count
};
typedef enum type_kind type_kind;

str8 TypeStrings[] = {
{(u8 *)"asm", 3},
 {(u8 *)"c", 1},
 
};

str8 TypePaths[] = {
{(u8 *)"embedded_fundamentals", 21},
 {(u8 *)"c_fundamentals", 14},
 
};

str8 ConfigTable[] = {
{(u8 *)"0", 1},
 
};



str8 CTemplateText = {(u8 *)"\n"
"#define _CRT_SECURE_NO_WARNINGS 1\n"
"#include <stdio.h>\n"
"  \n"
"  int main(int ArgCount, char **Args)\n"
"  {\n"
"   \n"
"   return 0;\n"
"  }\n"
"  \n"
"  ", 135}
str8 AsmTemplateText = {(u8 *)"$NOMOD51\n"
"   \n"
"   ;~ Defines\n"
"   $include \"aducgd1def.asm\"\n"
"   \n"
"   ;~ Inits\n"
"   .org 0x0000\n"
"   \n"
"   ;~ EntryPoint\n"
"   mov sp, #0x7F\n"
"  \n"
"  loop: \n"
"  \n"
"  ljmp loop\n"
"   \n"
"   ;~ Libraries\n"
"   $include \"aducgd1inc.asm\"\n"
"   \n"
"   end\n"
"   ", 234}
