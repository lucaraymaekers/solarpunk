//-
// SolarPunk 3D view: runs the 3D pass and draws the 2D overlay on top of it:
//   - the top bar: island status (left), time controls (middle), camera and help (right)
//   - under it, the products that are short worldwide
//   - cluster names, island labels (name + status dot), cable costs of the hovered island
//   - the help card ("?"), also shown on the first start
//-

//~ Constants
#define V3D_Margin       Ui_Margin
#define V3D_LabelPadding 5.f
#define V3D_ChipPadding  10.f

#define V3D_ColorLabelBg     0xF0FFF9EF

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

//~ Clusters
// NOTE: Generated worlds are made of clusters; the small hand-made world is not.
internal u32
V3ClusterCount(app_state *App)
{
 u32 Result = App->HasMapLayout ? WorldGen_ClusterCount : 0;
 return Result;
}

#define V3D_ClusterFocusDistance 40.f
#define V3D_ClusterLabelHeight   7.f

// NOTE: Show one cluster's island names and fly the camera to it; again to go back.
internal void
V3FocusCluster(app_state *App, s32 ClusterPlusOne)
{
 r3_state *R3 = &App->R3;
 if(ClusterPlusOne == 0 || R3->FocusedClusterPlusOne == ClusterPlusOne)
 {
  R3->FocusedClusterPlusOne = 0;
  R3CameraFlyTo(&R3->Camera, V3(0.f, -2.f, 0.f), R3_CameraDefaultDistance);
 }
 else
 {
  R3->FocusedClusterPlusOne = ClusterPlusOne;

  v3 Sum = V3(0.f, 0.f, 0.f);
  u32 Count = 0;
  for(u32 Idx = 0; Idx < R3->IslandCount; Idx += 1)
  {
   if(WorldGenClusterOf(Idx, R3->IslandCount) == (u32)(ClusterPlusOne - 1))
   {
    Sum = R3Add(Sum, R3->IslandTop[Idx]);
    Count += 1;
   }
  }
  if(Count)
  {
   v3 Center = R3Scale(Sum, 1.f/(f32)Count);
   R3CameraFlyTo(&R3->Camera, V3(Center.X, Center.Y - 2.f, Center.Z), V3D_ClusterFocusDistance);
  }
 }
}

// NOTE: One clickable name per cluster, floating above it, with how many of its
// islands are ok / waiting / short. Clicking it shows that cluster's island names.
internal void
V3DrawClusterLabels(app_state *App, app_input *Input)
{
 r3_state *R3 = &App->R3;
 font *Font = &App->Font;
 f32 LineHeight = Font->HeightPx;
 f32 Dot = LineHeight - 10.f;
 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 u32 ClusterCount = V3ClusterCount(App);
 v4 Taken[R3_MaxUIRects];
 u32 TakenCount = R3->UIRectCount;
 for(u32 Idx = 0; Idx < TakenCount; Idx += 1) Taken[Idx] = R3->UIRects[Idx];

 for(u32 Cluster = 0; Cluster < ClusterCount; Cluster += 1)
 {
  //- Where: above the middle of its islands
  v3 Sum = V3(0.f, 0.f, 0.f);
  f32 HighestTop = -1e30f;
  u32 Counts[IslandStatus_Count] = {0};
  u32 Count = 0;
  for(u32 Idx = 0; Idx < R3->IslandCount; Idx += 1)
  {
   if(WorldGenClusterOf(Idx, R3->IslandCount) == Cluster)
   {
    Sum = R3Add(Sum, R3->IslandTop[Idx]);
    HighestTop = Max(HighestTop, R3->IslandTop[Idx].Y);
    if(R3->IslandStatus[Idx] < IslandStatus_Count) Counts[R3->IslandStatus[Idx]] += 1;
    Count += 1;
   }
  }
  if(Count == 0) continue;

  v3 Center = R3Scale(Sum, 1.f/(f32)Count);
  v2 Screen;
  if(!R3ProjectToScreen(R3, V3(Center.X, HighestTop + V3D_ClusterLabelHeight, Center.Z), &Screen)) continue;

  //- Size: name, then a dot and a number per status that has islands
  str8 Name = S8FromCString(WorldGenClusterNames[Cluster]);
  f32 Width = V3Measure(App, Name) + 2.f*V3D_ChipPadding;
  for(u32 Status = 0; Status < IslandStatus_Count; Status += 1)
  {
   if(Counts[Status]) Width += 10.f + Dot + 4.f + V3Measure(App, Str8Fmt("%u", Counts[Status]));
  }
  f32 Height = LineHeight + 2.f*Ui_ButtonPadY;
  v4 Box = RectFromSize(V2(FloorF32(Screen.X - .5f*Width), FloorF32(Screen.Y - Height)), V2(Width, Height));

  // Not on top of the panel or the controls: slide it left of what covers it (the
  // panel is on the right), so every cluster stays clickable.
  for(u32 Try = 0; Try < 3; Try += 1)
  {
   for(u32 Idx = 0; Idx < TakenCount; Idx += 1)
   {
    if(V3RectsOverlap(Box, Taken[Idx]))
    {
     f32 Shift = (Box.Max.X - Taken[Idx].Min.X) + 8.f;
     Box.Min.X -= Shift;
     Box.Max.X -= Shift;
    }
   }
  }
  b32 Covered = (Box.Min.X < 0.f);
  for(u32 Idx = 0; Idx < TakenCount && !Covered; Idx += 1) Covered = V3RectsOverlap(Box, Taken[Idx]);
  if(Covered) continue;

  //- Draw and click
  b32 Focused = (R3->FocusedClusterPlusOne == (s32)Cluster + 1);
  b32 Hot = IsInsideRectV2(MousePos, Box) && !IsInsideRectV2(MousePos, R3->PopupRect);
  u32 Background = Focused ? Ui_ColorAccent : (Hot ? Ui_ColorButtonHot : Ui_ColorCard);
  UiCard(Box, .5f*Height);
  DrawRect(Box, V4FromU32(Background), .5f*Height, 0.f, 0.f);
  if(Hot)
  {
   UiSetHint(Focused ? "Click to see all clusters again" : "Click to fly here and show the island names",
             V2(.5f*(Box.Min.X + Box.Max.X), Box.Max.Y + 6.f));
  }

  f32 X = Box.Min.X + V3D_ChipPadding;
  f32 TextY = Box.Min.Y + .5f*(Height - LineHeight);
  UiText(App, V2(X, TextY), Name, Focused ? Ui_ColorOnAccent : Ui_ColorText);
  X += V3Measure(App, Name) + 10.f;
  for(u32 Status = 0; Status < IslandStatus_Count; Status += 1)
  {
   if(Counts[Status])
   {
    V3StatusDot(V2(X, Box.Min.Y + .5f*(Height - Dot)), Dot, W3StatusColor(Status));
    X += Dot + 4.f;
    str8 Number = Str8Fmt("%u", Counts[Status]);
    UiText(App, V2(X, TextY), Number, Focused ? Ui_ColorOnAccent : UiStatusText(Status));
    X += V3Measure(App, Number) + 10.f;
   }
  }

  V3AddUIRect(R3, Box);
  if(Hot && WasPressed(Input->Mouse.Buttons[PlatformMouseButton_Left]))
  {
   V3FocusCluster(App, (s32)Cluster + 1);
  }
 }
}

