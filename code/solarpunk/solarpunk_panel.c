//-
// SolarPunk islands panel (right side): totals per product, every island with its
// needs, and the details of one selected island (production, needs with the suppliers
// it knows about, network). Select an island by clicking it in 3D or in the list.
//-

//~ Constants
#define Panel_Width        420.f
#define Panel_Top          92.f
#define Panel_Margin       12.f
#define Panel_Pad          10.f
#define Panel_ScrollStep   160.f
#define Panel_MaxSuppliers 3

#define Panel_ColorBg        0xF02E3440
#define Panel_ColorRowHover  0xFF434C5E
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
 App->Panel.Scroll = 0.f;
 App->Panel.Hidden = false;
}

// NOTE: Text helpers; Y is the top of the line and is advanced by one line.
typedef struct panel_cursor panel_cursor;
struct panel_cursor
{
 app_state *App;
 app_input *Input;
 v4 Content;     // visible content area (clipped)
 f32 X;
 f32 Y;
 f32 Width;
 f32 LineHeight;
};

internal void
PanelText(panel_cursor *Cursor, f32 Indent, str8 Text, u32 Color)
{
 GraphDrawText(&Cursor->App->FontAtlas, &Cursor->App->Font, V2(Cursor->X + Indent, Cursor->Y), Text, V4FromU32(Color));
}

internal void
PanelSwatch(panel_cursor *Cursor, f32 Indent, product_kind Kind)
{
 f32 Size = Cursor->LineHeight - 6.f;
 DrawRect(RectFromSize(V2(Cursor->X + Indent, Cursor->Y + 3.f), V2(Size, Size)),
          V4FromU32(W3ProductColors[Kind]), 3.f, 0.f, 0.f);
}

internal void
PanelSection(panel_cursor *Cursor, str8 Title)
{
 Cursor->Y += .4f*Cursor->LineHeight;
 PanelText(Cursor, 0.f, Title, Panel_ColorSection);
 Cursor->Y += Cursor->LineHeight;
}

// NOTE: A full-width row that highlights on hover; returns true when clicked.
internal b32
PanelRow(panel_cursor *Cursor, b32 *Hovered)
{
 v4 Row = RectFromSize(V2(Cursor->X - 4.f, Cursor->Y), V2(Cursor->Width + 8.f, Cursor->LineHeight));
 v2 MousePos = V2V2S32(Cursor->Input->Mouse.Pos);
 *Hovered = (IsInsideRectV2(MousePos, Row) && IsInsideRectV2(MousePos, Cursor->Content));
 if(*Hovered)
 {
  DrawRect(Row, V4FromU32(Panel_ColorRowHover), 4.f, 0.f, 0.f);
 }
 b32 Result = (*Hovered && WasPressed(Cursor->Input->Mouse.Buttons[PlatformMouseButton_Left]));
 return Result;
}

internal f32
PanelMeasure(app_state *App, str8 Text)
{
 f32 Result = GraphMeasureText(&App->FontAtlas, &App->Font, Text);
 return Result;
}

