//-
// SolarPunk islands panel (right side), with three tabs (shortages are in the top bar):
//   Islands   : one table row per island (needs, knows N/M); click a row to open it
//   Selected  : everything about one island (router on/off, production, needs with the
//               nearest known supplier, food, neighbours); goods moving are the trains on the map
//   Trend     : the world balance of a food over the last rounds: storms make it dip,
//               producers react and it climbs back to a small surplus
// Select an island by clicking it in 3D or a row in a table.
//-

//~ Constants
#define Panel_Width        560.f    // at the base text size; scales with it
#define Panel_MaxScreenPart .46f    // never wider than this part of the window
#define Panel_Margin       Ui_Margin
#define Panel_Pad          12.f
#define Panel_ScrollStep   160.f
#define Panel_WheelStep    60.f

#define Panel_ColorRowAlt    Ui_ColorRowAlt
#define Panel_ColorRowHover  Ui_ColorRowHover
#define Panel_ColorLine      Ui_ColorLine
#define Panel_ColorSection   Ui_ColorTitle
#define Panel_ColorText      Ui_ColorText
#define Panel_ColorDim       Ui_ColorTextDim
#define Panel_ColorGood      Ui_ColorOkText
#define Panel_ColorShortage  Ui_ColorShortText
#define Panel_ColorSelected  Ui_ColorSelected
#define Panel_DoubleClickSeconds .4f   // two clicks on one island within this = double click
#define Panel_TintAlpha      0x2Cu    // status tint over a row

//~ Helpers
internal island *
PanelIslandAt(app_state *App, u32 Target)
{
 island *Result = 0;
 u32 Index = 0;
 for(island *Island = App->FirstIsland; Island && !Result; Island = Island->Next)
 {
  if(Index == Target)
  {
   Result = Island;
  }
  Index += 1;
 }
 return Result;
}

// NOTE: Only selects: the panel stays on its tab (the Selected tab is opened by hand).
internal void
PanelSelect(app_state *App, s32 IslandIdx)
{
 App->Panel.SelectedPlusOne = IslandIdx + 1;
 if(App->Panel.Tab == PanelTab_Selected) App->Panel.Scroll = 0.f;
}

internal f32
PanelMeasure(app_state *App, str8 Text)
{
 f32 Result = GraphMeasureText(&App->FontAtlas, &App->Font, Text);
 return Result;
}

// NOTE: Draws rows top to bottom; Y is the top of the current line.
typedef struct panel_cursor panel_cursor;
struct panel_cursor
{
 app_state *App;
 app_input *Input;
 v4 Content;      // visible content area (clipped)
 f32 X;
 f32 Y;
 f32 Width;
 f32 LineHeight;
 u32 RowIndex;    // for alternating row colours
};

internal void
PanelText(panel_cursor *Cursor, f32 Column, str8 Text, u32 Color)
{
 GraphDrawText(&Cursor->App->FontAtlas, &Cursor->App->Font, V2(Cursor->X + Column, Cursor->Y), Text, V4FromU32(Color));
}

internal void
PanelTextRight(panel_cursor *Cursor, f32 RightEdge, str8 Text, u32 Color)
{
 PanelText(Cursor, RightEdge - PanelMeasure(Cursor->App, Text), Text, Color);
}

internal void
PanelSwatch(panel_cursor *Cursor, f32 Column, product_kind Kind)
{
 // The product's icon (solarpunk_icons.ttf) in its colour.
 UiProductIconAt(Cursor->App, V2(Cursor->X + Column, Cursor->Y), Kind, W3ProductColors[Kind]);
}

// NOTE: Status colours (green ok, orange waiting, red short): a dot, and a light tint
// over a whole row.
internal void
PanelStatusDot(panel_cursor *Cursor, f32 Column, u32 Status)
{
 f32 Size = Cursor->LineHeight - 10.f;
 DrawRect(RectFromSize(V2(Cursor->X + Column, Cursor->Y + .5f*(Cursor->LineHeight - Size)), V2(Size, Size)),
          V4FromU32(W3StatusColor(Status)), .5f*Size, 0.f, .5f);
}

// NOTE: Room for a status dot before a name.
internal f32
PanelDotSpace(panel_cursor *Cursor)
{
 f32 Result = Cursor->LineHeight - 2.f;
 return Result;
}

internal void
PanelTintRow(panel_cursor *Cursor, u32 Status)
{
 u32 Color = (Panel_TintAlpha << 24) | (W3StatusColor(Status) & 0x00FFFFFFu);
 v4 Row = RectFromSize(V2(Cursor->X - 4.f, Cursor->Y), V2(Cursor->Width + 8.f, Cursor->LineHeight));
 DrawRect(Row, V4FromU32(Color), 0.f, 0.f, 0.f);
}

#define Panel_GridColumns 3   // produces / needs cells per row

// NOTE: Status tint over one cell of a grid row (see PanelTintRow).
internal void
PanelTintCell(panel_cursor *Cursor, f32 Column, f32 Width, u32 Status)
{
 u32 Color = (Panel_TintAlpha << 24) | (W3StatusColor(Status) & 0x00FFFFFFu);
 v4 Cell = RectFromSize(V2(Cursor->X + Column, Cursor->Y), V2(Width - 4.f, Cursor->LineHeight));
 DrawRect(Cell, V4FromU32(Color), 0.f, 0.f, 0.f);
}

internal void
PanelNewLine(panel_cursor *Cursor)
{
 Cursor->Y += Cursor->LineHeight;
}

// NOTE: A thin divider between sections. Resets the alternating rows.
internal void
PanelSection(panel_cursor *Cursor, str8 Title)
{
 // NOTE: Only a divider: the panel shows data, no titles (Title is kept for reading the code).
 Cursor->Y += .5f*Cursor->LineHeight;
 DrawRect(RectFromSize(V2(Cursor->X, Cursor->Y), V2(Cursor->Width, 1.f)), V4FromU32(Panel_ColorLine), 0.f, 0.f, 0.f);
 Cursor->Y += .5f*Cursor->LineHeight;
 Cursor->RowIndex = 0;
}

