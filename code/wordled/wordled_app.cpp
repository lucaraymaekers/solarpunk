#define BASE_EXTERNAL_LIBS 1
#define BASE_NO_ENTRYPOINT 1
#include "base/base.h"
#include "base/base.c"

#include "wordled_intrinsics.h"
#include "wordled_math.h"
#include "wordled_app.h"

#if OS_LINUX
#include <curl/curl.h>
#endif
#include <time.h>
#include "rl/generated/rl_tables.meta.c"
#include "rl/rl_platform.h"
#include "rl/rl_libs.h"

#if OS_WINDOWS
# pragma comment(linker, "/export:GetAudioSamples")
# pragma comment(linker, "/export:UpdateAndRender")
#endif

//- Helpers 
internal color_rgb
GetColorRGBForColorIndex(u32 Index)
{
 color_rgb Result = {};
 color_rgb ColorGray = {0.23f, 0.23f, 0.24f};
 color_rgb ColorYellow = {0.71f, 0.62f, 0.23f};
 color_rgb ColorGreen = {0.32f, 0.55f, 0.31f};
 
 if(0) {}
 else if(Index == SquareColor_Gray)   Result = ColorGray;
 else if(Index == SquareColor_Yellow) Result = ColorYellow;
 else if(Index == SquareColor_Green)  Result = ColorGreen;
 
 return Result;
}

//- Sound 
internal s16
GetSineSound(u32 SampleRate)
{
 s16 Result = 0;
 
 s16 ToneVolume = (s16)((1 << 15) * 0.01f);
 f32 ToneHz = 440;
 f32 WavePeriod = 0.f;
 local_persist f32 tSine = 0.f;
 f32 SineValue = 0.f;
 
 WavePeriod = (f32)SampleRate/ToneHz;
 SineValue = Sin(tSine);
 tSine += 2.0f*Pi32*1.0f/(f32)WavePeriod;
 if(tSine > 2*Pi32)
 {
  tSine -= 2*Pi32;
 }
 
 Result = (s16)(SineValue * ToneVolume);
 
 return Result;
}

#if 0
internal void
GameOutputSound(game_state *GameState, app_sound_buffer *SoundBuffer)
{
 s16 SampleValue = 0;
 
 s16 *SampleOut = SoundBuffer->Samples;
 for(int SampleIndex = 0;
     SampleIndex < SoundBuffer->SampleCount;
     ++SampleIndex)
 {
  
#if 0 
  SampleValue = GetSineSound(SoundBuffer->SamplesPerSecond);
#endif
  
  *SampleOut++ = SampleValue;
  *SampleOut++ = SampleValue;
 }
}
#endif

//- Rendering 
internal void
DrawRectangle(app_offscreen_buffer *Buffer,
              v2 vMin, v2 vMax,
              color_rgb Color)
{
 int MinX = RoundReal32ToInt32(vMin.X);
 int MaxX = RoundReal32ToInt32(vMax.X);
 int MinY = RoundReal32ToInt32(vMin.Y);
 int MaxY = RoundReal32ToInt32(vMax.Y);
 
 if(MinX < 0)
 {
  MinX = 0;
 }
 
 if(MinY < 0)
 {
  MinY = 0;
 }
 
 if(MaxX > Buffer->Dim.X)
 {
  MaxX = Buffer->Dim.X;
 }
 
 if(MaxY > Buffer->Dim.Y)
 {
  MaxY = Buffer->Dim.Y;
 }
 
 u32 ColorValue = 
    ((u32)(0xFF << 3*8)) | 
 (RoundReal32ToUInt32(Color.X * 255.0f) << 2*8) | 
 (RoundReal32ToUInt32(Color.Y * 255.0f) << 1*8) |
 (RoundReal32ToUInt32(Color.Z * 255.0f) << 0*8);
 
 u8 *Row = ((u8 *)(Buffer->Pixels) + 
            MinX*Buffer->BytesPerPixel + 
            MinY*Buffer->Pitch);
 
 for(int Y = MinY;
     Y < MaxY;
     Y++)
 {
  u32 *Pixel = (u32 *)Row;
  
  for(int X = MinX;
      X < MaxX;
      X++)
  {
   *Pixel++ = ColorValue;
  }
  
  Row += Buffer->Pitch;
 }
 
}

