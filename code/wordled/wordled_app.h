#if !defined(HANDMADE_H)

//~ Constants
#define WORDLE_LENGTH 5

//~ Types
enum square_colors
{
    SquareColor_Gray,
    SquareColor_Yellow,
    SquareColor_Green,
    SquareColor_Count
};

struct loaded_bitmap
{
    int Width;
    int Height;
    u32 *Pixels;
};

//~ Colors

typedef v3 color_rgb;
#define color_rgb(A) color_rgb{(A), (A), (A)}

inline color_rgb
operator*(color_rgb A, f32 B)
{
    color_rgb Result;
    
    Result.X = A.X * B;
    Result.Y = A.Y * B;
    Result.Z = A.Z * B;
    
    return Result;
}

inline color_rgb
operator*(f32 A, color_rgb B)
{
    color_rgb Result = B*A;
    
    return Result;
}

inline color_rgb
operator+(color_rgb A, f32 B)
{
    color_rgb Result;
    
    Result.X = A.X + B;
    Result.Y = A.Y + B;
    Result.Z = A.Z + B;
    
    return Result;
}

inline color_rgb
operator+(f32 A, color_rgb B)
{
    color_rgb Result = B+A;
    
    return Result;
}

inline color_rgb
operator-(color_rgb A)
{
    color_rgb Result;
    
    Result.X = -A.X;
    Result.Y = -A.Y;
    Result.Z = -A.Z;
    
    return Result;
}

#include "lib/stb_truetype.h"

//~ Game
struct game_font
{
    stbtt_fontinfo Info;
    s32 Ascent;
    s32 Descent;
    s32 LineGap;
    v2 BoundingBox[2];
    b32 Initialized; // For debugging.
};

struct game_state
{
    u32 PatternGrid[6][5];
    u32 SelectedColor;
    u32 ExportedPatternIndex;
    // TODO(luca): There is no need for utf8 since we know it will be between a-z.  We could use a single byte string and convert to char when appending to the input buffer.
    // But the only benefit would be storing less bytes which isn't really a big deal for 5 characters.
    b32 WordleWordIsValid;
    rune WordleWord[WORDLE_LENGTH];
    
    arena Arena;
    
    game_font RegularFont;
    game_font ItalicFont;
    game_font BoldFont;
    
    b32 TextInputMode;
    rune TextInputText[5];
    u32 TextInputCount;
};

#define HANDMADE_H
#endif