// NOTE: A table title (with the divider above it, except for the first table).
internal void
PanelTitle(panel_cursor *Cursor, str8 Title, b32 First)
{
 if(First) Cursor->Y += 4.f;
 else      PanelSection(Cursor, Title);
 PanelText(Cursor, 0.f, Title, Panel_ColorSection);
 PanelNewLine(Cursor);
 Cursor->RowIndex = 0;
}

internal void
PanelNote(panel_cursor *Cursor, str8 Text)
{
 PanelText(Cursor, 0.f, Text, Panel_ColorDim);
 PanelNewLine(Cursor);
}

// NOTE: Column titles of a table.
typedef struct panel_column panel_column;
struct panel_column
{
 char *Title;
 f32 X;          // left edge, or right edge when RightAligned
 b32 RightAligned;
};

// NOTE: Built with a function instead of an initializer list: MSVC warns (C4204)
// about aggregate initializers that are not constants.
internal panel_column
PanelColumn(char *Title, f32 X, b32 RightAligned)
{
 panel_column Result = {0};
 Result.Title = Title;
 Result.X = X;
 Result.RightAligned = RightAligned;
 return Result;
}

internal void
PanelTableHeader(panel_cursor *Cursor, panel_column *Columns, u32 ColumnCount)
{
 for(u32 Idx = 0; Idx < ColumnCount; Idx += 1)
 {
  str8 Title = S8FromCString(Columns[Idx].Title);
  if(Columns[Idx].RightAligned) PanelTextRight(Cursor, Columns[Idx].X, Title, Panel_ColorDim);
  else                          PanelText(Cursor, Columns[Idx].X, Title, Panel_ColorDim);
 }
 PanelNewLine(Cursor);
 Cursor->RowIndex = 0;
}

// NOTE: Starts a table row: alternating background, hover highlight. Returns true
// when the row is clicked. Pass Clickable = false for rows that do nothing.
internal b32
PanelBeginRow(panel_cursor *Cursor, b32 Clickable, b32 *Hovered)
{
 v4 Row = RectFromSize(V2(Cursor->X - 4.f, Cursor->Y), V2(Cursor->Width + 8.f, Cursor->LineHeight));
 v2 MousePos = V2V2S32(Cursor->Input->Mouse.Pos);
 *Hovered = (Clickable && IsInsideRectV2(MousePos, Row) && IsInsideRectV2(MousePos, Cursor->Content) &&
             !UiMouseBlocked(Cursor->App, MousePos));

 if(*Hovered)
 {
  DrawRect(Row, V4FromU32(Panel_ColorRowHover), 0.f, 0.f, 0.f);
 }
 else if(Cursor->RowIndex & 1)
 {
  DrawRect(Row, V4FromU32(Panel_ColorRowAlt), 0.f, 0.f, 0.f);
 }
 Cursor->RowIndex += 1;

 b32 Result = (*Hovered && WasPressed(Cursor->Input->Mouse.Buttons[PlatformMouseButton_Left]));
 return Result;
}

// NOTE: A clickable row that opens an island.
internal void
PanelIslandRow(panel_cursor *Cursor, u32 IslandIdx)
{
 b32 Hovered = false;
 if(PanelBeginRow(Cursor, true, &Hovered))
 {
  PanelSelect(Cursor->App, (s32)IslandIdx);
 }
 if(Hovered)
 {
  Cursor->App->Panel.RowHoverPlusOne = (s32)IslandIdx + 1;
 }
}

// NOTE: Index of the closest island this island has heard of that produces Kind.
// Returns false when it knows of none yet.
internal b32
PanelNearestSupplier(app_state *App, u32 IslandIdx, product_kind Kind, u32 *SupplierIdx, u32 *SupplierCount)
{
 broadcast_state *Broadcast = &App->Broadcast;
 b32 Result = false;
 u32 Count = 0;
 u32 BestHops = 0;
 if(IslandIdx < Broadcast->IslandCount)
 {
  for(u32 Origin = 0; Origin < Broadcast->IslandCount; Origin += 1)
  {
   island_ad *Ad = &Broadcast->Known[IslandIdx][Origin];
   if(Origin != IslandIdx && Ad->Valid && Ad->Produces[Kind] > 0)
   {
    if(!Result || Ad->HopCount < BestHops)
    {
     BestHops = Ad->HopCount;
     *SupplierIdx = Origin;
    }
    Result = true;
    Count += 1;
   }
  }
 }
 *SupplierCount = Count;
 return Result;
}

//~ Tab: Islands
internal void
PanelDrawIslands(panel_cursor *Cursor)
{
 app_state *App = Cursor->App;
 f32 W = Cursor->Width;
 f32 KnowsWidth = PanelMeasure(App, S8("48/48")) + 10.f;

 Cursor->RowIndex = 0;

 u32 Index = 0;
 for(island *Island = App->FirstIsland; Island; Island = Island->Next)
 {
  PanelIslandRow(Cursor, Index);
  u32 Status = App->R3.IslandStatus[Index];
  PanelTintRow(Cursor, Status);
  PanelStatusDot(Cursor, 0.f, Status);
  b32 Selected = (App->Panel.SelectedPlusOne == (s32)Index + 1);
  PanelText(Cursor, PanelDotSpace(Cursor), Island->Name, Selected ? Panel_ColorSelected : Panel_ColorText);

  f32 Column = .42f*W;
  for(product *Req = Island->FirstRequirement; Req; Req = Req->Next)
  {
   str8 Amount = Str8Fmt("%llu", Req->Amount);
   f32 ItemWidth = Cursor->LineHeight + PanelMeasure(App, Amount) + 6.f;
   if(Column + ItemWidth > W - KnowsWidth) break;   // no room left before "Knows"
   PanelSwatch(Cursor, Column, Req->Kind);
   PanelText(Cursor, Column + Cursor->LineHeight - 2.f, Amount, UiStatusText(W3NeedStatus(App, Index, Req->Kind)));
   Column += Cursor->LineHeight + PanelMeasure(App, Amount) + 6.f;
  }

  PanelTextRight(Cursor, W, Str8Fmt("%u/%u", BroadcastKnownCount(App, Island), App->Broadcast.IslandCount), Panel_ColorDim);
  PanelNewLine(Cursor);
  Index += 1;
 }
}