internal void
DrawCharacter(app_offscreen_buffer *Buffer,  u8 *FontBitmap,
              int FontWidth, int FontHeight, 
              int XOffset, int YOffset,
              color_rgb Color)
{
 s32 MinX = 0;
 s32 MinY = 0;
 s32 MaxX = FontWidth;
 s32 MaxY = FontHeight;
 
 if(XOffset < 0)
 {
  MinX = -XOffset;
  XOffset = 0;
 }
 if(YOffset < 0)
 {
  MinY = -YOffset;
  YOffset = 0;
 }
 if(XOffset + FontWidth > Buffer->Dim.X)
 {
  MaxX -= ((XOffset + FontWidth) - Buffer->Dim.X);
 }
 if(YOffset + FontHeight > Buffer->Dim.Y)
 {
  MaxY -= ((YOffset + FontHeight) - Buffer->Dim.Y);
 }
 
 u8 *Row = (u8 *)(Buffer->Pixels) + 
 (YOffset*Buffer->Pitch) +
 (XOffset*Buffer->BytesPerPixel);
 
 for(int  Y = MinY;
     Y < MaxY;
     Y++)
 {
  u32 *Pixel = (u32 *)Row;
  for(int X = MinX;
      X < MaxX;
      X++)
  {
   u8 Brightness = FontBitmap[Y*FontWidth+X];
   f32 Alpha = ((f32)Brightness/255.0f);
   
   f32 DR = (f32)((*Pixel >> 16) & 0xFF);
   f32 DG = (f32)((*Pixel >> 8) & 0xFF);
   f32 DB = (f32)((*Pixel >> 0) & 0xFF);
   
   f32 R = Color.X*255.0f*Alpha + DR*(1-Alpha);
   f32 G = Color.Y*255.0f*Alpha + DG*(1-Alpha);
   f32 B = Color.Z*255.0f*Alpha +  DB*(1-Alpha);
   
   u32 Value = ((u32)(0xFF << 24) |
                ((u32)(R) << 16) |
                ((u32)(G) << 8) |
                ((u32)(B) << 0));
   *Pixel++ = Value;
  }
  
  Row += Buffer->Pitch;
 }
}

internal void
DrawText(app_offscreen_buffer *Buffer, game_font *Font, f32 FontScale,
         u32 TextLen, u8 *Text, v2 Offset, color_rgb Color, b32 IsUTF8)
{
 Assert(Font->Initialized);
 
 for(u32 TextIndex = 0;
     TextIndex < TextLen;
     TextIndex++)
 {
  rune CharAt = 0;
  if(IsUTF8)
  {
   CharAt = *(((rune *)Text) + TextIndex);
  }
  else
  {
   CharAt = Text[TextIndex];
  }
  
  s32 FontWidth, FontHeight;
  s32 AdvanceWidth, LeftSideBearing;
  s32 X0, Y0, X1, Y1;
  u8 *FontBitmap = 0;
  // TODO(luca): Get rid of malloc.
  FontBitmap = stbtt_GetCodepointBitmap(&Font->Info, 
                                        FontScale, FontScale, 
                                        CharAt, 
                                        &FontWidth, &FontHeight, 0, 0);
  stbtt_GetCodepointBitmapBox(&Font->Info, CharAt, 
                              FontScale, FontScale, 
                              &X0, &Y0, &X1, &Y1);
  f32 YOffset = Offset.Y + (f32)Y0;
  stbtt_GetCodepointHMetrics(&Font->Info, CharAt, &AdvanceWidth, &LeftSideBearing);
  f32 XOffset = Offset.X + (f32)LeftSideBearing*FontScale;
  
  DrawCharacter(Buffer, FontBitmap, FontWidth, FontHeight, (int)XOffset, (int)YOffset, Color);
  
  Offset.X += ((f32)AdvanceWidth*FontScale);
  free(FontBitmap);
 }
}

