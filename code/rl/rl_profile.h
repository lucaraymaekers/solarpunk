//~ Types

typedef struct prof_marker prof_marker;
struct prof_marker
{
 char *Label;
 u64 TSCElapsedExclusive;
 u64 TSCElapsedInclusive;
 u64 HitCount;
 u64 Indent;
};

typedef struct prof_profiler prof_profiler;
struct prof_profiler
{
 u64 StartTSC;
 u64 EndTSC;
 
 u64 OSStart;
 
 prof_marker Markers[4096];
 u64 Indent;
 
 u64 ParentIdx;
};

typedef struct prof_block prof_block;
struct prof_block
{
 u64 StartTSC;
 u64 ParentIdx;
 u64 OldTSCElapsedInclusive;
 prof_marker *Marker;
};

//~ Globals 
global_variable prof_profiler Prof; 

//~ Macros
#define _Prof_TimeScope(Label, Counter) \
for(prof_block Glue(Block, Counter) = Prof_BlockBegin(Label, Counter + 1), \
*Glue(I, Counter) = 0; \
Glue(I, Counter) == 0; \
Prof_BlockEnd(Glue(Block, Counter)), \
Glue(I, Counter) = (prof_block *)1)

#define Prof_TimeScope(Label) _Prof_TimeScope(Label, __COUNTER__)
#define Prof_TimeFunc() Prof_TimeScope(__FUNCTION__)
#define Prof_Profile() DeferLoop(Prof_StartProfile(), Prof_EndProfileAndPrint())

//~ Functions
internal prof_block
Prof_BlockBegin(char *Label, u64 MarkerIdx)
{
 prof_block Result = {0};
 
 prof_marker *Marker = Prof.Markers + MarkerIdx;
 
 Result.Marker = Marker;
 Result.OldTSCElapsedInclusive = Marker->TSCElapsedInclusive; 
 
 Result.ParentIdx = Prof.ParentIdx;
 Result.StartTSC = rdtsc();
 Prof.ParentIdx = MarkerIdx;
 Prof.Indent += 1;
 
 // NOTE(luca): Could be computed at compile time, but not in C.
 Marker->Label = Label;
 
 return Result;
}

internal void
Prof_BlockEnd(prof_block Block)
{
 u64 Elapsed = rdtsc() - Block.StartTSC;
 Prof.ParentIdx = Block.ParentIdx;
 
 prof_marker *Parent = Prof.Markers + Block.ParentIdx;
 prof_marker *Marker = Block.Marker;
 
 Parent->TSCElapsedExclusive -= Elapsed;
 Marker->TSCElapsedExclusive += Elapsed;
 Marker->TSCElapsedInclusive = Block.OldTSCElapsedInclusive + Elapsed;
 Marker->HitCount += 1;
 Marker->Indent = Prof.Indent;
 
 Prof.Indent -= 1;
}

internal u64 
Prof_GuessCPUFreq(u64 OSElapsed, u64 CPUElapsed, u64 OSFreq)
{
 u64 CPUFreq = 0;
 if(OSElapsed)
	{
		CPUFreq = OSFreq * CPUElapsed / OSElapsed;
	}
	
 return CPUFreq;
}

internal void
Prof_StartProfile(void)
{
 Prof.StartTSC = rdtsc();
 Prof.OSStart = OS_ReadTimer();
}

internal void
Prof_EndProfileAndPrint(void)
{
 u64 TotalTSCElapsed = rdtsc() - Prof.StartTSC;
 u64 OSElapsed = OS_ReadTimer() - Prof.OSStart;
 
 u64 OSFreq = OS_GetTimerFreq();
 u64 CPUFreq = Prof_GuessCPUFreq(OSElapsed, TotalTSCElapsed, OSFreq);
 
 str8 Pad = S8("                                                         ");
 
 Log("\nTotal time: %0.4fms (CPU freq %llu)\n", 
     1000.0*((f64)TotalTSCElapsed/(f64)CPUFreq), CPUFreq);
 
 for EachIndex(Idx, ArrayCount(Prof.Markers))
 {
  Pad.Size = 64;
  
  prof_marker *Marker = Prof.Markers + Idx;
  if(Marker->TSCElapsedInclusive)
  {   
   Pad.Size = Marker->Indent + 2;
   f64 Percent = 100.0 * ((f64)Marker->TSCElapsedExclusive / (f64)TotalTSCElapsed);
   Log("%S%s[%llu]: %llu (%.2f%%", 
       Pad, Marker->Label, Marker->HitCount, Marker->TSCElapsedExclusive, Percent);
   if(Marker->TSCElapsedInclusive != Marker->TSCElapsedExclusive)
   {
    f64 PercentWithChildren = 100.0 * ((f64)Marker->TSCElapsedInclusive / (f64)TotalTSCElapsed);
    Log(", %.2f%% w/children", PercentWithChildren);
   }
   Log(")\n");
  }
 }
 
}