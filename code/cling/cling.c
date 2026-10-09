//~ Defines
#define CLING_BUILD_PATH "../build/"

//~ Libaries
#define BASE_FORCE_THREADS_COUNT 1
#define BASE_CONSOLE_APPLICATION 1

#include "base/base.h"
#include "base/base.c"

NO_WARNINGS_BEGIN
#include "lib/metadesk/md.h"
#include "lib/metadesk/md.c"
NO_WARNINGS_END

#define CLING_CODE_PATH "../code/"
#define CLING_SOURCE_PATH CLING_CODE_PATH "cling/cling.c"
#include "cling/cling.h"
#include "cling/generated/cling.meta.c"

//~ Globals
global_variable MD_Arena *GlobalMDArena = 0;

//~ Functions
internal void
RunCommand(void)
{
 str8 BuildCommand = Cng_S8ArrayJoin(' ');
 if(0)
 {
  Log("%S\n", BuildCommand);
 }
 Cng_RunCommand(BuildCommand);
}

internal void
WriteStreamToFile(MD_String8List Stream, char *FileName)
{
 MD_String8 Str = MD_S8ListJoin(GlobalMDArena, Stream, 0);
 if(Str.Size)
 {
  OS_WriteEntireFile(FileName, Str);
 }
}

internal u8
HexToByte(u8 Char)
{
 u8 Byte = 0;
 if(Char >= 'a' && Char <= 'f') Byte = ((Char - 'a') + 10);
 if(Char >= 'A' && Char <= 'F') Byte = ((Char - 'A') + 10);
 if(Char >= '0' && Char <= '9') Byte = ((Char - '0') + 0);
 return Byte;
}

internal void 
WindowsBuild(str8 Source, str8_array *ExtraCompilerFlags, str8 ExtraLinkerFlags, 
             b32 Debug, b32 Asan)
{
 Cng_Scratch();
 
 Cng_S8ArrayAppend(S8("cmd /c \"C:\\msvc\\setup_x64.bat"));
 Cng_S8ArrayAppend(S8("&&"));
 
 Cng_S8ArrayAppend(S8("cl"));
 Cng_S8ArrayAppend(S8("-MTd -Gm- -nologo -GR- -EHa- -Oi -FC -Z7"));
 
 if(Asan)
 {
  Cng_S8ArrayAppend(S8("/fsanitize=address"));
 }
 
 if(!Debug)
 {
  Cng_S8ArrayAppend(S8("-O2"));
 }
 
 Cng_S8ArrayAppend(S8("-Zc:strictStrings-"));
 Cng_S8ArrayAppend(S8("-WX -W4 -wd4459 -wd4201 -wd4100 -wd4101 -wd4189 -wd4505 -wd4996 -wd4389 -wd4244 -wd5287 -wd4063 -wd4127"));
 
 Cng_S8ArrayAppend(Cng_S8ArrayJoinFrom(ExtraCompilerFlags, ' '));
 Cng_S8ArrayAppend(Source);
 Cng_S8ArrayAppend(ExtraLinkerFlags);
 
 RunCommand();
}

internal void
LinuxBuildCommand(str8 Source, 
                  str8 OutputName, 
                  b32 GCC, b32 Clang, b32 Asan, b32 Debug,
                  str8_array *ExtraFlags,
                  b32 Run)
{
 Log("%S\n", Cng_GetBaseFileName(Source));
 
 Cng_Scratch();
 Cng_CommonBuildCommand(GCC, Clang, Debug, Asan);
 Cng_S8ArrayAppendMultiple(Cng_S8ArrayJoinFrom(ExtraFlags, ' '),
                           S8("-o"), OutputName,
                           Source);
 
 if(Run)
 {
  RunCommand();
 }
}