internal void
DrawTextWithAlternatingFonts(game_font *Font1, game_font *Font2,
                             app_offscreen_buffer *Buffer, 
                             f32 FontScale,
                             rune *Text, u32 TextLen, v2 Offset, color_rgb Color)
{
 b32 FontToggle = false;
 for(u32 TextIndex = 0;
     TextIndex < TextLen;
     TextIndex++)
 {
  FontToggle = !FontToggle;
  game_font *Font = (FontToggle) ? Font1 : Font2;
  Assert(Font->Initialized);
  
  rune CharAt = Text[TextIndex];
  
  s32 FontWidth, FontHeight;
  s32 AdvanceWidth, LeftSideBearing;
  s32 X0, Y0, X1, Y1;
  u8 *FontBitmap = 0;
  // TODO(luca): Get rid of malloc.
  FontBitmap = stbtt_GetCodepointBitmap(&Font->Info, 
                                        FontScale, FontScale, 
                                        CharAt, 
                                        &FontWidth, &FontHeight, 0, 0);
  stbtt_GetCodepointBitmapBox(&Font->Info, CharAt, 
                              FontScale, FontScale, 
                              &X0, &Y0, &X1, &Y1);
  f32 YOffset = Offset.Y + (f32)Y0;
  stbtt_GetCodepointHMetrics(&Font->Info, CharAt, &AdvanceWidth, &LeftSideBearing);
  f32 XOffset = Offset.X + (f32)LeftSideBearing*FontScale;
  
  DrawCharacter(Buffer, FontBitmap, FontWidth, FontHeight, (int)XOffset, (int)YOffset, Color);
  
  Offset.X += ((f32)AdvanceWidth*FontScale);
  free(FontBitmap);
 }
}

//- Wordled 
internal b32
ValidLetterCountInGuess(rune *Word, u8 *Guess, rune Letter)
{
 b32 Valid = false;
 
 int CountInGuess = 0;
 int CountInWord = 0;
 for(int ScanIndex = 0;
     ScanIndex < WORDLE_LENGTH;
     ScanIndex++)
 {
  if(Letter == Word[ScanIndex])
  {
   CountInWord++;
  }
  if(Letter == Guess[ScanIndex])
  {
   CountInGuess++;
  }
 }
 
 Valid = (CountInGuess <= CountInWord);
 
 return Valid;
}

//- Curl 
internal umm 
CurlCBLog(void *Buffer, umm ItemsSize, umm Size, void *UserPointer)
{
 umm Result = 0;
 if(Size == 1)
 {
  Result = ItemsSize;
 }
 else
 {
  Result = Size;
 }
 
 Log((char *)Buffer);
 return Result;
}

internal umm 
CurlCBWriteToArena(void *Buffer, umm Size, umm DataSize, void *UserPointer)
{
 Assert(Size == 1);
 umm Result = DataSize;
 
 arena *Arena = (arena *)UserPointer;
 
 MemoryCopy(((u8 *)Arena->Base + Arena->Pos), Buffer, DataSize);
 Assert(Arena->Pos + DataSize <= Arena->Size);
 Arena->Pos += DataSize;
 
 return Result;
}

struct get_todays_wordle_curl_params
{
 b32 *Done;
 arena *Arena;
 rune *Word;
};

typedef struct platform_work_queue platform_work_queue;

#define PLATFORM_WORK_QUEUE_CALLBACK(Name) void Name(platform_work_queue *Queue, void *Data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

struct platform_work_queue_entry
{
 platform_work_queue_callback *Callback;
 void *Data;
};

