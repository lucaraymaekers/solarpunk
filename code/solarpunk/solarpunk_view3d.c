//-
// SolarPunk 3D view: runs the 3D pass and draws the 2D overlay on top of it:
//   - the top bar: island status (left), time controls (middle), camera and help (right)
//   - under it, the foods islands went without lately
//   - cluster names, island labels (name + status dot)
//   - the help card ("?"), also shown on the first start
//-

//~ Constants
#define V3D_Margin       Ui_Margin
#define V3D_LabelPadding 5.f
#define V3D_ChipPadding  10.f

#define V3D_ColorLabelBg     0xF0F4EAD3

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
 DrawRect(RectFromSize(Pos, V2(Size, Size)), V4FromU32(Color), .5f*Size, 0.f, .5f);
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
#define V3D_RightClickSlop       5.f   // pixels a right click may move and still count as a click

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

// NOTE: One clickable name per cluster, floating above it. Clicking it shows that cluster's island names.
internal void
V3DrawClusterLabels(app_state *App, app_input *Input)
{
 r3_state *R3 = &App->R3;
 font *Font = &App->Font;
 f32 LineHeight = Font->HeightPx;
 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 u32 ClusterCount = V3ClusterCount(App);
 v4 Taken[R3_MaxUIRects];
 u32 TakenCount = R3->UIRectCount;
 for(u32 Idx = 0; Idx < TakenCount; Idx += 1) Taken[Idx] = R3->UIRects[Idx];
 v4 Placed[WorldGen_ClusterCount];   // cluster labels drawn so far, so they do not overlap
 u32 PlacedCount = 0;

 for(u32 Cluster = 0; Cluster < ClusterCount; Cluster += 1)
 {
  //- Where: above the middle of its islands
  v3 Sum = V3(0.f, 0.f, 0.f);
  f32 HighestTop = -1e30f;
  u32 Count = 0;
  for(u32 Idx = 0; Idx < R3->IslandCount; Idx += 1)
  {
   if(WorldGenClusterOf(Idx, R3->IslandCount) == Cluster)
   {
    Sum = R3Add(Sum, R3->IslandTop[Idx]);
    HighestTop = Max(HighestTop, R3->IslandTop[Idx].Y);
    Count += 1;
   }
  }
  if(Count == 0) continue;

  v3 Center = R3Scale(Sum, 1.f/(f32)Count);
  v2 Screen;
  if(!R3ProjectToScreen(R3, V3(Center.X, HighestTop + V3D_ClusterLabelHeight, Center.Z), &Screen)) continue;

  //- Size: just the name
  str8 Name = S8FromCString(WorldGenClusterNames[Cluster]);
  f32 Width = V3Measure(App, Name) + 2.f*V3D_ChipPadding;
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
  // Not on top of another cluster's label: move just below it.
  for(u32 Try = 0; Try < WorldGen_ClusterCount; Try += 1)
  {
   b32 Moved = false;
   for(u32 Idx = 0; Idx < PlacedCount; Idx += 1)
   {
    if(V3RectsOverlap(Box, Placed[Idx]))
    {
     f32 Shift = (Placed[Idx].Max.Y - Box.Min.Y) + 4.f;
     Box.Min.Y += Shift;
     Box.Max.Y += Shift;
     Moved = true;
    }
   }
   if(!Moved) break;
  }
  b32 Covered = (Box.Min.X < 0.f);
  for(u32 Idx = 0; Idx < TakenCount && !Covered; Idx += 1) Covered = V3RectsOverlap(Box, Taken[Idx]);
  if(Covered) continue;
  if(PlacedCount < ArrayCount(Placed)) Placed[PlacedCount++] = Box;

  //- Draw and click
  b32 Focused = (R3->FocusedClusterPlusOne == (s32)Cluster + 1);
  b32 Hot = IsInsideRectV2(MousePos, Box) && !IsInsideRectV2(MousePos, R3->PopupRect);
  u32 Background = Focused ? Ui_ColorAccent : (Hot ? Ui_ColorButtonHot : Ui_ColorCard);
  UiButtonFace(Box, Background);
  if(Hot)
  {
   UiSetHint(Focused ? "Click to see all clusters again" : "Click to fly here and show the island names",
             V2(.5f*(Box.Min.X + Box.Max.X), Box.Max.Y + 6.f));
  }

  f32 X = Box.Min.X + V3D_ChipPadding;
  f32 TextY = Box.Min.Y + .5f*(Height - LineHeight);
  UiText(App, V2(X, TextY), Name, Focused ? Ui_ColorOnAccent : Ui_ColorText);

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

   DrawRect(Box, V4FromU32(V3D_ColorLabelBg), 0.f, 0.f, 0.f);
   if(Hovered || Selected)
   {
    DrawRect(Box, V4FromU32(Hovered ? Ui_ColorAccent : Ui_ColorSelected), 0.f, 2.f, 0.f);
   }
   u32 DotColor = RouteIsDown(App, Index) ? W3_ColorStatusDown : W3StatusColor(R3->IslandStatus[Index]);
   V3StatusDot(V2(Min.X + V3D_LabelPadding + 2.f, Min.Y + .5f*(Height - Dot)), Dot, DotColor);
   UiText(App, V2(Min.X + V3D_LabelPadding + Dot + 8.f, Min.Y + V3D_LabelPadding), Island->Name, Ui_ColorText);
  }
 }
}

