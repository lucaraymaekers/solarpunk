//-
// SolarPunk 3D world: floating sky islands, product crates, cables and news pulses.
//-

//~ Constants
#define W3_WorldSize        80.f   // map 0..1 is spread over this many world units
#define W3_IslandRadius     2.5f
#define W3_IslandHeightVary 1.4f   // islands float at slightly different heights
#define W3_IslandSides      24
#define W3_Seed             7919

#define W3_CableRadius      .07f
#define W3_CableSegments    16
#define W3_CableSag         .06f   // sag as a fraction of the cable length

#define W3_CrateHalf        .24f
#define W3_CrateUnitsPerBox 70     // one stacked crate per this many units produced
#define W3_MaxCrateStack    3

#define W3_CloudCount       40

//- Colours
#define W3_ColorLawn        0xFF96D36E
#define W3_ColorStoneTop    0xFFF6E8CC
#define W3_ColorStoneMid    0xFFEBD6B2
#define W3_ColorStoneLow    0xFFDCC19A
#define W3_ColorStoneBottom 0xFFC9A883
#define W3_ColorTerraceWall 0xFFF2E2C4
#define W3_ColorCropSoil    0xFF8A6A4A
#define W3_ColorCropWheat   0xFFE8C66A
#define W3_ColorCropGreen   0xFF6FAE5A
#define W3_ColorDomeRim     0xFFF4F1EA
#define W3_ColorGlass       0xFFCDEFFF
#define W3_ColorWater       0xFF7FCBEA
#define W3_ColorMist        0xFFFFFFFF
#define W3_ColorTrunk       0xFF7A5230
#define W3_ColorLeaves      0xFF4F9D5A
#define W3_ColorFruit       0xFFF08A5D
#define W3_ColorPanel       0xFF24406E
#define W3_ColorPost        0xFFB0B7C3
#define W3_ColorProducePad  0xFF8FD18C
#define W3_ColorNeedPad     0xFFF2A65A
#define W3_ColorCable       0xFF4A3F5C
#define W3_ColorCableLit    0xFFEBCB8B
#define W3_ColorCableHover  0xFFFFE08A
#define W3_ColorPulse       0xFFFFF3B0
#define W3_ColorCrystalLow  0xFF8892A0
#define W3_ColorCrystalHigh 0xFF7FE3F0
#define W3_ColorHover       0xFFFFE08A
#define W3_ColorSelected    0xFF88E0FF

//- Island status colours (ring around the island and dot in its label)
#define W3_ColorStatusOk      0xFF7BD389
#define W3_ColorStatusWaiting 0xFFF5A65B
#define W3_ColorStatusShort   0xFFE0606A
#define W3_ColorCloud       0xFFFFE6D2

// NOTE: One colour per product kind, used for crates and in the legend.
global_variable u32 W3ProductColors[ProductKind_Count] =
{
 0xFF808080, // None
 0xFFD9B26F, // Seeds
 0xFF5DADE2, // Fresh Water
 0xFF34558F, // Solar Panels
 0xFFE5C453, // Batteries
 0xFFF08A5D, // Fish
 0xFF3FB59B, // Algae
 0xFFC46A3A, // Copper
 0xFF8B5A2B, // Timber
};

//~ Helpers
internal v3
W3IslandTopFromMap(v2 MapPos, u32 Index)
{
 random_series Series = {0};
 RandomSeed(&Series, W3_Seed + 31*(u64)Index);
 f32 Height = W3_IslandHeightVary*RandomBilateral(&Series);

 v3 Result = V3((MapPos.X - .5f)*W3_WorldSize, Height, (MapPos.Y - .5f)*W3_WorldSize);
 return Result;
}

// NOTE: Cable between two island tops. Point at t (0..1) going from A to B, with sag.
internal v3
W3CablePoint(v3 TopA, f32 RadiusA, v3 TopB, f32 RadiusB, f32 t)
{
 v3 Flat = V3(TopB.X - TopA.X, 0.f, TopB.Z - TopA.Z);
 v3 Direction = R3Normalize(Flat);
 v3 Down = V3(0.f, -.45f, 0.f);
 v3 A = R3Add(R3Add(TopA, R3Scale(Direction, .8f*RadiusA)), Down);
 v3 B = R3Add(R3Sub(TopB, R3Scale(Direction, .8f*RadiusB)), Down);

 f32 Sag = W3_CableSag*R3Length(R3Sub(B, A));
 v3 Result = R3Lerp(A, B, t);
 Result.Y -= 4.f*Sag*t*(1.f - t);
 return Result;
}

