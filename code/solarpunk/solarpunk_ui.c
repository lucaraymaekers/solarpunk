//-
// SolarPunk UI theme and widgets: warm, light cards that match the golden-hour sky.
// Every overlay (top bar, panel, labels, tooltips, help) draws with these.
//-

//~ Theme
#define Ui_Margin        10.f   // distance from the window edge
#define Ui_Pad           8.f    // inside a card
#define Ui_Gap           6.f    // between widgets
#define Ui_CardRadius    10.f
#define Ui_ButtonRadius  8.f
#define Ui_ButtonPadX    10.f
#define Ui_ButtonPadY    5.f

#define Ui_ColorCard        0xFFFFF9EF   // warm cream
#define Ui_ColorCardBorder  0xFFE6D2B0
#define Ui_ColorShadow      0x50402810
#define Ui_ColorText        0xFF3D3228   // dark warm brown
#define Ui_ColorTextDim     0xFF8C7B67
#define Ui_ColorTitle       0xFFB4552A   // terracotta
#define Ui_ColorAccent      0xFFE07A3F
#define Ui_ColorAccentHot   0xFFEA8D55
#define Ui_ColorAccentDown  0xFFC9672F
#define Ui_ColorOnAccent    0xFFFFFFFF
#define Ui_ColorButton      0xFFF3E5CE
#define Ui_ColorButtonHot   0xFFEBD6B2
#define Ui_ColorButtonDown  0xFFE0C59A
#define Ui_ColorRowAlt      0xFFF8EFE1
#define Ui_ColorRowHover    0xFFF0DFC2
#define Ui_ColorLine        0xFFE6D2B0
#define Ui_ColorSelected    0xFF2C7DAE   // the opened island
#define Ui_ColorDimBackdrop 0x66201810   // behind the help card

// NOTE: Status colours for text on the cream cards (darker than the 3D rings).
#define Ui_ColorOkText      0xFF2E8B4F
#define Ui_ColorWaitingText 0xFFC46F15
#define Ui_ColorShortText   0xFFC23B3B

//~ Text size
// NOTE: Big by default so it reads well on a projector; R3->TextSizeIdxPlusOne picks
// another size. The atlas is rebuilt at the start of a frame (UiUpdateTextSize), never
// in the middle, so the text already queued this frame stays valid.
global_variable f32 UiTextSizes[] = {20.f, 24.f, 28.f, 32.f, 36.f, 42.f};
#define Ui_DefaultTextSizeIdx 2
#define Ui_BaseTextSize       20.f   // the sizes below were designed for this
// NOTE: Only ASCII is drawn, so only those glyphs go in the atlas: all 2048 of them do
// not fit in the 1024x1024 atlas at the bigger sizes.
#define Ui_AtlasCodepoints    (0x7F - 0x20)

internal s32
UiTextSizeIdx(r3_state *R3)
{
 s32 Result = (R3->TextSizeIdxPlusOne > 0) ? R3->TextSizeIdxPlusOne - 1 : Ui_DefaultTextSizeIdx;
 Result = Clamp(0, Result, (s32)ArrayCount(UiTextSizes) - 1);
 return Result;
}

// NOTE: Call at the start of the frame, before any text is drawn or measured.
internal void
UiUpdateTextSize(app_state *App)
{
 f32 Want = UiTextSizes[UiTextSizeIdx(&App->R3)];
 if(App->Font.HeightPx != Want && App->Font.Initialized)
 {
  App->Font.CodepointCount = Min(App->Font.CodepointCount, Ui_AtlasCodepoints);
  font *Fonts[1];
  Fonts[0] = &App->Font;
  RenderBuildAtlas(&App->FontAtlas, ArrayCount(Fonts), Fonts, Want);
 }
}

// NOTE: How much bigger the text is than the base size; fixed widths scale with it.
internal f32
UiScale(app_state *App)
{
 f32 Result = App->Font.HeightPx/Ui_BaseTextSize;
 return Result;
}

internal f32
UiLineHeight(app_state *App)
{
 f32 Result = App->Font.HeightPx;
 return Result;
}

internal f32
UiButtonHeight(app_state *App)
{
 f32 Result = UiLineHeight(App) + 2.f*Ui_ButtonPadY;
 return Result;
}

internal f32
UiMeasure(app_state *App, str8 Text)
{
 f32 Result = GraphMeasureText(&App->FontAtlas, &App->Font, Text);
 return Result;
}

internal void
UiText(app_state *App, v2 Pos, str8 Text, u32 Color)
{
 GraphDrawText(&App->FontAtlas, &App->Font, Pos, Text, V4FromU32(Color));
}

// NOTE: Island status (see island_status) as a text colour for the cards.
internal u32
UiStatusText(u32 Status)
{
 u32 Result = Ui_ColorOkText;
 if(Status == IslandStatus_Waiting) Result = Ui_ColorWaitingText;
 if(Status == IslandStatus_Short)   Result = Ui_ColorShortText;
 return Result;
}

