//-
// SolarPunk broadcast view: the time controls (in the top bar) and the island tooltip.
// Drawn with the warm light theme from solarpunk_ui.c.
//-

//~ Constants
// NOTE: Kept for the other overlays, which share these sizes.
#define BroadcastView_Margin        Ui_Margin
#define BroadcastView_ButtonPadding Ui_ButtonPadY
#define BroadcastView_ButtonGap     Ui_Gap

global_variable char *BroadcastSpeedLabels[] = {"0x", "0.25x", "0.5x", "1x", "2x", "4x", "8x"};
StaticAssert(ArrayCount(BroadcastSpeedLabels) == ArrayCount(BroadcastSpeeds), BroadcastSpeedLabelsMatch);

//~ Button
// NOTE: The plain button used across the overlays.
internal b32
BroadcastButton(app_state *App, app_input *Input, v2 *Cursor, str8 Label)
{
 b32 Result = UiButtonEx(App, Input, Cursor, Label, UiButton_Normal, 0, 0.f);
 return Result;
}

// NOTE: Defined in solarpunk_world3d.c (included later): status colours.
internal u32 W3StatusColor(u32 Status);
internal u32 W3NeedStatus(app_state *App, u32 IslandIdx, product_kind Kind);
internal u32 W3ProductColor(u32 Kind);

//~ Tooltip
#define BroadcastView_HoverRadius 40.f

// NOTE: The island under the mouse: its status, what it produces and needs (each need
// in the colour of its own state) and how many islands it knows about.
internal void
DrawIslandTooltip(app_state *App, app_input *Input)
{
 f32 LineHeight = UiLineHeight(App);
 v2 MousePos = V2V2S32(Input->Mouse.Pos);

 // Not when the mouse is on the UI.
 if(App->R3.HelpOpen) return;
 for(u32 Idx = 0; Idx < App->R3.UIRectCount; Idx += 1)
 {
  if(IsInsideRectV2(MousePos, App->R3.UIRects[Idx])) return;
 }

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
  u32 Colors[32];
  u32 Kinds[32] = {0};    // product icon in front of the line (0 = none)
  s32 LineCount = 0;

  u32 HoveredIdx = BroadcastIslandIndex(App, Hovered);
  u32 Status = (HoveredIdx < R3_MaxIslands) ? App->R3.IslandStatus[HoveredIdx] : IslandStatus_Ok;
  char *StatusLines[IslandStatus_Count] = {"ok: a supplier for every need", "waiting for news", "short: went without food lately"};

  Lines[LineCount] = Hovered->Name;
  Colors[LineCount] = Ui_ColorText;
  LineCount += 1;

  Lines[LineCount] = S8FromCString(StatusLines[Status < IslandStatus_Count ? Status : 0]);
  Colors[LineCount] = UiStatusText(Status);
  LineCount += 1;

  for(product *Product = Hovered->FirstProduct; Product && LineCount < (s32)ArrayCount(Lines); Product = Product->Next)
  {
   Lines[LineCount] = Str8Fmt("+%llu", Product->Amount);
   Colors[LineCount] = Ui_ColorOkText;
   Kinds[LineCount] = Product->Kind;
   LineCount += 1;
  }

  for(product *Req = Hovered->FirstRequirement; Req && LineCount < (s32)ArrayCount(Lines); Req = Req->Next)
  {
   Lines[LineCount] = Str8Fmt("-%llu", Req->Amount);
   Colors[LineCount] = UiStatusText(W3NeedStatus(App, HoveredIdx, Req->Kind));
   Kinds[LineCount] = Req->Kind;
   LineCount += 1;
  }

  if(LineCount < (s32)ArrayCount(Lines))
  {
   Lines[LineCount] = Str8Fmt("knows %u of %u islands", BroadcastKnownCount(App, Hovered), App->Broadcast.IslandCount);
   Colors[LineCount] = Ui_ColorTextDim;
   LineCount += 1;
  }

  f32 MaxWidth = 0.f;
  for(s32 Idx = 0; Idx < LineCount; Idx += 1)
  {
   MaxWidth = Max(MaxWidth, UiMeasure(App, Lines[Idx]) + (Kinds[Idx] ? LineHeight + 4.f : 0.f));
  }

  f32 Stripe = 4.f;
  v2 Size = V2(MaxWidth + 2.f*Ui_Pad + Stripe, (f32)LineCount*LineHeight + 2.f*Ui_Pad);
  v2 Min = V2(MousePos.X + 18.f, MousePos.Y + 18.f);
  v4 Box = RectFromSize(Min, Size);
  UiCard(Box, Ui_CardRadius);
  DrawRect(RectFromSize(V2(Box.Min.X + 3.f, Box.Min.Y + 6.f), V2(Stripe, Size.Y - 12.f)), V4FromU32(W3StatusColor(Status)), 0.f, 0.f, 0.f);

  v2 Cursor = V2(Min.X + Ui_Pad + Stripe, Min.Y + Ui_Pad);
  for(s32 Idx = 0; Idx < LineCount; Idx += 1)
  {
   f32 TextX = Cursor.X;
   if(Kinds[Idx]) TextX += UiProductIconAt(App, Cursor, Kinds[Idx], W3ProductColor(Kinds[Idx])) + 4.f;
   UiText(App, V2(TextX, Cursor.Y), Lines[Idx], Colors[Idx]);
   Cursor.Y += LineHeight;
  }
 }
}