internal void
BuildAndRun(str8 Source, 
            str8 OutputName, 
            b32 GCC, b32 Clang, b32 Asan, b32 Debug,
            str8_array *ExtraFlags,
            str8 ExtraLinkerFlags,
            str8 ExtraSource,
            b32 IsDLL, b32 IsObject)
{
 if(0) {}
 else if(Cng_IsOs(CngOS_Linux))
 {
  if(0) {}
  else if(IsObject)
  {
   Cng_S8ArrayAppendTo(ExtraFlags, S8("-fPIC -x c++ -c"));
   OutputName = Str8Fmt("%S.o", OutputName);
  }
  else if(IsDLL)
  {
   Cng_S8ArrayAppendTo(ExtraFlags, S8("-fPIC --shared"));
   OutputName = Str8Fmt("%S.so", OutputName);
  }
  
  Cng_S8ArrayAppendTo(ExtraFlags, ExtraLinkerFlags);
  LinuxBuildCommand(Source, OutputName, GCC, Clang, Asan, Debug, ExtraFlags, false);
  Cng_S8ArrayAppend(ExtraSource);
  RunCommand();
 }
 else if(Cng_IsOs(CngOS_Windows))
 {
  if(0) {}
  else if(IsObject)
  {
   Cng_S8ArrayAppendTo(ExtraFlags, S8("-c /TP"));
   Cng_S8ArrayAppendTo(ExtraFlags, Str8Fmt("-Fo" "%S.obj", OutputName));
  }
  else if(IsDLL)
  {
   Cng_S8ArrayAppendTo(ExtraFlags, S8("/LD"));
   ExtraLinkerFlags = Str8Fmt("%S /DLL /OUT:%S.dll",
                              ExtraLinkerFlags, OutputName);
  }
  else
  {
   Cng_S8ArrayAppendTo(ExtraFlags, Str8Fmt("-Fe%S.exe", OutputName));
  }
  
  ExtraLinkerFlags = Str8Fmt("/link -opt:ref -incremental:no %S ", ExtraLinkerFlags);
  
  Cng_S8ArrayAppendTo(ExtraFlags, Str8Fmt("-Fm" "%S.map", OutputName));
  Cng_S8ArrayAppendTo(ExtraFlags, ExtraSource);
  WindowsBuild(Source, ExtraFlags, ExtraLinkerFlags, Debug, Asan);
 }
}

//- Metadesk library helpers 

typedef struct MD_String8ListNode MD_String8ListNode;
struct MD_String8ListNode
{
 MD_String8List V;
 MD_String8ListNode *Prev;
 str8 FileName;
};

global_variable MD_String8ListNode *S8ListTop;

internal void
S8ListPush(MD_String8 String)
{
 MD_S8ListPush(GlobalMDArena, &S8ListTop->V, String);
}

internal void
PushStream(str8 FileName)
{
 MD_String8List *Result = 0;
 
 arena *Arena = GetScratch();
 MD_String8ListNode *List = PushArrayZero(Arena, MD_String8ListNode, 1);
 List->Prev = S8ListTop;
 
 List->FileName = FileName;
 
 S8ListTop = List;
}

internal void
PopStream(str8 NewDirPath)
{
 str8 OutPath = Str8Fmt("%S/%S", NewDirPath, S8ListTop->FileName);
 
 WriteStreamToFile(S8ListTop->V, (char *)OutPath.Data);
 
 S8ListTop = S8ListTop->Prev;
}

internal void
S8ListPushFmt(char *Format, ...)
{
 va_list Args;
 va_start(Args, Format);
 MD_String8 String = MD_S8FmtV(GlobalMDArena, Format, Args);
 va_end(Args);
 S8ListPush(String);
}

//- Custom build params
internal void 
LogBuildMode(str8 Name, b32 Debug)
{
 str8 ModeString = (Debug ? S8("debug") : S8("release"));
 Log("[%S (%S)]\n", Name, ModeString);
}

