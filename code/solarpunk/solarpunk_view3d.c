//-
// SolarPunk 3D view: runs the 3D pass and draws the 2D overlay on top of it:
//   - summary bar (top left): round, speed, how many islands are ok / waiting / short,
//     and which products are short worldwide
//   - island labels (name + status dot), cable costs of the hovered island
//   - camera buttons (top right)
//   - "Legend" and "Controls" buttons (bottom left) that open those boxes
//-

//~ Constants
#define V3D_Margin       12.f
#define V3D_LabelPadding 5.f
#define V3D_ChipPadding  8.f

// NOTE: Area of the summary bar and the two rows of time controls under it (top left).
#define V3D_ControlsAreaWidth   840.f
#define V3D_ControlsAreaHeight  120.f

#define V3D_ColorLabelBg     0xE02E3440
#define V3D_ColorLabelBorder 0xFF88C0D0
#define V3D_ColorChip        0xE62E3440

//~ UI areas
internal void
V3AddUIRect(r3_state *R3, v4 Rect)
{
 if(R3->UIRectCount < R3_MaxUIRects)
 {
  R3->UIRects[R3->UIRectCount] = Rect;
  R3->UIRectCount += 1;
 }
}

internal b32
V3RectsOverlap(v4 A, v4 B)
{
 b32 Result = (A.Min.X < B.Max.X && B.Min.X < A.Max.X &&
               A.Min.Y < B.Max.Y && B.Min.Y < A.Max.Y);
 return Result;
}

internal f32
V3Measure(app_state *App, str8 Text)
{
 f32 Result = GraphMeasureText(&App->FontAtlas, &App->Font, Text);
 return Result;
}

internal void
V3StatusDot(v2 Pos, f32 Size, u32 Color)
{
 DrawRect(RectFromSize(Pos, V2(Size, Size)), V4FromU32(Color), .5f*Size, 0.f, 0.f);
}

//~ Labels
// NOTE: One line per island: a status dot and the name. Placed nearest-island first
// (hovered and selected islands first); a label that would overlap the UI or another
// label is skipped. Zoom in, or hover an island, to see the hidden ones.
internal void
V3DrawIslandLabels(app_state *App)
{
 r3_state *R3 = &App->R3;
 font_atlas *Atlas = &App->FontAtlas;
 font *Font = &App->Font;
 f32 LineHeight = Font->HeightPx;
 f32 Dot = LineHeight - 10.f;

 //- Order: hovered, selected, then by distance to the camera
 island *Islands[R3_MaxIslands];
 u32 Order[R3_MaxIslands];
 f32 SortKey[R3_MaxIslands];
 u32 Count = 0;
 {
  u32 Index = 0;
  for(island *Island = App->FirstIsland; Island && Index < R3->IslandCount; Island = Island->Next)
  {
   Islands[Index] = Island;
   if(R3->IslandVisible[Index])
   {
    f32 Key = R3Length(R3Sub(R3->IslandTop[Index], R3->Eye));
    if(R3->HoveredIdx == (s32)Index)                        Key = -2.f;
    else if(App->Panel.SelectedPlusOne == (s32)Index + 1)    Key = -1.f;

    // Insertion sort, there are only a few dozen islands.
    u32 At = Count;
    while(At > 0 && SortKey[At - 1] > Key)
    {
     Order[At] = Order[At - 1];
     SortKey[At] = SortKey[At - 1];
     At -= 1;
    }
    Order[At] = Index;
    SortKey[At] = Key;
    Count += 1;
   }
   Index += 1;
  }
 }

 //- Place and draw; the UI areas count as already placed
 v4 Placed[R3_MaxIslands + R3_MaxUIRects];
 u32 PlacedCount = 0;
 for(u32 Idx = 0; Idx < R3->UIRectCount; Idx += 1)
 {
  Placed[PlacedCount++] = R3->UIRects[Idx];
 }

 for(u32 OrderIdx = 0; OrderIdx < Count; OrderIdx += 1)
 {
  u32 Index = Order[OrderIdx];
  island *Island = Islands[Index];
  b32 Hovered = (R3->HoveredIdx == (s32)Index);
  b32 Selected = (App->Panel.SelectedPlusOne == (s32)Index + 1);

  f32 Width = Dot + 6.f + V3Measure(App, Island->Name) + 2.f*V3D_LabelPadding;
  f32 Height = LineHeight + 2.f*V3D_LabelPadding;
  v2 Anchor = R3->IslandScreen[Index];
  v2 Min = V2(FloorF32(Anchor.X - .5f*Width), FloorF32(Anchor.Y - Height));
  v4 Box = RectFromSize(Min, V2(Width, Height));

  b32 Overlaps = false;
  for(u32 PlacedIdx = 0; PlacedIdx < PlacedCount && !Overlaps; PlacedIdx += 1)
  {
   Overlaps = V3RectsOverlap(Box, Placed[PlacedIdx]);
  }

  if(!Overlaps)
  {
   Placed[PlacedCount] = Box;
   PlacedCount += 1;

   u32 Border = Hovered ? W3_ColorHover : (Selected ? W3_ColorSelected : V3D_ColorLabelBorder);
   DrawRect(Box, V4FromU32(V3D_ColorLabelBg), 5.f, 0.f, 0.f);
   if(Hovered || Selected)
   {
    DrawRect(Box, V4FromU32(Border), 5.f, 2.f, 0.f);
   }
   V3StatusDot(V2(Min.X + V3D_LabelPadding, Min.Y + .5f*(Height - Dot)), Dot, W3StatusColor(R3->IslandStatus[Index]));
   GraphDrawText(Atlas, Font, V2(Min.X + V3D_LabelPadding + Dot + 6.f, Min.Y + V3D_LabelPadding), Island->Name, V4FromU32(Graph_ColorTitle));
  }
 }
}