//~ Cargo signs
// NOTE: A small sign floating above every goods train with the icons of the foods it
// carries (filled in by W3AddCargoSign). Signs under the UI are skipped. Only shown
// while an island is selected (then only its own goods trains are drawn).
internal void
V3DrawCargoSigns(app_state *App)
{
 r3_state *R3 = &App->R3;
 if(App->Panel.SelectedPlusOne == 0) return;
 f32 Size = UiLineHeight(App);
 f32 Pad = 3.f;
 for(u32 Sign = 0; Sign < R3->CargoSignCount; Sign += 1)
 {
  v2 Screen;
  u32 Kinds = R3->CargoSignKinds[Sign];
  u32 KindCount = 0;
  for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1) if(Kinds & (1u << Kind)) KindCount += 1;
  if(KindCount > 0 && R3ProjectToScreen(R3, R3->CargoSignPos[Sign], &Screen))
  {
   f32 Width = (f32)KindCount*Size + 2.f*Pad;
   f32 Height = Size + 2.f*Pad;
   v2 Min = V2(FloorF32(Screen.X - .5f*Width), FloorF32(Screen.Y - Height));
   v4 Box = RectFromSize(Min, V2(Width, Height));
   b32 UnderUI = false;
   for(u32 Idx = 0; Idx < R3->UIRectCount && !UnderUI; Idx += 1) UnderUI = V3RectsOverlap(Box, R3->UIRects[Idx]);
   if(!UnderUI)
   {
    // Post down to the train, then the sign with a gold edge.
    DrawRect(RectFromSize(V2(FloorF32(Screen.X) - 1.f, Box.Max.Y), V2(2.f, 6.f)), V4FromU32(Ui_ColorCardBorder), 0.f, 0.f, 0.f);
    DrawRect(Box, V4FromU32(V3D_ColorLabelBg), 0.f, 0.f, 0.f);
    DrawRect(Box, V4FromU32(Ui_ColorCardBorder), 0.f, 1.f, 0.f);
    f32 X = Min.X + Pad;
    for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
    {
     if(Kinds & (1u << Kind))
     {
      UiProductIcon(App, RectFromSize(V2(X, Min.Y + Pad), V2(Size, Size)), Kind, W3ProductColor(Kind));
      X += Size;
     }
    }
   }
  }
 }
}

//~ Top bar
// NOTE: One bar across the top: island status chips (left), time controls (middle),
// camera buttons and help (right). Every status chip opens a list of those islands.

internal void PanelSelect(app_state *App, s32 IslandIdx);

#define V3D_PopupWidth 360.f

