#if !BASE_NO_ENTRYPOINT
ENTRY_POINT(OS_EntryPoint)
{
 arena *Arena = GetScratch();
 
 Params->CmdLine.EntryCapacity = 256;
 Params->CmdLine.Entries = PushArray(Arena, cmd_line_entry, Params->CmdLine.EntryCapacity);
 
 Params->CmdLine.ExePath = S8FromCString(Params->Args[0]);
 
 for(s32 Idx = 1; Idx < Params->ArgCount; Idx += 1)
 {
  str8 Arg = S8FromCString(Params->Args[Idx]);
  
  u64 NameOffset = 0;
  if(0) {}
  else if(S8Match(Arg, S8("--"), true))
  {
   NameOffset = 2;
  }
  else if(S8Match(Arg, S8("-"), true))
  {
   NameOffset = 1;
  }
  
  str8 Name = S8From(Arg, NameOffset);
  
  b32 HasComma = false;
  
  if(Name.Size)
  {
   if(NameOffset > 0)
   {
    cmd_line_entry *Entry = GetP(Params->CmdLine.Entries, Params->CmdLine.EntryCapacity, Params->CmdLine.EntryCount);
    
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
    Name = S8To(Name, EqualsOffset);
    
    // Append entry
    if(Name.Size)
    {
     Params->CmdLine.EntryCount += 1;
     Entry->Name = Name;
    }
    
    str8 NameAfterEquals = S8From(Name, EqualsOffset + 1);
    if(NameAfterEquals.Size)
    {
     // Append files to entry
     // TODO(luca): Handle comma separated list of files
     str8_node *Node = PushArray(Arena, str8_node, 1);
     DLLAppend(Node, Entry->First, Entry->Last);
     Node->V = NameAfterEquals;
    }
    
   }
   else
   {
    str8_node *Node = PushArray(Arena, str8_node, 1);
    DLLAppend(Node, Params->CmdLine.FirstFile, Params->CmdLine.LastFile);
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
CmdLineGetEntry(cmd_line *CmdLine, str8 Search)
{
 cmd_line_entry *Result = NilCmdLineEntry;
 
 for EachIndex(Idx, CmdLine->EntryCount)
 {
  cmd_line_entry *Entry = CmdLine->Entries + Idx;
  
  if(S8Match(Entry->Name, Search, false))
  {
   Result = Entry;
   break;
  }
 }
 
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
 fwrite(String.Data, 1, String.Size, stderr);
}