PLATFORM_WORK_QUEUE_CALLBACK(GetTodaysWordleCurl)
{
 arena *Arena = ((get_todays_wordle_curl_params *)Data)->Arena;
 b32 *Done = ((get_todays_wordle_curl_params *)Data)->Done;
 rune *Word = ((get_todays_wordle_curl_params *)Data)->Word;
 
 char URL[] = "https://www.nytimes.com/svc/wordle/v2";
 
 time_t Now = 0;
 time(&Now);
 struct tm *LocalTimeNow = localtime(&Now);
 
 u8 URLBuffer[256] = {};
 stbsp_sprintf((char *)URLBuffer, "%s/%d-%02d-%02d.json", URL,
               LocalTimeNow->tm_year + 1900,
               LocalTimeNow->tm_mon + 1,
               LocalTimeNow->tm_mday);
 
 umm StartPos = Arena->Pos;
 
 
#if OS_LINUX 
 // Get today's wordle
 {
  CURL *CurlHandle = curl_easy_init();
  
  // NOTE(luca): init to NULL is important
  struct curl_slist *Headers = 0;
  
  // NOTE(luca): Cosplay as my computer.
  Headers = curl_slist_append(Headers, "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:134.0) Gecko/20100101 Firefox/134.0");
#if 0        
  curl_easy_setopt(CurlHandle, CURLOPT_HEADERFUNCTION, CurlCBLog);
#endif
        umm MatchAt = 0;
        b32 IsMatch = false;
        char *Str = (char *)((u8 *)Arena->Base + StartPos);
        
  if(CURLE_OK == (curl_easy_setopt(CurlHandle, CURLOPT_VERBOSE, 0L)) &&
     CURLE_OK == (curl_easy_setopt(CurlHandle, CURLOPT_URL, URLBuffer)) &&
     CURLE_OK == (curl_easy_setopt(CurlHandle, CURLOPT_WRITEFUNCTION, CurlCBWriteToArena)) &&
     CURLE_OK == (curl_easy_setopt(CurlHandle, CURLOPT_WRITEDATA, Arena)) &&
     CURLE_OK == (curl_easy_setopt(CurlHandle, CURLOPT_FOLLOWLOCATION, 1L)) &&
     CURLE_OK == (curl_easy_setopt(CurlHandle, CURLOPT_HTTPHEADER, Headers)) &&
     CURLE_OK == (curl_easy_perform(CurlHandle)))
  {
   umm Size = (Arena->Pos - StartPos);
   char MatchStr[] = "\"solution\":";
   for(umm ScanIndex = 0;
       ((ScanIndex < Size) && (!IsMatch));
       ScanIndex++)
   {
    MatchAt = ScanIndex;
    IsMatch = true;
    for(umm MatchIndex = 0;
        ((MatchIndex < (sizeof(MatchStr) - 1)) &&
         (MatchIndex + ScanIndex < Size));
        MatchIndex++)
    {
     if(MatchStr[MatchIndex] != Str[ScanIndex + MatchIndex])
     {
      IsMatch = false;
      break;
     }
    }
            }
            
            curl_easy_cleanup(CurlHandle);
  }
  else
  {
   Log("Failed to get HTML.");
  }
  
  if(IsMatch)
  {
   MatchAt += (sizeof("\"solution\":\"") - 1);
   for(umm CharIndex = 0;
       CharIndex < WORDLE_LENGTH;
       CharIndex++)
   {
    Word[CharIndex] = Str[MatchAt + CharIndex];
   }
  }
  
 }
 
#endif
 
 *Done = true;
}

//- Input 
internal void 
AppendCharToInputText(game_state *GameState, rune Codepoint)
{
 if(GameState->TextInputCount < ArrayCount(GameState->TextInputText))
 {
  GameState->TextInputText[GameState->TextInputCount++] = Codepoint;
 }
}

//- Font 
internal void
InitFont(game_font *Font, char *FilePath)
{
 str8 File = OS_ReadEntireFileIntoMemory(FilePath);
 
 if(File.Data)
 {
  if(stbtt_InitFont(&Font->Info, (u8 *)File.Data, stbtt_GetFontOffsetForIndex((u8 *)File.Data, 0)))
  {
   Font->Info.data = (u8 *)File.Data;
   
   s32 X0, Y0, X1, Y1;
   stbtt_GetFontBoundingBox(&Font->Info, &X0, &Y0, &X1, &Y1);
   Font->BoundingBox[0] = v2{(f32)X0, (f32)Y0};
   Font->BoundingBox[1] = v2{(f32)X1, (f32)Y1};
   stbtt_GetFontVMetrics(&Font->Info, &Font->Ascent, &Font->Descent, &Font->LineGap);
   Font->Initialized = true;
  }
  else
  {
   // TODO(luca): Logging
  }
 }
 else
 {
  // TODO(luca): Logging
 }
}