//~ Timeline
#define BroadcastView_KnobSize 16.f

// NOTE: Click or drag on the track to jump to that round (forward or back).
internal void
DrawBroadcastTimeline(app_state *App, app_input *Input, v2 *Cursor, f32 Width, f32 Height)
{
 broadcast_state *Broadcast = &App->Broadcast;
 // The slider runs from round 1 to the highest round reached so far (its right end).
 Broadcast->TimelineHighRound = Max(Broadcast->TimelineHighRound, Broadcast->Round);
 f32 LastRound = (f32)Max(Broadcast->TimelineHighRound, (u64)1);
 v4 Track = RectFromSize(*Cursor, V2(Width, Height));

 //- Input
 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 app_button_state MouseLeft = Input->Mouse.Buttons[PlatformMouseButton_Left];
 b32 Hot = IsInsideRectV2(MousePos, Track) && !UiMouseBlocked(App, MousePos);
 if(Hot && WasPressed(MouseLeft))
 {
  Broadcast->TimelineDragging = true;
 }
 if(Broadcast->TimelineDragging)
 {
  f32 t = Clamp(0.f, (MousePos.X - Track.Min.X)/Width, 1.f);
  u64 Target = 1 + (u64)(t*(LastRound - 1.f) + .5f);
  // Going back replays from round 1, so far jumps wait for the release.
  u64 Cost = (Target >= Broadcast->Round) ? Target - Broadcast->Round : Target;
  if(Target == Broadcast->Round || Cost <= Broadcast_LiveScrubRounds)
  {
   if(Target != Broadcast->Round) BroadcastJumpToRound(App, Target);
   Broadcast->TimelineDragRound = 0;
  }
  else
  {
   Broadcast->TimelineDragRound = Target;
  }
 }
 if(!MouseLeft.EndedDown)
 {
  if(Broadcast->TimelineDragging && Broadcast->TimelineDragRound != 0)
  {
   BroadcastJumpToRound(App, Broadcast->TimelineDragRound);
  }
  Broadcast->TimelineDragging = false;
  Broadcast->TimelineDragRound = 0;
 }
 if(Hot)
 {
  UiSetHint("Drag to travel to any round (also back in time)", V2(.5f*(Track.Min.X + Track.Max.X), Track.Max.Y + 6.f));
 }

 //- Drawing
 u64 ShownRound = (Broadcast->TimelineDragRound != 0) ? Broadcast->TimelineDragRound : Broadcast->Round;
 f32 Done = (LastRound > 1.f) ? Clamp(0.f, ((f32)ShownRound - 1.f)/(LastRound - 1.f), 1.f) : 1.f;
 f32 TrackY = Track.Min.Y + .5f*Height;
 v4 Bar = RectFromSize(V2(Track.Min.X, TrackY - 3.f), V2(Width, 6.f));
 v4 Fill = RectFromSize(V2(Track.Min.X, TrackY - 3.f), V2(Max(Done*Width, 1.f), 6.f));
 DrawRect(Bar, V4FromU32(Ui_ColorButtonDown), 0.f, 0.f, 0.f);
 DrawRect(Fill, V4FromU32(Ui_ColorAccent), 0.f, 0.f, 0.f);

 f32 Knob = BroadcastView_KnobSize;
 v2 KnobMin = V2(Track.Min.X + Done*Width - .5f*Knob, TrackY - .5f*Knob);
 b32 KnobHot = (Hot || Broadcast->TimelineDragging);
 DrawRect(RectFromSize(KnobMin, V2(Knob, Knob)), V4FromU32(Ui_ColorCard), .5f*Knob, 0.f, .5f);
 DrawRect(RectFromSize(KnobMin, V2(Knob, Knob)), V4FromU32(KnobHot ? Ui_ColorAccentDown : Ui_ColorAccent), .5f*Knob, 3.f, .5f);

 Cursor->X = Track.Max.X + Ui_Gap;
}

