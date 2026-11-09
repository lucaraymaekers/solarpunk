//~ Libaries
#include <stdio.h>

#if RL_LIBS_IMPLEMENTATION && !BASE_EXTERNAL_LIBS
# define STB_SPRINTF_STATIC
# define STB_SPRINTF_IMPLEMENTATION
#endif 
NO_WARNINGS_BEGIN
#include "lib/stb_sprintf.h"
NO_WARNINGS_END

//~ Globals
global_variable arena *StringsScratch = 0;

#define SetStringsScratch(Arena) SetStringsScratch_(Arena, #Arena, __FILE__, __LINE__) 

read_only str8_node _NilS8Node = {&_NilS8Node, &_NilS8Node};
read_only str8_node *NilS8Node = &_NilS8Node;

//~ Functions

internal void
SetStringsScratch_(arena *Arena, const char *Name, const char *FileName, s32 Line)
{
 Assert(Arena);
 StringsScratch = Arena;
 
#if 0
 Log("%s(%d): Set strings scratch to '%s'.\n", FileName, Line, Name);
#endif
}

arena *
StringsScratchTempBegin(arena *Arena)
{
 arena *Result = StringsScratch;
 SetStringsScratch(Arena);
 return Result;
}

internal void
StringsScratchTempEnd(arena *Arena)
{
 SetStringsScratch(Arena);
}

internal b32 
IsPrintable(u8 Char)
{
 b32 Result = (Char >= ' ' && Char <= '~');
 return Result;
}

internal b32
IsRuneDigit(rune Char)
{
 b32 Result = (Char >= '0' && Char <= '9');
 return Result;
}

internal b32
IsRuneAlpha(rune Char)
{
 b32 Result = ((Char >= 'A' && Char <= 'Z') ||
               (Char >= 'a' && Char <= 'z'));
 return Result;
}

internal b32
IsDigit(u8 Char)
{
 b32 Result = (Char >= '0' && Char <= '9');
 return Result;
}

internal b32
IsAlpha(u8 Char)
{
 b32 Result = ((Char >= 'A' && Char <= 'Z') ||
               (Char >= 'a' && Char <= 'z'));
 return Result;
}

internal b32
IsAlphaNum(u8 Char)
{
 b32 Result = ((Char >= 'A' && Char <= 'Z') || (Char >= 'a' && Char <= 'z') ||
               (Char >= '0' && Char <= '9'));
 return Result;
}

internal b32
IsWhiteSpace(u8 Char)
{
 b32 Result = (Char == ' ' || Char == '\n' || Char == '\r' || Char == '\t');
 return Result;
}

internal u8
ToLowercase(u8 Char)
{
 u8 Result = Char;
 
 if(Char >= 'A' && Char <= 'Z')
 {
  Result = (Char - (u8)('A' - 'a'));
 }
 
 return Result;
}

internal str8 
S8SkipLastSlash(str8 String)
{
	u64 LastSlash = 0;
	for EachIndex(Idx, String.Size)
	{
		if(String.Data[Idx] == '/')
		{
			LastSlash = Idx;
		}
	}
	LastSlash += 1;
 
	str8 Result = S8From(String, LastSlash);
	return Result;
}

internal b32
S8Match(str8 A, str8 B, s8_match_flag Flags)
{
 b32 Match = false;
 
 b32 Requirements = false;
 
 b32 BIsPrefix = (Flags & S8MatchFlag_Prefix);
 
 Requirements |= ( BIsPrefix && (B.Size <= A.Size));
 Requirements |= (!BIsPrefix && (B.Size == A.Size));
 
 u64 MinSize = Min(A.Size, B.Size);
 
 if(Requirements)
 {
  Match = true;
  for EachIndex(Idx, MinSize)
  {
   u8 CharA = A.Data[Idx];
   u8 CharB = B.Data[Idx];
   
   if(Flags & S8MatchFlag_CaseInsensitive)
   {
    CharA = ToLowercase(CharA);
    CharB = ToLowercase(CharB);
   }
   
   if((CharA != CharB))
   {
    Match = false;
    break;
   }
  }
 }
 
 return Match;
}

internal u64
StringLength(char *String)
{
 u64 Result = 0;
 
 while(String && *String)
 {
  String += 1;
  Result += 1;
 }
 
 return Result;
}

internal str8
S8Cat(str8 Prefix, str8 Suffix)
{
 arena *Arena = StringsScratch;
 
 // NOTE(luca): Always null-terminate
 str8 Result = PushS8(Arena, Prefix.Size + Suffix.Size + 1);
 Result.Size -= 1;
 Result.Data[Result.Size] = 0;
 
 u64 At = 0;
 for EachIndex(Idx, Prefix.Size)
 {
  Result.Data[At + Idx] = Prefix.Data[Idx];
 }
 
 At += Prefix.Size;
 for EachIndex(Idx, Suffix.Size)
 {
  Result.Data[At + Idx] = Suffix.Data[Idx];
 }
 
 return Result;
}