//~ Tab: Selected island
internal void
PanelDrawSelected(panel_cursor *Cursor)
{
 app_state *App = Cursor->App;
 f32 W = Cursor->Width;

 if(App->Panel.SelectedPlusOne == 0)
 {
  u32 Index = 0;
  for(island *Island = App->FirstIsland; Island; Island = Island->Next, Index += 1)
  {
   PanelIslandRow(Cursor, Index);
   u32 Status = App->R3.IslandStatus[Index];
   PanelStatusDot(Cursor, 0.f, Status);
   PanelText(Cursor, PanelDotSpace(Cursor), Island->Name, Panel_ColorText);
   PanelNewLine(Cursor);
  }
  return;
 }

 u32 SelectedIdx = (u32)(App->Panel.SelectedPlusOne - 1);
 island *Island = PanelIslandAt(App, SelectedIdx);
 if(!Island)
 {
  return;
 }

 //- Router (link-state routing, solarpunk_routing.c)
 PanelTitle(Cursor, S8("Router"), true);
 {
  route_state *Route = &App->Route;
  b32 Down = RouteIsDown(App, SelectedIdx);
  f32 ButtonSize = UiButtonHeight(App);
  v2 ButtonPos = V2(Cursor->X, Cursor->Y + 2.f);
  v2 MousePos = V2V2S32(Cursor->Input->Mouse.Pos);
  // Power button: pressed in (toggled) while the island is shut down.
  b32 Clicked = UiIconButton(App, Cursor->Input, &ButtonPos, Icon_Power, Down ? UiButton_Toggled : UiButton_Normal,
                             Down ? "Start the island again: it says hello and relearns the map"
                                  : "Shut the island down: it stops sending hellos and forwarding goods");
  // The button can be scrolled under the tabs: only clicks inside the content count.
  if(Clicked && IsInsideRectV2(MousePos, Cursor->Content))
  {
   RouteSetDown(App, SelectedIdx, !Down);
   Down = !Down;
  }

  u32 ButtonCount = 1;

  // State next to the buttons: map size (LSAs it holds) and islands it can reach.
  f32 TextY = Cursor->Y + 2.f + Ui_ButtonPadY;
  f32 X = (f32)ButtonCount*(ButtonSize + Ui_Gap);
  if(Down)
  {
   UiText(App, V2(Cursor->X + X, TextY), S8("DOWN"), Panel_ColorShortage);
  }
  else
  {
   UiText(App, V2(Cursor->X + X, TextY), S8("map"), Panel_ColorDim);
   X += PanelMeasure(App, S8("map ")) ;
   str8 Map = Str8Fmt("%u/%u", RouteMapSize(App, SelectedIdx), Route->IslandCount);
   UiText(App, V2(Cursor->X + X, TextY), Map, Panel_ColorText);
   X += PanelMeasure(App, Map) + 16.f;
   UiText(App, V2(Cursor->X + X, TextY), S8("reachable"), Panel_ColorDim);
   X += PanelMeasure(App, S8("reachable "));
   UiText(App, V2(Cursor->X + X, TextY), Str8Fmt("%u", RouteReachableCount(App, SelectedIdx)), Panel_ColorText);
  }
  Cursor->Y += ButtonSize + 6.f;


  // Goods on their way to and from this island are the coloured trains on the map.
 }

 //- Produces and needs: three per row, icon and amount
 PanelTitle(Cursor, S8("Produces (per day)"), false);
 {
  f32 CellWidth = W/(f32)Panel_GridColumns;
  u32 Cell = 0;
  for(product *Product = Island->FirstProduct; Product; Product = Product->Next)
  {
   f32 CellX = (f32)(Cell % Panel_GridColumns)*CellWidth;
   PanelTintCell(Cursor, CellX, CellWidth, IslandStatus_Ok);
   PanelSwatch(Cursor, CellX + 2.f, Product->Kind);
   PanelTextRight(Cursor, CellX + CellWidth - 8.f, Str8Fmt("+%llu", Product->Amount), Panel_ColorGood);
   Cell += 1;
   if(Cell % Panel_GridColumns == 0 || !Product->Next) PanelNewLine(Cursor);
  }
 }

 //- Needs: coloured by how well they are met; a click opens the nearest supplier
 PanelTitle(Cursor, S8("Needs (per day)"), false);
 {
  f32 CellWidth = W/(f32)Panel_GridColumns;
  u32 Cell = 0;
  v2 MousePos = V2V2S32(Cursor->Input->Mouse.Pos);
  for(product *Req = Island->FirstRequirement; Req; Req = Req->Next)
  {
   f32 CellX = (f32)(Cell % Panel_GridColumns)*CellWidth;
   u32 SupplierIdx = 0;
   u32 SupplierCount = 0;
   b32 Known = PanelNearestSupplier(App, SelectedIdx, Req->Kind, &SupplierIdx, &SupplierCount);
   u32 NeedStatus = W3NeedStatus(App, SelectedIdx, Req->Kind);
   PanelTintCell(Cursor, CellX, CellWidth, NeedStatus);
   if(Known)
   {
    v4 Box = RectFromSize(V2(Cursor->X + CellX, Cursor->Y), V2(CellWidth - 4.f, Cursor->LineHeight));
    if(IsInsideRectV2(MousePos, Box) && IsInsideRectV2(MousePos, Cursor->Content) && !UiMouseBlocked(App, MousePos))
    {
     DrawRect(Box, V4FromU32(Panel_ColorRowHover), 0.f, 0.f, 0.f);
     App->Panel.RowHoverPlusOne = (s32)SupplierIdx + 1;
     if(WasPressed(Cursor->Input->Mouse.Buttons[PlatformMouseButton_Left])) PanelSelect(App, (s32)SupplierIdx);
    }
   }
   PanelSwatch(Cursor, CellX + 2.f, Req->Kind);
   PanelTextRight(Cursor, CellX + CellWidth - 8.f, Str8Fmt("-%llu", Req->Amount), UiStatusText(NeedStatus));
   Cell += 1;
   if(Cell % Panel_GridColumns == 0 || !Req->Next) PanelNewLine(Cursor);
  }
 }

 //- Food in store, one row per food: stock | going bad next | in rounds | thrown away | missing
 PanelTitle(Cursor, S8("Food in store"), false);
 {
  u32 SelectedFoodIdx = SelectedIdx;
  u64 Round = App->Broadcast.Round;
  f32 ColStock = .30f*W, ColBad = .47f*W, ColIn = .62f*W, ColWaste = .82f*W, ColMissing = W;
  b32 HeaderDone = false;
  for(u32 Kind = 1; Kind < ProductKind_Count && SelectedFoodIdx < Food_MaxIslands; Kind += 1)
  {
   if(!FoodIsPerishable((product_kind)Kind)) continue;
   food_stock *Stock = &App->Food.Stock[SelectedFoodIdx][Kind];
   food_counts *Counts = &App->Food.Counts[SelectedFoodIdx][Kind];
   u32 Qty = FoodStockQty(&App->Food, SelectedFoodIdx, (product_kind)Kind);
   u32 Missing = FoodRecentShortage(&App->Food, SelectedFoodIdx, (product_kind)Kind);
   if(Qty == 0 && Counts->Wasted == 0 && Missing == 0) continue;

   if(!HeaderDone)
   {
    panel_column Columns[6];
    Columns[0] = PanelColumn("food", 0.f, false);
    Columns[1] = PanelColumn("stock", ColStock, true);
    Columns[2] = PanelColumn("expiring", ColBad, true);
    Columns[3] = PanelColumn("rounds", ColIn, true);
    Columns[4] = PanelColumn("wasted", ColWaste, true);
    Columns[5] = PanelColumn("missing", ColMissing, true);
    PanelTableHeader(Cursor, Columns, ArrayCount(Columns));
    HeaderDone = true;
   }

   b32 Hovered = false;
   PanelBeginRow(Cursor, false, &Hovered);
   PanelSwatch(Cursor, 0.f, (product_kind)Kind);
   PanelTextRight(Cursor, ColStock, Str8Fmt("%u", Qty), Panel_ColorText);
   if(Stock->LotCount > 0)
   {
    food_lot *Next = &Stock->Lots[0];
    u64 Left = (Next->ExpiresRound > Round) ? Next->ExpiresRound - Round : 0;
    u32 BadColor = (Next->ExpiresRound <= Round + Food_SoonRounds) ? Panel_ColorShortage : Panel_ColorText;
    PanelTextRight(Cursor, ColBad, Str8Fmt("%u", Next->Qty), BadColor);
    PanelTextRight(Cursor, ColIn, Str8Fmt("%llu", Left), BadColor);
   }
   else
   {
    PanelTextRight(Cursor, ColBad, S8("-"), Panel_ColorDim);
    PanelTextRight(Cursor, ColIn, S8("-"), Panel_ColorDim);
   }
   PanelTextRight(Cursor, ColWaste, (Counts->Wasted > 0) ? Str8Fmt("%llu", Counts->Wasted) : S8("-"),
                  (Counts->Wasted > 0) ? Panel_ColorShortage : Panel_ColorDim);
   PanelTextRight(Cursor, ColMissing, (Missing > 0) ? Str8Fmt("%u", Missing) : S8("-"),
                  (Missing > 0) ? Panel_ColorShortage : Panel_ColorDim);
   PanelNewLine(Cursor);
  }

  //- Legend: clock, then every food's icon with the rounds it keeps
  {
   UiIconAt(App, V2(Cursor->X, Cursor->Y), Icon_Clock, Panel_ColorDim);
   f32 X = Cursor->LineHeight + 6.f;
   for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
   {
    if(!FoodIsPerishable((product_kind)Kind)) continue;
    str8 Rounds = Str8Fmt("%llu", FoodShelfLifeRounds((product_kind)Kind));
    f32 ItemWidth = Cursor->LineHeight + 2.f + PanelMeasure(App, Rounds) + 10.f;
    if(X > 0.f && X + ItemWidth > W)
    {
     X = 0.f;
     PanelNewLine(Cursor);
    }
    PanelSwatch(Cursor, X, (product_kind)Kind);
    PanelText(Cursor, X + Cursor->LineHeight + 2.f, Rounds, Panel_ColorDim);
    X += ItemWidth;
   }
   PanelNewLine(Cursor);
  }

 }
 
 //- Network
 PanelTitle(Cursor, S8("Neighbours"), false);
 {
  {
   panel_column Columns[3];
   Columns[0] = PanelColumn("island", 0.f, false);
   Columns[1] = PanelColumn("link", .58f*W, false);
   Columns[2] = PanelColumn("cost", W, true);
   PanelTableHeader(Cursor, Columns, ArrayCount(Columns));
  }
  b32 SelfDown = RouteIsDown(App, SelectedIdx);
  for(link *Link = Island->FirstLink; Link; Link = Link->Next)
  {
   u32 NeighbourIdx = BroadcastIslandIndex(App, Link->Dest);
   PanelIslandRow(Cursor, NeighbourIdx);
   u32 NeighbourStatus = (NeighbourIdx < R3_MaxIslands) ? App->R3.IslandStatus[NeighbourIdx] : IslandStatus_Ok;
   PanelTintRow(Cursor, NeighbourStatus);
   PanelStatusDot(Cursor, 0.f, NeighbourStatus);
   PanelText(Cursor, PanelDotSpace(Cursor), Link->Dest->Name, Panel_ColorText);

   // What this island's router thinks of the neighbour (solarpunk_routing.c).
   u64 Silent = 0;
   route_neighbour_state State = RouteNeighbourState(App, SelectedIdx, NeighbourIdx, &Silent);
   // check = up, clock + n = up but quiet n rounds, chevron = one-way, x + n = down (silent n rounds)
   u32 StateIcon = 0;
   str8 StateText = S8("");
   u32 StateColor = Panel_ColorDim;
   if(SelfDown)                              { StateText = S8("-"); }
   else if(State == RouteNeighbour_Up)       { StateIcon = (Silent > 0) ? Icon_Clock : Icon_Check;
                                               StateText = (Silent > 0) ? Str8Fmt("%llu", Silent) : S8("");
                                               StateColor = (Silent > 0) ? Ui_ColorWaitingText : Panel_ColorGood; }
   else if(State == RouteNeighbour_OneWay)   { StateIcon = Icon_ChevronRight; StateColor = Ui_ColorWaitingText; }
   else if(State == RouteNeighbour_Dead)     { StateIcon = Icon_Close; StateText = Str8Fmt("%llu", Silent); StateColor = Panel_ColorShortage; }
   else                                      { StateText = S8("-"); }
   f32 StateX = .58f*W;
   if(StateIcon)
   {
    UiIconAt(App, V2(Cursor->X + StateX, Cursor->Y), StateIcon, StateColor);
    StateX += Cursor->LineHeight + 2.f;
   }
   PanelText(Cursor, StateX, StateText, StateColor);
   PanelTextRight(Cursor, W, Str8Fmt("%d", Link->Cost), Ui_ColorTitle);
   PanelNewLine(Cursor);
  }

 }

}