//~ Labels
// NOTE: One line per island: a status dot and the name. Placed nearest-island first
// (hovered and selected islands first); a label that would overlap the UI or another
// label is skipped. Zoom in, or hover an island, to see the hidden ones.
internal void
V3DrawIslandLabels(app_state *App)
{
 r3_state *R3 = &App->R3;
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

   // NOTE: With clusters, only the focused cluster shows its island names; any other
   // island only gets a label while it is hovered (e.g. a supplier row) or opened.
   b32 Hovered = (R3->HoveredIdx == (s32)Index);
   b32 Selected = (App->Panel.SelectedPlusOne == (s32)Index + 1);
   b32 InFocus = (V3ClusterCount(App) == 0 ||
                  (R3->FocusedClusterPlusOne > 0 &&
                   WorldGenClusterOf(Index, R3->IslandCount) == (u32)(R3->FocusedClusterPlusOne - 1)));

   if(R3->IslandVisible[Index] && (InFocus || Hovered || Selected))
   {
    f32 Key = R3Length(R3Sub(R3->IslandTop[Index], R3->Eye));
    if(Hovered)       Key = -2.f;
    else if(Selected) Key = -1.f;

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

  f32 Width = Dot + 10.f + V3Measure(App, Island->Name) + 2.f*V3D_LabelPadding;
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

   DrawRect(Box, V4FromU32(V3D_ColorLabelBg), .5f*Height, 0.f, 0.f);
   if(Hovered || Selected)
   {
    DrawRect(Box, V4FromU32(Hovered ? Ui_ColorAccent : Ui_ColorSelected), .5f*Height, 2.f, 0.f);
   }
   V3StatusDot(V2(Min.X + V3D_LabelPadding + 2.f, Min.Y + .5f*(Height - Dot)), Dot, W3StatusColor(R3->IslandStatus[Index]));
   UiText(App, V2(Min.X + V3D_LabelPadding + Dot + 8.f, Min.Y + V3D_LabelPadding), Island->Name, Ui_ColorText);
  }
 }
}

// NOTE: Only the hovered island's cables show their cost, to keep the view readable.
internal void
V3DrawHoveredCableCosts(app_state *App)
{
 r3_state *R3 = &App->R3;
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
       DrawRect(Box, V4FromU32(V3D_ColorLabelBg), 5.f, 0.f, 0.f);
       UiText(App, V2(Min.X + V3D_LabelPadding, Min.Y + 1.f), Text, Ui_ColorTitle);
      }
     }
    }
   }
   FromIdx += 1;
  }
 }
}

//~ Top bar
// NOTE: One bar across the top: island status chips (left), time controls (middle),
// camera buttons and help (right). Every status chip opens a list of those islands.

