//~ Libraries
#if !defined(CLING_SOURCE_PATH)
# error "You must define CLING_SOURCE_PATH"
#endif
#if !defined(CLING_CODE_PATH)
# error "You must define CLING_CODE_PATH"
#endif
#if !defined(CLING_BUILD_PATH)
# error "You must define CLING_BUILD_PATH"
#endif

#define CLING_ARG_NOREBUILD "CLING_NOREBUILD"
#define CLING_ARG_PARENTPID "CLING_PARENTPID"

#define CLING_DEFAULT_S8ARRAY_CAPACITY 256

//~ Types

enum cng_os_kind
{
 CngOS_Windows,
 CngOS_Linux,
};
typedef enum cng_os_kind cng_os_kind;

enum cng_compiler
{
 CngCompiler_CC,
 CngCompiler_GCC,
 CngCompiler_Clang,
 CngCompiler_MSVC,
};
typedef enum cng_compiler cng_compiler;

enum cng_build_type
{
 CngBuildType_EXE,
 CngBuildType_OBJ,
 CngBuildType_DLL,
};
typedef enum cng_build_type cng_build_type;

typedef struct cng_build_options cng_build_options;
struct cng_build_options
{
 b32 Asan;
 b32 Release;
 b32 Slow;
 b32 CPP;
 cng_compiler Compiler;
};

typedef struct cng_command_result cng_command_result; 
struct cng_command_result
{
 b32 Success;
};

//~ Globals
global_variable str8_array *GlobalSelectedArray = 0;
global_variable char **GlobalEnv = 0;
global_variable cng_os_kind GlobalCurrentOS = 0;

//~ Platform API
// NOTE(luca): Implement these when adding a new platform.
internal void Cng_InitAndRebuildSelf(int ArgCount, char *Args[], char *Env[]);
internal cng_command_result Cng_RunCommand(str8 Command);

//~ Functions
internal b32
Cng_IsOS(cng_os_kind Kind)
{
 b32 Result = (GlobalCurrentOS == Kind);
 return Result;
}

//- Strings
internal void Cng_SetSelectedArray(str8_array Value) 
{ 
 str8_array *Array = PushArray(GetScratch(), str8_array, 1);
 *Array = Value;
 GlobalSelectedArray = Array; 
}

#define Cng_PushS8Array(Capacity) PushS8Array(GetScratch(), Capacity)
#define Cng_S8ArrayAppend(String) S8ArrayAppendTo(GlobalSelectedArray, String)
#define Cng_S8ArrayJoin(Char) S8ArrayJoinFrom(GetScratch(), GlobalSelectedArray, Char)
#define Cng_S8ArrayJoinFrom(Array, Char) S8ArrayJoinFrom(GetScratch(), Array, Char)
#define Cng_Scratch() \
Cng_SetSelectedArray(Cng_PushS8Array(CLING_DEFAULT_S8ARRAY_CAPACITY))
#define Cng_S8ArrayAppendMultiple(...) \
S8ArrayAppendMultipleTo(GlobalSelectedArray, __VA_ARGS__) 

//- Helpers

#if OS_LINUX
internal str8 LinuxFindCommandInPATH(umm BufferSize, u8 *Buffer, char *Command, char *Env[]);
#endif

internal str8
Cng_PathFromExe(str8 ExePath, str8 Path)
{
	str8 FileName = PushS8(GetScratch(), 512);
 
 MemoryCopy(FileName.Data, ExePath.Data, ExePath.Size);
 
 u64 OnePastLastSlash = S8FindChar(ExePath, OS_SlashChar, false, true);
 FileName.Size = OnePastLastSlash + Path.Size;
 
 u64 At = OnePastLastSlash;
 for EachIndex(Idx, Path.Size)
 {
  FileName.Data[At] = Path.Data[Idx];
  At += 1;
 }
 FileName.Data[FileName.Size] = 0;
 
 return FileName;
}

internal str8
Cng_GetBaseFileName(str8 FileName)
{
 str8 Result = {0};
 
 u64 OnePastLastSlash = S8FindChar(FileName, OS_SlashChar, false, true);
 Result = S8From(FileName, OnePastLastSlash);
 
 return Result;
}

#define S8ArrayLit(...) \
&((str8_array){ \
.Count    = sizeof((str8[]){__VA_ARGS__}) / sizeof(str8), \
.Capacity = sizeof((str8[]){__VA_ARGS__}) / sizeof(str8), \
.Strings  = (str8[]){__VA_ARGS__}, \
})


