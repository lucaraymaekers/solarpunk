/* date = October 27th 2026 3:55 pm */

#ifndef BASE_DEBUG_H
#define BASE_DEBUG_H

//~ Types
typedef struct debug_breakpoint debug_breakpoint;
struct debug_breakpoint
{
 b32 Active;
 b32 Disabled;
};

//~ Macros
#define Statement(X) do { X } while(0)
#define DebugBroke(X) Statement(if(!DebugBreakPoints[X].Disabled) \
{ \
DebugBreak(); \
DebugBreakPoints[X].Active = true; \
})
#define DebugBroken(X) Statement(if(!DebugBreakPoints[X].Disabled && \
DebugBreakPoints[X].Active) \
{ \
DebugBreak(); \
})

//~ Globals
global_variable b32 GlobalDebuggerIsAttached;
global_variable debug_breakpoint DebugBreakPoints[10];

//~ Functions
internal void DbgBrk(void) { DebugBreak(); }


#endif //BASE_DEBUG_H