internal void PanelSelect(app_state *App, s32 IslandIdx);

#define V3D_PopupWidth 360.f

// NOTE: A pill with a dot (round) or product swatch (square) and text. Clickable;
// the open one is filled with the accent colour. Returns true when clicked.
internal b32
V3Chip(app_state *App, app_input *Input, v2 *Cursor, str8 Text, u32 DotColor, b32 Square, u32 TextColor, b32 Open, char *Hint)
{
 f32 Height = UiButtonHeight(App);
 f32 Dot = UiLineHeight(App) - 8.f;
 f32 Width = 2.f*V3D_ChipPadding + Dot + 6.f + V3Measure(App, Text);
 v4 Box = RectFromSize(*Cursor, V2(Width, Height));

 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 b32 Hot = IsInsideRectV2(MousePos, Box) && !UiMouseBlocked(App, MousePos);
 u32 Background = Open ? Ui_ColorAccent : (Hot ? Ui_ColorButtonHot : Ui_ColorButton);
 DrawRect(Box, V4FromU32(Background), .5f*Height, 0.f, 0.f);

 v2 DotPos = V2(Box.Min.X + V3D_ChipPadding, Box.Min.Y + .5f*(Height - Dot));
 if(Open) DrawRect(RectFromSize(V2(DotPos.X - 2.f, DotPos.Y - 2.f), V2(Dot + 4.f, Dot + 4.f)), V4FromU32(Ui_ColorCard), Square ? 4.f : .5f*Dot + 2.f, 0.f, 0.f);
 DrawRect(RectFromSize(DotPos, V2(Dot, Dot)), V4FromU32(DotColor), Square ? 3.f : .5f*Dot, 0.f, 0.f);
 UiText(App, V2(DotPos.X + Dot + 6.f, Box.Min.Y + Ui_ButtonPadY), Text, Open ? Ui_ColorOnAccent : TextColor);

 if(Hot && Hint)
 {
  UiSetHint(Hint, V2(.5f*(Box.Min.X + Box.Max.X), Box.Max.Y + 6.f));
 }
 Cursor->X += Width + Ui_Gap;

 b32 Result = (Hot && WasPressed(Input->Mouse.Buttons[PlatformMouseButton_Left]));
 return Result;
}

// NOTE: Clicking a chip opens its list under it; clicking the open chip closes it.
internal void
V3ToggleSummary(r3_state *R3, s32 Which, v2 TopLeft)
{
 if(R3->SummaryOpen == Which)
 {
  R3->SummaryOpen = 0;
 }
 else
 {
  R3->SummaryOpen = Which;
  R3->PopupRect = RectFromSize(TopLeft, V2(0.f, 0.f)); // real size set when drawn
 }
}

