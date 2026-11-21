//-
// SolarPunk islands panel (right side), with three tabs:
//   Shortages : products that are short overall, and islands that do not yet know
//               a supplier for one of their needs
//   Islands   : one table row per island (needs, knows N/M); click a row to open it
//   Selected  : everything about one island (production, needs with the nearest
//               known supplier, network)
// Select an island by clicking it in 3D or a row in a table.
//-

//~ Constants
#define Panel_Width        440.f
#define Panel_Top          92.f
#define Panel_Margin       12.f
#define Panel_Pad          10.f
#define Panel_ScrollStep   160.f
#define Panel_WheelStep    60.f

#define Panel_ColorBg        0xF02E3440
#define Panel_ColorRowAlt    0xFF353C4A
#define Panel_ColorRowHover  0xFF4C566A
#define Panel_ColorTab       0xFF3B4252
#define Panel_ColorTabActive 0xFF5E81AC
#define Panel_ColorLine      0xFF4C566A
#define Panel_ColorSection   0xFFEBCB8B
#define Panel_ColorText      0xFFECEFF4
#define Panel_ColorDim       0xFF9AA3B5
#define Panel_ColorGood      0xFFA3BE8C
#define Panel_ColorShortage  0xFFE0707A
#define Panel_ColorSelected  0xFF88E0FF

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

internal void
PanelSelect(app_state *App, s32 IslandIdx)
{
 App->Panel.SelectedPlusOne = IslandIdx + 1;
 App->Panel.Tab = PanelTab_Selected;
 App->Panel.Scroll = 0.f;
 App->Panel.Hidden = false;
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
 f32 Size = Cursor->LineHeight - 6.f;
 DrawRect(RectFromSize(V2(Cursor->X + Column, Cursor->Y + 3.f), V2(Size, Size)),
          V4FromU32(W3ProductColors[Kind]), 3.f, 0.f, 0.f);
}

internal void
PanelNewLine(panel_cursor *Cursor)
{
 Cursor->Y += Cursor->LineHeight;
}