internal u64
S8FindChar(str8 String, u8 Char, b32 FromStart, b32 Next)
{
 u64 Result = 0;
 
 u64 Idx = (FromStart ? 0 : String.Size - 1);
 
 for EachCount(String.Size)
 {
  if(String.Data[Idx] == Char)
  {
   Result = Idx + !!Next;
   break;
  }
  
  Idx += (FromStart ? 1 : (u64)-1);
 }
 
 return Result;
}

internal str8
S8CutFromChar(str8 String, u8 Char, b32 FromStart)
{
 str8 Result = {0};
 u64 Idx = S8FindChar(String, Char, FromStart, false);
 Result = S8To(String, Idx);
 return Result;
}

internal str8
S8DupCString(arena *Arena, char *String)
{
 str8 Result = {0};
 
 Result.Size = StringLength(String);
 Result.Data = PushArray(Arena, u8, Result.Size);
 MemoryCopy(Result.Data, String, Result.Size);
 
 return Result;
}

internal str8
Str8VFmt(const char *Format, va_list Args)
{
 str8 Result = {0};
 
 arena *Arena = StringsScratch;
 
 S8OverPush(Arena, Result, MB(1), Result.Size - 1)
 {  
  // TODO(luca): stbsp doesn't work with ASAN enabled, we should fix that.
#if !ASAN_ENABLED
  Result.Size = (u64)stbsp_vsprintf((char *)Result.Data, Format, Args);
  Result.Data[Result.Size] = 0;
#else
  int Size = vsprintf((char *)Result.Data, Format, Args);
  if(Size == -1)
  {
   Result.Size = 1;
  }
  else
  {
   Result.Size = (u64)Size;
   // Null terminate, useful when cstring is needed out of str8.
   Result.Data[Result.Size] = 0;
  }
#endif
  
  Assert(Result.Size > 0);
 }
 
 return Result;
}

internal str8
Str8Fmt(const char *Format, ...)
{
 str8 Result = {0};
 
 va_list Args;
 va_start(Args, Format);
 
 Result = Str8VFmt(Format, Args);
 
 return Result;
}

//~ Hashing 
#if RL_LIBS_IMPLEMENTATION && !BASE_EXTERNAL_LIBS 
# if !defined(XXH_IMPLEMENTATION)
# define XXH_INLINE_ALL
# define XXH_STATIC_LINKING_ONLY
# define XXH_IMPLEMENTATION
# endif
#endif

NO_WARNINGS_BEGIN
#include "lib/xxHash/xxhash.h"
NO_WARNINGS_END

internal u64
U64HashFromSeedStr8(u64 Seed, str8 String)
{
 u64 Result = XXH3_64bits_withSeed(String.Data, String.Size, Seed);
 return Result;
}

internal u64
U64HashFromStr8(str8 String)
{
 u64 Result = U64HashFromSeedStr8(5381, String);
 return Result;
}

//~ String arrays
internal str8_array
PushS8Array(arena *Arena, u64 Capacity)
{
 str8_array Result = {0};
 Result.Capacity = Capacity;
 Result.Strings = PushArray(Arena, str8, Result.Capacity);
 return Result;
}

internal void 
S8ArrayAppendTo(str8_array *Array, str8 String)
{
 if(String.Size > 0)
 {
  Assert(Array->Count < Array->Capacity);
  Array->Strings[Array->Count] = String;
  Array->Count += 1;
 }
}

internal str8 
S8ArrayJoinFrom(arena *Arena, str8_array *Array, u8 Char)
{
 str8 Result = {0};
 
 if(Array && Array->Count)
 {    
  u64 TotalSize = 0;
  for EachIndex(Idx, Array->Count)
  {
   TotalSize += Array->Strings[Idx].Size;
  }
  
  if(Char)
  {
   TotalSize += (Array->Count - 1);
  }
  
  str8 Buffer = PushS8(Arena, TotalSize);
  
  u64 BufferIndex = 0;
  for EachIndex(At, Array->Count)
  {
   str8 *StringAt = Array->Strings + At;
   
   MemoryCopy(Buffer.Data + BufferIndex, StringAt->Data, StringAt->Size);
   BufferIndex += StringAt->Size;
   if(Char)
   {
    if(At + 1 != Array->Count)
    {
     Buffer.Data[BufferIndex] = Char;
     BufferIndex += 1;
    }
   }
  }
  
  Result.Data = Buffer.Data;
  Result.Size = BufferIndex;
 }
 
 
 return Result;
}

internal void 
S8ArrayAppendMultiple_(str8_array *Array, u64 Count, str8 *Strings)
{
 for EachIndex(At, Count)
 {
  S8ArrayAppendTo(Array, Strings[At]);
 }
}