//~ Overview of all islands
internal void
PanelDrawOverview(panel_cursor *Cursor)
{
 app_state *App = Cursor->App;

 //- Totals per product: produced vs needed over all islands
 u64 Produced[ProductKind_Count] = {0};
 u64 Needed[ProductKind_Count] = {0};
 u32 IslandCount = 0;
 for(island *Island = App->FirstIsland; Island; Island = Island->Next)
 {
  for(product *Product = Island->FirstProduct; Product; Product = Product->Next) Produced[Product->Kind] += Product->Amount;
  for(product *Req = Island->FirstRequirement; Req; Req = Req->Next) Needed[Req->Kind] += Req->Amount;
  IslandCount += 1;
 }

 PanelSection(Cursor, S8("Totals: produced / needed"));
 for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
 {
  PanelSwatch(Cursor, 0.f, (product_kind)Kind);
  PanelText(Cursor, Cursor->LineHeight, ProductNames[Kind], Panel_ColorText);
  str8 Numbers = Str8Fmt("%llu / %llu", Produced[Kind], Needed[Kind]);
  u32 Color = (Needed[Kind] > Produced[Kind]) ? Panel_ColorShortage : Panel_ColorGood;
  PanelText(Cursor, Cursor->Width - PanelMeasure(App, Numbers), Numbers, Color);
  Cursor->Y += Cursor->LineHeight;
 }
 PanelText(Cursor, 0.f, S8("red = more needed than produced"), Panel_ColorDim);
 Cursor->Y += Cursor->LineHeight;

 //- Every island with its needs
 PanelSection(Cursor, Str8Fmt("%u islands and their needs (click one)", IslandCount));
 u32 Index = 0;
 for(island *Island = App->FirstIsland; Island; Island = Island->Next)
 {
  b32 Hovered = false;
  if(PanelRow(Cursor, &Hovered))
  {
   PanelSelect(App, (s32)Index);
  }
  if(Hovered)
  {
   App->Panel.RowHoverPlusOne = (s32)Index + 1;
  }

  PanelText(Cursor, 0.f, Island->Name, Panel_ColorText);

  // Needs, right-aligned: [swatch]amount ...
  f32 Right = Cursor->Width;
  for(product *Req = Island->FirstRequirement; Req; Req = Req->Next)
  {
   str8 Amount = Str8Fmt("%llu", Req->Amount);
   f32 ItemWidth = Cursor->LineHeight + PanelMeasure(App, Amount) + 8.f;
   Right -= ItemWidth;
   PanelSwatch(Cursor, Right, Req->Kind);
   PanelText(Cursor, Right + Cursor->LineHeight - 2.f, Amount, Panel_ColorText);
  }

  Cursor->Y += Cursor->LineHeight;
  Index += 1;
 }
}