// NOTE: Only the hovered island's cables show their cost, to keep the view readable.
internal void
V3DrawHoveredCableCosts(app_state *App)
{
 r3_state *R3 = &App->R3;
 font_atlas *Atlas = &App->FontAtlas;
 font *Font = &App->Font;
 f32 LineHeight = Font->HeightPx;

 if(R3->HoveredIdx >= 0)
 {
  u32 FromIdx = 0;
  for(island *From = App->FirstIsland; From && FromIdx < R3->IslandCount; From = From->Next)
  {
   if((s32)FromIdx == R3->HoveredIdx)
   {
    for(link *Link = From->FirstLink; Link; Link = Link->Next)
    {
     u32 ToIdx = BroadcastIslandIndex(App, Link->Dest);
     if(ToIdx < R3->IslandCount)
     {
      v3 Mid = W3CablePoint(R3->IslandTop[FromIdx], R3->IslandRadius[FromIdx],
                            R3->IslandTop[ToIdx], R3->IslandRadius[ToIdx], .5f);
      v2 Screen;
      if(R3ProjectToScreen(R3, Mid, &Screen))
      {
       str8 Text = Str8Fmt("cost %d", Link->Cost);
       f32 Width = V3Measure(App, Text) + 2.f*V3D_LabelPadding;
       v2 Min = V2(FloorF32(Screen.X - .5f*Width), FloorF32(Screen.Y - .5f*LineHeight));
       v4 Box = RectFromSize(Min, V2(Width, LineHeight + 2.f));
       DrawRect(Box, V4FromU32(V3D_ColorLabelBg), 4.f, 0.f, 0.f);
       GraphDrawText(Atlas, Font, V2(Min.X + V3D_LabelPadding, Min.Y + 1.f), Text, V4FromU32(Graph_ColorCost));
      }
     }
    }
   }
   FromIdx += 1;
  }
 }
}