internal v4
W3Shade(v4 Color, random_series *Series)
{
 // Small per-face colour variation so surfaces do not look flat.
 f32 Factor = .92f + .16f*RandomUnilateral(Series);
 v4 Result = V4(Color.X*Factor, Color.Y*Factor, Color.Z*Factor, Color.W);
 return Result;
}

//~ Island status
internal u32
W3StatusColor(u32 Status)
{
 u32 Result = W3_ColorStatusOk;
 if(Status == IslandStatus_Waiting) Result = W3_ColorStatusWaiting;
 if(Status == IslandStatus_Short)   Result = W3_ColorStatusShort;
 return Result;
}

// NOTE: True when the island has heard (through the announcements) of another island
// that produces Kind.
internal b32
W3KnowsSupplier(app_state *App, u32 IslandIdx, product_kind Kind)
{
 broadcast_state *Broadcast = &App->Broadcast;
 b32 Result = false;
 if(IslandIdx < Broadcast->IslandCount)
 {
  for(u32 Origin = 0; Origin < Broadcast->IslandCount && !Result; Origin += 1)
  {
   island_ad *Ad = &Broadcast->Known[IslandIdx][Origin];
   Result = (Origin != IslandIdx && Ad->Valid && Ad->Produces[Kind] > 0);
  }
 }
 return Result;
}

// NOTE: World totals per product, and each island's status:
//   Waiting : it has not heard of a supplier for one of its needs yet
//   Short   : it knows its suppliers, but needs a product the whole world does not
//             make enough of
//   Ok      : otherwise
// Waiting comes first, so islands go orange -> green/red as the news spreads.
internal void
W3ComputeStatus(app_state *App)
{
 r3_state *R3 = &App->R3;
 MemorySet(R3->TotalProduced, 0, sizeof(R3->TotalProduced));
 MemorySet(R3->TotalNeeded, 0, sizeof(R3->TotalNeeded));
 for(island *Island = App->FirstIsland; Island; Island = Island->Next)
 {
  for(product *Product = Island->FirstProduct; Product; Product = Product->Next) R3->TotalProduced[Product->Kind] += Product->Amount;
  for(product *Req = Island->FirstRequirement; Req; Req = Req->Next) R3->TotalNeeded[Req->Kind] += Req->Amount;
 }

 u32 Index = 0;
 for(island *Island = App->FirstIsland; Island && Index < R3_MaxIslands; Island = Island->Next)
 {
  b32 AnyShort = false;
  b32 AnyWaiting = false;
  for(product *Req = Island->FirstRequirement; Req; Req = Req->Next)
  {
   if(R3->TotalNeeded[Req->Kind] > R3->TotalProduced[Req->Kind]) AnyShort = true;
   if(!W3KnowsSupplier(App, Index, Req->Kind)) AnyWaiting = true;
  }
  R3->IslandStatus[Index] = (AnyWaiting ? IslandStatus_Waiting : (AnyShort ? IslandStatus_Short : IslandStatus_Ok));
  Index += 1;
 }
}

// NOTE: True when P is on one of the 2D UI areas recorded last frame (R3->UIRects).
internal b32
W3PointOnUI(r3_state *R3, v2 P)
{
 b32 Result = false;
 for(u32 Idx = 0; Idx < R3->UIRectCount && !Result; Idx += 1)
 {
  Result = IsInsideRectV2(P, R3->UIRects[Idx]);
 }
 return Result;
}

//~ Island pieces
// NOTE: A terraced garden island: a pale stone base, three garden terraces (the lowest
// holds the crates, the middle one crop rows, the top one a glass greenhouse dome)
// and a small waterfall spilling off one edge.

// NOTE: Layout of the terraces relative to the island top centre (the lowest terrace).
#define W3_Terrace1Offset   V3(0.f, .6f, -1.f)
#define W3_Terrace1Radius   1.35f
#define W3_Terrace2Offset   V3(0.f, 1.2f, -1.3f)
#define W3_Terrace2Radius   .8f
#define W3_TerraceWall      .6f
#define W3_CrystalOffset    V3(0.f, 3.f, -1.3f)
#define W3_LabelOffset      V3(0.f, 4.5f, -1.f)