//~ Time controls (in the top bar)
// NOTE: Fixed widths, measured from the widest text they hold, so they grow with the text size.
internal f32 TimeControlsSpeedWidth(app_state *App) { return UiMeasure(App, S8("0.25x")) + 12.f; }
internal f32 TimeControlsRoundWidth(app_state *App) { return UiLineHeight(App) + UiMeasure(App, S8("99999")) + 8.f; }

// NOTE: Width of everything in the time controls except the timeline track.
internal f32
TimeControlsFixedWidth(app_state *App)
{
 f32 Square = UiButtonHeight(App);
 f32 Result = 0.f;
 Result += 4.f*(Square + Ui_Gap) + 12.f;     // |<  play  >  >|
 Result += Square + TimeControlsSpeedWidth(App) + Square + 3.f*Ui_Gap + 12.f;
 Result += TimeControlsRoundWidth(App) + Ui_Gap;
 return Result;
}

// NOTE: To start, play/pause, one round forward, to the end, speed and the timeline, in one row.
internal void
DrawTimeControls(app_state *App, app_input *Input, v2 *Cursor, f32 TimelineWidth)
{
 broadcast_state *Broadcast = &App->Broadcast;
 f32 Height = UiButtonHeight(App);

 //- Stepping and play/pause
 if(UiIconButton(App, Input, Cursor, Icon_ToStart, UiButton_Normal, "Go to round 1"))         BroadcastJumpToRound(App, 1);
 b32 Paused = BroadcastIsPaused(Broadcast);
 if(UiIconButton(App, Input, Cursor, Paused ? Icon_Play : Icon_Pause, UiButton_Primary,
                 Paused ? "Let time run" : "Stop time"))                                          BroadcastTogglePause(Broadcast);
 if(UiIconButton(App, Input, Cursor, Icon_ChevronRight, UiButton_Normal, "Go forward 1 round")) BroadcastStep(App);
 if(UiIconButton(App, Input, Cursor, Icon_ToEnd, UiButton_Normal, "Go to the latest round"))
 {
  BroadcastJumpToRound(App, Max(Broadcast->TimelineHighRound, Broadcast->Round));
 }
 Cursor->X += 12.f;

 //- Speed: [-] 1x [+]
 if(UiIconButton(App, Input, Cursor, Icon_Minus, UiButton_Normal, "Slower")) BroadcastSlower(Broadcast);
 {
  str8 Speed = S8FromCString(BroadcastSpeedLabels[BroadcastSpeedIdx(Broadcast)]);
  f32 SpeedWidth = TimeControlsSpeedWidth(App);
  f32 TextX = Cursor->X + .5f*(SpeedWidth - UiMeasure(App, Speed));
  UiText(App, V2(FloorF32(TextX), Cursor->Y + Ui_ButtonPadY), Speed, Ui_ColorText);
  Cursor->X += SpeedWidth + Ui_Gap;
 }
 if(UiIconButton(App, Input, Cursor, Icon_Plus, UiButton_Normal, "Faster")) BroadcastFaster(Broadcast);
 Cursor->X += 12.f;

 //- Timeline and round
 DrawBroadcastTimeline(App, Input, Cursor, TimelineWidth, Height);
 {
  // Clock icon + round number
  UiIconAt(App, V2(Cursor->X + 2.f, Cursor->Y + Ui_ButtonPadY), Icon_Clock, Ui_ColorTextDim);
  u64 ShownRound = (Broadcast->TimelineDragRound != 0) ? Broadcast->TimelineDragRound : Broadcast->Round;
  str8 Round = Str8Fmt("%llu", ShownRound);
  UiText(App, V2(Cursor->X + UiLineHeight(App) + 4.f, Cursor->Y + Ui_ButtonPadY), Round, Ui_ColorText);
  Cursor->X += TimeControlsRoundWidth(App) + Ui_Gap;
 }

}