internal void
ApplicationBuild(str8 ExeName, str8 AppName, str8 AppSource, str8 WindowName,
                 b32 GCC, b32 Clang, b32 Asan, b32 Debug, b32 Slow)
{
 LogBuildMode(ExeName,  Debug);
 
 // Compile
 {            
  str8_array *CommonFlags = Cng_PushS8Array(256);
  Cng_S8ArrayAppendTo(CommonFlags, 
                      Str8Fmt("-I" CLING_CODE_PATH " "
                              "-DRL_PLATFORM_APP_NAME=%S "
                              "-DRL_PLATFORM_WINDOW_NAME=%S",
                              AppName, WindowName));
  
  char *LibsFileName = (Cng_IsOs(CngOS_Windows) ? 
                        CLING_BUILD_PATH "rl_libs.obj" :
                        (Cng_IsOs(CngOS_Linux) ?
                         CLING_BUILD_PATH "rl_libs.o" :
                         0));
  
  if(!Slow && !OS_FileExists(LibsFileName))
  {
   Cng_S8ArrayTemp(CommonFlags)
   {
    Cng_S8ArrayAppendTo(CommonFlags, S8("-DRL_LIBS_IMPLEMENTATION=1"));
    
    BuildAndRun(S8("../code/rl/rl_libs.h"), 
                S8("rl_libs"), 
                GCC, Clang, Asan, Debug,
                CommonFlags, S8(""), S8(""),
                false, true);
   }
  }
  
  str8 LibsName = Slow ?  S8("") : S8FromCString(LibsFileName);
  
  Cng_S8ArrayTemp(CommonFlags)
  {
   str8 ExtraLinkerFlags = {0};
   
   Cng_S8ArrayAppendTo(CommonFlags, (Slow ? 
                                     S8("-DRL_LIBS_IMPLEMENTATION=1") :
                                     S8("-DRL_LIBS_IMPLEMENTATION=0")));
   
   BuildAndRun(AppSource, 
               AppName,
               GCC, Clang, Asan, Debug,
               CommonFlags, ExtraLinkerFlags, LibsName,
               true, false);
  }
  
  Cng_S8ArrayTemp(CommonFlags)
  {
   str8 ExtraLinkerFlags = {0};
   Cng_S8ArrayAppendTo(CommonFlags, (Slow ? 
                                     S8("-DRL_LIBS_IMPLEMENTATION=1") :
                                     S8("-DRL_LIBS_IMPLEMENTATION=0")));
   
   if(0) {}
   else if(Cng_IsOs(CngOS_Linux))
   {
    ExtraLinkerFlags = S8("-lasound -lX11 -lGL -lGLX");
   }
   
   BuildAndRun(S8("../code/rl/rl_platform.c"),
               ExeName,
               GCC, Clang, Asan, Debug,
               CommonFlags, ExtraLinkerFlags, LibsName, 
               false, false);
  }
  
 }
 
}

internal b32
ConfigMatch(MD_String8 Name, MD_String8 Match, MD_String8 Value)
{
 b32 Result = (MD_S8Match(Name, Match, 0) && 
               MD_S8Match(Value, S8("1"), 0));
 return Result;
}

internal b32
CmdLineHasEntryAndAddHelp(cmd_line *CmdLine, str8_array *Array, str8 Flag, str8 Help)
{
 b32 Result = false;
 
 str8 Pad = S8("                        ");
 if(Flag.Size < Pad.Size) Pad.Size -= Flag.Size;
 
 Cng_S8ArrayAppendTo(Array, Str8Fmt("-%S %S: %S", Flag, Pad, Help));
 Result = CmdLineHasEntry(CmdLine, Flag);
 
 return Result;
}

