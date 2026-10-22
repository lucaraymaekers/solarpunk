
#define ColorU32_Frost0 0xff8fbcbb
 #define ColorU32_Frost1 0xff88c0d0
 #define ColorU32_Frost2 0xff81a1c1
 #define ColorU32_Frost3 0xff5e81ac
 #define ColorU32_Snow0 0xffeceff4
 #define ColorU32_Snow1 0xffe5e9f0
 #define ColorU32_Snow2 0xffd8dee9
 #define ColorU32_Night0 0xff4c566a
 #define ColorU32_Night1 0xff434c5e
 #define ColorU32_Night2 0xff3b4252
 #define ColorU32_Night3 0xff2e3440
 #define ColorU32_Red 0xffbf616a
 #define ColorU32_Orange 0xffd08770
 #define ColorU32_Yellow 0xffebcb8b
 #define ColorU32_Green 0xffa3be8c
 #define ColorU32_Magenta 0xffb48ead
 #define ColorU32_Cyan 0xff81a1c1
 #define ColorU32_Blue 0xff5e81ac
 #define ColorU32_Black 0xff000000
 #define ColorU32_Primary 0xffFFCE00
 #define ColorU32_Secondary 0xff3016B0
 #define ColorU32_Contrast 0xff282525
 
v4 Color_Frost0 = ColorU32(Frost0);
 v4 Color_Frost1 = ColorU32(Frost1);
 v4 Color_Frost2 = ColorU32(Frost2);
 v4 Color_Frost3 = ColorU32(Frost3);
 v4 Color_Snow0 = ColorU32(Snow0);
 v4 Color_Snow1 = ColorU32(Snow1);
 v4 Color_Snow2 = ColorU32(Snow2);
 v4 Color_Night0 = ColorU32(Night0);
 v4 Color_Night1 = ColorU32(Night1);
 v4 Color_Night2 = ColorU32(Night2);
 v4 Color_Night3 = ColorU32(Night3);
 v4 Color_Red = ColorU32(Red);
 v4 Color_Orange = ColorU32(Orange);
 v4 Color_Yellow = ColorU32(Yellow);
 v4 Color_Green = ColorU32(Green);
 v4 Color_Magenta = ColorU32(Magenta);
 v4 Color_Cyan = ColorU32(Cyan);
 v4 Color_Blue = ColorU32(Blue);
 v4 Color_Black = ColorU32(Black);
 v4 Color_Primary = ColorU32(Primary);
 v4 Color_Secondary = ColorU32(Secondary);
 v4 Color_Contrast = ColorU32(Contrast);
 


enum base_note
{
BaseNote_C,
 BaseNote_D,
 BaseNote_E,
 BaseNote_F,
 BaseNote_G,
 BaseNote_A,
 BaseNote_B,
 BaseNote_Count
};
typedef enum base_note base_note;

enum note_pitch
{
Note_C,
 Note_Cs,
 Note_D,
 Note_Ds,
 Note_E,
 Note_F,
 Note_Fs,
 Note_G,
 Note_Gs,
 Note_A,
 Note_As,
 Note_B,
 Note_Count
};
typedef enum note_pitch note_pitch;

s32 NoteStepFromPitch[] = {
0,0,1,1,2,3,3,4,4,5,5,6,
};

b32 NoteIsWhite[] = {
true,false,true,false,true,true,false,true,false,true,false,true,
};

#define APP_MIDIInputKeys app_button_state KeyA; app_button_state KeyW; app_button_state KeyS; app_button_state KeyE; app_button_state KeyD; app_button_state KeyF; app_button_state KeyT; app_button_state KeyG; app_button_state KeyY; app_button_state KeyH; app_button_state KeyU; app_button_state KeyJ; 
str8 NotePitchStrings[] = {
{(u8 *)"C", 1},{(u8 *)"C#", 2},{(u8 *)"D", 1},{(u8 *)"D#", 2},{(u8 *)"E", 1},{(u8 *)"F", 1},{(u8 *)"F#", 2},{(u8 *)"G", 1},{(u8 *)"G#", 2},{(u8 *)"A", 1},{(u8 *)"A#", 2},{(u8 *)"B", 1},
};

#define Win32ColemakMIDIKeySymbolsDef 'A', 'W', 'R', 'F', 'S', 'T', 'G', 'D', 'J', 'H', 'L', 'N', 
#define Win32QwertyMIDIKeySymbolsDef 'A', 'W', 'S', 'E', 'D', 'F', 'T', 'G', 'Y', 'H', 'U', 'J', 
#define LinuxColemakMIDIKeySymbolsDef XK_a,XK_w,XK_r,XK_f,XK_s,XK_t,XK_g,XK_d,XK_j,XK_h,XK_l,XK_n,
#define LinuxQwertyMIDIKeySymbolsDef XK_a,XK_w,XK_s,XK_e,XK_d,XK_f,XK_t,XK_g,XK_y,XK_h,XK_u,XK_j,