//~ Summary bar (top left)
// NOTE: A chip: rounded background, optional coloured dot or product swatch, text.
internal void
V3Chip(app_state *App, v2 *Cursor, str8 Text, u32 DotColor, b32 HasDot, b32 Square, u32 TextColor)
{
 font *Font = &App->Font;
 f32 Height = Font->HeightPx + BroadcastView_ButtonPadding;
 f32 Dot = Font->HeightPx - 8.f;
 f32 DotSpace = HasDot ? Dot + 6.f : 0.f;
 f32 Width = 2.f*V3D_ChipPadding + DotSpace + V3Measure(App, Text);
 v4 Box = RectFromSize(*Cursor, V2(Width, Height));

 DrawRect(Box, V4FromU32(V3D_ColorChip), .5f*Height, 0.f, 0.f);
 if(HasDot)
 {
  v2 DotPos = V2(Box.Min.X + V3D_ChipPadding, Box.Min.Y + .5f*(Height - Dot));
  DrawRect(RectFromSize(DotPos, V2(Dot, Dot)), V4FromU32(DotColor), Square ? 3.f : .5f*Dot, 0.f, 0.f);
 }
 GraphDrawText(&App->FontAtlas, Font, V2(Box.Min.X + V3D_ChipPadding + DotSpace, Box.Min.Y + .5f*BroadcastView_ButtonPadding),
               Text, V4FromU32(TextColor));
 Cursor->X += Width + BroadcastView_ButtonGap;
}

internal void
V3DrawSummaryBar(app_state *App)
{
 r3_state *R3 = &App->R3;
 broadcast_state *Broadcast = &App->Broadcast;
 v2 Cursor = V2(V3D_Margin, V3D_Margin);

 //- Time
 V3Chip(App, &Cursor, Str8Fmt("Round %llu   %s", Broadcast->Round, BroadcastSpeedLabels[BroadcastSpeedIdx(Broadcast)]),
        0, false, false, Graph_ColorTitle);
 Cursor.X += 10.f;

 //- Islands per status
 u32 Counts[IslandStatus_Count] = {0};
 for(u32 Idx = 0; Idx < R3->IslandCount; Idx += 1)
 {
  if(R3->IslandStatus[Idx] < IslandStatus_Count) Counts[R3->IslandStatus[Idx]] += 1;
 }
 V3Chip(App, &Cursor, Str8Fmt("%u ok", Counts[IslandStatus_Ok]), W3_ColorStatusOk, true, false, Graph_ColorTitle);
 V3Chip(App, &Cursor, Str8Fmt("%u waiting for news", Counts[IslandStatus_Waiting]), W3_ColorStatusWaiting, true, false, Graph_ColorTitle);
 V3Chip(App, &Cursor, Str8Fmt("%u short", Counts[IslandStatus_Short]), W3_ColorStatusShort, true, false, Graph_ColorTitle);
 Cursor.X += 10.f;

 //- Products that are short worldwide
 b32 AnyShort = false;
 for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
 {
  if(R3->TotalNeeded[Kind] > R3->TotalProduced[Kind])
  {
   s64 Missing = (s64)R3->TotalNeeded[Kind] - (s64)R3->TotalProduced[Kind];
   str8 Name = ProductNames[Kind];
   V3Chip(App, &Cursor, Str8Fmt("%.*s short by %lld", (s32)Name.Size, Name.Data, Missing),
          W3ProductColors[Kind], true, true, W3_ColorStatusShort);
   AnyShort = true;
  }
 }
 if(!AnyShort)
 {
  V3Chip(App, &Cursor, S8("no product shortages"), 0, false, false, W3_ColorStatusOk);
 }

 // The summary bar can be wider than the time controls under it.
 f32 Height = App->Font.HeightPx + BroadcastView_ButtonPadding;
 V3AddUIRect(R3, RectFromSize(V2(0.f, 0.f), V2(Cursor.X, V3D_Margin + Height)));
}

//~ Camera controls (top right)
internal void
V3DrawCameraControls(app_state *App, app_input *Input, v2 BufferDim)
{
 r3_camera *Camera = &App->R3.Camera;
 f32 Height = App->Font.HeightPx + BroadcastView_ButtonPadding;

 // NOTE: Plain C strings: MSVC warns (C4204) about arrays initialized with S8(...).
 char *LabelStrings[] = {"Zoom in", "Zoom out", "Reset view"};
 str8 Labels[ArrayCount(LabelStrings)];
 f32 TotalWidth = 0.f;
 for EachElement(Idx, LabelStrings)
 {
  Labels[Idx] = S8FromCString(LabelStrings[Idx]);
  TotalWidth += V3Measure(App, Labels[Idx]) + 2.f*BroadcastView_ButtonPadding + BroadcastView_ButtonGap;
 }

 v2 Cursor = V2(BufferDim.X - V3D_Margin - TotalWidth, V3D_Margin);
 V3AddUIRect(&App->R3, RectFromSize(Cursor, V2(TotalWidth, Height)));
 if(BroadcastButton(App, Input, &Cursor, Labels[0])) R3CameraZoom(Camera, 1.f/R3_CameraZoomStep);
 if(BroadcastButton(App, Input, &Cursor, Labels[1])) R3CameraZoom(Camera, R3_CameraZoomStep);
 if(BroadcastButton(App, Input, &Cursor, Labels[2])) R3CameraReset(Camera);
}

