
{
 //- Custom build params
 internal void 
  LogBuildMode(str8 Name, cng_build_options *Options)
 {
  str8 ModeString = (!Options->Release ? S8("debug") : S8("release"));
  str8 Compiler = (Options->Clang ? S8("clang") :
                   Options->GCC ? S8("gcc") : S8(""));
  Log("[%S %S(%S)]\n", Name, Compiler, ModeString);
 }
 
 internal void
  ApplicationBuild(str8 ExeName, str8 AppName, str8 AppSource, str8 WindowName,
                   cng_build_options *BuildOptions)
 {
  LogBuildMode(ExeName, BuildOptions);
  
  // Compile
  {            
   str8_array *CommonFlags = Cng_PushS8Array(256);
   S8ArrayAppendTo(CommonFlags, 
                   Str8Fmt("-I" CLING_CODE_PATH " "
                           "-DRL_PLATFORM_APP_NAME=%S "
                           "-DRL_PLATFORM_WINDOW_NAME=%S",
                           AppName, WindowName));
   
   char *LibsFileName = (Cng_IsOs(CngOS_Windows) ? 
                         CLING_BUILD_PATH "rl_libs.obj" :
                         (Cng_IsOs(CngOS_Linux) ?
                          CLING_BUILD_PATH "rl_libs.o" :
                          0));
   
   if(!BuildOptions->Slow && !OS_FileExists(LibsFileName))
   {
    S8ArrayTemp(CommonFlags)
    {
     S8ArrayAppendTo(CommonFlags, S8("-DRL_LIBS_IMPLEMENTATION=1"));
     
     BuildOptions->CPP = true;
     BuildAndRun(S8("../code/rl/rl_libs.h"), S8("rl_libs"), 
                 BuildOptions,
                 CommonFlags, S8(""), S8(""),
                 false, true);
     BuildOptions->CPP = false;
    }
   }
   
   str8 LibsName = BuildOptions->Slow ?  S8("") : S8FromCString(LibsFileName);
   
   S8ArrayTemp(CommonFlags)
   {
    str8 ExtraLinkerFlags = {0};
    
    S8ArrayAppendTo(CommonFlags, (BuildOptions->Slow ? 
                                  S8("-DRL_LIBS_IMPLEMENTATION=1") :
                                  S8("-DRL_LIBS_IMPLEMENTATION=0")));
    
    BuildAndRun(AppSource, AppName, BuildOptions,
                CommonFlags, ExtraLinkerFlags, LibsName,
                true, false);
   }
   
   S8ArrayTemp(CommonFlags)
   {
    str8 ExtraLinkerFlags = {0};
    S8ArrayAppendTo(CommonFlags, (BuildOptions->Slow ? 
                                  S8("-DRL_LIBS_IMPLEMENTATION=1") :
                                  S8("-DRL_LIBS_IMPLEMENTATION=0")));
    
    if(0) {}
    else if(Cng_IsOs(CngOS_Linux))
    {
     ExtraLinkerFlags = S8("-lm -lasound -lX11 -lGL -lGLX");
    }
    
    BuildAndRun(S8("../code/rl/rl_platform.c"), ExeName, 
                BuildOptions,
                CommonFlags, ExtraLinkerFlags, LibsName, 
                false, false);
   }
   
  }
  
 }
 
 internal void
  SimpleBuildAndRun(cng_build_options *BuildOptions, str8 BuildName, str8 Source)
 {
  Cng_Scratch();
  
  Cng_CommonBuildCommand(BuildOptions);
  LogBuildMode(BuildName, BuildOptions);
  
  Cng_S8ArrayAppend(Source);
  
  if(Cng_IsOs(CngOS_Linux))
  {
   // Exename is source file name without the '.'
   
   str8 ExeName = {0};
   {
    u64 OnePastLastSlash = S8FindChar(Source, OS_SlashChar, false, true);
    u64 LastDot = S8FindChar(Source, '.', false, false);
    str8 BaseName = S8From(Source, OnePastLastSlash);
    Log("%S\n", BaseName); 
    
    ExeName = S8FromTo(Source, OnePastLastSlash, LastDot);
   }
   
   Cng_S8ArrayAppendMultiple(S8("-o"), ExeName);
   Cng_S8ArrayAppend(S8("-lm"));
  }
  
  if(Cng_IsOs(CngOS_Windows))
  {
   Cng_S8ArrayAppend(S8("/link -opt:ref -incremental:no"));
  }
  
  RunCommand();
 }
 
 //~ Haversine Generator
 if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("HaversineGenerator"), S8("Target")))
 {
  DidWork = true;
  
  SimpleBuildAndRun(&BuildOptions, S8("Haversine Generator"), S8("../code/compenhance/haversine_generator/haversine_generator.c"));
 }
 
 //~ Haversine Processor
 if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("HaversineProcessor"), S8("Target")))
 {
  DidWork = true;
  
  SimpleBuildAndRun(&BuildOptions, S8("Haversine Processor"), S8("../code/compenhance/haversine_processor/haversine_processor.c"));
 }
 
 //~ Sim86
 if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("Sim86"), S8("Target")))
 {
  DidWork = true;
  
  
  Cng_Scratch();
  Cng_CommonBuildCommand(&BuildOptions);
  LogBuildMode(S8("sim86"), &BuildOptions);
  
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
                   &BuildOptions);
 }
 
 //~ Muze
 if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("Muze"), S8("Target")))
 {
  DidWork = true;
  
  ApplicationBuild(S8("muze"), S8("muze_app"), 
                   S8("../code/muze/muze_app.c"), S8("Muze"),
                   &BuildOptions);
 }
 
 //~ WSDCOM
 S8ArrayAppendTo(Help, S8(" "));
 if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("WSD"), S8("(See subcommands with -WSD)")))
 {
  if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("compile"), S8("Compile wsdcom")))
  {   
   DidWork = true;
   SimpleBuildAndRun(&BuildOptions, S8("wsdcom"), S8("../code/wsdcom/wsdcom.c"));
  }
  
  if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("copy"), S8("Copy configuration files.")))
  {
   DidWork = true;
   OS_MakeDirectory("Configuration");
   Cng_RunCommand(S8("cmd.exe /c copy ..\\code\\wsdcom\\Configuration Configuration"));
  }
  
 }
 S8ArrayAppendTo(Help, S8(" "));
 
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
  
 }
 S8ArrayAppendTo(Help, S8(" "));
 
 if(CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("Zsh"), S8("Print out zsh completions")))
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
 
 
 internal void
  BuildAndRun(str8 Source, 
              str8 OutputName, 
              cng_build_options *BuildOptions,
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
    S8ArrayAppendTo(ExtraFlags, S8("-fPIC -x c++ -c"));
    OutputName = Str8Fmt("%S.o", OutputName);
   }
   else if(IsDLL)
   {
    S8ArrayAppendTo(ExtraFlags, S8("-fPIC --shared"));
    OutputName = Str8Fmt("%S.so", OutputName);
   }
   
   S8ArrayAppendTo(ExtraFlags, ExtraLinkerFlags);
   Log("%S\n", Cng_GetBaseFileName(Source));
   
   Cng_Scratch();
   Cng_CommonBuildCommand(BuildOptions);
   Cng_S8ArrayAppendMultiple(Cng_S8ArrayJoinFrom(ExtraFlags, ' '),
                             S8("-o"), OutputName,
                             Source);
   
   Cng_S8ArrayAppend(ExtraSource);
   RunCommand();
  }
  else if(Cng_IsOs(CngOS_Windows))
  {
   if(0) {}
   else if(IsObject)
   {
    S8ArrayAppendTo(ExtraFlags, S8("-c /TP"));
    S8ArrayAppendTo(ExtraFlags, Str8Fmt("-Fo" "%S.obj", OutputName));
   }
   else if(IsDLL)
   {
    S8ArrayAppendTo(ExtraFlags, S8("/LD"));
    ExtraLinkerFlags = Str8Fmt("%S /DLL /OUT:%S.dll",
                               ExtraLinkerFlags, OutputName);
   }
   else
   {
    S8ArrayAppendTo(ExtraFlags, Str8Fmt("-Fe%S.exe", OutputName));
   }
   
   ExtraLinkerFlags = Str8Fmt("/link -opt:ref -incremental:no %S ", ExtraLinkerFlags);
   
   S8ArrayAppendTo(ExtraFlags, Str8Fmt("-Fm" "%S.map", OutputName));
   S8ArrayAppendTo(ExtraFlags, ExtraSource);
   WindowsBuild(Source, ExtraFlags, ExtraLinkerFlags, !BuildOptions->Release, BuildOptions->Asan);
  }
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
  Cng_CommonBuildCommand(cng_build_options *Options)
 {
  // Exclusive arguments
  if(Options->GCC) Options->Clang = false;
  b32 Debug = !Options->Release;
  
#elif OS_WINDOWS    
  
#if defined(CLING_MSVC_SETUP)
  Cng_S8ArrayAppend(S8("cmd /c \"" CLING_MSVC_SETUP));
  Cng_S8ArrayAppend(S8("&&"));
#endif
  
  Cng_S8ArrayAppend(S8("cl"));
  //Cng_S8ArrayAppend(S8("/P /C"));
  Cng_S8ArrayAppend(S8("-MTd -Gm- -nologo -GR- -EHa- -Oi -FC -Z7"));
  Cng_S8ArrayAppend(S8("-Zc:strictStrings-"));
  Cng_S8ArrayAppend(S8("-WX -W4 -wd4459 -wd4456 -wd4201 -wd4100 -wd4101 -wd4189 -wd4505 -wd4996 -wd4389 -wd4244 -wd5287"));
  Cng_S8ArrayAppend(S8("-I" CLING_CODE_PATH));
#endif
 }
}