internal void
V3DrawTopBar(app_state *App, app_input *Input, v2 BufferDim)
{
 r3_state *R3 = &App->R3;
 r3_camera *Camera = &R3->Camera;
 f32 ButtonHeight = UiButtonHeight(App);
 v4 Bar = RectFromSize(V2(Ui_Margin, Ui_Margin), V2(BufferDim.X - 2.f*Ui_Margin, ButtonHeight + 2.f*Ui_Pad));
 UiCard(Bar, Ui_CardRadius);
 R3->TopBarRect = Bar;
 V3AddUIRect(R3, Bar);
 f32 Y = Bar.Min.Y + Ui_Pad;
 // Status lists open under the shortage strip (see V3DrawShortageStrip), so it stays visible.
 f32 PopupY = Bar.Max.Y + Ui_Gap + (ButtonHeight + 2.f*6.f) + Ui_Gap;

 //- Left: islands per status
 u32 Counts[IslandStatus_Count] = {0};
 for(u32 Idx = 0; Idx < R3->IslandCount; Idx += 1)
 {
  if(R3->IslandStatus[Idx] < IslandStatus_Count) Counts[R3->IslandStatus[Idx]] += 1;
 }

 // NOTE: Plain C strings: MSVC warns (C4204) about non-constant aggregate initializers.
 char *StatusFormats[IslandStatus_Count] = {"%u ok", "%u waiting", "%u short"};
 char *StatusHints[IslandStatus_Count] =
 {
  "Islands that know a supplier for every need. Click for the list",
  "Islands still waiting for news of a supplier. Click for the list",
  "Islands that need a product the world lacks. Click for the list",
 };
 v2 Cursor = V2(Bar.Min.X + Ui_Pad, Y);
 for(u32 Status = 0; Status < IslandStatus_Count; Status += 1)
 {
  s32 Which = 1 + (s32)Status;
  v2 ChipPos = Cursor;
  if(V3Chip(App, Input, &Cursor, Str8Fmt(StatusFormats[Status], Counts[Status]), W3StatusColor(Status), false,
            Ui_ColorText, (R3->SummaryOpen == Which), StatusHints[Status]))
  {
   V3ToggleSummary(R3, Which, V2(ChipPos.X, PopupY));
  }
 }
 f32 LeftEnd = Cursor.X;
 R3->SummaryBarRect = V4(Bar.Min.X, Bar.Min.Y, LeftEnd, Bar.Max.Y);

 //- Right: camera and help, laid out from the right edge
 b32 Focused = (R3->FocusedClusterPlusOne != 0);
 f32 RightWidth = (UiButtonWidth(App, S8("+"), 34.f) + UiButtonWidth(App, S8("-"), 34.f) +
                   UiButtonWidth(App, S8("Reset view"), 0.f) + UiButtonWidth(App, S8("?"), 34.f) + 4.f*Ui_Gap + 12.f);
 if(Focused) RightWidth += UiButtonWidth(App, S8("All clusters"), 0.f) + Ui_Gap;
 f32 RightStart = Bar.Max.X - Ui_Pad - RightWidth + Ui_Gap;

 Cursor = V2(RightStart, Y);
 if(Focused)
 {
  if(UiButtonEx(App, Input, &Cursor, S8("All clusters"), UiButton_Toggled, "Leave this cluster and see the whole world", 0.f))
  {
   V3FocusCluster(App, 0);
  }
 }
 if(UiButtonEx(App, Input, &Cursor, S8("+"), UiButton_Normal, "Zoom in (mouse wheel)", 34.f))  R3CameraZoom(Camera, 1.f/R3_CameraZoomStep);
 if(UiButtonEx(App, Input, &Cursor, S8("-"), UiButton_Normal, "Zoom out (mouse wheel)", 34.f)) R3CameraZoom(Camera, R3_CameraZoomStep);
 if(UiButtonEx(App, Input, &Cursor, S8("Reset view"), UiButton_Normal, "Back to the starting view", 0.f))
 {
  R3CameraReset(Camera);
  R3->FocusedClusterPlusOne = 0;
 }
 Cursor.X += 12.f;
 v2 HelpPos = Cursor;
 if(UiButtonEx(App, Input, &Cursor, S8("?"), R3->HelpOpen ? UiButton_Toggled : UiButton_Normal, "Help: what the colours mean, mouse and keys", 34.f))
 {
  R3->HelpOpen = !R3->HelpOpen;
  R3->SummaryOpen = 0;
 }
 R3->HelpButtonRect = RectFromSize(HelpPos, V2(34.f, ButtonHeight));

 //- Middle: time controls, centred on the bar when there is room
 f32 Fixed = TimeControlsFixedWidth(App);
 f32 Space = (RightStart - 16.f) - (LeftEnd + 16.f);
 f32 TimelineWidth = Clamp(100.f, Space - Fixed, 340.f);
 f32 Total = Fixed + TimelineWidth;
 f32 Start = .5f*(Bar.Min.X + Bar.Max.X) - .5f*Total;
 Start = Min(Start, RightStart - 16.f - Total);
 Start = Max(Start, LeftEnd + 16.f);

 // Thin dividers between the three groups.
 DrawRect(RectFromSize(V2(Start - 9.f, Y + 4.f), V2(1.f, ButtonHeight - 8.f)), V4FromU32(Ui_ColorLine), 0.f, 0.f, 0.f);
 DrawRect(RectFromSize(V2(RightStart - 9.f, Y + 4.f), V2(1.f, ButtonHeight - 8.f)), V4FromU32(Ui_ColorLine), 0.f, 0.f, 0.f);

 Cursor = V2(Start, Y);
 DrawTimeControls(App, Input, &Cursor, TimelineWidth);
}

//~ Products short worldwide (under the top bar)
internal void
V3DrawShortageStrip(app_state *App, app_input *Input)
{
 r3_state *R3 = &App->R3;
 f32 ButtonHeight = UiButtonHeight(App);
 v2 Origin = V2(R3->TopBarRect.Min.X, R3->TopBarRect.Max.Y + Ui_Gap);

 // Measure first, so the card can be drawn under the chips.
 str8 Label = S8("Short worldwide:");
 f32 Dot = UiLineHeight(App) - 8.f;
 f32 Width = Ui_Pad + UiMeasure(App, Label) + Ui_Gap;
 u32 ShortCount = 0;
 for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
 {
  if(R3->TotalNeeded[Kind] > R3->TotalProduced[Kind])
  {
   s64 Missing = (s64)R3->TotalNeeded[Kind] - (s64)R3->TotalProduced[Kind];
   str8 Name = ProductNames[Kind];
   Width += 2.f*V3D_ChipPadding + Dot + 6.f + UiMeasure(App, Str8Fmt("%.*s -%lld", (s32)Name.Size, Name.Data, Missing)) + Ui_Gap;
   ShortCount += 1;
  }
 }
 if(ShortCount == 0)
 {
  Label = S8("No product is short worldwide");
  Width = UiMeasure(App, Label) + 2.f*Ui_Pad + Dot + 6.f;
 }
 Width += Ui_Pad - Ui_Gap;

 v4 Box = RectFromSize(Origin, V2(Width, ButtonHeight + 2.f*6.f));
 UiCard(Box, Ui_CardRadius);
 V3AddUIRect(R3, Box);

 v2 Cursor = V2(Box.Min.X + Ui_Pad, Box.Min.Y + 6.f);
 if(ShortCount == 0)
 {
  UiDot(V2(Cursor.X, Cursor.Y + .5f*(ButtonHeight - Dot)), Dot, W3_ColorStatusOk);
  UiText(App, V2(Cursor.X + Dot + 6.f, Cursor.Y + Ui_ButtonPadY), Label, Ui_ColorOkText);
  return;
 }

 UiText(App, V2(Cursor.X, Cursor.Y + Ui_ButtonPadY), Label, Ui_ColorTextDim);
 Cursor.X += UiMeasure(App, Label) + Ui_Gap;
 for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
 {
  if(R3->TotalNeeded[Kind] > R3->TotalProduced[Kind])
  {
   s64 Missing = (s64)R3->TotalNeeded[Kind] - (s64)R3->TotalProduced[Kind];
   str8 Name = ProductNames[Kind];
   s32 Which = R3_SummaryProductBase + (s32)Kind;
   v2 ChipPos = Cursor;
   if(V3Chip(App, Input, &Cursor, Str8Fmt("%.*s -%lld", (s32)Name.Size, Name.Data, Missing), W3ProductColors[Kind], true,
             Ui_ColorShortText, (R3->SummaryOpen == Which), "More is needed than made. Click to see who needs and makes it"))
   {
    V3ToggleSummary(R3, Which, V2(ChipPos.X, Box.Max.Y + Ui_Gap));
   }
  }
 }
 // The chips open dropdowns too, so clicks here must not close them.
 R3->SummaryBarRect.Max.Y = Box.Max.Y;
 R3->ShortStripRect = Box;
}