//~ Legend (opened with the "Legend" button)
global_variable char *V3LegendNotes[] =
{
 "green ring: has a supplier for all its needs",
 "orange ring: still waiting for news of a supplier",
 "red ring: knows its suppliers, but a need is short worldwide",
 "solid crates, green pad: what it produces",
 "crate frames, orange pad: what it needs",
 "yellow pulse on a cable: news travelling",
};
global_variable u32 V3LegendNoteColors[] =
{
 W3_ColorStatusOk, W3_ColorStatusWaiting, W3_ColorStatusShort,
 W3_ColorProducePad, W3_ColorNeedPad, W3_ColorPulse,
};
StaticAssert(ArrayCount(V3LegendNotes) == ArrayCount(V3LegendNoteColors), V3DLegendNotesMatch);

typedef struct v3d_legend_layout v3d_legend_layout;
struct v3d_legend_layout
{
 v4 Box;
 u32 ProductRows;
 f32 ProductColumnWidth;
};

// NOTE: The box sits with its bottom-left corner at BottomLeft.
internal v3d_legend_layout
V3LegendLayout(app_state *App, v2 BottomLeft)
{
 f32 LineHeight = App->Font.HeightPx;
 f32 Pad = BroadcastView_ButtonPadding;
 f32 Swatch = LineHeight - 6.f;

 v3d_legend_layout Result = {0};
 u32 ProductCount = ProductKind_Count - 1;
 Result.ProductRows = (ProductCount + 1)/2;
 for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
 {
  Result.ProductColumnWidth = Max(Result.ProductColumnWidth, Swatch + Pad + V3Measure(App, ProductNames[Kind]));
 }
 Result.ProductColumnWidth += 2.f*Pad;

 f32 NotesWidth = 0.f;
 for EachElement(Idx, V3LegendNotes)
 {
  NotesWidth = Max(NotesWidth, Swatch + Pad + V3Measure(App, S8FromCString(V3LegendNotes[Idx])));
 }

 u32 Rows = Max(Result.ProductRows, (u32)ArrayCount(V3LegendNotes));
 f32 Width = 2.f*Result.ProductColumnWidth + NotesWidth + 2.f*Pad;
 f32 Height = (f32)(Rows + 1)*LineHeight + 2.f*Pad;
 Result.Box = RectFromSize(V2(BottomLeft.X, BottomLeft.Y - Height), V2(Width, Height));
 return Result;
}

// NOTE: Product colours in two columns on the left, what the colours and shapes mean on the right.
internal void
V3DrawLegend(app_state *App, v2 BottomLeft)
{
 font_atlas *Atlas = &App->FontAtlas;
 font *Font = &App->Font;
 f32 LineHeight = Font->HeightPx;
 f32 Pad = BroadcastView_ButtonPadding;
 f32 Swatch = LineHeight - 6.f;

 v3d_legend_layout Layout = V3LegendLayout(App, BottomLeft);
 v4 Box = Layout.Box;
 DrawRect(Box, V4FromU32(V3D_ColorLabelBg), 6.f, 0.f, 0.f);
 DrawRect(Box, V4FromU32(Graph_ColorNodeBorder), 6.f, 1.f, 0.f);

 v2 Origin = V2(Box.Min.X + Pad, Box.Min.Y + Pad);
 GraphDrawText(Atlas, Font, Origin, S8("Legend"), V4FromU32(Graph_ColorTitle));
 Origin.Y += LineHeight;

 for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
 {
  u32 Item = Kind - 1;
  v2 Line = V2(Origin.X + (f32)(Item/Layout.ProductRows)*Layout.ProductColumnWidth, Origin.Y + (f32)(Item % Layout.ProductRows)*LineHeight);
  DrawRect(RectFromSize(V2(Line.X, Line.Y + 3.f), V2(Swatch, Swatch)), V4FromU32(W3ProductColors[Kind]), 3.f, 0.f, 0.f);
  GraphDrawText(Atlas, Font, V2(Line.X + Swatch + Pad, Line.Y), ProductNames[Kind], V4FromU32(Graph_ColorTitle));
 }

 for EachElement(Idx, V3LegendNotes)
 {
  v2 Line = V2(Origin.X + 2.f*Layout.ProductColumnWidth, Origin.Y + (f32)Idx*LineHeight);
  DrawRect(RectFromSize(V2(Line.X, Line.Y + 3.f), V2(Swatch, Swatch)), V4FromU32(V3LegendNoteColors[Idx]), .5f*Swatch, 0.f, 0.f);
  GraphDrawText(Atlas, Font, V2(Line.X + Swatch + Pad, Line.Y), S8FromCString(V3LegendNotes[Idx]), V4FromU32(Graph_ColorKnows));
 }
}