// NOTE: A chip with a round status dot (ProductKind = 0) or a product's icon, and text.
// Clickable; the open one is filled with the accent colour. Returns true when clicked.
internal b32
V3Chip(app_state *App, app_input *Input, v2 *Cursor, str8 Text, u32 DotColor, u32 ProductKind, u32 TextColor, b32 Open, char *Hint)
{
 f32 Height = UiButtonHeight(App);
 f32 Dot = UiLineHeight(App) - 8.f;
 f32 Width = 2.f*V3D_ChipPadding + Dot + 6.f + V3Measure(App, Text);
 v4 Box = RectFromSize(*Cursor, V2(Width, Height));

 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 b32 Hot = IsInsideRectV2(MousePos, Box) && !UiMouseBlocked(App, MousePos);
 u32 Background = Open ? Ui_ColorAccent : (Hot ? Ui_ColorButtonHot : Ui_ColorButton);
 UiButtonFace(Box, Background);

 v2 DotPos = V2(Box.Min.X + V3D_ChipPadding, Box.Min.Y + .5f*(Height - Dot));
 if(ProductKind)
 {
  UiProductIcon(App, RectFromSize(V2(DotPos.X - 4.f, DotPos.Y - 4.f), V2(Dot + 8.f, Dot + 8.f)), ProductKind, DotColor);
 }
 else
 {
  if(Open) DrawRect(RectFromSize(V2(DotPos.X - 2.f, DotPos.Y - 2.f), V2(Dot + 4.f, Dot + 4.f)), V4FromU32(Ui_ColorCard), .5f*Dot + 2.f, 0.f, .5f);
  DrawRect(RectFromSize(DotPos, V2(Dot, Dot)), V4FromU32(DotColor), .5f*Dot, 0.f, .5f);
 }
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

internal f32
V3ChipWidth(app_state *App, str8 Text)
{
 f32 Dot = UiLineHeight(App) - 8.f;
 f32 Result = 2.f*V3D_ChipPadding + Dot + 6.f + V3Measure(App, Text);
 return Result;
}

// NOTE: When the text is big and everything does not fit in one row, the time controls
// move to a second row of the bar.
internal void
V3DrawTopBar(app_state *App, app_input *Input, v2 BufferDim)
{
 r3_state *R3 = &App->R3;
 f32 ButtonHeight = UiButtonHeight(App);
 f32 Square = ButtonHeight;
 f32 Scale = UiScale(App);
 
 //- Islands per status, and the width of each group
 u32 Counts[IslandStatus_Count] = {0};
 for(u32 Idx = 0; Idx < R3->IslandCount; Idx += 1)
 {
  if(R3->IslandStatus[Idx] < IslandStatus_Count) Counts[R3->IslandStatus[Idx]] += 1;
 }
 
 // NOTE: Plain C strings: MSVC warns (C4204) about non-constant aggregate initializers.
 // The coloured dot says which status it is (the hint says it in words).
 char *StatusFormats[IslandStatus_Count] = {"%u", "%u", "%u"};
 char *StatusHints[IslandStatus_Count] =
 {
  "Islands that know a supplier for every need. Click for the list",
  "Islands still waiting for news of a supplier. Click for the list",
  "Islands that need a product the world lacks. Click for the list",
 };
 str8 StatusTexts[IslandStatus_Count];
 f32 LeftWidth = 0.f;
 for(u32 Status = 0; Status < IslandStatus_Count; Status += 1)
 {
  StatusTexts[Status] = Str8Fmt(StatusFormats[Status], Counts[Status]);
  LeftWidth += V3ChipWidth(App, StatusTexts[Status]) + Ui_Gap;
 }
 
 b32 HasClusters = (V3ClusterCount(App) > 0);
 f32 RightWidth = Square + Ui_Gap;
 if(HasClusters) RightWidth += Square + 12.f + Ui_Gap;
 RightWidth += Square + 12.f + Ui_Gap;   // hide / show the islands panel

 
 f32 Fixed = TimeControlsFixedWidth(App);
 f32 MinTimeline = 120.f*Scale;
 f32 MaxTimeline = 340.f*Scale;
 f32 Inner = BufferDim.X - 2.f*Ui_Margin - 2.f*Ui_Pad;
 b32 OneRow = (LeftWidth + 16.f + Fixed + MinTimeline + 16.f + RightWidth <= Inner);
 
 //- The bar
 f32 BarHeight = (OneRow ? ButtonHeight : 2.f*ButtonHeight + Ui_Gap) + 2.f*Ui_Pad;
 v4 Bar = RectFromSize(V2(Ui_Margin, Ui_Margin), V2(BufferDim.X - 2.f*Ui_Margin, BarHeight));
 UiCard(Bar, Ui_CardRadius);
 R3->TopBarRect = Bar;
 V3AddUIRect(R3, Bar);
 f32 Y = Bar.Min.Y + Ui_Pad;
 // Status lists open under the shortage strip (see V3DrawShortageStrip), so it stays visible.
 f32 PopupY = Bar.Max.Y + Ui_Gap + (ButtonHeight + 2.f*6.f) + Ui_Gap;
 
 //- Left: islands per status
 v2 Cursor = V2(Bar.Min.X + Ui_Pad, Y);
 for(u32 Status = 0; Status < IslandStatus_Count; Status += 1)
 {
  s32 Which = 1 + (s32)Status;
  v2 ChipPos = Cursor;
  if(V3Chip(App, Input, &Cursor, StatusTexts[Status], W3StatusColor(Status), 0,
            Ui_ColorText, (R3->SummaryOpen == Which), StatusHints[Status]))
  {
   V3ToggleSummary(R3, Which, V2(ChipPos.X, PopupY));
  }
 }
 f32 LeftEnd = Cursor.X;
 R3->SummaryBarRect = V4(Bar.Min.X, Bar.Min.Y, LeftEnd, Y + ButtonHeight + Ui_Pad);
 
 //- Right: "All clusters" (a list of the clusters) and help, laid out from the right edge
 f32 RightStart = Bar.Max.X - Ui_Pad - RightWidth + Ui_Gap;
 Cursor = V2(RightStart, Y);
 R3->ClustersButtonRect = V4(0.f, 0.f, 0.f, 0.f);
 if(HasClusters)
 {
  v2 ButtonPos = Cursor;
  // Opens the Clusters tab of the panel; again goes back to the islands list.
  b32 Open = (!App->Panel.Hidden && App->Panel.Tab == PanelTab_Clusters);
  if(UiIconButton(App, Input, &Cursor, Icon_Grid, Open ? UiButton_Toggled : UiButton_Normal, "Clusters"))
  {
   App->Panel.Hidden = false;
   App->Panel.Tab = Open ? PanelTab_Islands : PanelTab_Clusters;
   App->Panel.Scroll = 0.f;
   R3->SummaryOpen = 0;
  }
  R3->ClustersButtonRect = RectFromSize(ButtonPos, V2(Square, ButtonHeight));
  Cursor.X += 12.f;
 }
 v2 HelpPos = Cursor;
 if(UiIconButton(App, Input, &Cursor, Icon_Question, R3->HelpOpen ? UiButton_Toggled : UiButton_Normal, "Help: what the colours mean, mouse and keys"))
 {
  R3->HelpOpen = !R3->HelpOpen;
  R3->SummaryOpen = 0;
 }
 R3->HelpButtonRect = RectFromSize(HelpPos, V2(Square, ButtonHeight));
 // Hide / show the islands panel: one button, last on the right (it points where the panel goes).
 {
  Cursor.X += 12.f;
  b32 Hidden = App->Panel.Hidden;
  if(UiIconButton(App, Input, &Cursor, Hidden ? Icon_ChevronLeft : Icon_ChevronRight, Hidden ? UiButton_Primary : UiButton_Normal,
                  Hidden ? "Show the islands panel" : "Hide the islands panel to see more of the map"))
  {
   App->Panel.Hidden = !Hidden;
  }
 }
 
 //- Time controls: in the middle of the row, or centred on a second row
 f32 RowY = Y;
 f32 SpaceMin = LeftEnd + 16.f;
 f32 SpaceMax = RightStart - 16.f;
 if(!OneRow)
 {
  RowY = Y + ButtonHeight + Ui_Gap;
  SpaceMin = Bar.Min.X + Ui_Pad;
  SpaceMax = Bar.Max.X - Ui_Pad;
 }
 f32 TimelineWidth = Clamp(60.f, (SpaceMax - SpaceMin) - Fixed, MaxTimeline);
 f32 Total = Fixed + TimelineWidth;
 f32 Start = .5f*(Bar.Min.X + Bar.Max.X) - .5f*Total;
 Start = Min(Start, SpaceMax - Total);
 Start = Max(Start, SpaceMin);
 
 // Thin dividers between the groups.
 if(OneRow)
 {
  DrawRect(RectFromSize(V2(Start - 9.f, Y + 4.f), V2(1.f, ButtonHeight - 8.f)), V4FromU32(Ui_ColorLine), 0.f, 0.f, 0.f);
  DrawRect(RectFromSize(V2(RightStart - 9.f, Y + 4.f), V2(1.f, ButtonHeight - 8.f)), V4FromU32(Ui_ColorLine), 0.f, 0.f, 0.f);
 }
 else
 {
  DrawRect(RectFromSize(V2(Bar.Min.X + Ui_Pad, RowY - .5f*Ui_Gap), V2(Bar.Max.X - Bar.Min.X - 2.f*Ui_Pad, 1.f)), V4FromU32(Ui_ColorLine), 0.f, 0.f, 0.f);
 }
 
 Cursor = V2(Start, RowY);
 DrawTimeControls(App, Input, &Cursor, TimelineWidth);
}

//~ Products short worldwide (under the top bar)
// NOTE: Right edge the floating cards may use: the left edge of the islands panel when it is
// shown (last frame's rect), else the window edge. Keeps the strip and dropdowns off the panel.
internal f32
V3UsableRight(app_state *App, f32 WindowRight)
{
 f32 Result = WindowRight;
 v4 Panel = App->Panel.LastRect;
 if(Panel.Max.X > Panel.Min.X && Panel.Min.X > 0.f)
 {
  Result = Min(Result, Panel.Min.X - Ui_Gap);
 }
 return Result;
}

// NOTE: A product is short when islands went without it in the last rounds (W3IsShort).
// Chips wrap to more rows instead of running under the islands panel.
#define V3D_MaxShortChips (ProductKind_Count)
internal void
V3DrawShortageStrip(app_state *App, app_input *Input)
{
 r3_state *R3 = &App->R3;
 f32 ButtonHeight = UiButtonHeight(App);
 f32 RowGap = .5f*Ui_Gap;
 v2 Origin = V2(R3->TopBarRect.Min.X, R3->TopBarRect.Max.Y + Ui_Gap);
 f32 Right = V3UsableRight(App, R3->TopBarRect.Max.X);
 
 // A warning icon in front of the foods islands went without (no label text).
 f32 LabelWidth = UiLineHeight(App);
 
 //- Lay the chips out first (rows), so the card can be drawn under them.
 u32 Kinds[V3D_MaxShortChips];
 str8 Texts[V3D_MaxShortChips];
 v2 Offsets[V3D_MaxShortChips];   // from the card's top-left corner
 u32 ChipCount = 0;
 f32 ChipsLeft = Ui_Pad + LabelWidth + Ui_Gap;
 f32 X = ChipsLeft;
 f32 Y = 6.f;
 f32 MaxRight = X;
 f32 MaxWidth = Max(Right - Origin.X, ChipsLeft + 2.f*Ui_Pad);
 for(u32 Kind = 1; Kind < ProductKind_Count && ChipCount < V3D_MaxShortChips; Kind += 1)
 {
  if(W3IsShort(R3, Kind))
  {
   str8 Text = Str8Fmt("-%lld", W3ShortBy(R3, Kind));
   f32 ChipWidth = V3ChipWidth(App, Text);
   if(X > ChipsLeft && X + ChipWidth + Ui_Pad > MaxWidth)
   {
    X = ChipsLeft;
    Y += ButtonHeight + RowGap;
   }
   Kinds[ChipCount] = Kind;
   Texts[ChipCount] = Text;
   Offsets[ChipCount] = V2(X, Y);
   ChipCount += 1;
   X += ChipWidth + Ui_Gap;
   MaxRight = Max(MaxRight, X - Ui_Gap);
  }
 }
 
 if(ChipCount == 0)
 {
  // Every island has enough food: no strip at all.
  R3->ShortStripRect = V4(0.f, 0.f, 0.f, 0.f);
  return;
 }
 
 v4 Box = RectFromSize(Origin, V2(MaxRight + Ui_Pad, Y + ButtonHeight + 6.f));
 UiCard(Box, Ui_CardRadius);
 V3AddUIRect(R3, Box);
 UiIconAt(App, V2(Box.Min.X + Ui_Pad, Box.Min.Y + 6.f + Ui_ButtonPadY), Icon_Warning, Ui_ColorShortText);
 
 for(u32 Idx = 0; Idx < ChipCount; Idx += 1)
 {
  u32 Kind = Kinds[Idx];
  s32 Which = R3_SummaryProductBase + (s32)Kind;
  v2 Cursor = V2(Box.Min.X + Offsets[Idx].X, Box.Min.Y + Offsets[Idx].Y);
  v2 ChipPos = Cursor;
  if(V3Chip(App, Input, &Cursor, Texts[Idx], W3ProductColors[Kind], Kind, Ui_ColorShortText, (R3->SummaryOpen == Which),
            "Islands went without this lately (units missing). Click to see who needs and makes it"))
  {
   V3ToggleSummary(R3, Which, V2(ChipPos.X, Box.Max.Y + Ui_Gap));
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
  DrawRect(Row, V4FromU32(Ui_ColorRowHover), 0.f, 0.f, 0.f);
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
 UiProductIconAt(App, V2(X, Line.Y), Kind, W3ProductColors[Kind]);
}

//~ Cluster totals (for the Clusters tab of the panel)

typedef struct v3d_cluster_info v3d_cluster_info;
struct v3d_cluster_info
{
 u32 Counts[IslandStatus_Count];
 u64 Produced[ProductKind_Count];
 u64 Needed[ProductKind_Count];
};

internal v3d_cluster_info
V3ClusterInfo(app_state *App, u32 Cluster)
{
 r3_state *R3 = &App->R3;
 v3d_cluster_info Result = {0};
 u32 Index = 0;
 for(island *Island = App->FirstIsland; Island && Index < R3->IslandCount; Island = Island->Next)
 {
  if(WorldGenClusterOf(Index, R3->IslandCount) == Cluster)
  {
   if(R3->IslandStatus[Index] < IslandStatus_Count) Result.Counts[R3->IslandStatus[Index]] += 1;
   for(product *Product = Island->FirstProduct; Product; Product = Product->Next) Result.Produced[Product->Kind] += Product->Amount;
   for(product *Req = Island->FirstRequirement; Req; Req = Req->Next) Result.Needed[Req->Kind] += Req->Amount;
  }
  Index += 1;
 }
 return Result;
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
    !IsInsideRectV2(MousePos, R3->ShortStripRect) && !IsInsideRectV2(MousePos, R3->ClustersButtonRect))
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
 if(IsProduct)
 {
  // The product's icon, then made per day (+) and needed per day (-).
  Title = Str8Fmt("+%llu  -%llu", R3->TotalProduced[Kind], R3->TotalNeeded[Kind]);
 }
 else
 {
  // A status list: the island rows with their dots say it all.
  Title = S8("");
 }

 u32 SectionRows = 0;
 v2 Min = R3->PopupRect.Min;
 f32 TitleIcon = IsProduct ? LineHeight + 4.f : 0.f;
 f32 Width = Max(V3D_PopupWidth*UiScale(App), TitleIcon + UiMeasure(App, Title) + 2.f*Pad);
 Min.X = Max(V3D_Margin, Min(Min.X, V3UsableRight(App, BufferDim.X - V3D_Margin) - Width));   // stay off the panel
 f32 MaxHeight = BufferDim.Y - Min.Y - V3D_Margin;
 u32 MaxRows = (u32)Max(1.f, (MaxHeight - 2.f*Pad)/LineHeight - 3.f - (f32)SectionRows);
 u32 ShownRows = Min(RowCount, MaxRows);
 u32 TitleRows = IsProduct ? 1u : 0u;
 f32 Height = (f32)(ShownRows + TitleRows + SectionRows + (RowCount > ShownRows ? 1 : 0))*LineHeight + 2.f*Pad + 4.f;
 if(RowCount == 0) Height += LineHeight;
 v4 Box = RectFromSize(Min, V2(Width, Height));
 R3->PopupRect = Box;
 UiCard(Box, Ui_CardRadius);

 v2 Line = V2(Box.Min.X + Pad, Box.Min.Y + Pad);
 f32 Inner = Width - 2.f*Pad;
 if(IsProduct)
 {
  UiProductIconAt(App, Line, Kind, W3ProductColors[Kind]);
  UiText(App, V2(Line.X + TitleIcon, Line.Y), Title, Ui_ColorTitle);
  Line.Y += LineHeight + 4.f;
 }

 if(RowCount == 0)
 {
  UiText(App, Line, S8("-"), Ui_ColorTextDim);
 }

 for(u32 Row = 0; Row < ShownRows; Row += 1)
 {
  // Needers come first, then makers; the amounts' signs (-/+) tell them apart.

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
     : (W3NeedStatus(App, IslandIdx, Req->Kind) == IslandStatus_Short);
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
 "Red: went without much of some food in the last 5 rounds",
};
global_variable char *V3HelpMapNotes[] =
{
 "Solid crates on a green pad: what an island makes",
 "Crate frames on an orange pad: what it needs",
 "Train on a rail: news travelling",
};
global_variable u32 V3HelpMapColors[] = {W3_ColorProducePad, W3_ColorNeedPad, W3_ColorTrainGlow};
StaticAssert(ArrayCount(V3HelpMapNotes) == ArrayCount(V3HelpMapColors), V3HelpMapNotesMatch);

global_variable char *V3HelpKeys[] =
{
 "Click",        "Right drag",   "Middle drag", "Mouse wheel",
 "Up / Down",    "Right",        "Left",        "Ctrl + / Ctrl -",
 "F11 / Alt+Enter", "Right click",  "Esc",
};
global_variable char *V3HelpActions[] =
{
 "open an island", "move the map", "turn the map", "zoom",
 "faster / slower",             "next round",   "restart",      "text size",
 "fullscreen",     "back to the whole world", "unselect everything",
};
StaticAssert(ArrayCount(V3HelpKeys) == ArrayCount(V3HelpActions), V3HelpKeysMatch);

// NOTE: What an island that makes each product looks like (its middle terrace).
global_variable char *V3HelpProductLooks[ProductKind_Count] =
{
 "", "apple orchard", "pear orchard", "cabbage patch", "carrot rows",
 "wheat field, windmill", "fish pond, dock", "cows, barn", "hens, hen house",
 "berry bushes", "coconut palms", "crab pots", "potato field",
};

#define V3D_HelpWidth   760.f
#define V3D_HelpColumns 2

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

 // NOTE: Closed at the start; the "?" button in the top bar opens it.
 if(!R3->HelpOpen) return;

 f32 LineHeight = UiLineHeight(App);
 f32 Pad = 20.f;
 f32 Swatch = LineHeight - 8.f;
 u32 ProductCount = ProductKind_Count - 1;
 u32 ProductRows = (ProductCount + V3D_HelpColumns - 1)/V3D_HelpColumns;
 u32 KeyRows = (u32)ArrayCount(V3HelpKeys);

 f32 Width = Min(V3D_HelpWidth*UiScale(App), BufferDim.X - 2.f*Ui_Margin);
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
 UiCard(Box, Ui_CardRadius);
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
  DrawRect(Dot, V4FromU32(V3HelpMapColors[Idx]), 0.f, 0.f, 0.f);
  DrawRect(Dot, V4FromU32(Ui_ColorCardBorder), 0.f, 1.f, 0.f);
  UiText(App, V2(X + Swatch + 8.f, Y), S8FromCString(V3HelpMapNotes[Idx]), Ui_ColorText);
  Y += LineHeight;
 }
 Y += 8.f;

 //- Products
 V3HelpSection(App, X, &Y, Inner, "Products: icon and crate colour, and what an island that makes it looks like");
 {
  f32 ColumnWidth = Inner/(f32)V3D_HelpColumns;
  for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
  {
   u32 Item = Kind - 1;
   v2 Pos = V2(X + (f32)(Item/ProductRows)*ColumnWidth, Y + (f32)(Item % ProductRows)*LineHeight);
   UiProductIconAt(App, Pos, Kind, W3ProductColors[Kind]);
   UiText(App, V2(Pos.X + LineHeight + 4.f, Pos.Y), ProductNames[Kind], Ui_ColorText);
   UiText(App, V2(Pos.X + LineHeight + 4.f + UiMeasure(App, ProductNames[Kind]) + 10.f, Pos.Y),
          S8FromCString(V3HelpProductLooks[Kind]), Ui_ColorTextDim);
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
   DrawRect(RectFromSize(V2(Pos.X - 4.f, Pos.Y + 1.f), V2(UiMeasure(App, Key) + 8.f, LineHeight - 2.f)), V4FromU32(Ui_ColorButton), 0.f, 0.f, 0.f);
   UiText(App, Pos, Key, Ui_ColorText);
   UiText(App, V2(Pos.X + KeyWidth, Pos.Y), S8FromCString(V3HelpActions[Idx]), Ui_ColorTextDim);
  }
  Y += (f32)Rows*LineHeight + 8.f;
 }

 //- Close button (bottom right). Drawn by hand: the other buttons are blocked while help is open.
 {
  f32 ButtonWidth = 2.f*UiButtonHeight(App);
  v4 Button = RectFromSize(V2(Box.Max.X - Pad - ButtonWidth, Box.Max.Y - Pad - UiButtonHeight(App)), V2(ButtonWidth, UiButtonHeight(App)));
  b32 Hot = IsInsideRectV2(MousePos, Button);
  UiButtonFace(Button, Hot ? Ui_ColorAccentHot : Ui_ColorAccent);
  UiIcon(App, Button, Icon_Check, Ui_ColorOnAccent);
  if(Hot && Clicked)
  {
   R3->HelpOpen = false;
  }
 }
}

//~ Clock
// NOTE: Bottom-left card with the day and time of day of the current round (L14), the
// time the sky shows. The dot is the sun by day and the moon by night.
#define V3D_ColorClockSun  0xFFF2B33D
#define V3D_ColorClockMoon 0xFF8FA3D9

internal void
V3DrawClock(app_state *App, v2 BufferDim)
{
 r3_state *R3 = &App->R3;
 u32 Hour = 0;
 f32 TimeOfDay = 0.f;
 u64 Day = BroadcastDayOfRound(App->Broadcast.Round, 0.f, &Hour, &TimeOfDay);
 b32 Daytime = (Hour >= 6 && Hour < 18);
 str8 Text = Str8Fmt("Day %llu, %02u:00", Day, Hour);

 f32 Height = UiButtonHeight(App);
 f32 Dot = UiLineHeight(App) - 8.f;
 f32 Width = 2.f*V3D_ChipPadding + Dot + 6.f + UiMeasure(App, Text);
 v4 Box = RectFromSize(V2(V3D_Margin, BufferDim.Y - V3D_Margin - Height), V2(Width, Height));
 UiCard(Box, Ui_CardRadius);
 V3AddUIRect(R3, Box);
 v2 Cursor = V2(Box.Min.X + V3D_ChipPadding, Box.Min.Y);
 UiDot(V2(Cursor.X, Cursor.Y + .5f*(Height - Dot)), Dot, Daytime ? V3D_ColorClockSun : V3D_ColorClockMoon);
 UiText(App, V2(Cursor.X + Dot + 6.f, Cursor.Y + Ui_ButtonPadY), Text, Ui_ColorText);
}

//~ Entry point
// NOTE: Call after RenderClear() and before DrawIslandPanel, so the panel, the dropdown,
// the help card and the hover tooltip end up on top.
internal void
DrawWorld3D(app_state *App, app_input *Input, v2 BufferDim)
{
 r3_state *R3 = &App->R3;
 UiTextSizeKeys(App, Input);
 UiUpdateTextSize(App);   // before any text this frame

 // NOTE: Over the panel the wheel scrolls the panel instead of zooming.
 b32 MouseOverUI = (IsInsideRectV2(V2V2S32(Input->Mouse.Pos), App->Panel.LastRect) || R3->HelpOpen);
 R3CameraUpdate(&R3->Camera, Input, MouseOverUI);

 //- Escape: unselect everything (island, open lists, help, focused cluster) and fly back to
 // the whole world.
 for(app_key_idx Idx = Input->Text.First; Idx; Idx = Input->Text.Buffer[Idx].Next)
 {
  app_key *Key = Input->Text.Buffer + Idx;
  if(Key->IsSymbol && Key->Codepoint == PlatformKey_Escape)
  {
   App->Panel.SelectedPlusOne = 0;
   App->Panel.Hidden = true;
   R3->SummaryOpen = 0;
   R3->HelpOpen = false;
   if(R3->FocusedClusterPlusOne != 0) V3FocusCluster(App, 0);
  }
 }

 //- Right click (without dragging) on the map: hide the focused cluster's island names and
 // fly back to the whole world, like at the start. A right drag still moves the map.
 {
  app_button_state Right = Input->Mouse.Buttons[PlatformMouseButton_Right];
  v2 MousePos = V2V2S32(Input->Mouse.Pos);
  if(WasPressed(Right))
  {
   R3->RightClickStart = MousePos;
   R3->RightClickArmed = (!MouseOverUI && !W3PointOnUI(R3, MousePos));
  }
  if(R3->RightClickArmed && !Right.EndedDown)
  {
   R3->RightClickArmed = false;
   v2 Moved = V2SubV2(MousePos, R3->RightClickStart);
   if(Moved.X*Moved.X + Moved.Y*Moved.Y < V3D_RightClickSlop*V3D_RightClickSlop && R3->FocusedClusterPlusOne != 0)
   {
    V3FocusCluster(App, 0);
   }
  }
 }
 
 // Centre the map in the space the top bar and the panel leave free (last frame's sizes).
 {
  f32 Right = App->Panel.Hidden ? BufferDim.X : App->Panel.LastRect.Min.X;
  if(Right <= 0.f) Right = BufferDim.X;
  f32 Top = R3->TopBarRect.Max.Y;
  v2 Target = V2(-(BufferDim.X - Right)/Max(BufferDim.X, 1.f), -Top/Max(BufferDim.Y, 1.f));
  f32 t = Min(1.f, 8.f*Input->dtForFrame);
  R3->LensShift.X += (Target.X - R3->LensShift.X)*t;
  R3->LensShift.Y += (Target.Y - R3->LensShift.Y)*t;
 }
 // Sky follows the clock, moving smoothly through each round (and stopping when paused).
 {
  broadcast_state *Broadcast = &App->Broadcast;
  u32 Hour = 0;
  f32 TimeOfDay = 0.f;
  BroadcastDayOfRound(Broadcast->Round, Broadcast->Timer/Broadcast_RoundSeconds, &Hour, &TimeOfDay);
  R3UpdateDayPhase(R3, TimeOfDay, Input->dtForFrame);
 }
 Prof_TimeScope("R3BeginFrame") { R3BeginFrame(R3, FrameArena, BufferDim); }
 Prof_TimeScope("W3ComputeStatus") { W3ComputeStatus(App); }
 Prof_TimeScope("W3PrepareIslands") { W3PrepareIslands(App, Input); }   // uses last frame's UI areas for hovering
 Prof_TimeScope("W3BuildWorld") { W3BuildWorld(App); }
 Prof_TimeScope("R3EndFrame") { R3EndFrame(R3, &App->Render); }


 //- UI, recording the areas it covers
 R3->UIRectCount = 0;
 R3->ShortStripRect = V4(0.f, 0.f, 0.f, 0.f);
 V3AddUIRect(R3, App->Panel.LastRect);
 V3AddUIRect(R3, R3->PopupRect);   // summary dropdown, drawn later (see DrawSummaryPopup)
 V3DrawTopBar(App, Input, BufferDim);
 V3DrawShortageStrip(App, Input);
 V3DrawClock(App, BufferDim);

 V3DrawCargoSigns(App);
 V3DrawClusterLabels(App, Input);
 V3DrawIslandLabels(App);
}
