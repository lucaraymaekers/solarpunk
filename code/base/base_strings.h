/* date = December 21st 2025 0:25 pm */

#if !defined(BASE_STRINGS_H)
#define BASE_STRINGS_H

#include "base/base_arenas.h"
#include <stdarg.h>

//~ Types

typedef struct str8 str8;
struct str8
{
 u8 *Data;
 u64 Size;
};
raddbg_type_view(str8, no_addr(array(Data, Size)));

typedef struct str8_node str8_node;
struct str8_node
{
 str8_node *Next;
 str8_node *Prev;
 str8 V;
};

struct str8_array
{
 u64 Count;
 u64 Capacity;
 str8 *Strings;
};
typedef struct str8_array str8_array;

//~ Globals
read_only str8_node _NilS8Node = {&_NilS8Node, &_NilS8Node};
read_only str8_node *NilS8Node = &_NilS8Node;

#define IsNilS8Node(Node) CheckNil(Node, NilS8Node)

//~ Functions

internal b32 IsPrintable(u8 Char);
internal b32 IsRuneDigit(rune Char);
internal b32 IsRuneAlpha(rune Char);
internal b32 IsDigit(u8 Char);
internal b32 IsAlpha(u8 Char);
internal b32 IsWhiteSpace(u8 Char);
internal u8  ToLowercase(u8 Char);

internal str8 S8SkipLastSlash(str8 String);
internal b32  S8Match(str8 A, str8 B, b32 BIsPrefix);
internal u64  StringLength(char *String);
internal str8 S8Cat(str8 Prefix, str8 Suffix);
internal str8 Str8DupCString(arena *Arena, char *String);
internal str8 Str8VFmt(const char *Format, va_list Args);
internal str8 Str8Fmt(const char *Format, ...);
internal u64  U64HashFromSeedStr8(u64 Seed, str8 String);
internal u64  U64HashFromStr8(str8 String);

internal str8_array *PushS8Array(arena *Arena, u64 Capacity);
internal void S8ArrayAppendTo(str8_array *Array, str8 String);
internal str8 S8ArrayJoinFrom(arena *Arena, str8_array *Array, u8 Char);

#define S8ArrayTemp(Array) for(u64 _Count_ = Array->Count, _i_ = 0; \
_i_ == 0; \
_i_ = _i_ + 1, Array->Count = _Count_)
#define S8ArrayAppendMultipleTo(Array, ...) \
do \
{ \
str8 Strings[] = {__VA_ARGS__}; \
S8ArrayAppendMultiple_(Array, ArrayCount(Strings), Strings); \
} while(0);


#if LANG_CPP
# define S8Cast str8
#else
# define S8Cast (str8)
#endif

#define S8(String)                   S8Cast{.Data = (u8 *)(String), .Size = (sizeof((String)) - 1)}
#define S8From(String, Start)        S8Cast{.Data = (u8 *)(String).Data + (Start), .Size = ((String).Size - (Start))}
#define S8To(String, End)            S8Cast{.Data = (u8 *)(String).Data, .Size = (End)}
#define S8FromTo(String, Start, End) S8Cast{.Data = (u8 *)(String).Data + (Start), .Size = ((End) - (Start))}
#define S8FromCString(String)        S8Cast{.Data = (u8 *)(String), .Size = StringLength((char *)(String))}
#define S8FromArray(Array)           S8Cast{.Data = (u8 *)(Array), .Size = sizeof((Array))}

// NOTE(luca): C string from str8
#define CSS8(String)  (char *)String.Data

#define PushS8_(Arena, Count) S8Cast{.Data = (PushArray((Arena), u8, (Count))), .Size = (Count)} 
#define PushS8(Arena, Size) PushS8_(Arena, Size)

#define S8OverPush(Arena, Result, MaxSize, EndSize) \
DeferLoop((Result = PushS8(Arena, MaxSize)), ArenaFree(Arena, MaxSize - EndSize))

#define StringsScratchTemp(Arena) \
StringsScratchTemp_(Arena, Glue(Counter, __COUNTER__), Glue(Temp, __COUNTER__))
#define StringsScratchTemp_(Arena, Counter, Temp) \
for(arena *Temp, *Counter = 0; !Counter; Counter = (arena *)1)  \
for(Temp = StringsScratch, SetStringsScratch(Arena); \
Arena == StringsScratch; \
SetStringsScratch(Temp))

#endif //BASE_STRINGS_H
