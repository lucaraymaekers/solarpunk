//-
// SolarPunk broadcast view: status line and clickable buttons to control time
//-

//~ Constants
#define BroadcastView_Margin        12.f
#define BroadcastView_ButtonPadding 8.f
#define BroadcastView_ButtonGap     6.f

#define BroadcastView_ColorButton        0xFF434C5E
#define BroadcastView_ColorButtonHot     0xFF4C566A
#define BroadcastView_ColorButtonPressed 0xFF5E81AC

global_variable char *BroadcastSpeedLabels[] = {"paused", "0.25x", "0.5x", "1x", "2x", "4x", "8x"};
StaticAssert(ArrayCount(BroadcastSpeedLabels) == ArrayCount(BroadcastSpeeds), BroadcastSpeedLabelsMatch);

//~ Button
// NOTE: Immediate-mode button: draws itself and returns true on the frame it is clicked.
internal b32
BroadcastButton(app_state *App, app_input *Input, v2 *Cursor, str8 Label)
{
 font_atlas *Atlas = &App->FontAtlas;
 font *Font = &App->Font;
 f32 LineHeight = Font->HeightPx;

 v2 Size = V2(GraphMeasureText(Atlas, Font, Label) + 2.f*BroadcastView_ButtonPadding,
              LineHeight + BroadcastView_ButtonPadding);
 v4 Dest = RectFromSize(*Cursor, Size);

 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 app_button_state MouseLeft = Input->Mouse.Buttons[PlatformMouseButton_Left];
 b32 IsHot = IsInsideRectV2(MousePos, Dest);
 b32 Result = (IsHot && WasPressed(MouseLeft));

 u32 Color = BroadcastView_ColorButton;
 if(IsHot) Color = (MouseLeft.EndedDown ? BroadcastView_ColorButtonPressed : BroadcastView_ColorButtonHot);

 DrawRect(Dest, V4FromU32(Color), 5.f, 0.f, 0.f);
 DrawRect(Dest, V4FromU32(Graph_ColorNodeBorder), 5.f, 1.f, 0.f);
 GraphDrawText(Atlas, Font, V2(Dest.Min.X + BroadcastView_ButtonPadding, Dest.Min.Y + .5f*BroadcastView_ButtonPadding),
               Label, V4FromU32(Graph_ColorTitle));

 Cursor->X += Size.X + BroadcastView_ButtonGap;

 return Result;
}

//~ Controls
internal void
DrawBroadcastControls(app_state *App, app_input *Input)
{
 broadcast_state *Broadcast = &App->Broadcast;
 font_atlas *Atlas = &App->FontAtlas;
 font *Font = &App->Font;
 f32 LineHeight = Font->HeightPx;

 v2 Pos = V2(BroadcastView_Margin, BroadcastView_Margin);

 //- Status
 str8 Status = Str8Fmt("Round %llu   Speed %s", Broadcast->Round,
                       BroadcastSpeedLabels[BroadcastSpeedIdx(Broadcast)]);
 GraphDrawText(Atlas, Font, Pos, Status, V4FromU32(Graph_ColorTitle));

 //- Buttons
 v2 Cursor = V2(Pos.X, Pos.Y + LineHeight + BroadcastView_ButtonGap);

 if(BroadcastButton(App, Input, &Cursor, S8("Slower"))) BroadcastSlower(Broadcast);
 if(BroadcastButton(App, Input, &Cursor, S8("Faster"))) BroadcastFaster(Broadcast);

 str8 PauseLabel = (BroadcastIsPaused(Broadcast) ? S8("Play") : S8("Pause"));
 if(BroadcastButton(App, Input, &Cursor, PauseLabel)) BroadcastTogglePause(Broadcast);

 if(BroadcastButton(App, Input, &Cursor, S8("Next round"))) BroadcastStep(App);
 if(BroadcastButton(App, Input, &Cursor, S8("Restart")))    BroadcastRestart(App);
 
 //- Keyboard legend
 {
  str8 Keys[] =
  {
   S8("Up"),    S8("Down"),   S8("Right"),      S8("Left"),
  };
  str8 Actions[] =
  {
   S8("faster"), S8("slower (lowest = pause)"), S8("next round"), S8("restart"),
  };
  StaticAssert(ArrayCount(Keys) == ArrayCount(Actions), BroadcastLegendMatch);
  
  f32 ButtonHeight = LineHeight + BroadcastView_ButtonPadding;
  v2 LegendPos = V2(Pos.X, Cursor.Y + ButtonHeight + 2.f*BroadcastView_ButtonGap);
  
  f32 KeyColumnWidth = 0.f;
  f32 ActionColumnWidth = 0.f;
  for EachElement(Idx, Keys)
  {
   KeyColumnWidth = Max(KeyColumnWidth, GraphMeasureText(Atlas, Font, Keys[Idx]));
   ActionColumnWidth = Max(ActionColumnWidth, GraphMeasureText(Atlas, Font, Actions[Idx]));
  }
  KeyColumnWidth += 2.f*BroadcastView_ButtonPadding;
  
  str8 Title = S8("Keyboard");
  f32 BoxWidth = Max(GraphMeasureText(Atlas, Font, Title), KeyColumnWidth + ActionColumnWidth) + 2.f*BroadcastView_ButtonPadding;
  f32 BoxHeight = (f32)(ArrayCount(Keys) + 1)*LineHeight + 2.f*BroadcastView_ButtonPadding;
  v4 Box = RectFromSize(LegendPos, V2(BoxWidth, BoxHeight));
  DrawRect(Box, V4FromU32(Graph_ColorNodeBg), 6.f, 0.f, 0.f);
  DrawRect(Box, V4FromU32(Graph_ColorNodeBorder), 6.f, 1.f, 0.f);
  
  v2 Line = V2(LegendPos.X + BroadcastView_ButtonPadding, LegendPos.Y + BroadcastView_ButtonPadding);
  GraphDrawText(Atlas, Font, Line, Title, V4FromU32(Graph_ColorTitle));
  Line.Y += LineHeight;
  
  for EachElement(Idx, Keys)
  {
   GraphDrawText(Atlas, Font, Line, Keys[Idx], V4FromU32(Graph_ColorCost));
   GraphDrawText(Atlas, Font, V2(Line.X + KeyColumnWidth, Line.Y), Actions[Idx], V4FromU32(Graph_ColorKnows));
   Line.Y += LineHeight;
  }
 }
}