// NOTE: A rounded cream card with a soft shadow under it.
internal void
UiCard(v4 Box, f32 Radius)
{
 v4 Shadow = V4(Box.Min.X - 1.f, Box.Min.Y + 2.f, Box.Max.X + 1.f, Box.Max.Y + 5.f);
 DrawRect(Shadow, V4FromU32(Ui_ColorShadow), Radius + 2.f, 0.f, 6.f);
 DrawRect(Box, V4FromU32(Ui_ColorCard), Radius, 0.f, 0.f);
 DrawRect(Box, V4FromU32(Ui_ColorCardBorder), Radius, 1.f, 0.f);
}

// NOTE: True while the mouse should not reach the widget under it: the help card is
// open, or the summary dropdown (drawn on top) covers it.
internal b32
UiMouseBlocked(app_state *App, v2 MousePos)
{
 // The "?" button stays usable, so it can close the help card again.
 b32 Help = (App->R3.HelpOpen && !IsInsideRectV2(MousePos, App->R3.HelpButtonRect));
 b32 Result = (Help || IsInsideRectV2(MousePos, App->R3.PopupRect));
 return Result;
}

internal void
UiDot(v2 Pos, f32 Size, u32 Color)
{
 DrawRect(RectFromSize(Pos, V2(Size, Size)), V4FromU32(Color), .5f*Size, 0.f, 0.f);
}

//~ Hover hints
// NOTE: One hint per frame, drawn last (UiDrawHint) so it is on top of everything.
// Per-frame only, so it does not need to survive a hot reload.
global_variable char *UiHintText;
global_variable v2 UiHintPos;

internal void
UiSetHint(char *Text, v2 Pos)
{
 UiHintText = Text;
 UiHintPos = Pos;
}

internal void
UiDrawHint(app_state *App, v2 BufferDim)
{
 if(UiHintText)
 {
  str8 Text = S8FromCString(UiHintText);
  f32 Width = UiMeasure(App, Text) + 2.f*Ui_Pad;
  f32 Height = UiLineHeight(App) + 2.f*Ui_ButtonPadY;
  v2 Pos = UiHintPos;
  Pos.X = Clamp(Ui_Margin, Pos.X - .5f*Width, BufferDim.X - Ui_Margin - Width);
  v4 Box = RectFromSize(Pos, V2(Width, Height));
  DrawRect(Box, V4FromU32(Ui_ColorText), 6.f, 0.f, 0.f);
  UiText(App, V2(Box.Min.X + Ui_Pad, Box.Min.Y + Ui_ButtonPadY), Text, Ui_ColorCard);
 }
 UiHintText = 0;
}

//~ Buttons
enum ui_button_kind
{
 UiButton_Normal,   // cream button
 UiButton_Primary,  // filled with the accent colour (the main action)
 UiButton_Toggled,  // a toggle that is on (accent colour)
};
typedef enum ui_button_kind ui_button_kind;

// NOTE: Immediate-mode button at *Cursor (top-left); advances Cursor->X. Returns true on
// the frame it is clicked. Hint (optional) shows under it while hovered. The summary
// dropdown, drawn on top, takes the mouse when it is over it.
internal b32
UiButtonEx(app_state *App, app_input *Input, v2 *Cursor, str8 Label, ui_button_kind Kind, char *Hint, f32 MinWidth)
{
 f32 Height = UiButtonHeight(App);
 f32 Width = Max(MinWidth, UiMeasure(App, Label) + 2.f*Ui_ButtonPadX);
 v4 Dest = RectFromSize(*Cursor, V2(Width, Height));

 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 app_button_state MouseLeft = Input->Mouse.Buttons[PlatformMouseButton_Left];
 b32 Hot = (IsInsideRectV2(MousePos, Dest) && !UiMouseBlocked(App, MousePos));
 b32 Result = (Hot && WasPressed(MouseLeft));

 b32 Accent = (Kind != UiButton_Normal);
 u32 Color = Accent ? Ui_ColorAccent : Ui_ColorButton;
 if(Hot) Color = MouseLeft.EndedDown ? (Accent ? Ui_ColorAccentDown : Ui_ColorButtonDown)
  : (Accent ? Ui_ColorAccentHot : Ui_ColorButtonHot);

 DrawRect(Dest, V4FromU32(Color), Ui_ButtonRadius, 0.f, 0.f);
 f32 TextX = Dest.Min.X + .5f*(Width - UiMeasure(App, Label));
 UiText(App, V2(FloorF32(TextX), Dest.Min.Y + Ui_ButtonPadY), Label, Accent ? Ui_ColorOnAccent : Ui_ColorText);

 if(Hot && Hint)
 {
  UiSetHint(Hint, V2(.5f*(Dest.Min.X + Dest.Max.X), Dest.Max.Y + 6.f));
 }

 Cursor->X += Width + Ui_Gap;
 return Result;
}

internal f32
UiButtonWidth(app_state *App, str8 Label, f32 MinWidth)
{
 f32 Result = Max(MinWidth, UiMeasure(App, Label) + 2.f*Ui_ButtonPadX);
 return Result;
}
