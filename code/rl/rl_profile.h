//~ Types

typedef struct prof_marker prof_marker;
struct prof_marker
{
 char *Label;
 u64 TSCElapsedExclusive;
 u64 TSCElapsedInclusive;
 u64 HitCount;
};

typedef struct prof_profiler prof_profiler;
struct prof_profiler
{
 u64 StartTSC;
 u64 EndTSC;
 
 prof_marker Markers[4096];
 
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

#define Prof_TimeScope__(Block, I, Label) \
for(prof_block Block = Prof_BlockBegin(Label, __COUNTER__ + 1), *I = 0;\
I == 0; \
Prof_BlockEnd(Block), I = (prof_block *)1)
#define Prof_TimeScope_(Label, Counter) \
Prof_TimeScope__(Glue(Block, Counter), Glue(BlockI, Counter), Label)
#define Prof_TimeScope(Label) Prof_TimeScope_(Label, __COUNTER__)
#define Prof_TimeScopeFunc() Prof_TimeScope(__FUNCTION__)

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
 Result.StartTSC = ReadCPUTimer();
 Prof.ParentIdx = MarkerIdx;
 
 // NOTE(luca): Could be computed at compile time, but not in C.
 Marker->Label = Label;
 
 return Result;
}

internal void
Prof_BlockEnd(prof_block Block)
{
 u64 Elapsed = ReadCPUTimer() - Block.StartTSC;
 Prof.ParentIdx = Block.ParentIdx;
 
 prof_marker *Parent = Prof.Markers + Block.ParentIdx;
 prof_marker *Marker = Block.Marker;
 
 Parent->TSCElapsedExclusive -= Elapsed;
 Marker->TSCElapsedExclusive += Elapsed;
 Marker->TSCElapsedInclusive = Block.OldTSCElapsedInclusive + Elapsed;
 Marker->HitCount += 1;
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
 Prof.StartTSC = ReadCPUTimer();
}

internal void
Prof_EndProfileAndPrint(void)
{
 u64 TotalTSCElapsed = ReadCPUTimer() - Prof.StartTSC;
 for EachIndex(Idx, ArrayCount(Prof.Markers))
 {
  prof_marker *Marker = Prof.Markers + Idx;
  if(Marker->TSCElapsedInclusive)
  {   
   f64 Percent = 100.0 * ((f64)Marker->TSCElapsedExclusive / (f64)TotalTSCElapsed);
   Log("  %s[%llu]: %llu (%.2f%%", Marker->Label, Marker->HitCount, Marker->TSCElapsedExclusive, Percent);
   if(Marker->TSCElapsedInclusive != Marker->TSCElapsedExclusive)
   {
    f64 PercentWithChildren = 100.0 * ((f64)Marker->TSCElapsedInclusive / (f64)TotalTSCElapsed);
    Log(", %.2f%% w/children", PercentWithChildren);
   }
   Log(")\n");
  }
 }
 
}