// NOTE: A flat-topped prism: top surface at Center, walls going down by Height.
internal void
W3PushTerrace(r3_mesh *Mesh, v3 Center, f32 Radius, f32 Height, u32 Sides, v4 TopColor, v4 WallColor)
{
 v3 Inside = V3(Center.X, Center.Y - .5f*Height, Center.Z);
 for(u32 Side = 0; Side < Sides; Side += 1)
 {
  f32 Angle0 = 2.f*Pi32*(f32)Side/(f32)Sides;
  f32 Angle1 = 2.f*Pi32*(f32)(Side + 1)/(f32)Sides;
  v3 Top0 = V3(Center.X + Radius*R3Cos(Angle0), Center.Y, Center.Z + Radius*R3Sin(Angle0));
  v3 Top1 = V3(Center.X + Radius*R3Cos(Angle1), Center.Y, Center.Z + Radius*R3Sin(Angle1));
  v3 Bottom0 = V3(Top0.X, Center.Y - Height, Top0.Z);
  v3 Bottom1 = V3(Top1.X, Center.Y - Height, Top1.Z);
  R3PushTri(Mesh, Center, Top0, Top1, V3(Center.X, Center.Y - Height, Center.Z), TopColor);
  R3PushQuad(Mesh, Top0, Top1, Bottom1, Bottom0, Inside, WallColor);
 }
}

// NOTE: A flat glowing ring just above the island's top, between two radii.
internal void
W3PushRing(r3_mesh *Mesh, v3 Top, f32 Inner, f32 Outer, v4 Color)
{
 f32 Y = Top.Y + .05f;
 for(u32 Side = 0; Side < W3_IslandSides; Side += 1)
 {
  f32 Angle0 = 2.f*Pi32*(f32)Side/(f32)W3_IslandSides;
  f32 Angle1 = 2.f*Pi32*(f32)(Side + 1)/(f32)W3_IslandSides;
  v3 P0 = V3(Top.X + Inner*R3Cos(Angle0), Y, Top.Z + Inner*R3Sin(Angle0));
  v3 P1 = V3(Top.X + Outer*R3Cos(Angle0), Y, Top.Z + Outer*R3Sin(Angle0));
  v3 P2 = V3(Top.X + Outer*R3Cos(Angle1), Y, Top.Z + Outer*R3Sin(Angle1));
  v3 P3 = V3(Top.X + Inner*R3Cos(Angle1), Y, Top.Z + Inner*R3Sin(Angle1));
  R3PushTriUnlit(Mesh, P0, P1, P2, Color);
  R3PushTriUnlit(Mesh, P0, P2, P3, Color);
 }
}

