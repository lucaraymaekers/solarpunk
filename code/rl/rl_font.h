#if !defined(RL_FONT_H)
#define RL_FONT_H
// TODO(luca): UTF8 handling 

enum font_kind
{
 FontKind_Text,
 FontKind_Icon
};
typedef enum font_kind font_kind;

typedef struct font font;
struct font
{
 s32 FirstCodepoint;
 s32 CodepointCount;
 s32 FirstPackedCharIdx;
 f32 Scale;
 f32 HeightPx;
 
 stbtt_fontinfo Info;
 s32 Ascent;
 s32 Descent;
 s32 LineGap;
 v2 BoundingBox[2];
 b32 Initialized;
};

typedef struct font_atlas font_atlas;
struct font_atlas
{
 s32 Width;
 s32 Height;
 u8 *Data;
 
 union
 {
  font *Fonts[2];
  struct 
  {  
   font *TextFont;
   font *IconsFont;
  };
 };
 
 f32 PixelScaleWidth;
 f32 PixelScaleHeight;
 
 stbtt_packedchar *PackedChars;
 stbtt_aligned_quad *AlignedQuads;
};

//~ API

internal s32
GetPackedCharIdx(font *Font, rune Char)
{
 s32 PackedCharIdx = Font->FirstPackedCharIdx + (Char - Font->FirstCodepoint);
 return PackedCharIdx; 
}

internal f32
GetBaseline(font *Font)
{
 f32 Result = Font->Scale*(f32)(Font->Ascent - Font->Descent + Font->LineGap);
 return Result;
}

internal void
InitFont(font *Font, char *FilePath)
{
 str8 File = OS_ReadEntireFileIntoMemory(FilePath);
 
 if(File.Size)
 {
  if(stbtt_InitFont(&Font->Info, File.Data, stbtt_GetFontOffsetForIndex(File.Data, 0)))
  {
   Font->Info.data = (u8 *)File.Data;
   
   s32 X0, Y0, X1, Y1;
   stbtt_GetFontBoundingBox(&Font->Info, &X0, &Y0, &X1, &Y1);
   Font->BoundingBox[0] = V2S32(X0, Y0);
   Font->BoundingBox[1] = V2S32(X1, Y1);
   stbtt_GetFontVMetrics(&Font->Info, &Font->Ascent, &Font->Descent, &Font->LineGap);
   Font->Initialized = true;
  }
  else
  {
   ErrorLog("Could not initialize font info from file.\n");
  }
 }
 else
 {
  ErrorLog("Could not read font file.\n");
 }
}

#endif //RL_FONT_H