internal cng_command_result
Cng_Build__(str8 BuildName, 
            cng_build_type Type, 
            cng_build_options *Options,
            str8_array *ExtraCompilerFlags,
            str8_array *ExtraLinkerFlags,
            str8_array *Sources,
            str8 OutputName)
{
 cng_command_result Result = {0};
 
#if OS_LINUX
 // Find out compiler if it is CC.
 {
  if(Options->Compiler == CngCompiler_CC)
  {
   u8 Buffer_[PATH_MAX] = {0};
   u8 *Buffer = Buffer_;
   
   str8 Command = LinuxFindCommandInPATH(PATH_MAX, Buffer, "cc", GlobalEnv);
   if(Command.Size)
   {
    // Find compiler cc is liked to.
    {   
     // NOTE(luca): Needed for Swap() below
     u8 Path_[PATH_MAX] = {0};
     u8 *Path = Path_;
     
     // Follow symlink chain
     s64 Size = 0;
     while(Size != -1)
     {
      Path[Size] = 0;
      
      Size = readlink((char *)Buffer, (char *)Path, PATH_MAX);
      
      b32 Break = false;
      for(s64 Idx = 0; Idx < Size && !Break; Idx += 1)
      {
       // If path from symlink contains a compiler word we set the compiler to that.
       Break = true;
       str8 Search = {.Data = Path + Idx, .Size = (u64)(Size - Idx)};
       if(0) {}
       else if(S8Match(Search, S8("clang"), true)) Options->Compiler = CngCompiler_Clang;
       else if(S8Match(Search, S8("gcc"), true)) Options->Compiler = CngCompiler_GCC;
       else Break = false;
      }
      if(Break) break;
      
      Swap(Buffer, Path);
     }
    }
   }
   else
   {
    // NOTE(luca): No cc, try "gcc" or "clang" if present.
    Command = LinuxFindCommandInPATH(PATH_MAX, Buffer, "gcc", GlobalEnv);
    if(Command.Size) 
    {
     Options->Compiler = CngCompiler_GCC;
     goto CompilerFound;
    }
    
    Command = LinuxFindCommandInPATH(PATH_MAX, Buffer, "clang", GlobalEnv);
    if(Command.Size) 
    {
     Options->Compiler = CngCompiler_Clang;
     goto CompilerFound;
    }
    
    CompilerFound:;
   }
  }
 }
#elif OS_WINDOWS
 Options->Compiler = CngCompiler_MSVC;
#endif
 
 if(!GlobalSelectedArray) Cng_Scratch();
 GlobalSelectedArray->Count = 0;
 
 // Create compile command 
 S8ArrayTemp(GlobalSelectedArray)
 { 
  b32 Clang = (Options->Compiler == CngCompiler_Clang);
  b32 GCC = (Options->Compiler == CngCompiler_GCC);
  b32 MSVC = (Options->Compiler == CngCompiler_MSVC);
  b32 Debug = (!Options->Release);
  
  str8 Compiler = {0};
  if(0) {}
  else if(Clang) Compiler = S8("clang");
  else if(GCC)   Compiler = S8("gcc");
  else if(MSVC)  Compiler = S8("cl");
  else           Compiler = S8("cc");
  
  if(Cng_IsOS(CngOS_Linux))
  {
   Cng_S8ArrayAppend(Compiler);
   
   if(!Options->CPP)
   {
    Cng_S8ArrayAppend(S8("-std=c99"));
   }
   else
   {
    Cng_S8ArrayAppend(S8("-fno-threadsafe-statics -nostdinc++ -fno-exceptions -fno-rtti"));
   }
   
   if(Options->Asan)
   {
    Cng_S8ArrayAppend(S8("-fsanitize=undefined,address"));
   }
   
   // Common compiler flags
   Cng_S8ArrayAppend(S8("-I" CLING_CODE_PATH " "
                        "-D_GNU_SOURCE=1"));
   // Common warning flags
   Cng_S8ArrayAppend(S8("-Wall -Wextra -Wconversion -Wswitch -Wshadow " 
                        "-Wno-double-promotion "
                        "-Wno-unused-but-set-variable "
                        "-Wno-write-strings "
                        "-Wno-pointer-arith "
                        "-Wno-missing-field-initializers "
                        "-Wno-unused-parameter "
                        "-Wno-unused-variable "
                        "-Wno-unused-function "));
   
   
   if(Clang) 
   {
    Cng_S8ArrayAppend(S8("-fdiagnostics-absolute-paths -ftime-trace"));
    Cng_S8ArrayAppend(S8("-Wno-null-dereference "
                         "-Wno-missing-braces "
                         "-Wno-undefined-internal "
                         "-Wno-initializer-overrides "
                         "-Wno-string-conversion "
                         "-Wno-vla-cxx-extension "
                         "-Wno-writable-strings "
                         "-Wno-missing-designated-field-initializers "
                         "-Wno-address-of-temporary "
                         "-Wno-int-to-void-pointer-cast "
                         "-Wno-incompatible-pointer-types-discards-qualifiers"));
   }
   
   if(GCC)
   {
    Cng_S8ArrayAppend(S8("-Wno-type-limits "
                         "-Wno-cast-function-type "
                         "-Wno-missing-field-initializers "
                         "-Wno-int-to-pointer-cast "
                         "-Wno-cast-function-type "
                         "-Wno-missing-field-initializers "
                         "-Wno-int-to-pointer-cast "));
    
   }
   
   if(Debug) Cng_S8ArrayAppend(S8("-g -ggdb -g3"));
   else      Cng_S8ArrayAppend(S8("-O2"));
   
   // Extra flags depending on build type
   {  
    if(Type != CngBuildType_EXE) Cng_S8ArrayAppend(S8("-fPIC"));
    
    if(Type == CngBuildType_OBJ) 
    {
     Cng_S8ArrayAppend(S8("-c"));
     if(Options->CPP) Cng_S8ArrayAppend(S8("-x c++"));
     else             Cng_S8ArrayAppend(S8("-x c"));
    }
    
    if(Type == CngBuildType_DLL) Cng_S8ArrayAppend(S8("--shared"));
   }
   Cng_S8ArrayAppend(Cng_S8ArrayJoinFrom(ExtraCompilerFlags, ' '));
   
   Cng_S8ArrayAppend(S8("-o"));
   
   if(!OutputName.Size)
   {   
    // Get output name from source file name without extension
    {   
     str8 Source = Sources->Strings[0];
     u64 OPLSlash = S8FindChar(Source, '/', false, true);
     u64 LastDot  = S8FindChar(Source, '.', false, false);
     OutputName = S8FromTo(Source, OPLSlash, LastDot);
    }
    
    str8 Suffix = {0};
    if(Type == CngBuildType_OBJ) Suffix = S8(".obj");
    if(Type == CngBuildType_DLL) Suffix = S8(".so");
    if(Type == CngBuildType_EXE) Suffix = S8("");
    
    OutputName = S8Cat(OutputName, Suffix);
   }
   
   Cng_S8ArrayAppend(OutputName);
   
   Cng_S8ArrayAppend(Cng_S8ArrayJoinFrom(Sources, ' '));
   
   // Common linker flags
   Cng_S8ArrayAppend(S8("-lm"));
   Cng_S8ArrayAppend(Cng_S8ArrayJoinFrom(ExtraLinkerFlags, ' '));
   
   // LogBuildMode()
  }
  else if(Cng_IsOS(CngOS_Windows))
  {
#if defined(CLING_MSVC_SETUP)
   Cng_S8ArrayAppend(S8("cmd /c \"" CLING_MSVC_SETUP));
   Cng_S8ArrayAppend(S8("&&"));
#endif
   
   Cng_S8ArrayAppend(Compiler);
   
   Cng_S8ArrayAppend(S8("-nologo "
                        "-MTd "  // Static cruntime
                        "-Gm- "  // Disable incremental build 
                        "-GR- "  // Disable RTTI (cpp)
                        "-EHa- " // Disable exceptions(cpp)
                        "-Oi "   // Enable intrinsics
                        "-FC"    // Full path names
                        ));
   
   Cng_S8ArrayAppend(S8("-I" CLING_CODE_PATH));
   
   Cng_S8ArrayAppend(S8("-WX "     // Warnings as errors
                        "-W4 "     // Enable most warnings
                        "-wd4459 " // Non-standard extension 
                        // "-wd4456 " // Local variable shadowing 
                        "-wd4201 " // Nameless struct/union 
                        "-wd4100 " // Unreferenced formal parameter 
                        "-wd4101 " // Unreferenced local variable 
                        "-wd4505 " // Unreferenced local function 
                        "-wd4189 " // Initialized but unreferenced variable 
                        "-wd4996 " // Deprecated function 
                        "-wd4820 " // Padding added 
                        "-wd4296 " // Epression always true 
                        "-wd4127 " // Epression constant 
                        "-wd5045 " //  
                        "-wd4061 " // Case not handled in switch  
                        // "-wd4389 " // Signed/unsigned comparison 
                        // "-wd4244 " // Possible data loss conversion 
                        "-wd5287 " // Enum comparison 
                        ));
   
   if(0) {}
   else if(Type == CngBuildType_OBJ) Cng_S8ArrayAppend(S8("-c /TP"));
   else if(Type == CngBuildType_DLL) Cng_S8ArrayAppend(S8("/LD"));
   
   if(OutputName.Size) 
   {
    Cng_S8ArrayAppend(Str8Fmt("-Fe%S", OutputName));
    Cng_S8ArrayAppend(Str8Fmt("-Fm" "%S.map", OutputName));
    if(0) {}
    else if(Type == CngBuildType_EXE) OutputName = S8Cat(OutputName, S8(".exe"));
    else if(Type == CngBuildType_OBJ) OutputName = S8Cat(OutputName, S8(".obj"));
    else if(Type == CngBuildType_DLL) OutputName = S8Cat(OutputName, S8(".dll"));
   }
   
   if(Options->Asan) Cng_S8ArrayAppend(S8("/fsanitize=address"));
   
   if(Debug) Cng_S8ArrayAppend(S8("-Z7")); // Debug info
   else      Cng_S8ArrayAppend(S8("-O2")); // Optimizations
   
   if(Options->CPP) Cng_S8ArrayAppend(S8("/std:c++20"));
   
   Cng_S8ArrayAppend(Cng_S8ArrayJoinFrom(ExtraCompilerFlags, ' '));
   
   Cng_S8ArrayAppend(Cng_S8ArrayJoinFrom(Sources, ' '));
   
   // Common linker flags
   Cng_S8ArrayAppend(S8("/link -opt:ref -incremental:no"));
   Cng_S8ArrayAppend(Cng_S8ArrayJoinFrom(ExtraLinkerFlags, ' '));
   
   if(OutputName.Size)
   {
    Cng_S8ArrayAppend(S8Cat(S8("/out:"), OutputName));
   }
  }
  
  Log("[%S %S-%S]\n", 
      BuildName,
      Compiler, 
      (Debug ? S8("debug") : S8("release")));
  
  if(Cng_IsOS(CngOS_Linux))
  {
   Log("%S\n", Cng_GetBaseFileName(Sources->Strings[0]));
  }
  
  str8 Command = Cng_S8ArrayJoin(' ');
  if(0)
  {
   Log("%S\n", Command);
  }
  
  Result = Cng_RunCommand(Command);
 }
 
 return Result;
};