internal void
W3PushIslandBody(r3_mesh *Opaque, r3_mesh *Translucent, v3 Top, f32 Radius, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor)
{
 random_series Series = {0};
 RandomSeed(&Series, W3_Seed*7 + 101*(u64)Index);

 //- Stone base: a smooth rounded bowl under the lowest terrace
 {
  f32 RingRadius[5] = {1.f, .93f, .74f, .45f, .12f};
  f32 RingDepth[5]  = {.35f, 1.2f, 2.3f, 3.3f, 4.f};
  u32 RingColor[4]  = {W3_ColorStoneTop, W3_ColorStoneMid, W3_ColorStoneLow, W3_ColorStoneBottom};

  for(u32 Side = 0; Side < W3_IslandSides; Side += 1)
  {
   f32 Angle0 = 2.f*Pi32*(f32)Side/(f32)W3_IslandSides;
   f32 Angle1 = 2.f*Pi32*(f32)(Side + 1)/(f32)W3_IslandSides;
   for(u32 Ring = 0; Ring < 4; Ring += 1)
   {
    f32 R0 = Radius*RingRadius[Ring];
    f32 R1 = Radius*RingRadius[Ring + 1];
    f32 Y0 = Top.Y - RingDepth[Ring];
    f32 Y1 = Top.Y - RingDepth[Ring + 1];
    v3 P0 = V3(Top.X + R0*R3Cos(Angle0), Y0, Top.Z + R0*R3Sin(Angle0));
    v3 P1 = V3(Top.X + R0*R3Cos(Angle1), Y0, Top.Z + R0*R3Sin(Angle1));
    v3 P2 = V3(Top.X + R1*R3Cos(Angle1), Y1, Top.Z + R1*R3Sin(Angle1));
    v3 P3 = V3(Top.X + R1*R3Cos(Angle0), Y1, Top.Z + R1*R3Sin(Angle0));
    R3PushQuad(Opaque, P0, P1, P2, P3, V3(Top.X, .5f*(Y0 + Y1), Top.Z),
               W3Shade(R3Color(RingColor[Ring]), &Series));
   }
  }

  // Close the bottom.
  f32 TipR = Radius*RingRadius[4];
  v3 TipCenter = V3(Top.X, Top.Y - RingDepth[4] - .15f, Top.Z);
  for(u32 Side = 0; Side < W3_IslandSides; Side += 1)
  {
   f32 Angle0 = 2.f*Pi32*(f32)Side/(f32)W3_IslandSides;
   f32 Angle1 = 2.f*Pi32*(f32)(Side + 1)/(f32)W3_IslandSides;
   v3 P0 = V3(Top.X + TipR*R3Cos(Angle0), Top.Y - RingDepth[4], Top.Z + TipR*R3Sin(Angle0));
   v3 P1 = V3(Top.X + TipR*R3Cos(Angle1), Top.Y - RingDepth[4], Top.Z + TipR*R3Sin(Angle1));
   R3PushTri(Opaque, P0, P1, TipCenter, V3(Top.X, Top.Y - 3.f, Top.Z), R3Color(W3_ColorStoneBottom));
  }
 }

 //- Terraces
 W3PushTerrace(Opaque, Top, Radius, .35f, W3_IslandSides, R3Color(W3_ColorLawn), R3Color(W3_ColorStoneTop));

 v3 Terrace1 = R3Add(Top, W3_Terrace1Offset);
 W3PushTerrace(Opaque, Terrace1, W3_Terrace1Radius, W3_TerraceWall + .05f, 16, R3Color(W3_ColorCropSoil), R3Color(W3_ColorTerraceWall));

 // Crop rows on the middle terrace, alternating wheat and greens.
 for(u32 Row = 0; Row < 4; Row += 1)
 {
  f32 Z = Terrace1.Z - .78f + .52f*(f32)Row;
  f32 HalfLength = .8f*R3Sqrt(Max(W3_Terrace1Radius*W3_Terrace1Radius - (Z - Terrace1.Z)*(Z - Terrace1.Z), .01f));
  u32 Color = (Row & 1) ? W3_ColorCropGreen : W3_ColorCropWheat;
  R3PushBox(Opaque, V3(Terrace1.X, Terrace1.Y + .07f, Z), V3(HalfLength, .07f, .12f), R3Color(Color));
 }

 v3 Terrace2 = R3Add(Top, W3_Terrace2Offset);
 W3PushTerrace(Opaque, Terrace2, W3_Terrace2Radius, W3_TerraceWall + .05f, 14, R3Color(W3_ColorLawn), R3Color(W3_ColorTerraceWall));

 // Greenhouse: a glass dome with a white rim.
 W3PushTerrace(Opaque, R3Add(Terrace2, V3(0.f, .06f, 0.f)), .62f, .06f, 14, R3Color(W3_ColorDomeRim), R3Color(W3_ColorDomeRim));
 R3PushEllipsoid(Translucent, R3Add(Terrace2, V3(0.f, .06f, 0.f)), V3(.58f, .62f, .58f), 6, 12,
                 R3WithAlpha(R3Color(W3_ColorGlass), .38f), false);

 //- Waterfall: a stream across the lawn that falls off the edge
 {
  f32 Angle = (RandomUnilateral(&Series) < .5f ? .15f : .85f)*Pi32 + .2f*RandomBilateral(&Series);
  v3 Out = V3(R3Cos(Angle), 0.f, R3Sin(Angle));
  v3 Side = V3(-Out.Z, 0.f, Out.X);
  v4 Water = R3WithAlpha(R3Color(W3_ColorWater), .75f);
  v4 Falling = R3WithAlpha(R3Color(W3_ColorWater), .5f);
  f32 Half = .17f;

  v3 StreamStart = R3Add(Top, R3Scale(Out, .55f*Radius));
  v3 Edge = R3Add(Top, R3Scale(Out, .98f*Radius));
  StreamStart.Y += .02f;
  Edge.Y += .02f;
  R3PushTriUnlit(Opaque, R3Add(StreamStart, R3Scale(Side, Half)), R3Add(Edge, R3Scale(Side, Half)), R3Add(Edge, R3Scale(Side, -Half)), Water);
  R3PushTriUnlit(Opaque, R3Add(StreamStart, R3Scale(Side, Half)), R3Add(Edge, R3Scale(Side, -Half)), R3Add(StreamStart, R3Scale(Side, -Half)), Water);

  v3 FallTop = R3Add(Edge, R3Scale(Out, .08f));
  v3 FallBottom = R3Add(R3Add(Top, R3Scale(Out, 1.12f*Radius)), V3(0.f, -3.2f, 0.f));
  R3PushTriUnlit(Translucent, R3Add(FallTop, R3Scale(Side, Half)), R3Add(FallBottom, R3Scale(Side, 1.6f*Half)), R3Add(FallBottom, R3Scale(Side, -1.6f*Half)), Falling);
  R3PushTriUnlit(Translucent, R3Add(FallTop, R3Scale(Side, Half)), R3Add(FallBottom, R3Scale(Side, -1.6f*Half)), R3Add(FallTop, R3Scale(Side, -Half)), Falling);
  R3PushEllipsoid(Translucent, FallBottom, V3(.55f, .3f, .55f), 4, 8, R3WithAlpha(R3Color(W3_ColorMist), .35f), true);
 }

 //- Rings around the rim: status (always) and hovered/selected (outside it)
 W3PushRing(Opaque, Top, 1.03f*Radius, 1.22f*Radius, R3Color(StatusColor));
 if(Highlight)
 {
  W3PushRing(Opaque, Top, 1.27f*Radius, 1.4f*Radius, R3Color(HighlightColor));
 }
}