//~ Summary dropdown
// NOTE: A row in the dropdown: island name with its status dot; returns true when clicked.
internal b32
V3PopupIslandRow(app_state *App, app_input *Input, v2 *Line, f32 Width, u32 IslandIdx, island *Island)
{
 r3_state *R3 = &App->R3;
 f32 LineHeight = UiLineHeight(App);
 f32 Dot = LineHeight - 10.f;
 v4 Row = RectFromSize(V2(Line->X - 4.f, Line->Y), V2(Width + 8.f, LineHeight));
 b32 Hot = IsInsideRectV2(V2V2S32(Input->Mouse.Pos), Row) && !R3->HelpOpen;
 if(Hot)
 {
  DrawRect(Row, V4FromU32(Ui_ColorRowHover), 4.f, 0.f, 0.f);
  R3->PopupHoverPlusOne = (s32)IslandIdx + 1;
 }
 V3StatusDot(V2(Line->X, Line->Y + .5f*(LineHeight - Dot)), Dot, W3StatusColor(R3->IslandStatus[IslandIdx]));
 UiText(App, V2(Line->X + Dot + 6.f, Line->Y), Island->Name, Ui_ColorText);
 b32 Result = (Hot && WasPressed(Input->Mouse.Buttons[PlatformMouseButton_Left]));
 return Result;
}

internal void
V3PopupSwatch(app_state *App, v2 Line, f32 X, product_kind Kind)
{
 f32 Size = UiLineHeight(App) - 6.f;
 DrawRect(RectFromSize(V2(X, Line.Y + 3.f), V2(Size, Size)), V4FromU32(W3ProductColors[Kind]), 3.f, 0.f, 0.f);
}