//~ Bottom left: Legend / Controls buttons and their boxes
internal void
V3DrawHelpButtons(app_state *App, app_input *Input, v2 BufferDim)
{
 r3_state *R3 = &App->R3;
 f32 Height = App->Font.HeightPx + BroadcastView_ButtonPadding;

 v2 Cursor = V2(V3D_Margin, BufferDim.Y - V3D_Margin - Height);
 v2 RowStart = Cursor;
 if(BroadcastButton(App, Input, &Cursor, R3->ShowLegend ? S8("Hide legend") : S8("Legend")))
 {
  R3->ShowLegend = !R3->ShowLegend;
 }
 if(BroadcastButton(App, Input, &Cursor, R3->ShowControls ? S8("Hide controls") : S8("Controls")))
 {
  R3->ShowControls = !R3->ShowControls;
 }
 V3AddUIRect(R3, RectFromSize(RowStart, V2(Cursor.X - RowStart.X, Height)));

 // Boxes open above the buttons, side by side.
 v2 BottomLeft = V2(V3D_Margin, RowStart.Y - BroadcastView_ButtonGap);
 if(R3->ShowLegend)
 {
  v3d_legend_layout Layout = V3LegendLayout(App, BottomLeft);
  V3DrawLegend(App, BottomLeft);
  V3AddUIRect(R3, Layout.Box);
  BottomLeft.X = Layout.Box.Max.X + BroadcastView_ButtonGap;
 }
 if(R3->ShowControls)
 {
  V3AddUIRect(R3, ControlsHelpRect(App, BottomLeft));
  DrawControlsHelp(App, BottomLeft);
 }
}

//~ Entry point
// NOTE: Call after RenderClear() and before DrawIslandPanel and DrawBroadcastControls,
// so the panel and the hover tooltip end up on top.
internal void
DrawWorld3D(app_state *App, app_input *Input, v2 BufferDim)
{
 r3_state *R3 = &App->R3;

 // NOTE: Over the panel the wheel scrolls the panel instead of zooming.
 b32 MouseOverUI = IsInsideRectV2(V2V2S32(Input->Mouse.Pos), App->Panel.LastRect);
 R3CameraUpdate(&R3->Camera, Input, MouseOverUI);
 R3BeginFrame(R3, FrameArena, BufferDim);
 W3ComputeStatus(App);
 W3PrepareIslands(App, Input);   // uses last frame's UI areas for hovering
 W3BuildWorld(App);
 R3EndFrame(R3, &App->Render);

 // Cable costs first, so the UI and labels are drawn on top of them.
 V3DrawHoveredCableCosts(App);

 //- UI, recording the areas it covers
 R3->UIRectCount = 0;
 V3AddUIRect(R3, RectFromSize(V2(0.f, 0.f), V2(V3D_ControlsAreaWidth, V3D_ControlsAreaHeight)));
 V3AddUIRect(R3, App->Panel.LastRect);
 V3DrawSummaryBar(App);
 V3DrawCameraControls(App, Input, BufferDim);
 V3DrawHelpButtons(App, Input, BufferDim);

 V3DrawIslandLabels(App);
}
