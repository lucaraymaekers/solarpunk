//-
// SolarPunk 3D view: runs the 3D pass and draws the 2D overlay on top of it
// (island labels, cable costs, camera buttons and the legend).
//-

//~ Constants
#define V3D_Margin       12.f
#define V3D_LabelPadding 5.f

// NOTE: Screen areas covered by the 2D controls (top left) and camera buttons (top right).
// Island labels are not placed there. Sizes match the 20px font.
#define V3D_ControlsAreaWidth   840.f
#define V3D_ControlsAreaHeight  235.f
#define V3D_CameraAreaWidth     330.f
#define V3D_CameraAreaHeight    80.f

#define V3D_ColorLabelBg     0xE02E3440
#define V3D_ColorLabelBorder 0xFF88C0D0

internal v4 V3LegendRect(app_state *App, v2 BufferDim);

//~ Labels
internal b32
V3RectsOverlap(v4 A, v4 B)
{
 b32 Result = (A.Min.X < B.Max.X && B.Min.X < A.Max.X &&
               A.Min.Y < B.Max.Y && B.Min.Y < A.Max.Y);
 return Result;
}

// NOTE: Labels are placed nearest-island first (the hovered island always first) and a
// label that would overlap one already placed is skipped, so the view stays readable.
// Zoom in, or hover an island, to see the hidden ones.
internal void
V3DrawIslandLabels(app_state *App)
{
 r3_state *R3 = &App->R3;
 font_atlas *Atlas = &App->FontAtlas;
 font *Font = &App->Font;
 f32 LineHeight = Font->HeightPx;

 //- Order: hovered first, then by distance to the camera
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
    if(R3->HoveredIdx == (s32)Index)
    {
     Key = -1.f;
    }

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

 //- Place and draw. The UI areas count as already placed, so labels avoid them.
 v4 Placed[R3_MaxIslands + 3];
 u32 PlacedCount = 0;
 Placed[PlacedCount++] = RectFromSize(V2(0.f, 0.f), V2(V3D_ControlsAreaWidth, V3D_ControlsAreaHeight));
 Placed[PlacedCount++] = RectFromSize(V2(R3->ViewportDim.X - V3D_CameraAreaWidth, 0.f), V2(V3D_CameraAreaWidth, V3D_CameraAreaHeight));
 Placed[PlacedCount++] = V3LegendRect(App, R3->ViewportDim);
 for(u32 OrderIdx = 0; OrderIdx < Count; OrderIdx += 1)
 {
  u32 Index = Order[OrderIdx];
  island *Island = Islands[Index];
  b32 Hovered = (R3->HoveredIdx == (s32)Index);
  str8 Name = Island->Name;
  str8 Knows = Str8Fmt("knows %u/%u", BroadcastKnownCount(App, Island), App->Broadcast.IslandCount);

  f32 Width = Max(GraphMeasureText(Atlas, Font, Name), GraphMeasureText(Atlas, Font, Knows)) + 2.f*V3D_LabelPadding;
  f32 Height = 2.f*LineHeight + 2.f*V3D_LabelPadding;
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

   DrawRect(Box, V4FromU32(V3D_ColorLabelBg), 5.f, 0.f, 0.f);
   DrawRect(Box, V4FromU32(Hovered ? W3_ColorHover : V3D_ColorLabelBorder), 5.f, Hovered ? 2.f : 1.f, 0.f);
   GraphDrawText(Atlas, Font, V2(Min.X + V3D_LabelPadding, Min.Y + V3D_LabelPadding), Name, V4FromU32(Graph_ColorTitle));
   GraphDrawText(Atlas, Font, V2(Min.X + V3D_LabelPadding, Min.Y + V3D_LabelPadding + LineHeight), Knows, V4FromU32(Graph_ColorKnows));
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
       f32 Width = GraphMeasureText(Atlas, Font, Text) + 2.f*V3D_LabelPadding;
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

//~ Camera controls (top right)
internal void
V3DrawCameraControls(app_state *App, app_input *Input, v2 BufferDim)
{
 r3_camera *Camera = &App->R3.Camera;
 font_atlas *Atlas = &App->FontAtlas;
 font *Font = &App->Font;
 f32 LineHeight = Font->HeightPx;

 // NOTE: Plain C strings: MSVC warns (C4204) about arrays initialized with S8(...).
 char *LabelStrings[] = {"Zoom in", "Zoom out", "Reset view"};
 str8 Labels[ArrayCount(LabelStrings)];
 f32 TotalWidth = 0.f;
 for EachElement(Idx, LabelStrings)
 {
  Labels[Idx] = S8FromCString(LabelStrings[Idx]);
  TotalWidth += GraphMeasureText(Atlas, Font, Labels[Idx]) + 2.f*BroadcastView_ButtonPadding + BroadcastView_ButtonGap;
 }

 v2 Cursor = V2(BufferDim.X - V3D_Margin - TotalWidth, V3D_Margin);
 if(BroadcastButton(App, Input, &Cursor, Labels[0])) R3CameraZoom(Camera, 1.f/R3_CameraZoomStep);
 if(BroadcastButton(App, Input, &Cursor, Labels[1])) R3CameraZoom(Camera, R3_CameraZoomStep);
 if(BroadcastButton(App, Input, &Cursor, Labels[2])) R3CameraReset(Camera);

 str8 Hint = S8("Right-drag to rotate");
 f32 HintWidth = GraphMeasureText(Atlas, Font, Hint);
 GraphDrawText(Atlas, Font, V2(BufferDim.X - V3D_Margin - HintWidth - BroadcastView_ButtonGap,
                               V3D_Margin + LineHeight + BroadcastView_ButtonPadding + BroadcastView_ButtonGap),
               Hint, V4FromU32(Graph_ColorTitle));
}

//~ Legend (bottom left)
global_variable char *V3LegendNotes[] =
{
 "solid crates, green pad: produces",
 "crate frames, orange pad: needs",
 "yellow pulse on a cable: news travelling",
 "crystal grey -> cyan: knows more islands",
 "hover an island for details",
};
global_variable u32 V3LegendNoteColors[] = {W3_ColorProducePad, W3_ColorNeedPad, W3_ColorPulse, W3_ColorCrystalHigh, W3_ColorHover};
StaticAssert(ArrayCount(V3LegendNotes) == ArrayCount(V3LegendNoteColors), V3DLegendNotesMatch);

typedef struct v3d_legend_layout v3d_legend_layout;
struct v3d_legend_layout
{
 v4 Box;
 u32 ProductRows;
 f32 ProductColumnWidth;
};

internal v3d_legend_layout
V3LegendLayout(app_state *App, v2 BufferDim)
{
 font_atlas *Atlas = &App->FontAtlas;
 font *Font = &App->Font;
 f32 LineHeight = Font->HeightPx;
 f32 Pad = BroadcastView_ButtonPadding;
 f32 Swatch = LineHeight - 6.f;

 v3d_legend_layout Result = {0};
 u32 ProductCount = ProductKind_Count - 1;
 Result.ProductRows = (ProductCount + 1)/2;
 for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
 {
  Result.ProductColumnWidth = Max(Result.ProductColumnWidth, Swatch + Pad + GraphMeasureText(Atlas, Font, ProductNames[Kind]));
 }
 Result.ProductColumnWidth += 2.f*Pad;

 f32 NotesWidth = 0.f;
 for EachElement(Idx, V3LegendNotes)
 {
  NotesWidth = Max(NotesWidth, Swatch + Pad + GraphMeasureText(Atlas, Font, S8FromCString(V3LegendNotes[Idx])));
 }

 u32 Rows = Max(Result.ProductRows, (u32)ArrayCount(V3LegendNotes));
 f32 Width = 2.f*Result.ProductColumnWidth + NotesWidth + 2.f*Pad;
 f32 Height = (f32)(Rows + 1)*LineHeight + 2.f*Pad;
 Result.Box = RectFromSize(V2(V3D_Margin, BufferDim.Y - V3D_Margin - Height), V2(Width, Height));
 return Result;
}

internal v4
V3LegendRect(app_state *App, v2 BufferDim)
{
 v4 Result = V3LegendLayout(App, BufferDim).Box;
 return Result;
}

// NOTE: Two columns of product colours on the left, what the shapes mean on the right.
internal void
V3DrawLegend(app_state *App, v2 BufferDim)
{
 font_atlas *Atlas = &App->FontAtlas;
 font *Font = &App->Font;
 f32 LineHeight = Font->HeightPx;
 f32 Pad = BroadcastView_ButtonPadding;
 f32 Swatch = LineHeight - 6.f;

 v3d_legend_layout Layout = V3LegendLayout(App, BufferDim);
 u32 ProductRows = Layout.ProductRows;
 f32 ProductColumnWidth = Layout.ProductColumnWidth;
 v4 Box = Layout.Box;
 v2 Min = Box.Min;
 DrawRect(Box, V4FromU32(V3D_ColorLabelBg), 6.f, 0.f, 0.f);
 DrawRect(Box, V4FromU32(Graph_ColorNodeBorder), 6.f, 1.f, 0.f);

 v2 Origin = V2(Min.X + Pad, Min.Y + Pad);
 GraphDrawText(Atlas, Font, Origin, S8("Legend"), V4FromU32(Graph_ColorTitle));
 Origin.Y += LineHeight;

 //- Products, two columns
 for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
 {
  u32 Item = Kind - 1;
  v2 Line = V2(Origin.X + (f32)(Item/ProductRows)*ProductColumnWidth, Origin.Y + (f32)(Item % ProductRows)*LineHeight);
  DrawRect(RectFromSize(V2(Line.X, Line.Y + 3.f), V2(Swatch, Swatch)), V4FromU32(W3ProductColors[Kind]), 3.f, 0.f, 0.f);
  GraphDrawText(Atlas, Font, V2(Line.X + Swatch + Pad, Line.Y), ProductNames[Kind], V4FromU32(Graph_ColorTitle));
 }

 //- Meaning of shapes and colours
 for EachElement(Idx, V3LegendNotes)
 {
  v2 Line = V2(Origin.X + 2.f*ProductColumnWidth, Origin.Y + (f32)Idx*LineHeight);
  DrawRect(RectFromSize(V2(Line.X, Line.Y + 3.f), V2(Swatch, Swatch)), V4FromU32(V3LegendNoteColors[Idx]), 3.f, 0.f, 0.f);
  GraphDrawText(Atlas, Font, V2(Line.X + Swatch + Pad, Line.Y), S8FromCString(V3LegendNotes[Idx]), V4FromU32(Graph_ColorKnows));
 }
}

//~ Entry point
// NOTE: Call after RenderClear() and before the other overlays (DrawBroadcastControls),
// so the hover tooltip ends up on top.
internal void
DrawWorld3D(app_state *App, app_input *Input, v2 BufferDim)
{
 r3_state *R3 = &App->R3;

 R3CameraUpdate(&R3->Camera, Input);
 R3BeginFrame(R3, FrameArena, BufferDim);
 W3PrepareIslands(App, Input);
 W3BuildWorld(App);
 R3EndFrame(R3, &App->Render);

 // Cable costs first, so the island labels are drawn on top of them.
 V3DrawHoveredCableCosts(App);
 V3DrawIslandLabels(App);
 V3DrawCameraControls(App, Input, BufferDim);
 V3DrawLegend(App, BufferDim);
}