// NOTE: Draws the open dropdown (call after the panel, so it is on top of it).
// Clicking a row opens that island in the panel; clicking outside closes the dropdown.
internal void
DrawSummaryPopup(app_state *App, app_input *Input, v2 BufferDim)
{
 r3_state *R3 = &App->R3;
 f32 LineHeight = UiLineHeight(App);
 f32 Pad = Ui_Pad + 2.f;
 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 b32 Clicked = WasPressed(Input->Mouse.Buttons[PlatformMouseButton_Left]);
 R3->PopupHoverPlusOne = 0;

 if(R3->SummaryOpen == 0)
 {
  R3->PopupRect = V4(0.f, 0.f, 0.f, 0.f);
  return;
 }

 //- Close when clicking anywhere else (the chips toggle themselves)
 if(Clicked && !IsInsideRectV2(MousePos, R3->PopupRect) && !IsInsideRectV2(MousePos, R3->SummaryBarRect) &&
    !IsInsideRectV2(MousePos, R3->ShortStripRect))
 {
  R3->SummaryOpen = 0;
  R3->PopupRect = V4(0.f, 0.f, 0.f, 0.f);
  return;
 }

 //- Collect the rows
 b32 IsProduct = (R3->SummaryOpen >= R3_SummaryProductBase);
 product_kind Kind = (product_kind)(IsProduct ? R3->SummaryOpen - R3_SummaryProductBase : 0);
 u32 Status = (u32)(IsProduct ? 0 : R3->SummaryOpen - 1);
 if(IsProduct && (u32)Kind >= ProductKind_Count) return;

 island *Islands[R3_MaxIslands];
 u32 Indices[R3_MaxIslands];
 u64 Amounts[R3_MaxIslands];
 b32 Produces[R3_MaxIslands];
 u32 RowCount = 0;
 u32 NeedCount = 0;
 {
  // Product view: first the islands that need it, then the ones that produce it.
  for(u32 Pass = 0; Pass < (IsProduct ? 2u : 1u); Pass += 1)
  {
   u32 Index = 0;
   for(island *Island = App->FirstIsland; Island && Index < R3->IslandCount; Island = Island->Next)
   {
    b32 Include = false;
    u64 Amount = 0;
    if(!IsProduct)
    {
     Include = (R3->IslandStatus[Index] == Status);
    }
    else
    {
     product *List = (Pass == 0) ? Island->FirstRequirement : Island->FirstProduct;
     for(product *Product = List; Product; Product = Product->Next)
     {
      if(Product->Kind == Kind) { Include = true; Amount += Product->Amount; }
     }
    }
    if(Include && RowCount < R3_MaxIslands)
    {
     Islands[RowCount] = Island;
     Indices[RowCount] = Index;
     Amounts[RowCount] = Amount;
     Produces[RowCount] = (Pass == 1);
     RowCount += 1;
     if(Pass == 0) NeedCount += 1;
    }
    Index += 1;
   }
  }
 }

 //- Title and layout
 str8 Title = {0};
 str8 Hint = S8("Click an island to open it");
 if(IsProduct)
 {
  str8 Name = ProductNames[Kind];
  Title = Str8Fmt("%.*s: %llu needed, %llu made", (s32)Name.Size, Name.Data, R3->TotalNeeded[Kind], R3->TotalProduced[Kind]);
 }
 else
 {
  char *Titles[IslandStatus_Count] =
  {
   "Ok: a supplier known for every need",
   "Waiting for news of a supplier for:",
   "Short: needs a product the world lacks:",
  };
  Title = S8FromCString(Titles[Status]);
 }

 u32 SectionRows = (IsProduct ? 2u : 0u);
 v2 Min = R3->PopupRect.Min;
 f32 Width = Max(V3D_PopupWidth, UiMeasure(App, Title) + 2.f*Pad);
 f32 MaxHeight = BufferDim.Y - Min.Y - V3D_Margin;
 u32 MaxRows = (u32)Max(1.f, (MaxHeight - 2.f*Pad)/LineHeight - 3.f - (f32)SectionRows);
 u32 ShownRows = Min(RowCount, MaxRows);
 f32 Height = (f32)(ShownRows + 2 + SectionRows + (RowCount > ShownRows ? 1 : 0))*LineHeight + 2.f*Pad + 4.f;
 if(RowCount == 0) Height += LineHeight;
 v4 Box = RectFromSize(Min, V2(Width, Height));
 R3->PopupRect = Box;
 UiCard(Box, Ui_CardRadius);

 v2 Line = V2(Box.Min.X + Pad, Box.Min.Y + Pad);
 f32 Inner = Width - 2.f*Pad;
 UiText(App, Line, Title, Ui_ColorTitle);
 Line.Y += LineHeight;
 UiText(App, Line, Hint, Ui_ColorTextDim);
 Line.Y += LineHeight + 4.f;

 if(RowCount == 0)
 {
  UiText(App, Line, S8("none"), Ui_ColorTextDim);
 }

 for(u32 Row = 0; Row < ShownRows; Row += 1)
 {
  if(IsProduct && (Row == 0 || (Row == NeedCount)))
  {
   str8 Section = (Row == 0 && NeedCount > 0) ? S8("Need it:") : S8("Make it:");
   UiText(App, Line, Section, Ui_ColorTextDim);
   Line.Y += LineHeight;
  }

  u32 IslandIdx = Indices[Row];
  island *Island = Islands[Row];
  if(V3PopupIslandRow(App, Input, &Line, Inner, IslandIdx, Island))
  {
   PanelSelect(App, (s32)IslandIdx);
   R3->SummaryOpen = 0;
  }

  // Right side: what makes it waiting / short, or the amount of the product.
  f32 Right = Line.X + Inner;
  if(IsProduct)
  {
   str8 Amount = Produces[Row] ? Str8Fmt("+%llu", Amounts[Row]) : Str8Fmt("-%llu", Amounts[Row]);
   UiText(App, V2(Right - UiMeasure(App, Amount), Line.Y), Amount, Produces[Row] ? Ui_ColorOkText : Ui_ColorWaitingText);
  }
  else if(Status != IslandStatus_Ok)
  {
   f32 X = Right;
   for(product *Req = Island->FirstRequirement; Req; Req = Req->Next)
   {
    b32 Show = (Status == IslandStatus_Waiting) ? !W3KnowsSupplier(App, IslandIdx, Req->Kind)
     : (R3->TotalNeeded[Req->Kind] > R3->TotalProduced[Req->Kind]);
    if(Show)
    {
     X -= LineHeight;
     V3PopupSwatch(App, Line, X, Req->Kind);
    }
   }
  }
  Line.Y += LineHeight;
 }

 if(RowCount > ShownRows)
 {
  UiText(App, Line, Str8Fmt("+%u more (see the Islands tab)", RowCount - ShownRows), Ui_ColorTextDim);
 }
}

