//-
// SolarPunk graph view: islands as nodes, links as lines
//-

//~ Constants
#define Graph_NodePadding   8.f
#define Graph_LineThickness 2.f

#define Graph_ColorNodeBg       0xFF3B4252
#define Graph_ColorNodeBorder   0xFF88C0D0
#define Graph_ColorTitle        0xFFECEFF4
#define Graph_ColorProduct      0xFFA3BE8C
#define Graph_ColorRequirement  0xFFD08770
#define Graph_ColorLink         0xFF4C566A
#define Graph_ColorCost         0xFFEBCB8B
#define Graph_ColorLinkFlash    0xFFEBCB8B
#define Graph_ColorKnows        0xFF81A1C1

// NOTE: Above this many islands, nodes only show their name; hover one for details.
#define Graph_MaxDetailedIslands 8
#define Graph_MapMarginX       90.f
#define Graph_MapMarginTop     290.f
#define Graph_MapMarginBottom  50.f

//~ Text
internal f32
GraphMeasureText(font_atlas *Atlas, font *Font, str8 String)
{
 f32 Result = 0.f;
 for EachIndex(Idx, String.Size)
 {
  s32 PackedCharIdx = GetPackedCharIdx(Font, (rune)String.Data[Idx]);
  Result += Atlas->PackedChars[PackedCharIdx].xadvance;
 }
 return Result;
}

internal void
GraphDrawText(font_atlas *Atlas, font *Font, v2 Pos, str8 String, v4 Color)
{
 v2 Cur = V2(FloorF32(Pos.X), FloorF32(Pos.Y));
 for EachIndex(Idx, String.Size)
 {
  rune Char = (rune)String.Data[Idx];
  s32 PackedCharIdx = GetPackedCharIdx(Font, Char);
  DrawRectChar(Atlas, Font, Cur, Char, Color);
  Cur.X += Atlas->PackedChars[PackedCharIdx].xadvance;
 }
}

//~ Lines
// NOTE: The renderer only draws axis-aligned rects, so a line is a row of small squares.
internal void
GraphDrawLine(v2 A, v2 B, f32 Thickness, v4 Color)
{
 v2 Delta = V2SubV2(B, A);
 f32 Length = (f32)SqrtF64((f64)(Delta.X*Delta.X + Delta.Y*Delta.Y));
 s32 StepCount = (s32)(Length/(.5f*Thickness)) + 1;
 
 for(s32 Step = 0; Step <= StepCount; Step += 1)
 {
  f32 t = (f32)Step/(f32)StepCount;
  v2 P = V2(A.X + t*Delta.X, A.Y + t*Delta.Y);
  v2 Min = V2(P.X - .5f*Thickness, P.Y - .5f*Thickness);
  DrawRect(RectFromSize(Min, V2(Thickness, Thickness)), Color, 0.f, 0.f, 0.f);
 }
}

//~ Graph
internal void
GraphLayoutIslandsInCircle(app_state *App, v2 BufferDim)
{
 s32 IslandCount = 0;
 for(island *Island = App->FirstIsland; Island; Island = Island->Next)
 {
  IslandCount += 1;
 }
 
 v2 Center = V2(.5f*BufferDim.X, .5f*BufferDim.Y);
 f32 Radius = .33f*Min(BufferDim.X, BufferDim.Y);
 
 s32 Idx = 0;
 for(island *Island = App->FirstIsland; Island; Island = Island->Next)
 {
  // Start at the top and go clockwise.
  f64 Angle = 2.0*(f64)Pi32*(f64)Idx/(f64)Max(IslandCount, 1) - .5*(f64)Pi32;
  Island->Pos = V2(Center.X + Radius*(f32)CosF64(Angle),
                   Center.Y + Radius*(f32)SinF64(Angle));
  Idx += 1;
 }
}

// NOTE: For generated worlds: spread the 0..1 map positions over the window,
// leaving room at the top for the controls.
internal void
GraphLayoutIslandsFromMap(app_state *App, v2 BufferDim)
{
 v2 Min = V2(Graph_MapMarginX, Graph_MapMarginTop);
 v2 Size = V2(Max(BufferDim.X - 2.f*Graph_MapMarginX, 1.f),
              Max(BufferDim.Y - Graph_MapMarginTop - Graph_MapMarginBottom, 1.f));
 
 for(island *Island = App->FirstIsland; Island; Island = Island->Next)
 {
  Island->Pos = V2(Min.X + Island->MapPos.X*Size.X,
                   Min.Y + Island->MapPos.Y*Size.Y);
 }
}

