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

//~ Tooltip
#define BroadcastView_HoverRadius 40.f

// NOTE: Shows the full production and needs of the island under the mouse,
// since nodes only show their name when there are many islands.
internal void
DrawIslandTooltip(app_state *App, app_input *Input)
{
 font_atlas *Atlas = &App->FontAtlas;
 font *Font = &App->Font;
 f32 LineHeight = Font->HeightPx;
 v2 MousePos = V2V2S32(Input->Mouse.Pos);

 //- Find the closest island to the mouse
 island *Hovered = 0;
 f32 BestDistanceSq = BroadcastView_HoverRadius*BroadcastView_HoverRadius;
 for(island *Island = App->FirstIsland; Island; Island = Island->Next)
 {
  f32 DX = Island->Pos.X - MousePos.X;
  f32 DY = Island->Pos.Y - MousePos.Y;
  f32 DistanceSq = DX*DX + DY*DY;
  if(DistanceSq < BestDistanceSq)
  {
   BestDistanceSq = DistanceSq;
   Hovered = Island;
  }
 }

 if(Hovered)
 {
  str8 Lines[32];
  v4 Colors[32];
  s32 LineCount = 0;

  Lines[LineCount] = Hovered->Name;
  Colors[LineCount] = V4FromU32(Graph_ColorTitle);
  LineCount += 1;

  for(product *Product = Hovered->FirstProduct; Product && LineCount < (s32)ArrayCount(Lines); Product = Product->Next)
  {
   Lines[LineCount] = Str8Fmt("+ %.*s %llu", (s32)ProductNames[Product->Kind].Size, ProductNames[Product->Kind].Data, Product->Amount);
   Colors[LineCount] = V4FromU32(Graph_ColorProduct);
   LineCount += 1;
  }

  for(product *Req = Hovered->FirstRequirement; Req && LineCount < (s32)ArrayCount(Lines); Req = Req->Next)
  {
   Lines[LineCount] = Str8Fmt("- %.*s %llu", (s32)ProductNames[Req->Kind].Size, ProductNames[Req->Kind].Data, Req->Amount);
   Colors[LineCount] = V4FromU32(Graph_ColorRequirement);
   LineCount += 1;
  }

  if(LineCount < (s32)ArrayCount(Lines))
  {
   Lines[LineCount] = Str8Fmt("knows %u/%u islands", BroadcastKnownCount(App, Hovered), App->Broadcast.IslandCount);
   Colors[LineCount] = V4FromU32(Graph_ColorKnows);
   LineCount += 1;
  }

  f32 MaxWidth = 0.f;
  for(s32 Idx = 0; Idx < LineCount; Idx += 1)
  {
   MaxWidth = Max(MaxWidth, GraphMeasureText(Atlas, Font, Lines[Idx]));
  }

  v2 Size = V2(MaxWidth + 2.f*BroadcastView_ButtonPadding,
               (f32)LineCount*LineHeight + 2.f*BroadcastView_ButtonPadding);
  v2 Min = V2(MousePos.X + 16.f, MousePos.Y + 16.f);
  v4 Box = RectFromSize(Min, Size);
  DrawRect(Box, V4FromU32(Graph_ColorNodeBg), 6.f, 0.f, 0.f);
  DrawRect(Box, V4FromU32(Graph_ColorCost), 6.f, 2.f, 0.f);

  v2 Cursor = V2(Min.X + BroadcastView_ButtonPadding, Min.Y + BroadcastView_ButtonPadding);
  for(s32 Idx = 0; Idx < LineCount; Idx += 1)
  {
   GraphDrawText(Atlas, Font, Cursor, Lines[Idx], Colors[Idx]);
   Cursor.Y += LineHeight;
  }
 }
}

//~ Timeline
#define BroadcastView_TimelineWidth 320.f
#define BroadcastView_KnobSize      14.f

#define BroadcastView_ColorTrack     0xFF434C5E
#define BroadcastView_ColorTrackFill 0xFF5E81AC

