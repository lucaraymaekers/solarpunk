
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

//~ Types
typedef struct table_hash_node table_hash_node;
struct table_hash_node
{
 MD_Node *Table;
 
 u64 Key;
 table_hash_node *Next;
};

typedef struct field_hash_node field_hash_node;
struct field_hash_node
{
 u64 Idx;
 
 u64 Key;
 field_hash_node *Next;
};

#include "cling/generated/cling.meta.c"

//~ Globals
global_variable MD_Arena *GlobalMDArena = 0;

//~ Functions
internal void
RunCommand(void)
{
 str8 BuildCommand = Cng_S8ArrayJoin(' ');
#if 0
 Log("%S\n", BuildCommand);
#endif
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

//~ Metadesk library helpers 

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

internal b32
ConfigMatch(MD_String8 Name, MD_String8 Match, MD_String8 Value)
{
 b32 Result = (MD_S8Match(Name, Match, 0) && 
               MD_S8Match(Value, S8("1"), 0));
 return Result;
}

//~ Build helpers

internal void
AppBuild(cng_build_options *BuildOptions, 
         str8 Name, str8 AppSource, str8 AppName, str8 ExeName)
{
 str8 LibFileName = S8("../build/rl_libs.obj");
 
 if(!OS_FileExists(CSS8(LibFileName)))
 {
  Cng_Build(Name, BuildOptions, 
            S8ArrayLit(S8("../code/rl/rl_libs.h")),
            .Type = CngBuildType_OBJ,
            .ExtraCompilerFlags = S8ArrayLit(S8("-DRL_LIBS_IMPLEMENTATION=1")),
            .OutputName = LibFileName);
 }
 
 Cng_Build(Name, BuildOptions, 
           S8ArrayLit(AppSource, LibFileName),
           .Type = CngBuildType_DLL,
           .ExtraCompilerFlags = S8ArrayLit(S8("-DRL_LIBS_IMPLEMENTATION=0")));
 
 str8 Defines = Str8Fmt("-DRL_PLATFORM_APP_NAME=%S "
                        "-DRL_PLATFORM_WINDOW_NAME=%S",
                        AppName, ExeName);
 
 str8 LinkerFlags = {0};
 if(Cng_IsOS(CngOS_Linux)) LinkerFlags = S8("-lasound -lX11 -lGL -lGLX");
 
 Cng_Build(Name, BuildOptions, 
           S8ArrayLit(S8("../code/rl/rl_platform.c"),
                      LibFileName),
           .ExtraCompilerFlags = S8ArrayLit(S8("-DRL_LIBS_IMPLEMENTATION=0"),
                                            Defines),
           .ExtraLinkerFlags   = S8ArrayLit(LinkerFlags),
           .OutputName = ExeName);
 
};

//~ Entrypoint
BASE_ENTRY_POINT()
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
 str8_array Help = Cng_PushS8Array(CLING_DEFAULT_S8ARRAY_CAPACITY);
 S8ArrayAppendTo(&Help, S8("Build parameters:"));
 
 //~ Config 
 cng_build_options BuildOptions = {0};
 {
  Cng_ConfigBools(&Params->CmdLine, &Help);
  
  S8ArrayAppendTo(&Help, S8("\nTargets:"));
  
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
  
  Slow |= Asan;
  
  if(GCC) BuildOptions.Compiler = CngCompiler_GCC;
  if(Clang) BuildOptions.Compiler = CngCompiler_Clang;
  
  BuildOptions.Asan = Asan;
  BuildOptions.Release = Release;
  BuildOptions.Slow = Slow;
 }
 
 b32 DidWork = false;
 cmd_line *CmdLine = &Params->CmdLine;
 
 //~ Metaprogram
 if(CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("Meta"), S8("Search all .mdesk files and generate code accordingly")))
 {
  DidWork = true;
  
  Log("Generating code...\n");
  
  arena *Arena = GetScratch();
  
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
            b32 GenCStringLiteral = MD_NodeHasTag(Row, S8("cstring_lit"), 0);
            
            if(!MD_NodeIsNil(ExpandTag))
            {
             str8 TableName = ExpandTag->first_child->string;
             
             table_hash_node *TableHash = GetTableHashNode(TableName, 0, HashArraySize, TableHashArray);
             Assert(TableHash);
             
             u64 RowCount = 0;
             for(MD_EachNode(TableRow, TableHash->Table->first_child))
             {
              
              str8 String = {0};
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
            else if(GenCStringLiteral)
            {
             u64 Extra = 0;
             str8 String = {0};
             S8OverPush(Arena, String, KB(4), String.Size)
             {
              String.Size = 0;
              
              String.Data[String.Size] = '"';
              String.Size += 1;
              Extra += 1;
              
              u8 Char = 0;
              for EachIndex(Idx, Row->string.Size)
              {
               if(Char == '\n')
               {
                String.Data[String.Size] = '"';
                String.Size += 1;
                Extra += 1;
               }
               
               Char = Row->string.Data[Idx];
               
               if(0) {}
               else if(Char == '"' || Char == '\\')
               {
                String.Data[String.Size] = '\\';
                String.Size += 1;
                Extra += 1;
               }
               else if(Char == '\n')
               {
                String.Data[String.Size] = '\\';
                String.Size += 1;
                String.Data[String.Size] = 'n';
                String.Size += 1;
                String.Data[String.Size] = '"';
                String.Size += 1;
                Extra += 3; // one for the newline and one for the quote
                // One more for the newline that's coming
               }
               
               String.Data[String.Size] = Char;
               String.Size += 1;
              }
              String.Data[String.Size] = '"';
              String.Size += 1;
              Extra += 1;
              
              S8ListPush(String);
             }
             
             if(GenStrings)
             {
              S8ListPush(Str8Fmt(", %llu", String.Size - Extra));
             }
             
            }
            else
            {
             S8ListPush(Row->string);
            }
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
 
 //~ Editor 
 if(CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("Editor"), S8("Target")))
 {
  DidWork = true;
  AppBuild(&BuildOptions, S8("Editor"), S8("../code/editor/editor_app.c"), S8("editor_app"), S8("editor"));
 }
 
 //~ Muze
 if(CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("Muze"), S8("Target")))
 {
  DidWork = true;
  AppBuild(&BuildOptions, S8("Muze"), S8("../code/muze/muze_app.c"), S8("muze_app"), S8("muze"));
 }
 
 //~ Sim86
 if(CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("Sim86"), S8("Target")))
 {
  DidWork = true;
  
  cng_build_options Options = BuildOptions;
  Options.CPP = true;
  Cng_Build(S8("Sim86"), &Options, S8ArrayLit(S8("../code/compenhance/sim86/sim86.cpp")));
 }
 
 //~ Haversine Generator
 if(CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("HaversineGenerator"), S8("Target")))
 {
  DidWork = true;
  
  Cng_Build(S8("Haversine Generator"), &BuildOptions,
            S8ArrayLit(S8("../code/compenhance/haversine_generator/haversine_generator.c")));
 }
 
 //~ Haversine Processor
 if(CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("HaversineProcessor"), S8("Target")))
 {
  DidWork = true;
  
  Cng_Build(S8("Haversine Processor"), &BuildOptions,
            S8ArrayLit(S8("../code/compenhance/haversine_processor/haversine_processor.c")));
  
 }
 
 //~ Linux Serial Downloader
 if(CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("LSD"), S8("Target")))
 {
  DidWork = true;
  Cng_Build(S8("LSD"), &BuildOptions, S8ArrayLit(S8("../code/lsd/lsd.c")));
 }
 
 //~ Samples: ADuC 832
 if(CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("Aduc"), S8("Target")))
 {
  DidWork = true;
#define Cng_Run(String) Cng_RunCommand(S8(String)).Succeeded
  
  Log("[Aduc]\n");
  
  if(0) {}
  else if(Cng_IsOS(CngOS_Windows))
  {   
   Log("Compiling main_sdcc.asm");
   Cng_Run("c51asm.exe "
           "--nopaging --quiet --columns "
           "--outfile main_sdcc.hex --include ../code/school/aduc832 " "../code/school/aduc832/main_sdcc.asm") &&
    Cng_Run("lsd -run main_sdcc.hex");
  }
  else if(Cng_IsOS(CngOS_Linux))
  {
   Log("Compiling main.asm\n");
   Cng_Run("asm51 -n -a -o main.hex ../code/school/aduc832/main.asm") &&
    Cng_Run("./lsd -run main.hex");
  }
  
 }
 
 //~ WSDCOM
 S8ArrayAppendTo(&Help, S8(" "));
 if(CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("WSD"), S8("(See subcommands with -WSD)")))
 {
  if(CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("compile"), S8("Compile wsdcom")))
  {   
   DidWork = true;
   Cng_Build(S8("WSDCOM"), &BuildOptions, S8ArrayLit(S8("../code/wsdcom/wsdcom.c")));
  }
  
  if(CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("copy"), S8("Copy configuration files.")))
  {
   DidWork = true;
   OS_MakeDirectory("Configuration");
   Cng_RunCommand(S8("cmd.exe /c copy ..\\code\\wsdcom\\Configuration Configuration"));
  }
 }
 S8ArrayAppendTo(&Help, S8(" "));
 
 //~ ZSH
 if(CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("Zsh"), S8("Print out zsh completions")))
 {
  DidWork = true;
  str8 Text = S8("_cling() \n"
                 "{\n"
                 "local cmd=\"${words[1]}\"\n"
                 "local help_output line token desc\n"
                 "local -a candidates\n"
                 "\n"
                 "help_output=\"$(CLING_NOREBUILD=1 \"$cmd\" -h 2>&1)\"\n"
                 "\n"
                 "# Extract entries formatted like:\n"
                 "# -Asan : Enable address sanitizer\n"
                 "while IFS= read -r line; do\n"
                 "if [[ \"$line\" =~ '^[[:space:]]*([^[:space:]]+)[[:space:]]*:[[:space:]]*(.*)$' ]]; then\n"
                 "token=\"${match[1]}\"\n"
                 "desc=\"${match[2]}\"\n"
                 "candidates+=(\"$token:$desc\")\n"
                 "fi\n"
                 "done <<< \"$help_output\"\n"
                 "_describe 'cling arguments' candidates\n"
                 "}\n"
                 "compdef _cling cling\n");
  Log("%S\n", Text);
 }
 
 //~ Sampler
 if(CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("Sampler"), S8("Target")))
 {
  Cng_Build(S8("Sampler"), &BuildOptions, S8ArrayLit(S8("../code/school/sampler/sampler.c")));
 }
 
 //~ School
 S8ArrayAppendTo(&Help, S8(" "));
 if(CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("School"), S8("(See subcommands with -School)")))
 {
  b32 Compile  = CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("compile"), S8("Compile the source file"));
  b32 Run      = CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("run"), S8("Run the built program"));
  b32 Last     = CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("last"), S8("Use the last found source file as input"));
  b32 All      = CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("all"), S8("Use all source files as input"));
  b32 Template = CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("template"), S8("Create a template source file"));
  b32 Print    = CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("print"), S8("Print the source file path"));
  b32 Convert  = CmdLineHasEntryAndAddHelp(CmdLine, &Help, S8("convert"), S8("Convert spaces to tabs in the source file"));
  
  if(All) Last = true;
  
  str8 TemplateText = S8(
                         "#define _CRT_SECURE_NO_WARNINGS\n"
                         "#include <stdio.h>\n"
                         ."\n"
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
  
  arena *Arena = GetScratch();
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
     str8_node *Node = PushArray(Arena, str8_node, 1);
     DLLAppendNil(Node, CmdLine->FirstFile, CmdLine->LastFile, NilS8Node);
     Node->V = LastIdxString; 
     
     LastIdx = Idx;
    }
    
    Assert(LastIdx < MaxLastIdx);
   }
  }
  
  b32 StartAtLast = (Last && !All);
  str8_node *First = (!StartAtLast ? CmdLine->FirstFile : CmdLine->LastFile);
  
  for(str8_node *Node = First; !IsNilS8Node(Node); Node = Node->Next)
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
    
    str8 Out = PushS8(Arena, File.Size);
    
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
    Cng_Build(S8("Les1"), &BuildOptions, S8ArrayLit(Path));
   }
   
   if(Run)
   {
    if(!GlobalSelectedArray) Cng_Scratch();
    GlobalSelectedArray->Count = 0;
    
    S8ArrayTemp(GlobalSelectedArray)
    {
     str8 BuildPath = Str8Fmt("../build/%S_%S_%S.exe", NamePrefix, Hoofdstuk, Node->V);
     
     Cng_S8ArrayAppend(BuildPath);
     RunCommand();
    }
   }
   
   DidWork = true;
  }
 }
 
 //~ Show help
 
 if(!DidWork)
 {
  str8 HelpText = Cng_S8ArrayJoinFrom(&Help, '\n');
  Log("\nNo target provided.\n\n%S\n", HelpText);
 }
 
 return 0;
}
