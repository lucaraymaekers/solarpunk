
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

str8 AsmTemplateText = {(u8 *)"\n"
"  $NOMOD51\n"
"  \n"
"  ;~ Inits\n"
"  .org 0x0000\n"
"  mov sp, 0x7F\n"
"  \n"
"  ;~ EntryPoint\n"
"  loop:\n"
" \n"
" ljmp loop\n"
"  \n"
"  ;~ Includes\n"
"  $include \"aducgd1.asm\"\n"
"  ", 139};
str8 CTemplateText = {(u8 *)"\n"
"#define _CRT_SECURE_NO_WARNINGS\n"
"#include <stdio.h>\n"
" \n"
" int main(int ArgCount, char **Args)\n"
" {\n"
"  \n"
"  return 0;\n"
" }\n"
" \n"
" ", 115};
