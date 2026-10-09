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
 u64 OSStart;
 
 u64 MarkerCount;
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
for(prof_block Block = Prof_BlockBegin(Label), *I = 0;\
I == 0; \
Prof_BlockEnd(Block), I = (prof_block *)1)
#define Prof_TimeScope_(Label, Counter) \
Prof_TimeScope__(Glue(Block, Counter), Glue(BlockI, Counter), Label)
#define Prof_TimeScope(Label) Prof_TimeScope_(Label, __COUNTER)
#define Prof_TimeScopeFunc() Prof_TimeScope(__FUNCTION__)

//~ Functions
internal prof_block
Prof_BlockBegin(char *Label)
{
 prof_block Result = {0};
 
 u64 MarkerIdx = Prof.MarkerCount;
 prof_marker *Marker = Prof.Markers + Prof.MarkerCount;
 Prof.MarkerCount += 1;
 
 Result.OldTSCElapsedInclusive = Marker->TSCElapsedInclusive; 
 Result.StartTSC = ReadCPUTimer();
 Result.ParentIdx = Prof.ParentIdx;
 Result.Marker = Marker;
 
 // NOTE(luca): Could be computed at compile time, but not in C.
 Marker->Label = Label;
 
 Prof.ParentIdx = MarkerIdx;
 
 return Result;
}

internal void
Prof_BlockEnd(prof_block Block)
{
 u64 Elapsed = ReadCPUTimer() - Block.StartTSC;
 
 prof_marker *Parent = Prof.Markers + Block.ParentIdx;
 prof_marker *Marker = Block.Marker;
 
 Marker->HitCount += 1;
 Marker->TSCElapsedExclusive += Elapsed;
 Parent->TSCElapsedExclusive -= Elapsed;
 
 Marker->TSCElapsedInclusive = Block.OldTSCElapsedInclusive + Elapsed;
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
Prof_PrintTimeElapsed(char const *Label, u64 TotalTSCElapsed, u64 Begin, u64 End)
{
 u64 Elapsed = End - Begin;
 f64 Percent = 100.0 * ((f64)Elapsed / (f64)TotalTSCElapsed);
 Log("  %s: %lu (%.2f%%)\n", Label, Elapsed, Percent);
}