raddbg_entry_point(EntryPoint);
ENTRY_POINT(EntryPoint)
{
 if(LaneIndex() == 0)
 {
  Cng_InitAndRebuildSelf(Params->ArgCount, Params->Args, Params->Env);
  
  // Change to build directory
  {        
   str8 Path = Cng_PathFromExe(S8FromCString(Params->Args[0]), S8(CLING_BUILD_PATH));
   OS_ChangeDirectory((char *)Path.Data);
  }
  
  // Globals
  {        
   GlobalMDArena = MD_ArenaAlloc();
  }
  
  //~ Help
  str8_array *Help = Cng_PushS8Array(CLING_DEFAULT_S8ARRAY_CAPACITY);
  Cng_S8ArrayAppendTo(Help, S8("Build parameters:"));
  
  //~ Config 
  Cng_ConfigBools(&Params->CmdLine, Help);
  
  Cng_S8ArrayAppendTo(Help, S8("\nTargets:"));
  
  MD_String8 FileName = S8("../code/cling/cling.mdesk");
  MD_ParseResult Parse = MD_ParseWholeFile(GlobalMDArena, FileName);
  
  for(MD_Message *Message = Parse.errors.first; Message; Message = Message->next)
  {
   MD_CodeLoc Loc = MD_CodeLocFromNode(Message->node);
   MD_PrintMessage(stderr, Loc, Message->kind, Message->string);
  }
  
  if(Parse.errors.max_message_kind < MD_MessageKind_Error)
  {
   MD_Node *Root = Parse.node->first_child;
   for(MD_EachNode(Table, Root))
   {
    if(S8Match(Table->string, S8("Config"), false))
    {
     for(MD_EachNode(Node, Table->first_child))
     {      
      MD_String8 Name = MD_NodeAtIndex(Node->first_child, 0)->string;
      MD_String8 Value = MD_NodeAtIndex(Node->first_child, 1)->string;
      Cng_ConfigMatchers(Name, Value);
     }
     break;
    }
    
   }
  }
  
  b32 DidWork = false;
  b32 Debug = !Release;
  Slow |= Asan;
  if(GCC) Clang = false;
  
  cmd_line *CmdLine = &Params->CmdLine;
  
  //~ Metaprogram
  if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("Meta"), S8("Search all .mdesk files and generate code accordingly")))
  {
   DidWork = true;
   
   Log("Generating code...\n");
   
   arena *Arena = GlobalClingArena;
   
   u64 HashArraySize = KB(4);
   table_hash_node *TableHashArray = PushArrayZero(Arena, table_hash_node, HashArraySize);
   field_hash_node *FieldHashArray = PushArrayZero(Arena, field_hash_node, HashArraySize);
   
   for(os_dir *Dir = OS_WalkDirPush(S8("../code"), 0);
       Dir != 0;
       )
   {
    os_dir_entry *Entry = OS_WalkDirGetNext(Dir);
    
    if(Entry != 0)
    {
     str8 Name = Entry->Name;
     
     b32 InvalidDir = (S8Match(Entry->Name, S8("."), false) || 
                       S8Match(Entry->Name, S8(".."), false));
     if(!InvalidDir)
     {
      if(0) {}
      else if(Entry->Kind == OS_DirEntryKind_Directory)
      {
       Dir = OS_WalkDirPush(Entry->Name, Dir);
      }
      else if(Entry->Kind == OS_DirEntryKind_File)
      {
       str8 Ext = S8(".mdesk");
       str8 Scan = S8From(Name, Name.Size - Ext.Size);
       
       b32 Match = (Name.Size > Ext.Size &&
                    S8Match(Scan, Ext, false));
       
       if(Match)
       {
        str8 NewDirPath = Str8Fmt("%S/generated", Dir->Path);
        OS_MakeDirectory((char *)NewDirPath.Data);
        
        str8 BaseName = S8To(Name, Name.Size - Ext.Size);
        str8 OutFile = Str8Fmt("%S.meta.c", BaseName); 
        
        MD_String8 FilePath = Str8Fmt("%S/%S", Dir->Path, Name);
        MD_ParseResult FileParse = MD_ParseWholeFile(GlobalMDArena, FilePath);
        
        // Print metadesk errors
        for(MD_Message *Message = FileParse.errors.first;
            Message != 0;
            Message = Message->next)
        {
         MD_CodeLoc code_loc = MD_CodeLocFromNode(Message->node);
         MD_PrintMessage(stderr, code_loc, Message->kind, Message->string);
        }
        if(FileParse.errors.max_message_kind < MD_MessageKind_Error)
        {
         MD_Node *Root = FileParse.node->first_child;
         
         PushStream(OutFile);
         
         for(MD_EachNode(Node, Root))
         {
          //- NOTE(luca): Header "file" tag  
          b32 FilePushed = false;
          str8 FilePushedName = {0};
          {
           MD_Node *FileTag = MD_TagFromString(Node, S8("file"), 0);
           if(!MD_NodeIsNil(FileTag))
           {
            FilePushedName = FileTag->first_child->string;
            PushStream(FilePushedName);
            FilePushed = true;
           }
          }
          
          //- NOTE(luca): Add tables
          {
           MD_Node *TableTag = MD_TagFromString(Node, S8("table"), 0);
           if(!MD_NodeIsNil(TableTag))
           {
            str8 TableName = Node->string;
            table_hash_node *TableHash = AddTableHashNode(Arena, TableName, 0, 
                                                          HashArraySize, TableHashArray);
            TableHash->Table = Node;
            
            u64 FieldIdx = 0;
            for(MD_EachNode(Field, TableTag->first_child))
            {
             AddFieldHashNode(Arena, Field->string, TableHash->Key, 
                              HashArraySize, FieldHashArray)->Idx = FieldIdx;
             
             FieldIdx += 1;
            }
           }
          }
          
          //- NOTE(luca): Gen data
          {
           b32 ToLower = MD_NodeHasTag(Node, S8("to_lower"), 0);
           b32 GenData = MD_NodeHasTag(Node, S8("data"), 0);
           b32 NoPadding = MD_NodeHasTag(Node, S8("no_padding"), 0);
           MD_Node *GenDataTable = MD_TagFromString(Node, S8("data_table"), 0);
           MD_Node *GenDataEnum = MD_TagFromString(Node, S8("data_enum"), 0);
           MD_Node *GenDataStruct = MD_TagFromString(Node, S8("data_struct"), 0);
           str8 TypeName = {0};
           
           GenData |= (!MD_NodeIsNil(GenDataTable));
           GenData |= (!MD_NodeIsNil(GenDataEnum));
           GenData |= (!MD_NodeIsNil(GenDataStruct));
           
           if(GenData)
           {
            // Add headers
            {                                                
             if(NoPadding)
             {
              S8ListPush(S8("NO_STRUCT_PADDING_BEGIN\n"));
             }
             
             if(!MD_NodeIsNil(GenDataTable))
             {
              TypeName = GenDataTable->first_child->string;
              S8ListPush(Str8Fmt("%S %S[] = {\n", TypeName, Node->string)); 
             }
             
             if(!MD_NodeIsNil(GenDataStruct))
             {
              TypeName = GenDataStruct->first_child->string;
              S8ListPush(Str8Fmt("typedef struct %S %S;\n"
                                 "struct %S\n"
                                 "{\n", TypeName, TypeName, TypeName)); 
             }
             
             if(!MD_NodeIsNil(GenDataEnum))
             {
              TypeName = GenDataEnum->first_child->string;
              S8ListPush(Str8Fmt("enum %S\n"
                                 "{\n", TypeName, TypeName, TypeName)); 
              
             }
            }
            
            for(MD_EachNode(Row, Node->first_child))
            {
             MD_Node *ExpandTag = MD_TagFromString(Row, S8("expand"), 0);
             b32 GenStrings = MD_NodeHasTag(Row, S8("strings"), 0);
             if(!MD_NodeIsNil(ExpandTag))
             {
              str8 TableName = ExpandTag->first_child->string;
              
              table_hash_node *TableHash = GetTableHashNode(TableName, 0, HashArraySize, TableHashArray);
              Assert(TableHash);
              
              u64 RowCount = 0;
              for(MD_EachNode(TableRow, TableHash->Table->first_child))
              {
               
               str8 String;
               
               S8OverPush(Arena, String, KB(4), String.Size)
               {
                u64 StringIdx = 0;
                
                str8 RowString = Row->string;
                
                for EachIndex(Idx, Row->string.Size)
                {
                 if(RowString.Data[Idx] == '$')
                 {
                  Idx += 1;
                  
                  if(RowString.Data[Idx] == '(')
                  {
                   Idx += 1;
                   
                   u64 Start = Idx;
                   
                   while(RowString.Data[Idx] != ')') Idx += 1;
                   
                   str8 FieldName = S8FromTo(RowString, Start, Idx); 
                   str8 OutString = {0};
                   
                   if(S8Match(FieldName, S8("_Idx"), false))
                   {
                    OutString = Str8Fmt("%llu", RowCount);
                   }
                   else
                   {
                    field_hash_node *FieldHash = GetFieldHashNode(FieldName, TableHash->Key, HashArraySize, FieldHashArray);
                    MD_Node *Field = MD_NodeAtIndex(TableRow->first_child, (int)FieldHash->Idx);
                    
                    if(GenStrings)
                    {
                     OutString = Str8Fmt("(u8 *)\"%S\", %llu", 
                                         Field->string, Field->string.Size);
                    }
                    else
                    {
                     OutString = Field->string;
                    }
                    
                   }
                   
                   // Output that string
                   {                 
                    for EachIndex(CharIdx, OutString.Size)
                    {
                     u8 Char = OutString.Data[CharIdx];
                     if(ToLower)
                     {
                      Char = ToLowercase(Char);
                     }
                     String.Data[StringIdx] = Char;
                     StringIdx += 1;
                    }
                   }
                   
                   if(OutString.Size == 0)
                   {
                    Log("ERROR: field \"%S\" not found\n", FieldName);
                   }
                   
                  }
                 }
                 else
                 {
                  String.Data[StringIdx] = RowString.Data[Idx];
                  StringIdx += 1;
                 }
                }
                
                String.Size = StringIdx;
               }
               
               S8ListPush(String);
               
               RowCount += 1;
              }
              
             }
             else
             {
              S8ListPush(Row->string);
             }
            }
            
            // Add footers 
            {           
             if(!MD_NodeIsNil(GenDataTable) ||
                !MD_NodeIsNil(GenDataStruct))
             {
              S8ListPush(S8("\n};\n")); 
             }
             
             if(!MD_NodeIsNil(GenDataEnum))
             {
              S8ListPush(Str8Fmt("\n};\n"
                                 "typedef enum %S %S;\n", TypeName, TypeName));
             }
             
             if(NoPadding)
             {
              S8ListPush(S8("NO_STRUCT_PADDING_END\n"));
             }
             
            }
            
            // Pretty
            S8ListPush(S8("\n"));
           }
          }
          
          //- NOTE(luca): Footer "file" tag  
          if(FilePushed)
          {
           PopStream(NewDirPath);
          }
          //- 
         }
         
         PopStream(NewDirPath);
        }
       }
      }
     }
    }
    else
    {
     Dir = OS_WalkDirPop(Dir);
    }
   }
  }
  
  //~ Haversine Generator
  if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("HaversineGenerator"), S8("Target")))
  {
   DidWork = true;
   
   LogBuildMode(S8("Haversine generator"), Debug);
   
   Cng_Scratch();
   Cng_CommonBuildCommand(GCC, Clang, Debug, Asan);
   Cng_S8ArrayAppend(S8("../code/compenhance/haversine_generator/haversine_generator.c"));
   RunCommand();
  }
  
  //~ Haversine Processor
  if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("HaversineProcessor"), S8("Target")))
  {
   DidWork = true;
   
   LogBuildMode(S8("Haversine processor"), Debug);
   
   Cng_Scratch();
   Cng_CommonBuildCommand(GCC, Clang, Debug, Asan);
   Cng_S8ArrayAppend(S8("../code/compenhance/haversine_processor/haversine_processor.c"));
   
   if(Cng_IsOs(CngOS_Windows))
   {
    Cng_S8ArrayAppend(S8("/link -opt:ref -incremental:no"));
   }
   
   RunCommand();
  }
  
  //~ Sim86
  if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("Sim86"), S8("Target")))
  {
   DidWork = true;
   
   LogBuildMode(S8("sim86"), Debug);
   
   Cng_Scratch();
   Cng_CommonBuildCommand(GCC, Clang, Debug, Asan);
   if(Cng_IsOs(CngOS_Windows))
   {
    Cng_S8ArrayAppend(S8("/std:c++20"));
   }
   
   Cng_S8ArrayAppend(S8("../code/compenhance/sim86/sim86.cpp"));
   
   RunCommand();
   
  }
  
  //~ Editor
  if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("Editor"), S8("Target")))
  {
   DidWork = true;
   
   ApplicationBuild(S8("editor"), S8("editor_app"), 
                    S8("../code/editor/editor_app.c"), S8("Editor"),
                    GCC, Clang, Asan, Debug, Slow);
  }
  
  //~ Muze
  if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("Muze"), S8("Target")))
  {
   DidWork = true;
   
   ApplicationBuild(S8("muze"), S8("muze_app"), 
                    S8("../code/muze/muze_app.c"), S8("Muze"),
                    GCC, Clang, Asan, Debug, Slow);
  }
  
  //~ School
  if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("School"), S8("(see subcommands with -School)")))
  {
   Cng_Scratch();
   
   Cng_S8ArrayAppendTo(Help, S8("\nSchool:"));
   
   b32 Compile  = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("compile"), S8("Compile the source file"));
   b32 Run      = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("run"), S8("Run the built program"));
   b32 Last     = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("last"), S8("Use the last found source file as input"));
   b32 All      = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("all"), S8("Use all source files as input"));
   b32 Template = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("template"), S8("Create a template source file"));
   b32 Print    = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("print"), S8("Print the source file path"));
   b32 Convert  = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("convert"), S8("Convert spaces to tabs in the source file"));
   
   if(Compile)
   {
    LogBuildMode(S8("Les1"), Debug);
   }
   
   if(All) Last = true;
   
   str8 TemplateText = S8(
                          "#include <stdio.h>\n"
                          "#define _CRT_SECURE_NO_WARNINGS\n"
                          "\n"
                          "int main(int ArgCount, char **Args)\n"
                          "{\n"
                          "\tprintf(\"Hello, world!\\n\");\n"
                          "\treturn 0;\n"
                          "}\n"
                          );
   
   // Sanitization
   {
    b32 Error = false;
    if(Template)
    {
     if(Compile) { Log("ERROR: -compile is incompatible with -template.\n"); Error = true; } 
     if(Run)     { Log("ERROR: -run is incompatible with -template.\n"); Error = true; }
    }
    
    if(Error)
    {
     Compile = false;
     Run = false;
     Template = false;
     Last = false;
    }
   }
   
   str8 NamePrefix = S8("raymaekersLuca");
   str8 Hoofdstuk = S8("6");
   
   char *PathFormat = (char *)Str8Fmt("../code/school/les1_%S/%S_%S_%%S.c", 
                                      Hoofdstuk, NamePrefix, Hoofdstuk).Data;
   
   if(Last)
   {
    u64 MaxLastIdx = 64;
    u64 LastIdx = 1;
    for(u64 Idx = LastIdx; Idx < MaxLastIdx; Idx += 1)
    {
     str8 LastIdxString = Str8Fmt("%llu", Idx);
     
     str8 Path = Str8Fmt(PathFormat, LastIdxString);
     if(OS_FileExists((char *)Path.Data))
     {
      str8_node *Node = PushArray(GlobalClingArena, str8_node, 1);
      DLLAppend(Node, CmdLine->FirstFile, CmdLine->LastFile);
      Node->V = LastIdxString; 
      
      LastIdx = Idx;
     }
     
     Assert(LastIdx < MaxLastIdx);
    }
   }
   
   b32 StartAtLast = (Last && !All);
   str8_node *First = (!StartAtLast ? CmdLine->FirstFile : CmdLine->LastFile);
   
   for EachNode(Node, str8_node, First)
   {
    str8 Path = Str8Fmt(PathFormat, Node->V);
    
    if(Print)
    {
     Log("%S\n", Path);
    }
    
    if(Template)
    {
     OS_WriteEntireFile((char *)Path.Data, TemplateText);
    }
    
    if(Convert)
    {
     str8 File = OS_ReadEntireFileIntoMemory((char *)Path.Data);
     
     str8 Out = PushS8(GlobalClingArena, File.Size);
     
     // Convert tab to spaces.
     b32 OnStartOfLine = true;
     for EachIndex(Idx, File.Size)
     {
      u8 Char = File.Data[Idx];
      
      if(0) {}
      else if(Char == '\n') OnStartOfLine = true;
      else if(Char != ' ') OnStartOfLine = false;
      else
      {
       if(OnStartOfLine) Char = '\t';
      }
      
      Out.Data[Idx] = Char;
     }
     
     OS_WriteEntireFile((char *)Path.Data, Out);
     OS_FreeFileMemory(File);
    }
    
    if(Compile)
    {
     Cng_S8ArrayTemp(GlobalSelectedArray)
     {
      Cng_CommonBuildCommand(GCC, Clang, Debug, Asan);
      Cng_S8ArrayAppend(Path);
      RunCommand();
     }
    }
    
    if(Run)
    {
     Cng_S8ArrayTemp(GlobalSelectedArray)
     {
      str8 BuildPath = Str8Fmt("../build/%S_%S_%S.exe", NamePrefix, Hoofdstuk, Node->V);
      
      Cng_S8ArrayAppend(BuildPath);
      RunCommand();
     }
    }
    
    DidWork = true;
   }
   
  }
  
  if(DidWork)
  {
   Log("Done.\n");
  }
  else
  {
   str8 HelpText = Cng_S8ArrayJoinFrom(Help, '\n');
   Log("\nNo target provided.\n\n%S\n", HelpText);
  }
 }
 
 return 0;
}