//~ Help card ("?" in the top bar; also open on the first start)
global_variable char *V3HelpStatusNotes[] =
{
 "Green: knows a supplier for everything it needs",
 "Orange: still waiting for news of a supplier",
 "Red: knows its suppliers, but a need is short worldwide",
};
global_variable char *V3HelpMapNotes[] =
{
 "Solid crates on a green pad: what an island makes",
 "Crate frames on an orange pad: what it needs",
 "Light pulse on a cable: news travelling",
};
global_variable u32 V3HelpMapColors[] = {W3_ColorProducePad, W3_ColorNeedPad, W3_ColorPulse};
StaticAssert(ArrayCount(V3HelpMapNotes) == ArrayCount(V3HelpMapColors), V3HelpMapNotesMatch);

global_variable char *V3HelpKeys[] =
{
 "Click",        "Right drag",   "Middle drag", "Mouse wheel",
 "Up / Down",    "Right",        "Left",
};
global_variable char *V3HelpActions[] =
{
 "open an island", "move the map", "turn the map", "zoom",
 "faster / slower",             "next round",   "restart",
};
StaticAssert(ArrayCount(V3HelpKeys) == ArrayCount(V3HelpActions), V3HelpKeysMatch);

#define V3D_HelpWidth   760.f
#define V3D_HelpColumns 3

// NOTE: Section title with a thin line under it; advances Y.
internal void
V3HelpSection(app_state *App, f32 X, f32 *Y, f32 Width, char *Title)
{
 UiText(App, V2(X, *Y), S8FromCString(Title), Ui_ColorTitle);
 *Y += UiLineHeight(App) + 2.f;
 DrawRect(RectFromSize(V2(X, *Y), V2(Width, 1.f)), V4FromU32(Ui_ColorLine), 0.f, 0.f, 0.f);
 *Y += 6.f;
}