//~ Details of one island
internal void
PanelDrawIsland(panel_cursor *Cursor, u32 SelectedIdx)
{
 app_state *App = Cursor->App;
 broadcast_state *Broadcast = &App->Broadcast;
 island *Island = PanelIslandAt(App, SelectedIdx);

 if(Island)
 {
  //- Produces
  PanelSection(Cursor, S8("Produces"));
  for(product *Product = Island->FirstProduct; Product; Product = Product->Next)
  {
   PanelSwatch(Cursor, 0.f, Product->Kind);
   PanelText(Cursor, Cursor->LineHeight, ProductNames[Product->Kind], Panel_ColorText);
   str8 Amount = Str8Fmt("+%llu", Product->Amount);
   PanelText(Cursor, Cursor->Width - PanelMeasure(App, Amount), Amount, Panel_ColorGood);
   Cursor->Y += Cursor->LineHeight;
  }

  //- Needs, each with the suppliers this island has heard of
  PanelSection(Cursor, S8("Needs, and suppliers it knows of"));
  for(product *Req = Island->FirstRequirement; Req; Req = Req->Next)
  {
   PanelSwatch(Cursor, 0.f, Req->Kind);
   PanelText(Cursor, Cursor->LineHeight, ProductNames[Req->Kind], Panel_ColorText);
   str8 Amount = Str8Fmt("-%llu", Req->Amount);
   PanelText(Cursor, Cursor->Width - PanelMeasure(App, Amount), Amount, W3_ColorNeedPad);
   Cursor->Y += Cursor->LineHeight;

   // NOTE: Suppliers come from this island's own announcement table, so far-away
   // producers only show up after their news has travelled here.
   u32 SupplierIdx[Broadcast_MaxIslands];
   u32 SupplierCount = 0;
   if(SelectedIdx < Broadcast->IslandCount)
   {
    for(u32 Origin = 0; Origin < Broadcast->IslandCount; Origin += 1)
    {
     island_ad *Ad = &Broadcast->Known[SelectedIdx][Origin];
     if(Origin != SelectedIdx && Ad->Valid && Ad->Produces[Req->Kind] > 0)
     {
      // Insertion sort by hop count: closest first.
      u32 At = SupplierCount;
      while(At > 0 && Broadcast->Known[SelectedIdx][SupplierIdx[At - 1]].HopCount > Ad->HopCount)
      {
       SupplierIdx[At] = SupplierIdx[At - 1];
       At -= 1;
      }
      SupplierIdx[At] = Origin;
      SupplierCount += 1;
     }
    }
   }

   if(SupplierCount == 0)
   {
    PanelText(Cursor, Cursor->LineHeight, S8("no supplier known yet"), Panel_ColorDim);
    Cursor->Y += Cursor->LineHeight;
   }

   for(u32 Shown = 0; Shown < SupplierCount && Shown < Panel_MaxSuppliers; Shown += 1)
   {
    u32 Origin = SupplierIdx[Shown];
    island *Supplier = PanelIslandAt(App, Origin);
    island_ad *Ad = &Broadcast->Known[SelectedIdx][Origin];
    if(Supplier)
    {
     b32 Hovered = false;
     if(PanelRow(Cursor, &Hovered))
     {
      PanelSelect(App, (s32)Origin);
     }
     if(Hovered)
     {
      App->Panel.RowHoverPlusOne = (s32)Origin + 1;
     }
     PanelText(Cursor, Cursor->LineHeight, Supplier->Name, Panel_ColorText);
     str8 Info = Str8Fmt("%llu, %u hop%s", Ad->Produces[Req->Kind], Ad->HopCount, (Ad->HopCount == 1) ? "" : "s");
     PanelText(Cursor, Cursor->Width - PanelMeasure(App, Info), Info, Panel_ColorDim);
     Cursor->Y += Cursor->LineHeight;
    }
   }

   if(SupplierCount > Panel_MaxSuppliers)
   {
    PanelText(Cursor, Cursor->LineHeight, Str8Fmt("+%u more", SupplierCount - Panel_MaxSuppliers), Panel_ColorDim);
    Cursor->Y += Cursor->LineHeight;
   }
  }

  //- Network
  PanelSection(Cursor, S8("Network"));
  PanelText(Cursor, 0.f, Str8Fmt("knows %u/%u islands", BroadcastKnownCount(App, Island), Broadcast->IslandCount), Panel_ColorText);
  Cursor->Y += Cursor->LineHeight;
  PanelText(Cursor, 0.f, S8("Neighbours (click to open):"), Panel_ColorDim);
  Cursor->Y += Cursor->LineHeight;
  for(link *Link = Island->FirstLink; Link; Link = Link->Next)
  {
   u32 NeighbourIdx = BroadcastIslandIndex(App, Link->Dest);
   b32 Hovered = false;
   if(PanelRow(Cursor, &Hovered))
   {
    PanelSelect(App, (s32)NeighbourIdx);
   }
   if(Hovered)
   {
    App->Panel.RowHoverPlusOne = (s32)NeighbourIdx + 1;
   }
   PanelText(Cursor, Cursor->LineHeight, Link->Dest->Name, Panel_ColorText);
   str8 Cost = Str8Fmt("cable cost %d", Link->Cost);
   PanelText(Cursor, Cursor->Width - PanelMeasure(App, Cost), Cost, Graph_ColorCost);
   Cursor->Y += Cursor->LineHeight;
  }
 }
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
                        V2(Panel_Width, Max(BufferDim.Y - Panel_Top - Panel_Margin, 4.f*LineHeight)));
  Panel->LastRect = Box;
  DrawRect(Box, V4FromU32(Panel_ColorBg), 8.f, 0.f, 0.f);
  DrawRect(Box, V4FromU32(Graph_ColorNodeBorder), 8.f, 1.f, 0.f);

  //- Header: title and buttons
  f32 ButtonHeight = LineHeight + BroadcastView_ButtonPadding;
  v2 HeaderPos = V2(Box.Min.X + Panel_Pad, Box.Min.Y + Panel_Pad);
  {
   str8 HideLabel = S8("Hide");
   v2 Cursor = V2(Box.Max.X - Panel_Pad - PanelMeasure(App, HideLabel) - 2.f*BroadcastView_ButtonPadding, HeaderPos.Y);
   if(BroadcastButton(App, Input, &Cursor, HideLabel))
   {
    Panel->Hidden = true;
   }

   if(Panel->SelectedPlusOne)
   {
    v2 BackCursor = HeaderPos;
    if(BroadcastButton(App, Input, &BackCursor, S8("< All islands")))
    {
     Panel->SelectedPlusOne = 0;
     Panel->Scroll = 0.f;
    }
   }
   else
   {
    GraphDrawText(&App->FontAtlas, Font, V2(HeaderPos.X, HeaderPos.Y + .5f*BroadcastView_ButtonPadding),
                  S8("Islands overview"), V4FromU32(Panel_ColorText));
   }
  }

  f32 HeaderBottom = HeaderPos.Y + ButtonHeight + Panel_Pad;
  if(Panel->SelectedPlusOne)
  {
   island *Selected = PanelIslandAt(App, (u32)(Panel->SelectedPlusOne - 1));
   if(Selected)
   {
    GraphDrawText(&App->FontAtlas, Font, V2(HeaderPos.X, HeaderBottom), Selected->Name, V4FromU32(Panel_ColorSelected));
    HeaderBottom += LineHeight + .5f*Panel_Pad;
   }
  }

  //- Content (clipped and scrollable)
  f32 FooterHeight = ButtonHeight + 2.f*Panel_Pad;
  v4 Content = V4(Box.Min.X + Panel_Pad, HeaderBottom, Box.Max.X - Panel_Pad, Box.Max.Y - FooterHeight);

  panel_cursor Cursor = {0};
  Cursor.App = App;
  Cursor.Input = Input;
  Cursor.Content = Content;
  Cursor.X = Content.Min.X + 4.f;
  Cursor.Y = Content.Min.Y - Panel->Scroll;
  Cursor.Width = (Content.Max.X - Content.Min.X) - 8.f;
  Cursor.LineHeight = LineHeight;

  RenderPushClip(Content);
  if(Panel->SelectedPlusOne)
  {
   PanelDrawIsland(&Cursor, (u32)(Panel->SelectedPlusOne - 1));
  }
  else
  {
   PanelDrawOverview(&Cursor);
  }
  RenderPopClip();

  //- Scrolling, only when the content does not fit
  f32 ContentHeight = (Cursor.Y + Panel->Scroll) - Content.Min.Y;
  f32 VisibleHeight = Content.Max.Y - Content.Min.Y;
  f32 MaxScroll = Max(0.f, ContentHeight - VisibleHeight);
  if(MaxScroll > 0.f)
  {
   v2 FooterCursor = V2(Box.Min.X + Panel_Pad, Box.Max.Y - Panel_Pad - ButtonHeight);
   if(BroadcastButton(App, Input, &FooterCursor, S8("Scroll up")))   Panel->Scroll -= Panel_ScrollStep;
   if(BroadcastButton(App, Input, &FooterCursor, S8("Scroll down"))) Panel->Scroll += Panel_ScrollStep;
  }
  Panel->Scroll = Clamp(0.f, Panel->Scroll, MaxScroll);
 }

 //- Clicking an island in the 3D view selects it (not when clicking on the UI)
 b32 OverUI = (IsInsideRectV2(MousePos, Panel->LastRect) ||
               IsInsideRectV2(MousePos, RectFromSize(V2(0.f, 0.f), V2(V3D_ControlsAreaWidth, V3D_ControlsAreaHeight))) ||
               IsInsideRectV2(MousePos, RectFromSize(V2(BufferDim.X - V3D_CameraAreaWidth, 0.f), V2(V3D_CameraAreaWidth, V3D_CameraAreaHeight))) ||
               IsInsideRectV2(MousePos, V3LegendRect(App, BufferDim)));
 if(!OverUI && WasPressed(MouseLeft) && R3->HoveredIdx >= 0)
 {
  PanelSelect(App, R3->HoveredIdx);
 }
}
