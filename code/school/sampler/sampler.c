//-
// Program for easily creating code samples when doing school exercises.
//-
// TODO(luca): 
// - 4coder plugin to create new sample and open new file in buffer.
//   - This plugin must also close all other samples, because otherwise the indexing will be
//     wrong.
//-

#define BASE_CONSOLE_APPLICATION 1
#define BASE_FORCE_THREADS_COUNT 1
#include "base/base.h"
#include "base/base.c"

#include "school/sampler/generated/sampler.meta.c"

BASE_ENTRY_POINT()
{
 arena *Arena = GetScratch();
 str8_array Help = PushS8Array(Arena, 256);
 
 type_kind Type = Type_c;
 
 b32 ShowHelp = true;
 b32 ShouldQuit = false;
 
 u64 MaxSampleCount = 128;
 str8 ChapterIdx = {0};
 
 cmd_line *CmdLine = &Params->CmdLine;
 cmd_line_entry *TypeEntry = CmdLineGetEntryAndAddHelp(CmdLine, &Help, S8("type"), S8("Set sample type e.g., asm/c"));
 if(!IsNilCmdLineEntry(TypeEntry))
 {
  str8 Param = TypeEntry->First->V;;
  
  b32 Found = false;
  for EachElement(Idx, TypeStrings)
  {
   if(S8Match(Param, TypeStrings[Idx], false))
   {
    Type = (type_kind)Idx;
    Found = true;
    break;
   }
  }
  
  if(!Found)
  {
   Log("Error: Invalid type \"%S\", not one of { ", Param);
   for EachElement(Idx, TypeStrings)
   {
    Log("%S, ", TypeStrings[Idx]);
   }
   Log("}\n");
   
   ShouldQuit = true;
  }
 }
 
 cmd_line_entry *ChapterIdxEntry = CmdLineGetEntryAndAddHelp(CmdLine, &Help, S8("chapter"), S8("Use this chapter idx when -type=c is used"));
 if(!IsNilCmdLineEntry(ChapterIdxEntry))
 {
  ChapterIdx = ChapterIdxEntry->First->V;
 }
 
 str8 ExeDirPath = S8CutFromChar(CmdLine->ExePath, OS_SlashChar, false);
 str8 PrefixPath = Str8Fmt("%S" SLASH ".." SLASH "code" SLASH "school" SLASH "%S", 
                           ExeDirPath, TypePaths[Type]);
 
 //- Type specific inits 
 if(Type == Type_c)
 {
  //ChapterIdx = S8("6_4");
  if(ChapterIdx.Size == 0) 
  {
   Log("Error: No chapter index supplied, please provide one with -chapter=...\n");
   ShouldQuit = true;
  }
 }
 
 //- Create format strings and templates based on type 
 str8 FileNameFormat = {0};
 str8 TemplateText = {0};
 if(0) {} 
 else if(Type == Type_asm) 
 {
  FileNameFormat = S8("lesson_%llu.asm");
  TemplateText = AsmTemplateText;
 }
 else if(Type == Type_c)   
 {
  FileNameFormat = Str8Fmt("raymaekersLuca_%S_%%llu.c", ChapterIdx);
  TemplateText = CTemplateText;
 }
 str8 LogFormat = Str8Fmt("school" SLASH "%%S" SLASH "%S\n", FileNameFormat); 
 //- 
 
 if(CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("new"), S8("create new sample")))
 {
  if(!ShouldQuit)
  {
   ShowHelp = false;
   
   str8 FilePathFormat = Str8Fmt("%%S" SLASH "%S", FileNameFormat); 
   
   str8 LastPath = {0};
   u64 LastIdx = 0;
   for EachIndex(Idx, MaxSampleCount)
   {
    str8 Path = Str8Fmt(CSS8(FilePathFormat), PrefixPath, Idx);
    if(OS_FileExists(CSS8(Path)))
    {
     LastIdx = Idx + 1;
    }
   }
   
   if(TemplateText.Size)
   {
    LastPath = Str8Fmt(CSS8(FilePathFormat), PrefixPath, LastIdx);
    Log(CSS8(LogFormat), TypePaths[Type], LastIdx);
    OS_WriteEntireFile(CSS8(LastPath), TemplateText);
   }
   else
   {
    Log("Error: No template for this type.\n");
   }
  }
 }
 
 
 //~ Help 
 if(ShowHelp)
 {
  str8 HelpText = S8ArrayJoinFrom(Arena, &Help, '\n');
  Log("Usage: sampler [options]\n"
      "\n"
      "Options:\n"
      "%S\n", 
      HelpText);
 }
 
 return 0;
}