//~ Tab: Clusters
// NOTE: Two tables. Clusters: name | ok | waiting | short | down; a click flies to the
// cluster and shows its island names (again: back to the whole world). Foods: made |
// needed | balance per round, for the open cluster, or the whole world when none is open.
internal void
PanelDrawClusters(panel_cursor *Cursor)
{
 app_state *App = Cursor->App;
 r3_state *R3 = &App->R3;
 f32 W = Cursor->Width;
 u32 ClusterCount = V3ClusterCount(App);
 s32 FocusedPlusOne = R3->FocusedClusterPlusOne;

 //- Clusters
 // The last column is the cluster's power button (shut it all down / start it all again).
 f32 Power = Cursor->LineHeight;
 f32 ColOk = .45f*W, ColWaiting = .58f*W, ColShort = .71f*W, ColDown = W - Power - 16.f;
 f32 Dot = Cursor->LineHeight - 10.f;
 f32 DotY = .5f*(Cursor->LineHeight - Dot);
 UiDot(V2(Cursor->X + ColOk - Dot, Cursor->Y + DotY), Dot, W3StatusColor(IslandStatus_Ok));
 UiDot(V2(Cursor->X + ColWaiting - Dot, Cursor->Y + DotY), Dot, W3StatusColor(IslandStatus_Waiting));
 UiDot(V2(Cursor->X + ColShort - Dot, Cursor->Y + DotY), Dot, W3StatusColor(IslandStatus_Short));
 UiIconAt(App, V2(Cursor->X + ColDown - Cursor->LineHeight, Cursor->Y), Icon_Power, Panel_ColorDim);
 PanelNewLine(Cursor);
 Cursor->RowIndex = 0;

 u32 IslandCount = R3->IslandCount;
 for(u32 Cluster = 0; Cluster < ClusterCount; Cluster += 1)
 {
  v3d_cluster_info Info = V3ClusterInfo(App, Cluster);
  u32 Down = 0;
  for(u32 Idx = 0; Idx < IslandCount; Idx += 1)
  {
   if(WorldGenClusterOf(Idx, IslandCount) == Cluster && RouteIsDown(App, Idx)) Down += 1;
  }
  b32 Focused = (FocusedPlusOne == (s32)Cluster + 1);
  u32 ClusterSize = 0;
  for(u32 Idx = 0; Idx < IslandCount; Idx += 1)
  {
   if(WorldGenClusterOf(Idx, IslandCount) == Cluster) ClusterSize += 1;
  }
  b32 AllDown = (ClusterSize > 0 && Down == ClusterSize);

  // Power button: a click on it toggles the cluster, a click anywhere else on the row flies to it.
  v4 PowerBox = RectFromSize(V2(Cursor->X + W - Power, Cursor->Y), V2(Power, Cursor->LineHeight));
  v2 MousePos = V2V2S32(Cursor->Input->Mouse.Pos);
  b32 OverPower = (IsInsideRectV2(MousePos, PowerBox) && IsInsideRectV2(MousePos, Cursor->Content) && !UiMouseBlocked(App, MousePos));
  b32 Hovered = false;
  if(PanelBeginRow(Cursor, !OverPower, &Hovered))
  {
   V3FocusCluster(App, (s32)Cluster + 1);
  }
  if(Focused && !Hovered) DrawRect(RectFromSize(V2(Cursor->X - 4.f, Cursor->Y), V2(Cursor->Width + 8.f, Cursor->LineHeight)),
                                   V4FromU32(Ui_ColorAccent), 0.f, 0.f, 0.f);
  PanelText(Cursor, 0.f, S8FromCString(WorldGenClusterNames[Cluster]), Focused ? Panel_ColorSelected : Panel_ColorText);
  f32 Cols[IslandStatus_Count];   // filled one by one: MSVC warns (C4204) about non-constant initializers
  Cols[IslandStatus_Ok] = ColOk;
  Cols[IslandStatus_Waiting] = ColWaiting;
  Cols[IslandStatus_Short] = ColShort;
  for(u32 Status = 0; Status < IslandStatus_Count; Status += 1)
  {
   u32 N = Info.Counts[Status];
   PanelTextRight(Cursor, Cols[Status], (N > 0) ? Str8Fmt("%u", N) : S8("-"), (N > 0) ? UiStatusText(Status) : Panel_ColorDim);
  }
  PanelTextRight(Cursor, ColDown, (Down > 0) ? Str8Fmt("%u", Down) : S8("-"), (Down > 0) ? Panel_ColorShortage : Panel_ColorDim);

  if(OverPower) DrawRect(PowerBox, V4FromU32(Panel_ColorRowHover), 0.f, 0.f, 0.f);
  if(AllDown)   DrawRect(PowerBox, V4FromU32((Panel_TintAlpha << 24) | (W3StatusColor(IslandStatus_Short) & 0x00FFFFFFu)), 0.f, 0.f, 0.f);
  UiIconAt(App, PowerBox.Min, Icon_Power, AllDown ? Panel_ColorShortage : (OverPower ? Panel_ColorText : Panel_ColorDim));
  if(OverPower && WasPressed(Cursor->Input->Mouse.Buttons[PlatformMouseButton_Left]))
  {
   for(u32 Idx = 0; Idx < IslandCount; Idx += 1)
   {
    if(WorldGenClusterOf(Idx, IslandCount) == Cluster) RouteSetDown(App, Idx, !AllDown);
   }
  }
  PanelNewLine(Cursor);
 }

 //- Foods of the open cluster (or the whole world)
 PanelSection(Cursor, S8("Foods"));
 u64 Made[ProductKind_Count] = {0};
 u64 Needed[ProductKind_Count] = {0};
 for(u32 Cluster = 0; Cluster < ClusterCount; Cluster += 1)
 {
  if(FocusedPlusOne != 0 && FocusedPlusOne != (s32)Cluster + 1) continue;
  v3d_cluster_info Info = V3ClusterInfo(App, Cluster);
  for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
  {
   Made[Kind] += Info.Produced[Kind];
   Needed[Kind] += Info.Needed[Kind];
  }
 }
 f32 ColMade = .45f*W, ColNeeded = .70f*W, ColBalance = W;
 PanelText(Cursor, 0.f, (FocusedPlusOne > 0 && FocusedPlusOne <= (s32)ClusterCount) ?
           S8FromCString(WorldGenClusterNames[FocusedPlusOne - 1]) : S8(""), Panel_ColorSelected);
 PanelTextRight(Cursor, ColMade, S8("+"), Panel_ColorDim);
 PanelTextRight(Cursor, ColNeeded, S8("-"), Panel_ColorDim);
 PanelTextRight(Cursor, ColBalance, S8("="), Panel_ColorDim);
 PanelNewLine(Cursor);
 Cursor->RowIndex = 0;
 for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
 {
  if(Made[Kind] == 0 && Needed[Kind] == 0) continue;
  b32 Hovered = false;
  PanelBeginRow(Cursor, false, &Hovered);
  s64 Balance = (s64)Made[Kind] - (s64)Needed[Kind];
  PanelTintRow(Cursor, (Balance < 0) ? IslandStatus_Short : IslandStatus_Ok);
  PanelSwatch(Cursor, 0.f, (product_kind)Kind);
  PanelTextRight(Cursor, ColMade, Str8Fmt("%llu", Made[Kind]), Panel_ColorText);
  PanelTextRight(Cursor, ColNeeded, Str8Fmt("%llu", Needed[Kind]), Panel_ColorText);
  PanelTextRight(Cursor, ColBalance, Str8Fmt("%+lld", Balance), (Balance < 0) ? Panel_ColorShortage : Panel_ColorGood);
  PanelNewLine(Cursor);
 }
}