// NOTE: Round fruit trees on both sides and solar panels at the back corners
// (the front of the lawn holds the crates).
internal void
W3PushDecorations(r3_mesh *Mesh, v3 Top, f32 Radius, u32 Index)
{
 random_series Series = {0};
 RandomSeed(&Series, W3_Seed*13 + 977*(u64)Index);

 for(u32 Tree = 0; Tree < 2; Tree += 1)
 {
  f32 SideSign = (Tree == 0) ? -1.f : 1.f;
  f32 Scale = .85f + .35f*RandomUnilateral(&Series);
  v3 Base = V3(Top.X + SideSign*.72f*Radius, Top.Y, Top.Z + .1f + .35f*RandomBilateral(&Series));
  R3PushCylinder(Mesh, Base, R3Add(Base, V3(0.f, .75f*Scale, 0.f)), .09f*Scale, 6, R3Color(W3_ColorTrunk));
  R3PushEllipsoid(Mesh, R3Add(Base, V3(0.f, 1.05f*Scale, 0.f)), V3(.5f*Scale, .45f*Scale, .5f*Scale), 5, 8,
                  W3Shade(R3Color(W3_ColorLeaves), &Series), false);

  // A few fruits
  for(u32 Fruit = 0; Fruit < 3; Fruit += 1)
  {
   f32 Angle = 2.f*Pi32*RandomUnilateral(&Series);
   v3 P = R3Add(Base, V3(.42f*Scale*R3Cos(Angle), (.95f + .25f*RandomUnilateral(&Series))*Scale, .42f*Scale*R3Sin(Angle)));
   R3PushEllipsoid(Mesh, P, V3(.07f, .07f, .07f), 3, 5, R3Color(W3_ColorFruit), false);
  }
 }

 for(u32 Panel = 0; Panel < 2; Panel += 1)
 {
  f32 SideSign = (Panel == 0) ? -1.f : 1.f;
  v3 Base = V3(Top.X + SideSign*.62f*Radius, Top.Y, Top.Z - .6f*Radius);
  v3 PanelCenter = R3Add(Base, V3(0.f, .55f, 0.f));
  R3PushCylinder(Mesh, Base, PanelCenter, .05f, 5, R3Color(W3_ColorPost));
  R3PushOrientedBox(Mesh, PanelCenter,
                    V3(.36f, 0.f, 0.f),
                    V3(0.f, .028f, -.016f),
                    V3(0.f, .14f, .25f),
                    R3Color(W3_ColorPanel));
 }
}

