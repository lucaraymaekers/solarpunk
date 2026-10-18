//~ Globals
read_only cmd_line_entry _NilCmdLineEntry = {&_NilS8Node, &_NilS8Node};
read_only cmd_line_entry *NilCmdLineEntry = &_NilCmdLineEntry;

global_variable u8 LogBuffer[KB(64)];
global_variable os_profiler GlobalProfiler = {0}; 
global_variable char *GlobalProfilerPrefix; 

#if BASE_PROFILE
global_variable b32 GlobalIsProfiling = true;
#else
global_variable b32 GlobalIsProfiling = false;
#endif

#define IsNilCmdLineEntry(Entry) CheckNil(Entry, NilCmdLineEntry)

//~ Functions
#if !BASE_NO_ENTRYPOINT
ENTRY_POINT(OS_EntryPoint)
{
 arena *Arena = GetScratch();
 
 // NOTE(luca): Disable line buffering, (workaround for cling and 4coder)
 setvbuf(stdout, NULL, _IONBF, 0);
 
 cmd_line *CmdLine = &Params->CmdLine;
 CmdLine->EntryCapacity = 256;
 CmdLine->Entries = PushArray(Arena, cmd_line_entry, CmdLine->EntryCapacity);
 
 CmdLine->ExePath = S8FromCString(Params->Args[0]);
 CmdLine->FirstFile = NilS8Node;
 CmdLine->LastFile = NilS8Node;
 
 for(s32 Idx = 1; Idx < Params->ArgCount; Idx += 1)
 {
  str8 Arg = S8FromCString(Params->Args[Idx]);
  
  u64 NameOffset = 0;
  if(S8Match(Arg, S8("-"), true))
  {
   NameOffset = 1;
  }
  
  str8 Name = S8From(Arg, NameOffset);
  
  b32 HasComma = false;
  
  if(Name.Size)
  {
   if(NameOffset > 0)
   {
    cmd_line_entry *Entry = GetP(CmdLine->Entries, CmdLine->EntryCapacity, CmdLine->EntryCount);
    
    // Search if name contains equals
    u64 EqualsOffset = Name.Size;
    for EachIndex(CharIdx, Name.Size)
    {
     if(Name.Data[CharIdx] == '=')
     {
      EqualsOffset = CharIdx;
      break;
     }
    }
    
    // If we found an equals truncate 
    str8 NameAfterEquals = S8From(Name, EqualsOffset + 1 - !!(EqualsOffset == Name.Size));
    Name = S8To(Name, EqualsOffset);
    
    // Append entry
    if(Name.Size)
    {
     CmdLine->EntryCount += 1;
     Entry->Name = Name;
     Entry->First = NilS8Node;
     Entry->Last = NilS8Node;
    }
    
    if(NameAfterEquals.Size)
    {
     // Append files to entry
     // TODO(luca): Handle comma separated list of files
     str8_node *Node = PushArray(Arena, str8_node, 1);
     DLLAppendNil(Node, Entry->First, Entry->Last, NilS8Node);
     Node->V = NameAfterEquals;
    }
    
   }
   else
   {
    str8_node *Node = PushArray(Arena, str8_node, 1);
    DLLAppendNil(Node, CmdLine->FirstFile, CmdLine->LastFile, NilS8Node);
    Node->V = Name;
   }
  }
  else
  {
   // NOTE(luca): This means '-' or '--' was supplied / end of parameters ?
  }
  
 }
 
 return EntryPoint(Params);
}
#endif

internal cmd_line_entry *
CmdLineGetEntry(cmd_line *CmdLine, str8 Flag)
{
 cmd_line_entry *Result = NilCmdLineEntry;
 
 for EachIndex(Idx, CmdLine->EntryCount)
 {
  cmd_line_entry *Entry = CmdLine->Entries + Idx;
  
  if(S8Match(Entry->Name, Flag, false))
  {
   Result = Entry;
   break;
  }
 }
 
 return Result;
}

internal b32
CmdLineHasEntry(cmd_line *CmdLine, str8 Flag)
{
 b32 Result = !IsNilCmdLineEntry(CmdLineGetEntry(CmdLine, Flag));
 return Result;
}

internal cmd_line_entry *
CmdLineGetEntryAndAddHelp(cmd_line *CmdLine, str8_array *Help, str8 Flag, str8 Description)
{
 cmd_line_entry *Result = false;
 
 str8 Pad = S8("                        ");
 if(Flag.Size < Pad.Size) Pad.Size -= Flag.Size;
 
 S8ArrayAppendTo(Help, Str8Fmt("-%S %S: %S", Flag, Pad, Description));
 Result = CmdLineGetEntry(CmdLine, Flag);
 
 return Result;
}

internal b32
CmdLineHasEntryAndAddHelp(cmd_line *CmdLine, str8_array *Help, str8 Flag, str8 Description)
{
 b32 Result = false;
 
 cmd_line_entry *Entry = CmdLineGetEntryAndAddHelp(CmdLine, Help, Flag, Description);
 Result = !IsNilCmdLineEntry(Entry);
 
 return Result;
}

#if OS_LINUX || OS_ANDROID
# include "base_os_linux.c"
#elif OS_WINDOWS
# include "base_os_windows.c"
#else 
# error "Operating system not provided or supported."
#endif

internal inline f64
OS_SecondsElapsed(f64 Start, f64 End)
{
 f64 Result = (End - Start);
 return Result;
}

internal inline f64
OS_MSElapsed(f64 Start, f64 End)
{
 f64 Result = ((End - Start)*1000.0);
 return Result;
}

internal void
OS_ProfileInit(char *Prefix)
{
 GlobalProfiler.Start = OS_GetWallClock();
 GlobalProfiler.End = GlobalProfiler.Start;
 GlobalProfilerPrefix = Prefix;
}

internal void
OS_ProfileAndPrint(char *Label)
{
 if(GlobalIsProfiling)
 {        
  GlobalProfiler.End = OS_GetWallClock();
  Log(" %s_%s: %.4f\n", GlobalProfilerPrefix, Label, OS_MSElapsed(GlobalProfiler.Start, GlobalProfiler.End));
  GlobalProfiler.Start = GlobalProfiler.End;
 }
}

internal void 
OS_PrintFormat(char *Format, ...)
{
 va_list Args;
 va_start(Args, Format);
 
 // NOTE(luca): Writing to stderr disables line buffering.
 str8 String = Str8VFmt(Format, Args);
 fwrite(String.Data, 1, String.Size, stdout);
}