//~ Tabs
//~ Tab: Trend
// NOTE: The world balance of one food (or all foods) over the last rounds: made / needed - 1.
// Storms push it below zero; producers react and it climbs back to a small surplus.
#define Panel_TrendRounds   30
#define Panel_TrendTop      .25f    // chart range, as a balance fraction
#define Panel_TrendBottom   -.5f

internal void
PanelDrawTrend(panel_cursor *Cursor)
{
 app_state *App = Cursor->App;
 broadcast_state *Broadcast = &App->Broadcast;
 island_panel_state *Panel = &App->Panel;
 f32 W = Cursor->Width;
 f32 LineHeight = Cursor->LineHeight;
 product_kind Kind = (product_kind)((Panel->TrendKind < ProductKind_Count) ? Panel->TrendKind : 0);
 u64 Round = Broadcast->Round;
 
 //- Title
 if(Kind == ProductKind_None) UiIconAt(App, V2(Cursor->X, Cursor->Y), Icon_Grid, Panel_ColorText);
 else                         PanelSwatch(Cursor, 0.f, Kind);
 PanelNewLine(Cursor);
 
 //- Chart
 f32 LabelWidth = PanelMeasure(App, S8("-40%")) + 8.f;
 f32 ChartHeight = 8.f*LineHeight;
 v4 Chart = RectFromSize(V2(Cursor->X + LabelWidth, Cursor->Y + .5f*LineHeight), V2(W - LabelWidth, ChartHeight));
 f32 Range = Panel_TrendTop - Panel_TrendBottom;
 f32 ZeroY = Chart.Min.Y + (Panel_TrendTop/Range)*ChartHeight;
 DrawRect(Chart, V4FromU32(Ui_ColorRowAlt), 0.f, 0.f, 0.f);
 
 // Grid lines with labels: +20%, 0, -20%, -40%.
 for(s32 Percent = 20; Percent >= -40; Percent -= 20)
 {
  f32 Y = Chart.Min.Y + ((Panel_TrendTop - (f32)Percent/100.f)/Range)*ChartHeight;
  u32 Color = (Percent == 0) ? Ui_ColorText : Ui_ColorLine;
  DrawRect(RectFromSize(V2(Chart.Min.X, Y), V2(Chart.Max.X - Chart.Min.X, (Percent == 0) ? 2.f : 1.f)), V4FromU32(Color), 0.f, 0.f, 0.f);
  str8 Label = (Percent == 0) ? S8("0") : Str8Fmt("%+d%%", Percent);
  GraphDrawText(&App->FontAtlas, &App->Font, V2(Chart.Min.X - 6.f - PanelMeasure(App, Label), Y - .5f*LineHeight), Label, V4FromU32(Panel_ColorDim));
 }
 // The target: a small surplus, dashed.
 {
  f32 Y = Chart.Min.Y + ((Panel_TrendTop - Adapt_Target + 1.f)/Range)*ChartHeight;
  for(f32 X = Chart.Min.X; X < Chart.Max.X; X += 12.f)
  {
   DrawRect(RectFromSize(V2(X, Y - 1.f), V2(Min(6.f, Chart.Max.X - X), 2.f)), V4FromU32(Ui_ColorOkText), 0.f, 0.f, 0.f);
  }
 }
 
 if(Round == 0)
 {
  Cursor->Y = Chart.Max.Y + .5f*LineHeight;
  return;
 }
 
 u64 First = (Round > Panel_TrendRounds) ? Round - Panel_TrendRounds + 1 : 1;
 u32 Count = (u32)(Round - First + 1);
 f32 Step = (Chart.Max.X - Chart.Min.X)/(f32)Panel_TrendRounds;
 v2 Last = V2(0.f, 0.f);
 for(u32 Idx = 0; Idx < Count; Idx += 1)
 {
  u64 R = First + Idx;
  f32 Value = Clamp(Panel_TrendBottom, AdaptBalance(App, R, Kind), Panel_TrendTop);
  f32 X = Chart.Min.X + Step*((f32)Idx + .5f);
  f32 Y = Chart.Min.Y + ((Panel_TrendTop - Value)/Range)*ChartHeight;
  b32 Short = (Value < 0.f);
  
  // Area between the line and zero: red below, green above.
  f32 Top = Min(Y, ZeroY), Bottom = Max(Y, ZeroY);
  u32 Fill = Short ? 0x55E0606A : 0x447BD389;
  DrawRect(V4(X - .5f*Step, Top, X + .5f*Step, Max(Bottom, Top + 1.f)), V4FromU32(Fill), 0.f, 0.f, 0.f);
  
  // A storm starts here: a grey marker at the top.
  u32 StormCluster = 0;
  u64 StormStart = 0;
  if(WorldGenStormAt(R, &StormCluster, &StormStart) && StormStart == R)
  {
   DrawRect(RectFromSize(V2(X - 1.f, Chart.Min.Y), V2(2.f, ChartHeight)), V4FromU32(0x55808080), 0.f, 0.f, 0.f);
   DrawRect(RectFromSize(V2(X - 7.f, Chart.Min.Y + 3.f), V2(14.f, 8.f)), V4FromU32(0xFF8892A0), 0.f, 0.f, 0.f);
  }
  
  v2 P = V2(X, Y);
  if(Idx > 0) GraphDrawLine(Last, P, 3.f, V4FromU32(Ui_ColorText));
  DrawRect(RectFromSize(V2(X - 4.f, Y - 4.f), V2(8.f, 8.f)), V4FromU32(Short ? Ui_ColorShortText : Ui_ColorOkText), 4.f, 0.f, 0.f);
  Last = P;
 }
 Cursor->Y = Chart.Max.Y + 4.f;
 PanelText(Cursor, LabelWidth, Str8Fmt("%llu", First), Panel_ColorDim);
 PanelTextRight(Cursor, W, Str8Fmt("%llu", Round), Panel_ColorDim);
 PanelNewLine(Cursor);
 
 //- Now
 {
  f32 Now = AdaptBalance(App, Round, Kind);
  s32 Percent = (s32)(100.f*Now + ((Now < 0.f) ? -.5f : .5f));
  b32 Short = (Now < 0.f);
  PanelText(Cursor, 0.f, Str8Fmt("%+d%%", Percent), Short ? Panel_ColorShortage : Panel_ColorGood);
  PanelNewLine(Cursor);
  u32 StormCluster = 0;
  u64 StormStart = 0;
  if(WorldGenStormAt(Round, &StormCluster, &StormStart))
  {
   // A storm is raging: warning icon and the cluster it hits.
   UiIconAt(App, V2(Cursor->X, Cursor->Y), Icon_Warning, Panel_ColorShortage);
   PanelText(Cursor, LineHeight + 4.f, S8FromCString(WorldGenClusterNames[StormCluster]), Panel_ColorShortage);
   PanelNewLine(Cursor);
  }
 }
 
 //- Per food: balance now and how much more (or less) its makers grow. Click to chart it.
 PanelSection(Cursor, S8("Pick a food to chart"));
 for(u32 Row = 0; Row < ProductKind_Count; Row += 1)
 {
  product_kind RowKind = (product_kind)Row;
  b32 Hovered = false;
  b32 Selected = (RowKind == Kind);
  if(Selected)
  {
   DrawRect(RectFromSize(V2(Cursor->X - 4.f, Cursor->Y), V2(W + 8.f, LineHeight)), V4FromU32(Ui_ColorButtonHot), 0.f, 0.f, 0.f);
  }
  if(PanelBeginRow(Cursor, true, &Hovered))
  {
   Panel->TrendKind = Row;
  }
  f32 Now = AdaptBalance(App, Round, RowKind);
  s32 Percent = (s32)(100.f*Now + ((Now < 0.f) ? -.5f : .5f));
  if(RowKind == ProductKind_None)
  {
   UiIconAt(App, V2(Cursor->X, Cursor->Y), Icon_Grid, Panel_ColorText);   // all foods
  }
  else
  {
   PanelSwatch(Cursor, 0.f, RowKind);
   f32 Boost = AdaptBoost(App, RowKind);
   s32 Extra = (s32)(100.f*(Boost - 1.f) + ((Boost < 1.f) ? -.5f : .5f));
   PanelTextRight(Cursor, W, Str8Fmt("%+d%%", Extra), (Extra > 0) ? Ui_ColorWaitingText : Panel_ColorDim);
  }
  PanelTextRight(Cursor, .62f*W, Str8Fmt("%+d%%", Percent), (Now < 0.f) ? Panel_ColorShortage : Panel_ColorGood);
  PanelNewLine(Cursor);
 }
}