// NOTE: Green pad with solid crates = what the island produces.
//       Orange pad with empty crate frames = what the island needs.
internal void
W3PushProductsAndNeeds(r3_mesh *Opaque, r3_mesh *Translucent, island *Island, v3 Top)
{
 v3 ProducePad = R3Add(Top, V3(-.75f, .04f, .82f));
 v3 NeedPad = R3Add(Top, V3(.75f, .04f, .82f));
 v3 PadHalf = V3(.6f, .04f, .78f);
 R3PushBox(Opaque, ProducePad, PadHalf, R3Color(W3_ColorProducePad));
 R3PushBox(Opaque, NeedPad, PadHalf, R3Color(W3_ColorNeedPad));

 f32 PadTop = Top.Y + .08f;
 f32 Half = W3_CrateHalf;

 //- Produced: stacked solid crates
 u32 Slot = 0;
 for(product *Product = Island->FirstProduct; Product && Slot < 3; Product = Product->Next)
 {
  u32 Stack = Clamp(1, (u32)(Product->Amount/W3_CrateUnitsPerBox) + 1, W3_MaxCrateStack);
  f32 Z = ProducePad.Z - .5f + .62f*(f32)Slot;
  v4 Color = R3Color(W3ProductColors[Product->Kind]);
  for(u32 Level = 0; Level < Stack; Level += 1)
  {
   v3 Center = V3(ProducePad.X, PadTop + Half + 2.02f*Half*(f32)Level, Z);
   R3PushBox(Opaque, Center, V3(Half, Half, Half), Color);
  }
  Slot += 1;
 }

 //- Needed: crate frame (edges) with a see-through fill
 Slot = 0;
 for(product *Req = Island->FirstRequirement; Req && Slot < 3; Req = Req->Next)
 {
  f32 Z = NeedPad.Z - .5f + .62f*(f32)Slot;
  v4 Color = R3Color(W3ProductColors[Req->Kind]);
  v3 Center = V3(NeedPad.X, PadTop + Half, Z);

  R3PushBox(Translucent, Center, V3(Half, Half, Half), R3WithAlpha(Color, .38f));

  f32 Edge = .03f;
  for(u32 Axis = 0; Axis < 3; Axis += 1)
  {
   for(u32 Corner = 0; Corner < 4; Corner += 1)
   {
    f32 A = (Corner & 1) ? Half : -Half;
    f32 B = (Corner & 2) ? Half : -Half;
    v3 Offset = V3(0.f, 0.f, 0.f);
    v3 HalfSize = V3(Edge, Edge, Edge);
    if(Axis == 0) { Offset = V3(0.f, A, B); HalfSize.X = Half + Edge; }
    if(Axis == 1) { Offset = V3(A, 0.f, B); HalfSize.Y = Half + Edge; }
    if(Axis == 2) { Offset = V3(A, B, 0.f); HalfSize.Z = Half + Edge; }
    R3PushBox(Opaque, R3Add(Center, Offset), HalfSize, Color);
   }
  }
  Slot += 1;
 }
}

// NOTE: A crystal on a pole that turns from grey to cyan as the island learns about more islands.
internal void
W3PushKnowledgeCrystal(r3_mesh *Mesh, v3 Top, f32 KnownFraction)
{
 // NOTE: It sits on a thin mast on top of the greenhouse dome.
 v3 Center = R3Add(Top, W3_CrystalOffset);
 v3 Base = R3Add(R3Add(Top, W3_Terrace2Offset), V3(0.f, .65f, 0.f));
 R3PushCylinder(Mesh, Base, R3Add(Center, V3(0.f, -.3f, 0.f)), .04f, 5, R3Color(W3_ColorPost));

 v4 Color = R3LerpColor(R3Color(W3_ColorCrystalLow), R3Color(W3_ColorCrystalHigh), KnownFraction);
 f32 Size = .32f;
 R3PushCone(Mesh, Center, R3Add(Center, V3(0.f, 1.3f*Size, 0.f)), Size, 4, Color);
 R3PushCone(Mesh, Center, R3Add(Center, V3(0.f, -1.3f*Size, 0.f)), Size, 4, Color);
}

