//~ Strings

#include <stdio.h>

#if !BASE_EXTERNAL_LIBS || RL_LIBS_IMPLEMENTATION
// NOTE(luca): RL_LIBS_IMPLEMENTATION is set when everything is to be compiled statically.
# define STB_SPRINTF_STATIC
# define STB_SPRINTF_IMPLEMENTATION
#endif 
NO_WARNINGS_BEGIN
#include "lib/stb_sprintf.h"
NO_WARNINGS_END

global_variable arena *StringsScratch = 0;

#define SetStringsScratch(Arena) SetStringsScratch_(Arena, #Arena, __FILE__, __LINE__) 

internal void
SetStringsScratch_(arena *Arena, const char *Name, const char *FileName, s32 Line)
{
 Assert(Arena);
 StringsScratch = Arena;
 
#if 0
 Log("%s(%d): Set strings scratch to '%s'.\n", FileName, Line, Name);
#endif
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
S8Match(str8 A, str8 B, b32 BIsPrefix)
{
 b32 Match = false;
 
 b32 Requirements = false;
 Requirements |= ( BIsPrefix && (B.Size <= A.Size));
 Requirements |= (!BIsPrefix && (B.Size == A.Size));
 
 u64 MinSize = Min(A.Size, B.Size);
 
 if(Requirements)
 {
  Match = true;
  for EachIndex(Idx, MinSize)
  {
   if((A.Data[Idx] != B.Data[Idx]))
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
 
 str8 Result = PushS8(Arena, Prefix.Size + Suffix.Size);
 
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

internal str8
Str8DupCString(arena *Arena, char *String)
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
 
 S8OverPush(Arena, Result, KB(64), Result.Size - 1)
 {  
  Result.Size = (u64)stbsp_vsprintf((char *)Result.Data, Format, Args);
  
  // Null terminate just in case.
  Result.Data[Result.Size] = 0;
  
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
#if !BASE_EXTERNAL_LIBS
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