// NOTE: One segment of the tab bar, an icon with a hint; the active tab is filled. Returns true when clicked.
internal b32
PanelTabButton(app_state *App, app_input *Input, v2 *Cursor, u32 Icon, char *Hint, f32 Width, b32 Active)
{
 f32 Height = UiButtonHeight(App);
 v4 Dest = RectFromSize(*Cursor, V2(Width, Height));
 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 b32 Hot = IsInsideRectV2(MousePos, Dest) && !UiMouseBlocked(App, MousePos);

 // Small gap between neighbouring tabs so their borders do not touch.
 v4 Face = V4(Dest.Min.X + 1.5f, Dest.Min.Y, Dest.Max.X - 1.5f, Dest.Max.Y);
 UiButtonFace(Face, Active ? Ui_ColorAccent : (Hot ? Ui_ColorButtonHot : Ui_ColorButton));
 UiIcon(App, Face, Icon, Active ? Ui_ColorOnAccent : Ui_ColorText);
 if(Hot && Hint)
 {
  UiSetHint(Hint, V2(.5f*(Dest.Min.X + Dest.Max.X), Dest.Max.Y + 6.f));
 }

 Cursor->X += Width;
 b32 Result = (Hot && WasPressed(Input->Mouse.Buttons[PlatformMouseButton_Left]));
 return Result;
}