// NOTE: Click or drag on the track to jump to that round (forward or back).
internal void
DrawBroadcastTimeline(app_state *App, app_input *Input, v2 *Cursor, f32 Height)
{
 broadcast_state *Broadcast = &App->Broadcast;
 font_atlas *Atlas = &App->FontAtlas;
 font *Font = &App->Font;

 f32 Width = BroadcastView_TimelineWidth;
 f32 LastRound = (f32)Broadcast_TimelineRounds;
 v4 Track = RectFromSize(*Cursor, V2(Width, Height));

 //- Input
 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 app_button_state MouseLeft = Input->Mouse.Buttons[PlatformMouseButton_Left];
 if(IsInsideRectV2(MousePos, Track) && WasPressed(MouseLeft))
 {
  Broadcast->TimelineDragging = true;
 }
 if(!MouseLeft.EndedDown)
 {
  Broadcast->TimelineDragging = false;
 }
 if(Broadcast->TimelineDragging)
 {
  f32 t = Clamp(0.f, (MousePos.X - Track.Min.X)/Width, 1.f);
  u64 Target = 1 + (u64)(t*(LastRound - 1.f) + .5f);
  if(Target != Broadcast->Round)
  {
   BroadcastJumpToRound(App, Target);
  }
 }

 //- Drawing
 f32 Done = Clamp(0.f, ((f32)Broadcast->Round - 1.f)/(LastRound - 1.f), 1.f);
 f32 TrackY = Track.Min.Y + .5f*Height;
 v4 Bar = RectFromSize(V2(Track.Min.X, TrackY - 3.f), V2(Width, 6.f));
 v4 Fill = RectFromSize(V2(Track.Min.X, TrackY - 3.f), V2(Max(Done*Width, 1.f), 6.f));
 DrawRect(Bar, V4FromU32(BroadcastView_ColorTrack), 3.f, 0.f, 0.f);
 DrawRect(Fill, V4FromU32(BroadcastView_ColorTrackFill), 3.f, 0.f, 0.f);

 v2 KnobCenter = V2(Track.Min.X + Done*Width, TrackY);
 f32 Knob = BroadcastView_KnobSize;
 b32 KnobHot = (IsInsideRectV2(MousePos, Track) || Broadcast->TimelineDragging);
 DrawRect(RectFromSize(V2(KnobCenter.X - .5f*Knob, KnobCenter.Y - .5f*Knob), V2(Knob, Knob)),
          V4FromU32(KnobHot ? Graph_ColorCost : Graph_ColorTitle), .5f*Knob, 0.f, 0.f);

 str8 Label = Str8Fmt("round %llu / %u", Broadcast->Round, (u32)Broadcast_TimelineRounds);
 v2 LabelPos = V2(Track.Max.X + 12.f, Track.Min.Y + .5f*BroadcastView_ButtonPadding);
 DrawRect(RectFromSize(V2(LabelPos.X - 6.f, LabelPos.Y - 3.f), V2(GraphMeasureText(Atlas, Font, Label) + 12.f, Font->HeightPx + 6.f)),
          V4FromU32(Graph_ColorNodeBg), 5.f, 0.f, 0.f);
 GraphDrawText(Atlas, Font, LabelPos, Label, V4FromU32(Graph_ColorKnows));

 Cursor->X = Track.Max.X + BroadcastView_ButtonGap;
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
 {
  f32 StatusWidth = GraphMeasureText(Atlas, Font, Status);
  DrawRect(RectFromSize(V2(Pos.X - 6.f, Pos.Y - 3.f), V2(StatusWidth + 12.f, LineHeight + 6.f)),
           V4FromU32(Graph_ColorNodeBg), 5.f, 0.f, 0.f);
 }
 GraphDrawText(Atlas, Font, Pos, Status, V4FromU32(Graph_ColorTitle));

 //- Buttons
 v2 Cursor = V2(Pos.X, Pos.Y + LineHeight + BroadcastView_ButtonGap);

 if(BroadcastButton(App, Input, &Cursor, S8("Slower"))) BroadcastSlower(Broadcast);
 if(BroadcastButton(App, Input, &Cursor, S8("Faster"))) BroadcastFaster(Broadcast);

 str8 PauseLabel = (BroadcastIsPaused(Broadcast) ? S8("Play") : S8("Pause"));
 if(BroadcastButton(App, Input, &Cursor, PauseLabel)) BroadcastTogglePause(Broadcast);

 if(BroadcastButton(App, Input, &Cursor, S8("Next round"))) BroadcastStep(App);
 if(BroadcastButton(App, Input, &Cursor, S8("Restart")))    BroadcastRestart(App);

 //- Time travel: step buttons and a timeline slider
 f32 ButtonHeight = LineHeight + BroadcastView_ButtonPadding;
 Cursor = V2(Pos.X, Cursor.Y + ButtonHeight + BroadcastView_ButtonGap);

 if(BroadcastButton(App, Input, &Cursor, S8("Back 5")))    BroadcastJumpBy(App, -5);
 if(BroadcastButton(App, Input, &Cursor, S8("Back 1")))    BroadcastJumpBy(App, -1);
 if(BroadcastButton(App, Input, &Cursor, S8("Forward 1"))) BroadcastJumpBy(App, 1);
 if(BroadcastButton(App, Input, &Cursor, S8("Forward 5"))) BroadcastJumpBy(App, 5);

 DrawBroadcastTimeline(App, Input, &Cursor, ButtonHeight);

 //- Keyboard legend
 {
  // NOTE: Plain C strings: MSVC warns (C4204) about arrays initialized with S8(...).
  char *Keys[] =
  {
   "Up",     "Down",                    "Right",      "Left",
   "Wheel",  "Right drag",              "Middle drag", "Click",
  };
  char *Actions[] =
  {
   "faster", "slower (lowest = pause)", "next round", "restart",
   "zoom",   "move the map",            "rotate",      "open an island",
  };
  StaticAssert(ArrayCount(Keys) == ArrayCount(Actions), BroadcastLegendMatch);

  v2 LegendPos = V2(Pos.X, Cursor.Y + ButtonHeight + 2.f*BroadcastView_ButtonGap);

  f32 KeyColumnWidth = 0.f;
  f32 ActionColumnWidth = 0.f;
  for EachElement(Idx, Keys)
  {
   KeyColumnWidth = Max(KeyColumnWidth, GraphMeasureText(Atlas, Font, S8FromCString(Keys[Idx])));
   ActionColumnWidth = Max(ActionColumnWidth, GraphMeasureText(Atlas, Font, S8FromCString(Actions[Idx])));
  }
  KeyColumnWidth += 2.f*BroadcastView_ButtonPadding;

  str8 Title = S8("Controls");
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
   GraphDrawText(Atlas, Font, Line, S8FromCString(Keys[Idx]), V4FromU32(Graph_ColorCost));
   GraphDrawText(Atlas, Font, V2(Line.X + KeyColumnWidth, Line.Y), S8FromCString(Actions[Idx]), V4FromU32(Graph_ColorKnows));
   Line.Y += LineHeight;
  }
 }

 DrawIslandTooltip(App, Input);
}
