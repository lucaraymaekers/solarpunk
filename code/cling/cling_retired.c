
//~ WSD51
if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("M51"), S8("Target")))
{
 DidWork = true;
 
 LogBuildMode(S8("main.asm"), &BuildOptions);
 
 Cng_RunCommand(S8("sdas8051 -plosgff main.rel ../code/school/aduc832v4/main.asm"));
 Cng_RunCommand(S8("sdld -n -i main.ihx main.rel"));
 Cng_RunCommand(S8("wsdcom /C:4 /R:0000 main.ihx"));
}

//~ School
S8ArrayAppendTo(Help, S8(" "));
if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("School"), S8("(See subcommands with -School)")))
{
 Cng_Scratch();
 
 b32 Compile  = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("compile"), S8("Compile the source file"));
 b32 Run      = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("run"), S8("Run the built program"));
 b32 Last     = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("last"), S8("Use the last found source file as input"));
 b32 All      = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("all"), S8("Use all source files as input"));
 b32 Template = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("template"), S8("Create a template source file"));
 b32 Print    = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("print"), S8("Print the source file path"));
 b32 Convert  = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("convert"), S8("Convert spaces to tabs in the source file"));
 
 if(Compile)
 {
  LogBuildMode(S8("Les1"), &BuildOptions);
 }
 
 if(All) Last = true;
 
 str8 TemplateText = S8(
                        "#define _CRT_SECURE_NO_WARNINGS\n"
                        "#include <stdio.h>\n"
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
   S8ArrayTemp(GlobalSelectedArray)
   {
    Cng_CommonBuildCommand(&BuildOptions);
    Cng_S8ArrayAppend(Path);
    RunCommand();
   }
  }
  
  if(Run)
  {
   S8ArrayTemp(GlobalSelectedArray)
   {
    str8 BuildPath = Str8Fmt("../build/%S_%S_%S.exe", NamePrefix, Hoofdstuk, Node->V);
    
    Cng_S8ArrayAppend(BuildPath);
    RunCommand();
   }
  }
  
  DidWork = true;
 }