typedef struct cng_build_params cng_build_params;
struct cng_build_params
{
 str8 BuildName; 
 cng_build_type Type; 
 cng_build_options *Options;
 str8_array *ExtraCompilerFlags;
 str8_array *ExtraLinkerFlags;
 str8_array *Sources;
 str8 OutputName;
};

internal cng_command_result 
Cng_Build_(cng_build_params Params)
{
 cng_command_result Result = Cng_Build__(Params.BuildName, 
                                         Params.Type, 
                                         Params.Options,
                                         Params.ExtraCompilerFlags,
                                         Params.ExtraLinkerFlags,
                                         Params.Sources,
                                         Params.OutputName);
 return Result;
}

#define Cng_Build(Name, BuildOptions, Sources_, ...) \
Cng_Build_((cng_build_params) { \
.BuildName = (Name), \
.Options = (BuildOptions), \
.Type = CngBuildType_EXE, \
.ExtraCompilerFlags = NilP(str8_array), \
.ExtraLinkerFlags = NilP(str8_array), \
.Sources = (Sources_), \
.OutputName = Nil(str8), \
##__VA_ARGS__})

//~ Platform

#if OS_WINDOWS
//~ Constants
// NOTE(luca): Because windows cannot overwrite an opened file, we create a helper executable that will do the rest for us.
#define CLING_EXE "cling.exe"
#define CLING_TEMP_EXE "cling_temp.exe"

//~ Implementation
internal cng_command_result 
Cng_RunCommandAsync(str8 Command, b32 Async)
{
 cng_command_result Result = {0};
 
 STARTUPINFOA StartupInfo = {0};
 StartupInfo.cb = sizeof(StartupInfo);
 
 PROCESS_INFORMATION ProcessInfo = {0};
 
 
 arena *Arena = GetScratch();
 
 u8 *CommandCString = PushArray(Arena, u8, Command.Size + 1);
 MemoryCopy(CommandCString, Command.Data, Command.Size);
 CommandCString[Command.Size] = 0;
 
 b32 Succeeded = CreateProcessA(0, 
                                (char *)CommandCString, 
                                0,
                                0,
                                false, // Inherit handle 
                                0,
                                0, 
                                0,
                                &StartupInfo, &ProcessInfo);
 if(Succeeded)
 {
  if(!Async)
  {
   WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
  }
  CloseHandle(ProcessInfo.hProcess);
  CloseHandle(ProcessInfo.hThread);
 } 
 else 
 {
  Win32Log();
  Log("Command: %s\n", CommandCString);
 }
 
 return Result;
}

internal cng_command_result
Cng_RunCommand(str8 Command)
{
 cng_command_result Result = Cng_RunCommandAsync(Command, false);
 return Result;
}

internal str8
Win32GetEnvironmentVar(char *Name)
{
 str8 Result;
 
 arena *Arena = GetScratch();
 
 S8OverPush(Arena, Result, KB(64), Result.Size)
 { 
  Result.Size = GetEnvironmentVariableA(Name, (char *)Result.Data, (DWORD)Result.Size);
 }
 
 return Result;
}

internal void
Cng_InitAndRebuildSelf(int ArgCount, char *Args[], char *Env[])
{
 arena *Arena = GetScratch();
 
 // Globals
 {
  GlobalCurrentOS = CngOS_Windows;
  GlobalEnv = Env;
  SetStringsScratch(Arena);
 }
 
 str8 NoRebuildString = Win32GetEnvironmentVar(CLING_ARG_NOREBUILD);
 str8 ParentPidString = Win32GetEnvironmentVar(CLING_ARG_PARENTPID);
 
 str8 ExePath = {0};
 {
  ExePath = PushS8(Arena, MAX_PATH);
  ExePath.Size = GetModuleFileNameA(0, (char *)ExePath.Data, (DWORD)ExePath.Size);
  ExePath.Data[ExePath.Size] = 0;
 }
 
 // NOTE(luca): Wait on parent to close so we can write to the file.
 { 
  DWORD ParentPid = 0;
  // Convert PidString to number 
  {
   for EachIndex(Idx, ParentPidString.Size)
   {
    u8 Digit = ParentPidString.Data[Idx] - '0';
    ParentPid *= 10;
    ParentPid += Digit;
   }
  }
  
  if(ParentPid != 0)
  {
   HANDLE Parent = OpenProcess(SYNCHRONIZE, false, ParentPid);
   if(Parent != 0)
   {
    DWORD Result = WaitForSingleObject(Parent, INFINITE);
    CloseHandle(Parent);
   }
   else
   {
    // NOTE(luca): This will mostly result in incorrect parameter if the parent has already exit.
    //Win32Log();
   }
  }
 }
 
 // Rebuild self
 {
  // NOTE(luca): These are the operations that need to happen.
  // old: copy self to temp
  // old: run temp and exit
  // temp: make new replacing old
  // temp: run new and exit
  // new: delete temp
  
  b32 IsOld = false;
  b32 IsTemp = false;
  
  str8 NewPath = Cng_PathFromExe(ExePath, S8(CLING_EXE));
  str8 TempPath = Cng_PathFromExe(ExePath, S8(CLING_TEMP_EXE));
  
  IsTemp = S8Match(TempPath, ExePath, false);
  IsOld = (NoRebuildString.Size == 0);
  str8 RunPath = {0};
  
  if(IsTemp)
  {
   // Make new
   {
    str8 BuildDirPath = Cng_PathFromExe(ExePath, S8(CLING_BUILD_PATH));
    str8 ClingSourcePath = Cng_PathFromExe(ExePath, S8(CLING_SOURCE_PATH));
    str8 ClingCodePath = Cng_PathFromExe(ExePath, S8(CLING_CODE_PATH));
    
    // NOTE(luca): So that build artifacts end up in the right directory.
    OS_ChangeDirectory((char *)BuildDirPath.Data);
    
    Cng_Build(S8("Cling"), NilP(cng_build_options), S8ArrayLit(S8(CLING_SOURCE_PATH)),
              .OutputName = NewPath);
    
   }
   
   RunPath = NewPath;
  }
  else if(IsOld)
  {
   // Copy to temp
   Win32LogIf(!CopyFile((char *)NewPath.Data, 
                        (char *)TempPath.Data,
                        
                        .false));
   RunPath = TempPath;
  }
  else
  {
   // Delete temp
   DeleteFile((char *)TempPath.Data);
  }
  
  if(IsOld || IsTemp)
  {
   // Run temp or new depending on RunPath
   {
    DWORD ParentPid = GetCurrentProcessId();
    char *ParentPidCString = (char *)Str8Fmt("%u", ParentPid).Data;
    SetEnvironmentVariableA(CLING_ARG_PARENTPID, ParentPidCString);
    
    if(IsTemp) SetEnvironmentVariableA(CLING_ARG_NOREBUILD, "1");
   }
   
   // Run new executable and pass passed flags to it.
   {   
    Cng_Scratch();
    Cng_S8ArrayAppend(RunPath);
    for(int Idx = 1; Idx < ArgCount; Idx += 1)
    {
     Cng_S8ArrayAppend(S8FromCString(Args[Idx]));
    }
    str8 Command = Cng_S8ArrayJoin(' ');
    
    Cng_RunCommandAsync(Command, true);
   }
   
   ExitProcess(0);
  }
 }
}

#elif OS_LINUX

#include <unistd.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <linux/limits.h>

//- Error wrappers 
internal void 
LinuxErrorWrapperPipe(int Pipe[2])
{
 int Ret = pipe(Pipe);
 if(Ret == -1)
 {
  perror("pipe");
  Assert(0);
 }
}

internal void 
LinuxErrorWrapperDup2(int OldFile, int NewFile)
{
 int Ret = dup2(OldFile, NewFile);
 if(Ret == -1)
 {
  perror("dup2");
  Assert(0);
 }
}

internal int 
LinuxErrorWrapperGetAvailableBytesToRead(int File)
{
 int BytesToRead = 0;
 
 int Ret = ioctl(File, FIONREAD, &BytesToRead);
 if(Ret == -1)
 {
  perror("read(ioctl)");
  Assert(0);
 }
 
 return BytesToRead;
}

internal smm 
LinuxErrorWrapperRead(int File, void *Buffer, umm BytesToRead)
{
 smm BytesRead = read(File, Buffer, BytesToRead);
 if(BytesRead == -1)
 {
  perror("read");
  Assert(0);
 }
 Assert(BytesRead == BytesRead);
 
 return BytesRead;
}

// TODO(luca): Simplify
internal str8 
LinuxFindCommandInPATH(umm BufferSize, u8 *Buffer, char *Command, char *Env[])
{
 u8 **VarAt = (u8 **)Env;
 char Search[] = "PATH=";
 int MatchedSearch = false;
 
 str8 Result = {0};
 Result.Data = Buffer;
 
 while(*VarAt && !MatchedSearch)
 {
  MatchedSearch = true;
  
  for(u64 At = 0;
      (At < sizeof(Search) - 1) && (VarAt[At]);
      At++)
  {
   if(Search[At] != VarAt[0][At])
   {
    MatchedSearch = false;
    break;
   }
  }
  
  VarAt++;
 }
 
 if(MatchedSearch)
 {
  VarAt -= 1;
  u8 *Scan = (u8 *)VarAt[0];
  while(*Scan && *Scan != '=') Scan += 1;
  Scan += 1;
  
  while((*Scan) && (Scan != VarAt[1]))
  {
   u64 Len = 0;
   while(Scan[Len] && Scan[Len] != ':' && 
         (Scan+Len != VarAt[1])) Len++;
   
   // Add the PATH entry
   u64 At = 0;
   for(; At < Len; At += 1)
   {
    Result.Data[At] = Scan[At];
   }
   Result.Data[At] = '/';
   At += 1;
   
   // Add the executable name
   for(char *CharAt = Command;
       *CharAt;
       CharAt += 1)
   {
    Result.Data[At] = (u8)CharAt[0];
    At += 1;
   }
   Result.Data[At] = 0;
   
   // Check if it exists
   int AccessMode = F_OK | X_OK;
   int Ret = access((char *)Result.Data, AccessMode);
   if(Ret == 0)
   {
    Result.Size = At;
    break;
   }
   
   Scan += Len + 1;
  }
 }
 
 return Result;
}

internal void 
LinuxRunCommand(char *Args[])
{
 int Ret = 0;
 int WaitStatus = 0;
 b32 Error = false;
 
 pid_t ChildPID = fork();
 if(ChildPID != -1)
 {
  if(ChildPID == 0)
  {
   if(execve(Args[0], Args, environ) == -1)
   {
    Log("Could not execute %s.\n", Args[0]);
    PrintErrnoIf(true);
   }
  }
  else
  {
   wait(&WaitStatus);
  }
 }
 else
 {
  PrintErrnoIf(true);
 }
 
}

//- OS implementation
internal cng_command_result 
Cng_RunCommand(str8 Command)
{
 cng_command_result Result = {0};
 
 char *Args[64] = {0};
 
 u8 ArgsBuffer[1024] = {0};
 u64 ArgsBufferIndex = 0;
 
 // 1. split on whitespace into null-terminated strings.
 //    TODO: skip quotes
 s32 ArgCount = 0;
 
 for(u64 At = 0; At < Command.Size;)
 {
  u64 Start = At;
  while(At < Command.Size && !IsWhiteSpace(Command.Data[At])) At += 1;
  
  u64 End = At;
  
  // Set pointer to ArgsBuffer into Args
  {            
   Args[ArgCount] = (char *)(ArgsBuffer + ArgsBufferIndex);
   ArgCount += 1;
   Assert(ArgCount < (s32)ArrayCount(Args));
  }
  
  // Create null terminated string and copy it into ArgsBuffer
  {            
   u64 Size = End - Start;
   MemoryCopy(ArgsBuffer + ArgsBufferIndex, Command.Data + Start, Size);
   
   ArgsBufferIndex += Size;
   ArgsBuffer[ArgsBufferIndex] = 0;
   ArgsBufferIndex += 1;
   Assert(ArgsBufferIndex < ArrayCount(ArgsBuffer));
  }
  
  while(At < Command.Size && IsWhiteSpace(Command.Data[At])) At += 1;
 }
 
 if(Args[0] && Args[0][0] != '/')
 {
  u8 Buffer[PATH_MAX] = {0};
  str8 ExePath = LinuxFindCommandInPATH(sizeof(Buffer), Buffer, Args[0], GlobalEnv);
  if(ExePath.Size)
  {
   Args[0] = (char *)ExePath.Data;
  }
 }
 
 LinuxRunCommand(Args);
 
 return Result;
}

internal void
Cng_InitAndRebuildSelf(int ArgCount, char *Args[], char *Env[])
{
 arena *Arena = GetScratch();
 
 // Globals
 {
  GlobalCurrentOS = CngOS_Linux;
  GlobalEnv = Env;
  SetStringsScratch(Arena);
 }
 
 str8 OutputBuffer = PushS8(Arena, KB(2));
 str8 TempBuffer = PushS8(Arena, KB(2));
 
 str8 CommandName = S8FromCString(Args[0]);
 
 char *Value = getenv(CLING_ARG_NOREBUILD);
 b32 Rebuild = (Value == 0);
 
 if(Rebuild)
 {
  int Ret = setenv(CLING_ARG_NOREBUILD, "1", 0);
  PrintErrnoIf(Ret == -1); 
  
  Cng_SetSelectedArray(Cng_PushS8Array(256));
  
  str8 ClingBuildPath = Cng_PathFromExe(CommandName, S8(CLING_BUILD_PATH));
  OS_ChangeDirectory(CSS8(ClingBuildPath));
  
  Cng_Build(S8("Cling"), NilP(cng_build_options), S8ArrayLit(S8(CLING_SOURCE_PATH)));
  
  // NOTE(luca): LinuxRunCommand expects Args to have {Args[0], Args[0], Arg1, Arg2, ..., 0} format
  char **Arguments = PushArray(Arena, char *, (u64)(ArgCount + 3));
  {
   Arguments[0] = Args[0];
   Arguments[1] = Args[0];
   for EachIndex(Idx, ArgCount)
   {
    Arguments[Idx + 2] = Args[Idx];
   }
   Arguments[ArgCount + 2] = 0;
  }
  LinuxRunCommand(Arguments);;
  
  _exit(0);
 }
 
}

#endif