internal void
DrawHelpCard(app_state *App, app_input *Input, v2 BufferDim)
{
 r3_state *R3 = &App->R3;

 // Open on the first start, so new users see what everything means.
 if(!R3->HelpShownOnce)
 {
  R3->HelpShownOnce = true;
  R3->HelpOpen = true;
 }
 if(!R3->HelpOpen) return;

 f32 LineHeight = UiLineHeight(App);
 f32 Pad = 20.f;
 f32 Swatch = LineHeight - 8.f;
 u32 ProductCount = ProductKind_Count - 1;
 u32 ProductRows = (ProductCount + V3D_HelpColumns - 1)/V3D_HelpColumns;
 u32 KeyRows = (u32)ArrayCount(V3HelpKeys);

 f32 Width = Min(V3D_HelpWidth, BufferDim.X - 2.f*Ui_Margin);
 f32 Inner = Width - 2.f*Pad;
 f32 Height = (3.f*LineHeight + 10.f) +                                          // title, intro
  3.f*(LineHeight + 12.f) +                                                      // section titles
  (f32)(ArrayCount(V3HelpStatusNotes) + ArrayCount(V3HelpMapNotes))*LineHeight + 8.f +
  (f32)ProductRows*LineHeight + 8.f +
  (f32)((KeyRows + 1)/2)*LineHeight + 8.f +
  UiButtonHeight(App) + 12.f + 2.f*Pad;
 v4 Box = RectFromSize(V2(FloorF32(.5f*(BufferDim.X - Width)), FloorF32(Max(Ui_Margin, .5f*(BufferDim.Y - Height)))), V2(Width, Height));

 //- Close: the button, or a click outside the card (but not on "?", which toggles it)
 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 b32 Clicked = WasPressed(Input->Mouse.Buttons[PlatformMouseButton_Left]);
 if(Clicked && !IsInsideRectV2(MousePos, Box) && !IsInsideRectV2(MousePos, R3->HelpButtonRect))
 {
  R3->HelpOpen = false;
  return;
 }

 DrawRect(V4(0.f, 0.f, BufferDim.X, BufferDim.Y), V4FromU32(Ui_ColorDimBackdrop), 0.f, 0.f, 0.f);
 UiCard(Box, 14.f);
 V3AddUIRect(R3, Box);

 f32 X = Box.Min.X + Pad;
 f32 Y = Box.Min.Y + Pad;
 UiText(App, V2(X, Y), S8("How to read the map"), Ui_ColorText);
 Y += LineHeight;
 UiText(App, V2(X, Y), S8("Every island tells its neighbours what it makes and needs."), Ui_ColorTextDim);
 Y += LineHeight;
 UiText(App, V2(X, Y), S8("Each round, that news moves one cable further."), Ui_ColorTextDim);
 Y += LineHeight + 10.f;

 //- Colours and shapes
 V3HelpSection(App, X, &Y, Inner, "Ring around an island");
 for EachElement(Idx, V3HelpStatusNotes)
 {
  UiDot(V2(X, Y + .5f*(LineHeight - Swatch)), Swatch, W3StatusColor((u32)Idx));
  UiText(App, V2(X + Swatch + 8.f, Y), S8FromCString(V3HelpStatusNotes[Idx]), Ui_ColorText);
  Y += LineHeight;
 }
 for EachElement(Idx, V3HelpMapNotes)
 {
  v4 Dot = RectFromSize(V2(X, Y + .5f*(LineHeight - Swatch)), V2(Swatch, Swatch));
  DrawRect(Dot, V4FromU32(V3HelpMapColors[Idx]), 3.f, 0.f, 0.f);
  DrawRect(Dot, V4FromU32(Ui_ColorCardBorder), 3.f, 1.f, 0.f);
  UiText(App, V2(X + Swatch + 8.f, Y), S8FromCString(V3HelpMapNotes[Idx]), Ui_ColorText);
  Y += LineHeight;
 }
 Y += 8.f;

 //- Products
 V3HelpSection(App, X, &Y, Inner, "Products (crate colours)");
 {
  f32 ColumnWidth = Inner/(f32)V3D_HelpColumns;
  for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
  {
   u32 Item = Kind - 1;
   v2 Pos = V2(X + (f32)(Item/ProductRows)*ColumnWidth, Y + (f32)(Item % ProductRows)*LineHeight);
   DrawRect(RectFromSize(V2(Pos.X, Pos.Y + .5f*(LineHeight - Swatch)), V2(Swatch, Swatch)), V4FromU32(W3ProductColors[Kind]), 3.f, 0.f, 0.f);
   UiText(App, V2(Pos.X + Swatch + 8.f, Pos.Y), ProductNames[Kind], Ui_ColorText);
  }
  Y += (f32)ProductRows*LineHeight + 8.f;
 }

 //- Mouse and keys, in two columns
 V3HelpSection(App, X, &Y, Inner, "Mouse and keys");
 {
  f32 KeyWidth = 0.f;
  for EachElement(Idx, V3HelpKeys) KeyWidth = Max(KeyWidth, UiMeasure(App, S8FromCString(V3HelpKeys[Idx])));
  KeyWidth += 16.f;
  u32 Rows = (KeyRows + 1)/2;
  for EachElement(Idx, V3HelpKeys)
  {
   v2 Pos = V2(X + (f32)((u32)Idx/Rows)*.5f*Inner, Y + (f32)((u32)Idx % Rows)*LineHeight);
   str8 Key = S8FromCString(V3HelpKeys[Idx]);
   DrawRect(RectFromSize(V2(Pos.X - 4.f, Pos.Y + 1.f), V2(UiMeasure(App, Key) + 8.f, LineHeight - 2.f)), V4FromU32(Ui_ColorButton), 4.f, 0.f, 0.f);
   UiText(App, Pos, Key, Ui_ColorText);
   UiText(App, V2(Pos.X + KeyWidth, Pos.Y), S8FromCString(V3HelpActions[Idx]), Ui_ColorTextDim);
  }
  Y += (f32)Rows*LineHeight + 8.f;
 }

 //- Close button (bottom right). Drawn by hand: the other buttons are blocked while help is open.
 {
  str8 Label = S8("Got it");
  f32 ButtonWidth = UiMeasure(App, Label) + 2.f*Ui_ButtonPadX + 20.f;
  v4 Button = RectFromSize(V2(Box.Max.X - Pad - ButtonWidth, Box.Max.Y - Pad - UiButtonHeight(App)), V2(ButtonWidth, UiButtonHeight(App)));
  b32 Hot = IsInsideRectV2(MousePos, Button);
  DrawRect(Button, V4FromU32(Hot ? Ui_ColorAccentHot : Ui_ColorAccent), Ui_ButtonRadius, 0.f, 0.f);
  UiText(App, V2(FloorF32(Button.Min.X + .5f*(ButtonWidth - UiMeasure(App, Label))), Button.Min.Y + Ui_ButtonPadY), Label, Ui_ColorOnAccent);
  UiText(App, V2(X, Button.Min.Y + Ui_ButtonPadY), S8("Open this again with the ? button, top right."), Ui_ColorTextDim);
  if(Hot && Clicked)
  {
   R3->HelpOpen = false;
  }
 }
}

//~ Entry point
// NOTE: Call after RenderClear() and before DrawIslandPanel, so the panel, the dropdown,
// the help card and the hover tooltip end up on top.
internal void
DrawWorld3D(app_state *App, app_input *Input, v2 BufferDim)
{
 r3_state *R3 = &App->R3;

 // NOTE: Over the panel the wheel scrolls the panel instead of zooming.
 b32 MouseOverUI = (IsInsideRectV2(V2V2S32(Input->Mouse.Pos), App->Panel.LastRect) || R3->HelpOpen);
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
 R3->ShortStripRect = V4(0.f, 0.f, 0.f, 0.f);
 V3AddUIRect(R3, App->Panel.LastRect);
 V3AddUIRect(R3, R3->PopupRect);   // summary dropdown, drawn later (see DrawSummaryPopup)
 V3DrawTopBar(App, Input, BufferDim);
 V3DrawShortageStrip(App, Input);

 V3DrawClusterLabels(App, Input);
 V3DrawIslandLabels(App);
}