// NOTE: Section title with a thin line under it. Resets the alternating rows.
internal void
PanelSection(panel_cursor *Cursor, str8 Title)
{
 Cursor->Y += .5f*Cursor->LineHeight;
 PanelText(Cursor, 0.f, Title, Panel_ColorSection);
 PanelNewLine(Cursor);
 DrawRect(RectFromSize(V2(Cursor->X, Cursor->Y), V2(Cursor->Width, 1.f)), V4FromU32(Panel_ColorLine), 0.f, 0.f, 0.f);
 Cursor->Y += 3.f;
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
 *Hovered = (Clickable && IsInsideRectV2(MousePos, Row) && IsInsideRectV2(MousePos, Cursor->Content));

 if(*Hovered)
 {
  DrawRect(Row, V4FromU32(Panel_ColorRowHover), 3.f, 0.f, 0.f);
 }
 else if(Cursor->RowIndex & 1)
 {
  DrawRect(Row, V4FromU32(Panel_ColorRowAlt), 3.f, 0.f, 0.f);
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

//~ Tab: Shortages
internal void
PanelDrawShortages(panel_cursor *Cursor)
{
 app_state *App = Cursor->App;
 f32 W = Cursor->Width;

 //- Products, most short first
 u64 Produced[ProductKind_Count] = {0};
 u64 Needed[ProductKind_Count] = {0};
 for(island *Island = App->FirstIsland; Island; Island = Island->Next)
 {
  for(product *Product = Island->FirstProduct; Product; Product = Product->Next) Produced[Product->Kind] += Product->Amount;
  for(product *Req = Island->FirstRequirement; Req; Req = Req->Next) Needed[Req->Kind] += Req->Amount;
 }

 u32 Order[ProductKind_Count];
 s64 Balance[ProductKind_Count];
 u32 OrderCount = 0;
 for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
 {
  s64 Value = (s64)Produced[Kind] - (s64)Needed[Kind];
  u32 At = OrderCount;
  while(At > 0 && Balance[Order[At - 1]] > Value)
  {
   Order[At] = Order[At - 1];
   At -= 1;
  }
  Order[At] = Kind;
  Balance[Kind] = Value;
  OrderCount += 1;
 }

 PanelSection(Cursor, S8("Products, whole world"));
 panel_column Columns[4];
 Columns[0] = PanelColumn("Product", 0.f, false);
 Columns[1] = PanelColumn("Produced", .64f*W, true);
 Columns[2] = PanelColumn("Needed", .82f*W, true);
 Columns[3] = PanelColumn("Balance", W, true);
 PanelTableHeader(Cursor, Columns, (u32)ArrayCount(Columns));
 for(u32 Idx = 0; Idx < OrderCount; Idx += 1)
 {
  u32 Kind = Order[Idx];
  b32 Hovered = false;
  PanelBeginRow(Cursor, false, &Hovered);
  PanelSwatch(Cursor, 0.f, (product_kind)Kind);
  PanelText(Cursor, Cursor->LineHeight, ProductNames[Kind], Panel_ColorText);
  PanelTextRight(Cursor, .64f*W, Str8Fmt("%llu", Produced[Kind]), Panel_ColorText);
  PanelTextRight(Cursor, .82f*W, Str8Fmt("%llu", Needed[Kind]), Panel_ColorText);
  str8 BalanceText = (Balance[Kind] < 0) ? Str8Fmt("%lld", Balance[Kind]) : Str8Fmt("+%lld", Balance[Kind]);
  PanelTextRight(Cursor, W, BalanceText, (Balance[Kind] < 0) ? Panel_ColorShortage : Panel_ColorGood);
  PanelNewLine(Cursor);
 }

 //- Islands that do not know a supplier for some of their needs yet
 u32 IslandCount = 0;
 u32 WaitingCount = 0;
 {
  u32 Index = 0;
  for(island *Island = App->FirstIsland; Island; Island = Island->Next)
  {
   b32 Waiting = false;
   for(product *Req = Island->FirstRequirement; Req; Req = Req->Next)
   {
    u32 Supplier, Count;
    if(!PanelNearestSupplier(App, Index, Req->Kind, &Supplier, &Count)) Waiting = true;
   }
   if(Waiting) WaitingCount += 1;
   IslandCount += 1;
   Index += 1;
  }
 }

 PanelSection(Cursor, Str8Fmt("Waiting for a supplier: %u of %u islands", WaitingCount, IslandCount));
 if(WaitingCount == 0)
 {
  PanelNote(Cursor, S8("Every island knows where to get what it needs."));
 }
 else
 {
  panel_column WaitColumns[2];
  WaitColumns[0] = PanelColumn("Island", 0.f, false);
  WaitColumns[1] = PanelColumn("No supplier known for", .5f*W, false);
  PanelTableHeader(Cursor, WaitColumns, (u32)ArrayCount(WaitColumns));

  u32 Index = 0;
  for(island *Island = App->FirstIsland; Island; Island = Island->Next)
  {
   b32 Waiting = false;
   for(product *Req = Island->FirstRequirement; Req; Req = Req->Next)
   {
    u32 Supplier, Count;
    if(!PanelNearestSupplier(App, Index, Req->Kind, &Supplier, &Count)) Waiting = true;
   }

   if(Waiting)
   {
    PanelIslandRow(Cursor, Index);
    PanelText(Cursor, 0.f, Island->Name, Panel_ColorText);
    f32 Column = .5f*W;
    for(product *Req = Island->FirstRequirement; Req; Req = Req->Next)
    {
     u32 Supplier, Count;
     if(!PanelNearestSupplier(App, Index, Req->Kind, &Supplier, &Count))
     {
      PanelSwatch(Cursor, Column, Req->Kind);
      Column += Cursor->LineHeight;
     }
    }
    PanelNewLine(Cursor);
   }
   Index += 1;
  }
  PanelNote(Cursor, S8("This shrinks as the announcements spread."));
 }
}

//~ Tab: Islands
internal void
PanelDrawIslands(panel_cursor *Cursor)
{
 app_state *App = Cursor->App;
 f32 W = Cursor->Width;

 PanelSection(Cursor, S8("All islands (click a row to open it)"));
 panel_column Columns[3];
 Columns[0] = PanelColumn("Island", 0.f, false);
 Columns[1] = PanelColumn("Needs", .42f*W, false);
 Columns[2] = PanelColumn("Knows", W, true);
 PanelTableHeader(Cursor, Columns, (u32)ArrayCount(Columns));

 u32 Index = 0;
 for(island *Island = App->FirstIsland; Island; Island = Island->Next)
 {
  PanelIslandRow(Cursor, Index);
  b32 Selected = (App->Panel.SelectedPlusOne == (s32)Index + 1);
  PanelText(Cursor, 0.f, Island->Name, Selected ? Panel_ColorSelected : Panel_ColorText);

  f32 Column = .42f*W;
  for(product *Req = Island->FirstRequirement; Req; Req = Req->Next)
  {
   str8 Amount = Str8Fmt("%llu", Req->Amount);
   PanelSwatch(Cursor, Column, Req->Kind);
   PanelText(Cursor, Column + Cursor->LineHeight - 2.f, Amount, Panel_ColorText);
   Column += Cursor->LineHeight + PanelMeasure(App, Amount) + 6.f;
  }

  PanelTextRight(Cursor, W, Str8Fmt("%u/%u", BroadcastKnownCount(App, Island), App->Broadcast.IslandCount), Graph_ColorKnows);
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
  PanelSection(Cursor, S8("No island selected"));
  PanelNote(Cursor, S8("Click an island on the map,"));
  PanelNote(Cursor, S8("or a row in the Islands tab."));
  return;
 }

 u32 SelectedIdx = (u32)(App->Panel.SelectedPlusOne - 1);
 island *Island = PanelIslandAt(App, SelectedIdx);
 if(!Island)
 {
  return;
 }

 //- Title
 Cursor->Y += 4.f;
 PanelText(Cursor, 0.f, Island->Name, Panel_ColorSelected);
 PanelTextRight(Cursor, W, Str8Fmt("knows %u/%u islands", BroadcastKnownCount(App, Island), App->Broadcast.IslandCount), Graph_ColorKnows);
 PanelNewLine(Cursor);

 //- Produces
 PanelSection(Cursor, S8("Produces"));
 {
  panel_column Columns[2];
  Columns[0] = PanelColumn("Product", 0.f, false);
  Columns[1] = PanelColumn("Amount", W, true);
  PanelTableHeader(Cursor, Columns, (u32)ArrayCount(Columns));
  for(product *Product = Island->FirstProduct; Product; Product = Product->Next)
  {
   b32 Hovered = false;
   PanelBeginRow(Cursor, false, &Hovered);
   PanelSwatch(Cursor, 0.f, Product->Kind);
   PanelText(Cursor, Cursor->LineHeight, ProductNames[Product->Kind], Panel_ColorText);
   PanelTextRight(Cursor, W, Str8Fmt("+%llu", Product->Amount), Panel_ColorGood);
   PanelNewLine(Cursor);
  }
 }

 //- Needs, with the nearest supplier this island has heard of
 PanelSection(Cursor, S8("Needs"));
 {
  panel_column Columns[3];
  Columns[0] = PanelColumn("Product", 0.f, false);
  Columns[1] = PanelColumn("Amount", .46f*W, true);
  Columns[2] = PanelColumn("Nearest supplier", .51f*W, false);
  PanelTableHeader(Cursor, Columns, (u32)ArrayCount(Columns));
  for(product *Req = Island->FirstRequirement; Req; Req = Req->Next)
  {
   u32 SupplierIdx = 0;
   u32 SupplierCount = 0;
   b32 Known = PanelNearestSupplier(App, SelectedIdx, Req->Kind, &SupplierIdx, &SupplierCount);
   island *Supplier = Known ? PanelIslandAt(App, SupplierIdx) : 0;

   b32 Hovered = false;
   if(Supplier)
   {
    PanelIslandRow(Cursor, SupplierIdx);
   }
   else
   {
    PanelBeginRow(Cursor, false, &Hovered);
   }

   PanelSwatch(Cursor, 0.f, Req->Kind);
   PanelText(Cursor, Cursor->LineHeight, ProductNames[Req->Kind], Panel_ColorText);
   PanelTextRight(Cursor, .46f*W, Str8Fmt("-%llu", Req->Amount), W3_ColorNeedPad);
   if(Supplier)
   {
    island_ad *Ad = &App->Broadcast.Known[SelectedIdx][SupplierIdx];
    PanelText(Cursor, .51f*W, Supplier->Name, Panel_ColorText);
    PanelTextRight(Cursor, W, Str8Fmt("%u hop%s", Ad->HopCount, (Ad->HopCount == 1) ? "" : "s"), Panel_ColorDim);
   }
   else
   {
    PanelText(Cursor, .51f*W, S8("none known yet"), Panel_ColorShortage);
   }
   PanelNewLine(Cursor);
  }
  PanelNote(Cursor, S8("Click a supplier to open it."));
 }

 //- Network
 PanelSection(Cursor, S8("Neighbours"));
 {
  panel_column Columns[2];
  Columns[0] = PanelColumn("Island", 0.f, false);
  Columns[1] = PanelColumn("Cable cost", W, true);
  PanelTableHeader(Cursor, Columns, (u32)ArrayCount(Columns));
  for(link *Link = Island->FirstLink; Link; Link = Link->Next)
  {
   u32 NeighbourIdx = BroadcastIslandIndex(App, Link->Dest);
   PanelIslandRow(Cursor, NeighbourIdx);
   PanelText(Cursor, 0.f, Link->Dest->Name, Panel_ColorText);
   PanelTextRight(Cursor, W, Str8Fmt("%d", Link->Cost), Graph_ColorCost);
   PanelNewLine(Cursor);
  }
 }
}

//~ Tabs
// NOTE: Tab button; the active tab is filled. Returns true when clicked.
internal b32
PanelTabButton(app_state *App, app_input *Input, v2 *Cursor, str8 Label, f32 Width, b32 Active)
{
 font *Font = &App->Font;
 f32 Height = Font->HeightPx + BroadcastView_ButtonPadding;
 v4 Dest = RectFromSize(*Cursor, V2(Width, Height));
 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 b32 Hot = IsInsideRectV2(MousePos, Dest);

 u32 Color = Active ? Panel_ColorTabActive : (Hot ? Panel_ColorRowHover : Panel_ColorTab);
 DrawRect(Dest, V4FromU32(Color), 5.f, 0.f, 0.f);
 f32 TextWidth = PanelMeasure(App, Label);
 GraphDrawText(&App->FontAtlas, Font, V2(Dest.Min.X + .5f*(Width - TextWidth), Dest.Min.Y + .5f*BroadcastView_ButtonPadding),
               Label, V4FromU32(Panel_ColorText));

 Cursor->X += Width + BroadcastView_ButtonGap;
 b32 Result = (Hot && WasPressed(Input->Mouse.Buttons[PlatformMouseButton_Left]));
 return Result;
}

//~ Panel
internal void
DrawIslandPanel(app_state *App, app_input *Input, v2 BufferDim)
{
 island_panel_state *Panel = &App->Panel;
 r3_state *R3 = &App->R3;
 font *Font = &App->Font;
 f32 LineHeight = Font->HeightPx;
 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 app_button_state MouseLeft = Input->Mouse.Buttons[PlatformMouseButton_Left];

 Panel->RowHoverPlusOne = 0;

 // NOTE: Ignore a selection that no longer exists (e.g. after switching worlds).
 if(Panel->SelectedPlusOne > (s32)R3->IslandCount)
 {
  Panel->SelectedPlusOne = 0;
 }
 if((u32)Panel->Tab >= PanelTab_Count)
 {
  Panel->Tab = PanelTab_Shortages;
 }

 if(Panel->Hidden)
 {
  str8 Label = S8("Show islands panel");
  v2 Cursor = V2(BufferDim.X - Panel_Margin - PanelMeasure(App, Label) - 2.f*BroadcastView_ButtonPadding - BroadcastView_ButtonGap, Panel_Top);
  Panel->LastRect = RectFromSize(Cursor, V2(PanelMeasure(App, Label) + 2.f*BroadcastView_ButtonPadding, LineHeight + BroadcastView_ButtonPadding));
  if(BroadcastButton(App, Input, &Cursor, Label))
  {
   Panel->Hidden = false;
  }
 }
 else
 {
  v4 Box = RectFromSize(V2(BufferDim.X - Panel_Margin - Panel_Width, Panel_Top),
                        V2(Panel_Width, Max(BufferDim.Y - Panel_Top - Panel_Margin, 6.f*LineHeight)));
  Panel->LastRect = Box;
  DrawRect(Box, V4FromU32(Panel_ColorBg), 8.f, 0.f, 0.f);
  DrawRect(Box, V4FromU32(Graph_ColorNodeBorder), 8.f, 1.f, 0.f);

  f32 ButtonHeight = LineHeight + BroadcastView_ButtonPadding;

  //- Header: title and Hide
  v2 HeaderPos = V2(Box.Min.X + Panel_Pad, Box.Min.Y + Panel_Pad);
  GraphDrawText(&App->FontAtlas, Font, V2(HeaderPos.X, HeaderPos.Y + .5f*BroadcastView_ButtonPadding),
                S8("Islands"), V4FromU32(Panel_ColorText));
  {
   str8 HideLabel = S8("Hide");
   v2 Cursor = V2(Box.Max.X - Panel_Pad - PanelMeasure(App, HideLabel) - 2.f*BroadcastView_ButtonPadding, HeaderPos.Y);
   if(BroadcastButton(App, Input, &Cursor, HideLabel))
   {
    Panel->Hidden = true;
   }
  }

  //- Tabs
  {
   char *TabNames[PanelTab_Count] = {"Shortages", "Islands", "Selected"};
   f32 TabsWidth = Panel_Width - 2.f*Panel_Pad;
   f32 TabWidth = (TabsWidth - 2.f*BroadcastView_ButtonGap)/(f32)PanelTab_Count;
   v2 TabCursor = V2(Box.Min.X + Panel_Pad, HeaderPos.Y + ButtonHeight + BroadcastView_ButtonGap);
   for(u32 Tab = 0; Tab < PanelTab_Count; Tab += 1)
   {
    if(PanelTabButton(App, Input, &TabCursor, S8FromCString(TabNames[Tab]), TabWidth, (Panel->Tab == (panel_tab)Tab)))
    {
     Panel->Tab = (panel_tab)Tab;
     Panel->Scroll = 0.f;
    }
   }
  }

  //- Content (clipped and scrollable)
  f32 ContentTop = HeaderPos.Y + 2.f*ButtonHeight + BroadcastView_ButtonGap + .5f*Panel_Pad;
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
   case PanelTab_Shortages: PanelDrawShortages(&Cursor); break;
   case PanelTab_Islands:   PanelDrawIslands(&Cursor); break;
   case PanelTab_Selected:  PanelDrawSelected(&Cursor); break;
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
   if(BroadcastButton(App, Input, &FooterCursor, S8("Scroll up")))   Panel->Scroll -= Panel_ScrollStep;
   if(BroadcastButton(App, Input, &FooterCursor, S8("Scroll down"))) Panel->Scroll += Panel_ScrollStep;
  }
  Panel->Scroll = Clamp(0.f, Panel->Scroll, MaxScroll);
 }

 //- Clicking an island in the 3D view selects it (not when clicking on the UI)
 b32 OverUI = (IsInsideRectV2(MousePos, Panel->LastRect) || W3PointOnUI(R3, MousePos));
 if(!OverUI && WasPressed(MouseLeft) && R3->HoveredIdx >= 0)
 {
  PanelSelect(App, R3->HoveredIdx);
 }
}