internal void
W3PushClouds(r3_mesh *Mesh)
{
 random_series Series = {0};
 RandomSeed(&Series, W3_Seed*3);

 // NOTE: Unlit and see-through, so clouds read as soft white puffs, not grey rocks.
 v4 Color = R3WithAlpha(R3Color(W3_ColorCloud), .42f);
 for(u32 Cloud = 0; Cloud < W3_CloudCount; Cloud += 1)
 {
  v3 Center = V3(70.f*RandomBilateral(&Series),
                 -13.f - 3.f*RandomUnilateral(&Series),
                 70.f*RandomBilateral(&Series));
  u32 Puffs = 3 + RandomNext(&Series) % 3;
  for(u32 Puff = 0; Puff < Puffs; Puff += 1)
  {
   v3 Offset = V3(3.5f*RandomBilateral(&Series), .6f*RandomBilateral(&Series), 2.5f*RandomBilateral(&Series));
   v3 Radii = V3(3.f + 3.f*RandomUnilateral(&Series), 1.2f + .8f*RandomUnilateral(&Series), 2.5f + 2.f*RandomUnilateral(&Series));
   R3PushEllipsoid(Mesh, R3Add(Center, Offset), Radii, 5, 9, Color, true);
  }
 }
}

//~ World
// NOTE: Positions, screen projections and the hovered island, before building geometry.
internal void
W3PrepareIslands(app_state *App, app_input *Input)
{
 r3_state *R3 = &App->R3;

 u32 Index = 0;
 for(island *Island = App->FirstIsland; Island && Index < R3_MaxIslands; Island = Island->Next)
 {
  v3 Top = W3IslandTopFromMap(Island->MapPos, Index);
  R3->IslandTop[Index] = Top;
  R3->IslandRadius[Index] = W3_IslandRadius;

  v2 Screen = V2(-1000.f, -1000.f);
  R3->IslandVisible[Index] = R3ProjectToScreen(R3, R3Add(Top, W3_LabelOffset), &Screen);
  R3->IslandScreen[Index] = Screen;

  // NOTE: The hover tooltip (DrawIslandTooltip) looks at Island->Pos.
  Island->Pos = (R3->IslandVisible[Index] ? Screen : V2(-1000.f, -1000.f));

  Index += 1;
 }
 R3->IslandCount = Index;

 //- Hovered island: closest label anchor or island top to the mouse.
 // Over the islands panel, the row under the mouse (last frame) is the hovered island.
 R3->HoveredIdx = -1;
 if(IsInsideRectV2(V2V2S32(Input->Mouse.Pos), App->Panel.LastRect))
 {
  R3->HoveredIdx = App->Panel.RowHoverPlusOne - 1;
 }
 else if(IsInsideRectV2(V2V2S32(Input->Mouse.Pos), R3->PopupRect))
 {
  R3->HoveredIdx = R3->PopupHoverPlusOne - 1;
 }
 else if(W3PointOnUI(R3, V2V2S32(Input->Mouse.Pos)))
 {
  // Over buttons, the legend, ...: no island is hovered.
 }
 else
 {
  v2 MousePos = V2V2S32(Input->Mouse.Pos);
  f32 BestDistanceSq = 60.f*60.f;
  for(u32 Idx = 0; Idx < R3->IslandCount; Idx += 1)
  {
   v2 TopScreen;
   if(R3->IslandVisible[Idx] && R3ProjectToScreen(R3, R3->IslandTop[Idx], &TopScreen))
   {
    v2 Candidates[2];
    Candidates[0] = R3->IslandScreen[Idx];
    Candidates[1] = TopScreen;
    for(u32 C = 0; C < 2; C += 1)
    {
     f32 DX = Candidates[C].X - MousePos.X;
     f32 DY = Candidates[C].Y - MousePos.Y;
     f32 DistanceSq = DX*DX + DY*DY;
     if(DistanceSq < BestDistanceSq)
     {
      BestDistanceSq = DistanceSq;
      R3->HoveredIdx = (s32)Idx;
     }
    }
   }
  }
 }
}