//~ Panel
internal void
DrawIslandPanel(app_state *App, app_input *Input, v2 BufferDim)
{
 island_panel_state *Panel = &App->Panel;
 r3_state *R3 = &App->R3;
 f32 LineHeight = UiLineHeight(App);
 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 app_button_state MouseLeft = Input->Mouse.Buttons[PlatformMouseButton_Left];
 f32 PanelTop = R3->TopBarRect.Max.Y + Ui_Gap;
 f32 PanelWidth = Min(Panel_Width*UiScale(App), Panel_MaxScreenPart*BufferDim.X);

 Panel->RowHoverPlusOne = 0;

 // NOTE: Ignore a selection that no longer exists (e.g. after switching worlds).
 if(Panel->SelectedPlusOne > (s32)R3->IslandCount)
 {
  Panel->SelectedPlusOne = 0;
 }
 if((u32)Panel->Tab >= PanelTab_Count)
 {
  Panel->Tab = PanelTab_Islands;
 }

 // NOTE: The button that hides and shows this panel is in the top bar (V3DrawTopBar).

 if(Panel->Hidden)
 {
  Panel->LastRect = V4(0.f, 0.f, 0.f, 0.f);
 }
 else
 {
  v4 Box = RectFromSize(V2(BufferDim.X - Panel_Margin - PanelWidth, PanelTop),
                        V2(PanelWidth, Max(BufferDim.Y - PanelTop - Panel_Margin, 6.f*LineHeight)));
  Panel->LastRect = Box;
  UiCard(Box, Ui_CardRadius);

  f32 ButtonHeight = UiButtonHeight(App);

  //- Header: the tabs
  v2 HeaderPos = V2(Box.Min.X + Panel_Pad, Box.Min.Y + Panel_Pad);

  //- Tabs
  {
   u32 TabIcons[PanelTab_Count] = {Icon_List, Icon_Target, Icon_Gauge, Icon_Grid};
   char *TabHints[PanelTab_Count] = {"All islands", "Selected island", "Trend: how a food's balance changes", "Clusters"};
   f32 TabsWidth = PanelWidth - 2.f*Panel_Pad;
   v4 Track = RectFromSize(HeaderPos, V2(TabsWidth, ButtonHeight + 6.f));
   UiField(Track);
   f32 TabWidth = (TabsWidth - 6.f)/(f32)PanelTab_Count;
   v2 TabCursor = V2(Track.Min.X + 3.f, Track.Min.Y + 3.f);
   for(u32 Tab = 0; Tab < PanelTab_Count; Tab += 1)
   {
    if(PanelTabButton(App, Input, &TabCursor, TabIcons[Tab], TabHints[Tab], TabWidth, (Panel->Tab == (panel_tab)Tab)))
    {
     Panel->Tab = (panel_tab)Tab;
     Panel->Scroll = 0.f;
    }
   }
  }

  //- Content (clipped and scrollable)
  f32 ContentTop = HeaderPos.Y + ButtonHeight + 6.f + .5f*Panel_Pad;
  f32 FooterHeight = ButtonHeight + 2.f*Panel_Pad;
  v4 Content = V4(Box.Min.X + Panel_Pad, ContentTop, Box.Max.X - Panel_Pad, Box.Max.Y - FooterHeight);

  panel_cursor Cursor = {0};
  Cursor.App = App;
  Cursor.Input = Input;
  Cursor.Content = Content;
  Cursor.X = Content.Min.X + 4.f;
  Cursor.Y = Content.Min.Y - Panel->Scroll;
  Cursor.Width = (Content.Max.X - Content.Min.X) - 8.f;
  Cursor.LineHeight = LineHeight;

  RenderPushClip(Content);
  switch(Panel->Tab)
  {
   case PanelTab_Islands:   PanelDrawIslands(&Cursor); break;
   case PanelTab_Selected:  PanelDrawSelected(&Cursor); break;
   case PanelTab_Trend:     PanelDrawTrend(&Cursor); break;
   case PanelTab_Clusters:  PanelDrawClusters(&Cursor); break;
   default: break;
  }
  RenderPopClip();

  //- Scrolling: mouse wheel over the panel, or the buttons when the content does not fit
  f32 ContentHeight = (Cursor.Y + Panel->Scroll) - Content.Min.Y;
  f32 VisibleHeight = Content.Max.Y - Content.Min.Y;
  f32 MaxScroll = Max(0.f, ContentHeight - VisibleHeight);
  if(IsInsideRectV2(MousePos, Box))
  {
   Panel->Scroll -= (f32)R3WheelSteps(Input)*Panel_WheelStep;
  }
  if(MaxScroll > 0.f)
  {
   v2 FooterCursor = V2(Box.Min.X + Panel_Pad, Box.Max.Y - Panel_Pad - ButtonHeight);
   if(UiIconButton(App, Input, &FooterCursor, Icon_ChevronUp, UiButton_Normal, "Scroll up"))     Panel->Scroll -= Panel_ScrollStep;
   if(UiIconButton(App, Input, &FooterCursor, Icon_ChevronDown, UiButton_Normal, "Scroll down")) Panel->Scroll += Panel_ScrollStep;
  }
  Panel->Scroll = Clamp(0.f, Panel->Scroll, MaxScroll);
 }

 //- Clicks in the 3D view (not on the UI): one click on an island selects it, a double
 // click also opens its Selected tab; a click on empty space closes the panel.
 b32 OverUI = (IsInsideRectV2(MousePos, Panel->LastRect) || W3PointOnUI(R3, MousePos));
 if(!OverUI && WasPressed(MouseLeft))
 {
  if(R3->HoveredIdx >= 0)
  {
   b32 Double = (Panel->LastClickPlusOne == R3->HoveredIdx + 1 &&
                 R3->AnimTime - Panel->LastClickTime < Panel_DoubleClickSeconds);
   PanelSelect(App, R3->HoveredIdx);
   if(Double)
   {
    Panel->Hidden = false;
    Panel->Tab = PanelTab_Selected;
    Panel->Scroll = 0.f;
    Panel->LastClickPlusOne = 0;
   }
   else
   {
    Panel->LastClickPlusOne = R3->HoveredIdx + 1;
    Panel->LastClickTime = R3->AnimTime;
   }
  }
  else
  {
   Panel->Hidden = true;
   Panel->LastClickPlusOne = 0;
  }
 }
}