internal void
DrawIslandGraph(app_state *App, v2 BufferDim)
{
 font_atlas *Atlas = &App->FontAtlas;
 font *Font = &App->Font;
 f32 LineHeight = Font->HeightPx;
 
 if(App->HasMapLayout)
 {
  GraphLayoutIslandsFromMap(App, BufferDim);
 }
 else
 {
  GraphLayoutIslandsInCircle(App, BufferDim);
 }
 
 b32 Compact = (BroadcastCountIslands(App) > Graph_MaxDetailedIslands);
 
 //- Links (drawn first so nodes sit on top)
 for(island *Island = App->FirstIsland; Island; Island = Island->Next)
 {
  for(link *Link = Island->FirstLink; Link; Link = Link->Next)
  {
   // Links exist in both directions, only draw one of them.
   if(Link->Start < Link->Dest)
   {
    v2 A = Link->Start->Pos;
    v2 B = Link->Dest->Pos;
    // Links light up while they carry news about an island the receiver did not know yet.
    f32 Flash = BroadcastLinkFlash(App, Link->Start, Link->Dest);
    v4 Quiet = V4FromU32(Graph_ColorLink);
    v4 Lit = V4FromU32(Graph_ColorLinkFlash);
    v4 LinkColor = V4(Lerp(Quiet.X, Lit.X, Flash), Lerp(Quiet.Y, Lit.Y, Flash),
                      Lerp(Quiet.Z, Lit.Z, Flash), Lerp(Quiet.W, Lit.W, Flash));
    GraphDrawLine(A, B, Graph_LineThickness*(1.f + 1.5f*Flash), LinkColor);
    
    str8 CostText = Str8Fmt("%d", Link->Cost);
    f32 TextWidth = GraphMeasureText(Atlas, Font, CostText);
    v2 Mid = V2(.5f*(A.X + B.X), .5f*(A.Y + B.Y));
    v2 LabelMin = V2(Mid.X - .5f*TextWidth - 4.f, Mid.Y - .5f*LineHeight);
    DrawRect(RectFromSize(LabelMin, V2(TextWidth + 8.f, LineHeight)), 
             V4FromU32(0xFF2E3440), 4.f, 0.f, .5f);
    GraphDrawText(Atlas, Font, V2(LabelMin.X + 4.f, LabelMin.Y), CostText, V4FromU32(Graph_ColorCost));
   }
  }
 }
 
 //- Nodes
 for(island *Island = App->FirstIsland; Island; Island = Island->Next)
 {
  // Build the lines first so we can size the box.
  str8 Lines[32];
  v4 Colors[32];
  s32 LineCount = 0;
  
  Lines[LineCount] = Island->Name;
  Colors[LineCount] = V4FromU32(Graph_ColorTitle);
  LineCount += 1;
  
  for(product *Product = Island->FirstProduct; Product && !Compact && LineCount < (s32)ArrayCount(Lines); Product = Product->Next)
  {
   Lines[LineCount] = Str8Fmt("+ %.*s %llu", (s32)ProductNames[Product->Kind].Size, ProductNames[Product->Kind].Data, Product->Amount);
   Colors[LineCount] = V4FromU32(Graph_ColorProduct);
   LineCount += 1;
  }
  
  for(product *Req = Island->FirstRequirement; Req && !Compact && LineCount < (s32)ArrayCount(Lines); Req = Req->Next)
  {
   Lines[LineCount] = Str8Fmt("- %.*s %llu", (s32)ProductNames[Req->Kind].Size, ProductNames[Req->Kind].Data, Req->Amount);
   Colors[LineCount] = V4FromU32(Graph_ColorRequirement);
   LineCount += 1;
  }
  
  if(LineCount < (s32)ArrayCount(Lines))
  {
   Lines[LineCount] = Str8Fmt("knows %u/%u", BroadcastKnownCount(App, Island), App->Broadcast.IslandCount);
   Colors[LineCount] = V4FromU32(Graph_ColorKnows);
   LineCount += 1;
  }
  
  f32 MaxWidth = 0.f;
  for(s32 Idx = 0; Idx < LineCount; Idx += 1)
  {
   MaxWidth = Max(MaxWidth, GraphMeasureText(Atlas, Font, Lines[Idx]));
  }
  
  v2 Size = V2(CeilF32(MaxWidth) + 2.f*Graph_NodePadding,
               (f32)LineCount*LineHeight + 2.f*Graph_NodePadding);
  v2 Min = V2(FloorF32(Island->Pos.X - .5f*Size.X), 
              FloorF32(Island->Pos.Y - .5f*Size.Y));
  v4 Dest = RectFromSize(Min, Size);
  
  DrawRect(Dest, V4FromU32(Graph_ColorNodeBg), 6.f, 0.f, .5f);
  DrawRect(Dest, V4FromU32(Graph_ColorNodeBorder), 6.f, 2.f, .5f);
  
  v2 Cur = V2(Min.X + Graph_NodePadding, Min.Y + Graph_NodePadding);
  for(s32 Idx = 0; Idx < LineCount; Idx += 1)
  {
   GraphDrawText(Atlas, Font, Cur, Lines[Idx], Colors[Idx]);
   Cur.Y += LineHeight;
  }
 }
}