internal void
W3BuildWorld(app_state *App)
{
 r3_state *R3 = &App->R3;
 broadcast_state *Broadcast = &App->Broadcast;

 //- Islands
 u32 Index = 0;
 for(island *Island = App->FirstIsland; Island && Index < R3->IslandCount; Island = Island->Next)
 {
  v3 Top = R3->IslandTop[Index];
  f32 Radius = R3->IslandRadius[Index];

  b32 Hovered = (R3->HoveredIdx == (s32)Index);
  b32 Selected = (App->Panel.SelectedPlusOne == (s32)Index + 1);
  W3PushIslandBody(&R3->Opaque, &R3->Translucent, Top, Radius, Index, W3StatusColor(R3->IslandStatus[Index]),
                   Hovered || Selected, Hovered ? W3_ColorHover : W3_ColorSelected);
  W3PushDecorations(&R3->Opaque, Top, Radius, Index);
  W3PushProductsAndNeeds(&R3->Opaque, &R3->Translucent, Island, Top);

  f32 KnownFraction = 0.f;
  if(Broadcast->IslandCount > 1)
  {
   KnownFraction = ((f32)BroadcastKnownCount(App, Island) - 1.f)/((f32)Broadcast->IslandCount - 1.f);
  }
  W3PushKnowledgeCrystal(&R3->Opaque, Top, Clamp(0.f, KnownFraction, 1.f));

  Index += 1;
 }

 //- Cables and news pulses
 u32 FromIdx = 0;
 for(island *From = App->FirstIsland; From && FromIdx < R3->IslandCount; From = From->Next)
 {
  for(link *Link = From->FirstLink; Link; Link = Link->Next)
  {
   u32 ToIdx = BroadcastIslandIndex(App, Link->Dest);
   if(ToIdx < R3->IslandCount)
   {
    v3 TopA = R3->IslandTop[FromIdx];
    v3 TopB = R3->IslandTop[ToIdx];
    f32 RadiusA = R3->IslandRadius[FromIdx];
    f32 RadiusB = R3->IslandRadius[ToIdx];

    // Every link exists in both directions; draw the cable once.
    if(FromIdx < ToIdx)
    {
     b32 Highlight = (R3->HoveredIdx == (s32)FromIdx || R3->HoveredIdx == (s32)ToIdx);
     f32 Flash = BroadcastLinkFlash(App, From, Link->Dest);
     v4 Color = R3LerpColor(R3Color(W3_ColorCable), R3Color(W3_ColorCableLit), Flash);
     f32 Radius = W3_CableRadius;
     if(Highlight)
     {
      Color = R3Color(W3_ColorCableHover);
      Radius *= 1.6f;
     }

     for(u32 Segment = 0; Segment < W3_CableSegments; Segment += 1)
     {
      f32 T0 = (f32)Segment/(f32)W3_CableSegments;
      f32 T1 = (f32)(Segment + 1)/(f32)W3_CableSegments;
      R3PushCylinder(&R3->Opaque, W3CablePoint(TopA, RadiusA, TopB, RadiusB, T0),
                     W3CablePoint(TopA, RadiusA, TopB, RadiusB, T1), Radius, 5, Color);
     }
    }

    // A pulse travels from sender to receiver while this direction carries news.
    if(FromIdx < Broadcast_MaxIslands && ToIdx < Broadcast_MaxIslands)
    {
     f32 Flash = Broadcast->Flash[FromIdx][ToIdx];
     if(Flash > 0.f)
     {
      f32 Progress = Clamp(0.f, 1.f - Flash/Broadcast_FlashSeconds, 1.f);
      v3 P = W3CablePoint(TopA, RadiusA, TopB, RadiusB, Progress);
      R3PushEllipsoid(&R3->Opaque, P, V3(.32f, .32f, .32f), 5, 8, R3Color(W3_ColorPulse), true);
      R3PushEllipsoid(&R3->Translucent, P, V3(.75f, .75f, .75f), 5, 8, R3WithAlpha(R3Color(W3_ColorPulse), .3f), true);
     }
    }
   }
  }
  FromIdx += 1;
 }

 //- Clouds below the islands
 W3PushClouds(&R3->Translucent);
}
