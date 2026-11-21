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
#define V3D_ColorChipHot     0xE64C566A
#define V3D_ColorChipOpen    0xE65E81AC

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
 font_atlas *Atlas = &App->FontAtlas;
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
  f32 Height = LineHeight + BroadcastView_ButtonPadding + 4.f;
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
  u32 Background = Focused ? V3D_ColorChipOpen : (Hot ? V3D_ColorChipHot : V3D_ColorChip);
  DrawRect(Box, V4FromU32(Background), .5f*Height, 0.f, 0.f);
  DrawRect(Box, V4FromU32(Focused ? W3_ColorSelected : V3D_ColorLabelBorder), .5f*Height, Focused ? 2.f : 1.f, 0.f);

  f32 X = Box.Min.X + V3D_ChipPadding;
  f32 TextY = Box.Min.Y + .5f*(Height - LineHeight);
  GraphDrawText(Atlas, Font, V2(X, TextY), Name, V4FromU32(Graph_ColorTitle));
  X += V3Measure(App, Name) + 10.f;
  for(u32 Status = 0; Status < IslandStatus_Count; Status += 1)
  {
   if(Counts[Status])
   {
    V3StatusDot(V2(X, Box.Min.Y + .5f*(Height - Dot)), Dot, W3StatusColor(Status));
    X += Dot + 4.f;
    str8 Number = Str8Fmt("%u", Counts[Status]);
    GraphDrawText(Atlas, Font, V2(X, TextY), Number, V4FromU32(Focused ? Graph_ColorTitle : W3StatusColor(Status)));
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
// NOTE: Each chip is a button: it opens a dropdown listing the islands behind the number.

internal void PanelSelect(app_state *App, s32 IslandIdx);

#define V3D_PopupWidth     360.f
#define V3D_ColorPopupBg   0xF02E3440
#define V3D_ColorRowHover  0xFF4C566A
#define V3D_ColorSection   0xFFEBCB8B
#define V3D_ColorDim       0xFF9AA3B5
#define V3D_ColorGood      0xFFA3BE8C

// NOTE: A chip: rounded background, optional coloured dot (or product swatch), text.
// Clickable chips highlight on hover and return true when clicked.
internal b32
V3Chip(app_state *App, app_input *Input, v2 *Cursor, str8 Text, u32 DotColor, b32 HasDot, b32 Square, u32 TextColor, b32 Clickable, b32 Open)
{
 font *Font = &App->Font;
 f32 Height = Font->HeightPx + BroadcastView_ButtonPadding;
 f32 Dot = Font->HeightPx - 8.f;
 f32 DotSpace = HasDot ? Dot + 6.f : 0.f;
 f32 Width = 2.f*V3D_ChipPadding + DotSpace + V3Measure(App, Text);
 v4 Box = RectFromSize(*Cursor, V2(Width, Height));

 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 b32 Hot = Clickable && IsInsideRectV2(MousePos, Box);
 u32 Background = Open ? V3D_ColorChipOpen : (Hot ? V3D_ColorChipHot : V3D_ColorChip);

 DrawRect(Box, V4FromU32(Background), .5f*Height, 0.f, 0.f);
 if(HasDot)
 {
  v2 DotPos = V2(Box.Min.X + V3D_ChipPadding, Box.Min.Y + .5f*(Height - Dot));
  DrawRect(RectFromSize(DotPos, V2(Dot, Dot)), V4FromU32(DotColor), Square ? 3.f : .5f*Dot, 0.f, 0.f);
 }
 // White text on the open (blue) chip, so coloured text stays readable.
 GraphDrawText(&App->FontAtlas, Font, V2(Box.Min.X + V3D_ChipPadding + DotSpace, Box.Min.Y + .5f*BroadcastView_ButtonPadding),
               Text, V4FromU32(Open ? Graph_ColorTitle : TextColor));
 Cursor->X += Width + BroadcastView_ButtonGap;

 b32 Result = (Hot && WasPressed(Input->Mouse.Buttons[PlatformMouseButton_Left]));
 return Result;
}

// NOTE: Clicking a chip opens its list; clicking the open chip closes it.
internal void
V3ToggleSummary(r3_state *R3, s32 Which, v2 ChipBottomLeft)
{
 if(R3->SummaryOpen == Which)
 {
  R3->SummaryOpen = 0;
 }
 else
 {
  R3->SummaryOpen = Which;
  R3->PopupRect = RectFromSize(ChipBottomLeft, V2(0.f, 0.f)); // real size set when drawn
 }
}

internal void
V3DrawSummaryBar(app_state *App, app_input *Input)
{
 r3_state *R3 = &App->R3;
 broadcast_state *Broadcast = &App->Broadcast;
 f32 Height = App->Font.HeightPx + BroadcastView_ButtonPadding;
 v2 Cursor = V2(V3D_Margin, V3D_Margin);

 //- Time (not a button)
 V3Chip(App, Input, &Cursor, Str8Fmt("Round %llu   %s", Broadcast->Round, BroadcastSpeedLabels[BroadcastSpeedIdx(Broadcast)]),
        0, false, false, Graph_ColorTitle, false, false);
 Cursor.X += 10.f;

 //- Islands per status
 u32 Counts[IslandStatus_Count] = {0};
 for(u32 Idx = 0; Idx < R3->IslandCount; Idx += 1)
 {
  if(R3->IslandStatus[Idx] < IslandStatus_Count) Counts[R3->IslandStatus[Idx]] += 1;
 }

 // NOTE: Plain C strings: MSVC warns (C4204) about non-constant aggregate initializers.
 char *StatusFormats[IslandStatus_Count] = {"%u ok", "%u waiting for news", "%u short"};
 u32 Order[IslandStatus_Count] = {IslandStatus_Ok, IslandStatus_Waiting, IslandStatus_Short};
 for(u32 OrderIdx = 0; OrderIdx < IslandStatus_Count; OrderIdx += 1)
 {
  u32 Status = Order[OrderIdx];
  s32 Which = 1 + (s32)Status;
  v2 ChipPos = Cursor;
  if(V3Chip(App, Input, &Cursor, Str8Fmt(StatusFormats[Status], Counts[Status]), W3StatusColor(Status), true, false,
            Graph_ColorTitle, true, (R3->SummaryOpen == Which)))
  {
   V3ToggleSummary(R3, Which, V2(ChipPos.X, ChipPos.Y + Height + BroadcastView_ButtonGap));
  }
 }
 Cursor.X += 10.f;

 //- Products that are short worldwide
 b32 AnyShort = false;
 for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
 {
  if(R3->TotalNeeded[Kind] > R3->TotalProduced[Kind])
  {
   s64 Missing = (s64)R3->TotalNeeded[Kind] - (s64)R3->TotalProduced[Kind];
   str8 Name = ProductNames[Kind];
   s32 Which = R3_SummaryProductBase + (s32)Kind;
   v2 ChipPos = Cursor;
   if(V3Chip(App, Input, &Cursor, Str8Fmt("%.*s short by %lld", (s32)Name.Size, Name.Data, Missing),
             W3ProductColors[Kind], true, true, W3_ColorStatusShort, true, (R3->SummaryOpen == Which)))
   {
    V3ToggleSummary(R3, Which, V2(ChipPos.X, ChipPos.Y + Height + BroadcastView_ButtonGap));
   }
   AnyShort = true;
  }
 }
 if(!AnyShort)
 {
  V3Chip(App, Input, &Cursor, S8("no product shortages"), 0, false, false, W3_ColorStatusOk, false, false);
 }

 R3->SummaryBarRect = RectFromSize(V2(0.f, 0.f), V2(Cursor.X, V3D_Margin + Height));
 V3AddUIRect(R3, R3->SummaryBarRect);
}

//~ Summary dropdown
// NOTE: A row in the dropdown: island name with its status dot; returns true when clicked.
internal b32
V3PopupIslandRow(app_state *App, app_input *Input, v2 *Line, f32 Width, u32 IslandIdx, island *Island)
{
 r3_state *R3 = &App->R3;
 f32 LineHeight = App->Font.HeightPx;
 f32 Dot = LineHeight - 10.f;
 v4 Row = RectFromSize(V2(Line->X - 4.f, Line->Y), V2(Width + 8.f, LineHeight));
 b32 Hot = IsInsideRectV2(V2V2S32(Input->Mouse.Pos), Row);
 if(Hot)
 {
  DrawRect(Row, V4FromU32(V3D_ColorRowHover), 3.f, 0.f, 0.f);
  R3->PopupHoverPlusOne = (s32)IslandIdx + 1;
 }
 V3StatusDot(V2(Line->X, Line->Y + .5f*(LineHeight - Dot)), Dot, W3StatusColor(R3->IslandStatus[IslandIdx]));
 GraphDrawText(&App->FontAtlas, &App->Font, V2(Line->X + Dot + 6.f, Line->Y), Island->Name, V4FromU32(Graph_ColorTitle));
 b32 Result = (Hot && WasPressed(Input->Mouse.Buttons[PlatformMouseButton_Left]));
 return Result;
}

internal void
V3PopupSwatch(app_state *App, v2 Line, f32 X, product_kind Kind)
{
 f32 Size = App->Font.HeightPx - 6.f;
 DrawRect(RectFromSize(V2(X, Line.Y + 3.f), V2(Size, Size)), V4FromU32(W3ProductColors[Kind]), 3.f, 0.f, 0.f);
}

// NOTE: Draws the open dropdown (call last, so it is on top of the other controls).
// Clicking a row opens that island in the panel; clicking outside closes the dropdown.
internal void
DrawSummaryPopup(app_state *App, app_input *Input, v2 BufferDim)
{
 r3_state *R3 = &App->R3;
 font_atlas *Atlas = &App->FontAtlas;
 font *Font = &App->Font;
 f32 LineHeight = Font->HeightPx;
 f32 Pad = BroadcastView_ButtonPadding;
 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 b32 Clicked = WasPressed(Input->Mouse.Buttons[PlatformMouseButton_Left]);

 if(R3->SummaryOpen == 0)
 {
  R3->PopupRect = V4(0.f, 0.f, 0.f, 0.f);
  return;
 }

 //- Close when clicking anywhere else (the chips toggle themselves)
 if(Clicked && !IsInsideRectV2(MousePos, R3->PopupRect) && !IsInsideRectV2(MousePos, R3->SummaryBarRect))
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
 str8 Hint = S8("click an island to open it");
 if(IsProduct)
 {
  str8 Name = ProductNames[Kind];
  Title = Str8Fmt("%.*s: %llu needed, %llu produced", (s32)Name.Size, Name.Data, R3->TotalNeeded[Kind], R3->TotalProduced[Kind]);
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
 f32 Width = Max(V3D_PopupWidth, V3Measure(App, Title) + 2.f*Pad);
 f32 MaxHeight = BufferDim.Y - Min.Y - V3D_Margin;
 u32 MaxRows = (u32)Max(1.f, (MaxHeight - 2.f*Pad)/LineHeight - 3.f - (f32)SectionRows);
 u32 ShownRows = Min(RowCount, MaxRows);
 f32 Height = (f32)(ShownRows + 2 + SectionRows + (RowCount > ShownRows ? 1 : 0))*LineHeight + 2.f*Pad;
 if(RowCount == 0) Height += LineHeight;
 v4 Box = RectFromSize(Min, V2(Width, Height));
 R3->PopupRect = Box;

 DrawRect(Box, V4FromU32(V3D_ColorPopupBg), 8.f, 0.f, 0.f);
 DrawRect(Box, V4FromU32(Graph_ColorNodeBorder), 8.f, 1.f, 0.f);

 v2 Line = V2(Box.Min.X + Pad, Box.Min.Y + Pad);
 f32 Inner = Width - 2.f*Pad;
 GraphDrawText(Atlas, Font, Line, Title, V4FromU32(V3D_ColorSection));
 Line.Y += LineHeight;
 GraphDrawText(Atlas, Font, Line, Hint, V4FromU32(V3D_ColorDim));
 Line.Y += LineHeight;

 if(RowCount == 0)
 {
  GraphDrawText(Atlas, Font, Line, S8("none"), V4FromU32(V3D_ColorDim));
 }

 for(u32 Row = 0; Row < ShownRows; Row += 1)
 {
  if(IsProduct && (Row == 0 || (Row == NeedCount)))
  {
   str8 Section = (Row == 0 && NeedCount > 0) ? S8("Need it:") : S8("Produce it:");
   GraphDrawText(Atlas, Font, Line, Section, V4FromU32(V3D_ColorDim));
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
   GraphDrawText(Atlas, Font, V2(Right - V3Measure(App, Amount), Line.Y), Amount,
                 V4FromU32(Produces[Row] ? V3D_ColorGood : W3_ColorNeedPad));
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
  GraphDrawText(Atlas, Font, Line, Str8Fmt("+%u more (see the Islands tab)", RowCount - ShownRows), V4FromU32(V3D_ColorDim));
 }
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
 if(BroadcastButton(App, Input, &Cursor, Labels[2]))
 {
  R3CameraReset(Camera);
  App->R3.FocusedClusterPlusOne = 0;
 }

 // While a cluster is open: a button to go back to all clusters, under the others.
 if(App->R3.FocusedClusterPlusOne)
 {
  str8 Back = S8("All clusters");
  f32 BackWidth = V3Measure(App, Back) + 2.f*BroadcastView_ButtonPadding;
  v2 BackCursor = V2(BufferDim.X - V3D_Margin - BackWidth - BroadcastView_ButtonGap, V3D_Margin + Height + BroadcastView_ButtonGap);
  V3AddUIRect(&App->R3, RectFromSize(BackCursor, V2(BackWidth, Height)));
  if(BroadcastButton(App, Input, &BackCursor, Back))
  {
   V3FocusCluster(App, 0);
  }
 }
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
 V3AddUIRect(R3, R3->PopupRect);   // summary dropdown, drawn last (see DrawSummaryPopup)
 V3DrawSummaryBar(App, Input);
 V3DrawCameraControls(App, Input, BufferDim);
 V3DrawHelpButtons(App, Input, BufferDim);

 V3DrawClusterLabels(App, Input);
 V3DrawIslandLabels(App);
}
