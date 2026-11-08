
enum font_kind
{
FontKind_Text,
 FontKind_Icons,
 FontKind_Music,
 FontKind_Count
};
typedef enum font_kind font_kind;

s32 FontCodepointCounts[] = {
2048,15,2048,
};

s32 FontFirstCodepoints[] = {
0x20,0x20,0xE000,
};

str8 FontPaths[] = {
{(u8 *)"font_text_regular.ttf", 21},
 {(u8 *)"font_icons.ttf", 14},
 {(u8 *)"font_music_text.otf", 19},
 
};