//- Game callbacks 
C_LINKAGE 
UPDATE_AND_RENDER(UpdateAndRender)
{
 game_state *GameState = (game_state *)Memory->Memory;
 
 ExeDirPath = Memory->ExeDirPath;
 
 if(!Memory->Initialized)
 {
  GameState->SelectedColor = SquareColor_Yellow;
  GameState->ExportedPatternIndex = 0;
  
  u64 MemoryPos = sizeof(game_state);
  u64 AlignedPos = AlignPow2(MemoryPos, AlignOf(arena));
  GameState->Arena.Base = (u8 *)Memory->Memory + AlignedPos;
  GameState->Arena.Size = GB(1);
  GameState->WordleWordIsValid = false;
  
  GameState->TextInputCount = 0;
  GameState->TextInputMode = true;
  
#if OS_LINUX
  curl_global_init(CURL_GLOBAL_ALL);
#endif
  
  FrameArena = PushArena(&GameState->Arena, MB(64), false);
  
  InitFont(&GameState->RegularFont, PathFromExe(&GameState->Arena, S8("../data/font_regular.ttf")));
  InitFont(&GameState->ItalicFont, PathFromExe(&GameState->Arena, S8("../data/font_italic.ttf")));
  InitFont(&GameState->BoldFont, PathFromExe(&GameState->Arena, S8("../data/font_bold.ttf")));
  Memory->Initialized = true;
 }
 
 FrameArena = &GameState->Arena;
 SetStringsScratch(FrameArena);
 FrameArena->Pos = 0;
 
 // Input
 {
  if(Input->Text.Count != 0) DebugBreak();
  // Text input 
  {
   // TODO(luca): This should use the minimum in case where there are more keys in the Keyboard.TextInputBuffer than in GameState->TextInputText
   for EachTextButton(Key, Id, Input)
   {
    // TODO(luca): Check that Codepoint is not shrunk because that would cause inequal value to still be equal if the shrunk part is equal. 
    
    u32 *InputCount = &GameState->TextInputCount;
    
    if(!Key->IsSymbol)
    {
     if(0) {}
                    else if(Key->Codepoint == 'u' && Key->Modifiers & PlatformKeyModifier_Control)
     {
      *InputCount = 0;
     }
     else if(Key->Codepoint == 'h' && Key->Modifiers & PlatformKeyModifier_Control)
     {
      if(*InputCount)
      {
       *InputCount -= 1;
      }
                    }
                    else if('1' <= Key->Codepoint && Key->Codepoint <= '3')
                    {
                        GameState->SelectedColor = (u32)(Key->Codepoint - '1');
                    }
     else
     {
      if((Key->Codepoint >= 'A' &&
          (Key->Codepoint) <= 'Z'))
      {
       Key->Codepoint = (Key->Codepoint - 'A') + 'a';
      }
      
      if((Key->Codepoint >= 'a' && Key->Codepoint <= 'z') || Key->Codepoint == ' ')
      {
       AppendCharToInputText(GameState, Key->Codepoint);
      }
     }
    }
    else
    {
     if(0) {}
     else if(Key->Codepoint == PlatformKey_BackSpace)
     {
      if(*InputCount)
      {
       *InputCount -= 1;
      }
     }
     else if(Key->Codepoint == PlatformKey_Return)
     {
      for(u32 CharIdx = 0;
          CharIdx < ArrayCount(GameState->WordleWord);
          CharIdx++)
      {
       if(CharIdx < GameState->TextInputCount)
       {
        GameState->WordleWord[CharIdx] = (GameState->TextInputText[CharIdx]);
       }
       else
       {
        GameState->WordleWord[CharIdx] = ' ';
       }
      }
      
      GameState->WordleWordIsValid = true;
     }
    }
    
   }
  }
  
  if(WasPressed(Input->Mouse.Buttons[PlatformMouseButton_ScrollUp]))
  {
   u32 ColorIndex = GameState->SelectedColor;
   if(ColorIndex) ColorIndex--;
   GameState->SelectedColor = ColorIndex;
  }
  if(WasPressed(Input->Mouse.Buttons[PlatformMouseButton_ScrollDown]))
  {
   u32 ColorIndex = GameState->SelectedColor;
   if(ColorIndex + 1 < SquareColor_Count) ColorIndex++;
   GameState->SelectedColor = ColorIndex;
  }
  
  if(WasPressed(Input->ActionLeft))
  {
   GameState->TextInputMode = !GameState->TextInputMode;
  }
  
  if(WasPressed(Input->ActionUp))
  {
   GameState->WordleWordIsValid = false;
   
   //GetTodaysWordleCurl();
  }
 }
 
 Assert(GameState->SelectedColor < SquareColor_Count);
 
 // Clear
 MemorySet(Buffer->Pixels, 0, (u64)(Buffer->Dim.Y*Buffer->Pitch));
 
 // Draw wordle rectangles
 f32 WordleRectWidth = 48.0f;
 s32 WordleRectsRows = 6;
 s32 WordleRectsColumns = 5;
 v2 WordleRectsBase = 0.5f*v2{
  Buffer->Dim.X - WordleRectsColumns*WordleRectWidth, 
  Buffer->Dim.Y - WordleRectsRows*WordleRectWidth
 };
 v2 WordleRectsPadding = {2.0f, 2.0f};
 {
  f32 Width = WordleRectWidth;
  v2 Base = WordleRectsBase;
  s32 Rows = WordleRectsRows;
  s32 Columns = WordleRectsColumns;
  
  v2 Padding = WordleRectsPadding;
  
  s32 SelectedX = CeilReal32ToInt32((f32)((f32)Input->Mouse.Pos.X - Base.X)/(f32)(Width + Padding.X)) - 1;
  s32 SelectedY = CeilReal32ToInt32((f32)((f32)Input->Mouse.Pos.Y - Base.Y)/(f32)(Width + Padding.Y)) - 1;
  
  v2 vMin = {};
  v2 vMax = {};
  
  vMin = Base;
  for(s32 Y = 0;
      Y < Rows;
      Y++)
  {
   for(s32 X = 0;
       X < Columns;
       X++)
   {
    color_rgb Color = {1.0f, 1.0f, 0.0f};
    
    vMax = vMin + Width;
    
    if((X == SelectedX) &&
       (Y == SelectedY))
    {
     if(Input->Mouse.Buttons[PlatformMouseButton_Left].EndedDown)
     {
      GameState->PatternGrid[Y][X] = GameState->SelectedColor;
     }
     Color = GetColorRGBForColorIndex(GameState->SelectedColor);
     DrawRectangle(Buffer, vMin, vMax, color_rgb(0.0f)); // Change to white
     DrawRectangle(Buffer, 
                   vMin + Padding, vMax - Padding,
                   Color);
    }
    else
    {
     u32 PatternValue = GameState->PatternGrid[Y][X];
     Color = GetColorRGBForColorIndex(PatternValue);
     DrawRectangle(Buffer, vMin, vMax, Color);
    }
    
    
    vMin.X += Padding.X + Width;
   }
   vMin.X = Base.X;
   vMin.Y += Padding.Y + Width;
  }
 }
 
 // Selected color wheel
 {
  v2 Padding = WordleRectsPadding;
  v2 SelectedBorder = WordleRectsPadding*1;
  v2 Base = WordleRectsBase;
  f32 Width = 0.5f*WordleRectWidth;
  color_rgb BorderColor = color_rgb(0.7f);
  color_rgb SelectedBorderColor = color_rgb(0.7f);
  
  Base.X -= (Width + Padding.X + SelectedBorder.X)+ Width ;
  Base.Y += SelectedBorder.Y;
  
  // Draw Background
  {
   v2 vMin = Base - Padding;
   v2 vMax = Base + Width + Padding;
   vMax.Y += (Width + Padding.Y + SelectedBorder.Y)*(SquareColor_Count - 1);
   DrawRectangle(Buffer, vMin, vMax, color_rgb(0.2f));
  }
  
  // Draw color rectangle
  {        
   v2 vMin = Base;
   for(u32 ColorIndex = 0;
       ColorIndex < SquareColor_Count;
       ColorIndex++)
   {
    color_rgb Color = GetColorRGBForColorIndex(ColorIndex);
    
    
    if(GameState->SelectedColor == ColorIndex)
    {
     DrawRectangle(Buffer, vMin, vMin + Width, 1.2f*Color);
    }
    
    DrawRectangle(Buffer, vMin + SelectedBorder, vMin + Width - SelectedBorder, Color);
    
    vMin.Y += Width + Padding.Y + SelectedBorder.X;
   }
  }
  
 }
 
 char Text[256] = {};
 int TextLen = 0;
 f32 YAdvance = 0.0f;
 f32 Baseline = 0.0f;
 v2 TextOffset = {};
 int AdvanceWidth = 0;
 
 game_font DefaultFont = GameState->RegularFont;
 
 // Prepare drawing of the guesses.
 {
  f32 FontScale = stbtt_ScaleForPixelHeight(&DefaultFont.Info, 24.0f);
  YAdvance = FontScale*(f32)(DefaultFont.Ascent - 
                             DefaultFont.Descent + 
                             DefaultFont.LineGap);
  Baseline = ((f32)DefaultFont.Ascent*FontScale);
  
  TextOffset = v2{16.0f, 16.0f + Baseline};
  
  if(GameState->WordleWordIsValid)
  {
   DrawText(Buffer, &DefaultFont, FontScale, 
            ArrayCount(GameState->WordleWord), (u8 *)GameState->WordleWord,
            TextOffset + -v2{8.0f, 0.0f}, color_rgb(1.0f), true);
   
   TextOffset.Y += YAdvance*2.0f;
   
   //-Matche the pattern
   rune *Word = GameState->WordleWord;
   
   str8 WordsFile = OS_ReadEntireFileIntoMemory(PathFromExe(FrameArena, S8("../data/wordled/words.txt")));
   
   u64 WordsCount = WordsFile.Size / WORDLE_LENGTH;
   if(WordsFile.Data)
   {
    u8 *Words = (u8 *)WordsFile.Data;
    
    s32 PatternRowAt = 0;
    s32 PatternRowsCount = 6;
    
    for(PatternRowAt = 0;
        PatternRowAt < PatternRowsCount;
        PatternRowAt++)
    {
     s32 PatternMatches = 0;
     u8 *Guess = 0;
     
     for(u64 WordsIndex = 0;
         ((WordsIndex < WordsCount) &&
          (!PatternMatches));
         WordsIndex++)
     {
      // Match the pattern's row against the guess.
      // TODO(luca): Check if the guess == the word and skip it otherwise it would end the game.
      Guess = &Words[WordsIndex*(WORDLE_LENGTH+1)];
      PatternMatches = 1;
      for(s32 CharIndex = 0;
          ((CharIndex < WORDLE_LENGTH) &&
           (PatternMatches));
          CharIndex++)
      {
       u8 GuessCh = Guess[CharIndex];
       u32 PatternValue = GameState->PatternGrid[PatternRowAt][CharIndex];
       
       if(PatternValue == SquareColor_Green)
       {
        PatternMatches = (GuessCh == Word[CharIndex]);
       }
       else if(PatternValue == SquareColor_Yellow)
       {
        PatternMatches = 0;
        for(s32 CharAt = 0;
            CharAt < WORDLE_LENGTH;
            CharAt++)
        {
         if(Word[CharAt] == GuessCh)
         {
          if(CharAt != CharIndex)
          {
           // TODO(luca): Should also check that position does not match.
           PatternMatches = ValidLetterCountInGuess(Word, Guess, GuessCh);
          }
          else
          {
           PatternMatches = 0;
          }
          
          break;
         }
        }
        
       }
       // TODO(luca): Have one that can be either yellow/green
#if 0
       else if(PatternValue == 1)
       {
        PatternMatches = 0;
        for(s32 CharAt = 0;
            CharAt < WORDLE_LENGTH;
            CharAt++)
        {
         if(Word[CharAt] == GuessCh)
         {
          PatternMatches = ValidLetterCountInGuess(Word, Guess, GuessCh);
          break;
         }
        }
       }
#endif
       else if(PatternValue == SquareColor_Gray)
       {
        PatternMatches = 1;
        for(s32 CharAt = 0;
            CharAt < WORDLE_LENGTH;
            CharAt++)
        {
         if(Word[CharAt] == GuessCh)
         {
          PatternMatches = 0;
          break;
         }
        }
       }
      }
     }
     
     if(PatternMatches)
     {
      DrawText(Buffer, &DefaultFont, FontScale,
               WORDLE_LENGTH, Guess, 
               TextOffset, color_rgb(1.0f), false);
     }
     else
     {
      u8 MatchText[] = "no match";
      DrawText(Buffer, &GameState->ItalicFont, FontScale, 
               sizeof(MatchText) - 1, MatchText, 
               TextOffset, color_rgb(0.6f), false);
     }
     TextOffset.Y += YAdvance;
    }
   }
   
   OS_FreeFileMemory(WordsFile);
  }
 }
 
 // NOTE(luca): Debug code for drawing inputted text.
#if 1
 {
  f32 FontScale = stbtt_ScaleForPixelHeight(&DefaultFont.Info, 22.0);
  
  v2 Offset = {100.0f, 30.0f};
  
  f32 TextHeight = FontScale*(f32)(DefaultFont.Ascent - 
                                   DefaultFont.Descent + 
                                   DefaultFont.LineGap);
  f32 TextWidth = 0;
  for(u32 InputIndex = 0;
      InputIndex < GameState->TextInputCount;
      InputIndex++)
  {
   s32 LeftSideBearing;
   rune Codepoint = GameState->TextInputText[InputIndex];
   stbtt_GetCodepointHMetrics(&DefaultFont.Info, Codepoint,
                              &AdvanceWidth, &LeftSideBearing);
   TextWidth += (f32)AdvanceWidth*FontScale;
  }
  
  Baseline = ((f32)DefaultFont.Ascent*FontScale);
  
  color_rgb ColorBorder = color_rgb(1.0f);
  color_rgb ColorBG = {0.007f, 0.007f, 0.007f};
  color_rgb ColorFG = {0.682f, 0.545f, 0.384f};
  
  f32 CursorWidth = 7.0f;
  f32 CursorHeight = TextHeight;
  
  // Draw text box
  {        
   v2 vMin = {Offset.X, Offset.Y - Baseline};
   v2 vMax = {Offset.X + TextWidth + CursorWidth, vMin.Y + TextHeight};
   DrawRectangle(Buffer, vMin + -2, vMax + 2, ColorBorder);
   DrawRectangle(Buffer, vMin + -1, vMax + 1, ColorBG);
  }
  
  // TODO(luca): Placeholder text when text is empty. 
  // Draw the text
  {        
   DrawText(Buffer, &DefaultFont, FontScale,
            GameState->TextInputCount, (u8 *)GameState->TextInputText,  
            Offset, ColorFG, true);
   
   Assert(GameState->TextInputCount <= ArrayCount(GameState->TextInputText));
  }
  
  // Draw cursor
  {
   v2 vMin = {Offset.X + TextWidth, Offset.Y - Baseline};
   v2 vMax = {vMin.X + CursorWidth, vMin.Y + TextHeight};
   DrawRectangle(Buffer, vMin, vMax, ColorFG);
  }
 }
 
 {
  f32 FontScale = stbtt_ScaleForPixelHeight(&DefaultFont.Info, 20.0);
  
  const char *HelpLines[] = 
  {
   "key binds",
   "- [Enter] to use new word",
   "- ['a-z'] type new word",
   "- [Control + 'u'] delete word",
   "- [Up] get today's wordle",
   "- [Left Button] paint with color",
   "- [Scroll Up/Down] change color",
   "- ['1'-'3'] change color",
  };
  u32 LinesCount = ArrayCount(HelpLines);
  // color_rgb TextColor = {0.8f, 0.8f, 0.8f};
  color_rgb TextColor = {0.71f, 0.62f, 0.53f};
  
  v2 Offset = {650.0f, 50.0f};
  YAdvance = FontScale*(f32)(DefaultFont.Ascent - 
                             DefaultFont.Descent + 
                             DefaultFont.LineGap);
  
  for(u32 LinesIndex = 0;
      LinesIndex < ArrayCount(HelpLines);
      LinesIndex++)
  {
   const char *LineText = HelpLines[LinesIndex];
   u64 LineTextLen = StringLength((char *)LineText);
   
   DrawText(Buffer, &DefaultFont, FontScale, 
            (u32)LineTextLen, (u8 *)LineText, Offset, TextColor, false);
   Offset.Y += YAdvance;
  }
  
 }
 
#endif
 
 return 0;
}

C_LINKAGE
GET_AUDIO_SAMPLES(GetAudioSamples)
{
}
