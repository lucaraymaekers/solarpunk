//-
// Program for easily creating code samples when doing school exercises.
//-
// TODO(luca): 
// - 4coder plugin to create new sample and open new file in buffer.
//   - This plugin must also close all other samples, because otherwise the indexing will be
//     wrong.
// - convert spaces to tabs
//-

//~ Libraries
#define BASE_CONSOLE_APPLICATION 1
#define BASE_FORCE_THREADS_COUNT 1
#include "base/base.h"
#include "base/base.c"

#include "cling/cling.h"

#include "school/sampler/generated/sampler.meta.c"

//~ EntryPoint
BASE_ENTRY_POINT()
{
 arena *Arena = GetScratch();
 str8_array Help = PushS8Array(Arena, 256);
 
 u64 MaxSampleCount = 128;
 b32 ShouldQuit = false;
 b32 ShowHelp = false;
 
 type_kind Type = Type_c;
 str8 ChapterIdx = {0};
 b32 HasExplicitIdx = false;
 u64 ExplicitIdx = {0};
 b32 New = false;
 b32 Last = false;
 b32 Copy = false;
 b32 Compile = false;
 b32 Run = false;
 
 //- Command line options 
 cmd_line *CmdLine = &Params->CmdLine;
 
 New = CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("new"), S8("create new sample, after the last one found"));
 Last = CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("last"), S8("use last sample"));
 Copy = CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("copy"), S8("copy previous sample when creating"));
 Compile = CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("compile"), S8("compile sample"));
 Run = CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("run"), S8("run sample"));
 
 if(Copy || Compile || Run) Last = true;
 if(Copy) New  = true;
 
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
 
 // Chapter Idx
 { 
  cmd_line_entry *ChapterIdxEntry = CmdLineGetEntryAndAddHelp(CmdLine, &Help, S8("chapter"), S8("Use this chapter idx when -type=c is used"));
  ChapterIdx = ChapterIdxEntry->First->V;
 }
 
 // Explicit Idx
 { 
  cmd_line_entry *FileIdxEntry = CmdLineGetEntryAndAddHelp(CmdLine, &Help, S8("idx"), S8("use this explicit index"));
  str8 String = FileIdxEntry->First->V;
  HasExplicitIdx = (String.Size != 0);
  for EachIndex(Idx, String.Size)
  {
   if('0' <= String.Data[Idx] && String.Data[Idx] <= '9')
   {
    ExplicitIdx *= 10;
    ExplicitIdx += String.Data[Idx] - '0';
   }
   else
   {
    Log("Error: Please provide a valid number for the idx.\n");
    ShouldQuit = true;
    break;
   }
  }
 }
 
 str8 ExeDirPath = S8CutFromChar(CmdLine->ExePath, OS_SlashChar, false);
 str8 PrefixPath = Str8Fmt(".." SLASH "code" SLASH "school" SLASH "%S", 
                           TypePaths[Type]);
 // Change directory to ./build so artifacts get placed there
 str8 BuildPath = Str8Fmt("%S", ExeDirPath);
 OS_ChangeDirectory(CSS8(BuildPath));
 
 //- Type specific inits 
 if(Type == Type_c)
 {
  if(ChapterIdx.Size == 0) 
  {
   Log("Error: No chapter index supplied, please provide one with -chapter=...\n");
   ShowHelp = true;
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
 // school/type_path/file_name_%llu.ext
 str8 LogFormat = Str8Fmt("school" SLASH "%%S" SLASH "%S\n", FileNameFormat); 
 
 if(TemplateText.Size == 0)
 {
  Log("Error: No template for this type.\n");
  ShouldQuit = true;
 }
 
 //- Do all command-line options 
 if(!ShouldQuit && !ShowHelp)
 {
  ShowHelp = false;
  
  u64 LastIdx = 0;
  // %s/file_name_%llu.ext
  str8 FilePathFormat = Str8Fmt("%%S" SLASH "%S", FileNameFormat); 
  b32 FileFound = false;
  
  //- Find the file idx 
  for EachIndex(Idx, MaxSampleCount)
  {
   str8 Path = Str8Fmt(CSS8(FilePathFormat), PrefixPath, Idx);
   if(OS_FileExists(CSS8(Path)))
   {
    LastIdx = Idx + !!(!Last);
    FileFound = true;
   }
  }
  
  u64 Idx = (HasExplicitIdx ? ExplicitIdx : LastIdx);
  if(Copy && Last) Idx += 1;
  
  str8 Path = Str8Fmt(CSS8(FilePathFormat), PrefixPath, Idx);
  
  if(New)
  {
   if(Copy && FileFound)
   {
    // If Copy and there is a file to copy, we use this file as the template text instead.
    str8 LastPath = Str8Fmt(CSS8(FilePathFormat), PrefixPath, LastIdx);
    TemplateText = OS_ReadEntireFileIntoMemory(CSS8(LastPath));
   }
   
   Log(CSS8(LogFormat), TypePaths[Type], Idx);
   OS_WriteEntireFile(CSS8(Path), TemplateText);
  }
        
        str8 FileName = Str8Fmt(CSS8(FileNameFormat), Idx);
        str8 FileNameWithoutExt = S8CutFromChar(FileName, '.', false);
        
  if(Compile) 
  {
            if(0) {}
            else if(Type == Type_c)
{                
                if(!Cng_Build(S8("Sampler"), NilP(cng_build_options), S8ArrayLit(Path)).Succeeded) 
                    goto Fail;
            }
            else if(Type == Type_asm)
            {
                if(OS_Is(OSKind_Linux))
{
                    Log("[Sampler asm51]\n");
                    
                    Log("Preprocessing %S\n", FileName);
                    cng_command_result CommandResult = Cng_RunCommand(Str8Fmt("./pp -o=%S.pp %S", FileNameWithoutExt, Path));
                    if(!CommandResult.Succeeded)
                        goto Fail;
                    
                    Log("Compiling %S\n", FileName);
                    if(!Cng_RunCommand(Str8Fmt("asm51 -n -a -o %S.hex %S.pp", 
                                               FileNameWithoutExt, FileNameWithoutExt)).Succeeded) 
                    goto Fail;
}
            }
  }
  
  if(Run)
        {
            if(0) {}
            else if(Type == Type_c)
{
            str8 ExeFilePath = S8Cat(S8("." SLASH), FileNameWithoutExt);
   Cng_RunCommand(ExeFilePath);
            }
            else if(Type == Type_asm)
            {
                Cng_RunCommand(Str8Fmt("." SLASH "lsd -run %S.hex", FileNameWithoutExt));
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
    
    Fail:;
    
 return 0;
}