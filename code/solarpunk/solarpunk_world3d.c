//-
// SolarPunk 3D world: floating sky islands, product crates, cables and news pulses.
//-

//~ Constants
#define W3_WorldSize        150.f   // map 0..1 is spread over this many world units
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
#define W3_ColorTerraceWall 0xFFF2E2C4
#define W3_ColorCropSoil    0xFF8A6A4A
#define W3_ColorCropWheat   0xFFE8C66A
#define W3_ColorCropGreen   0xFF6FAE5A
#define W3_ColorDomeRim     0xFFF4F1EA
#define W3_ColorGlass       0xFFCDEFFF
#define W3_ColorWater       0xFF7FCBEA
#define W3_ColorMist        0xFFFFFFFF
#define W3_ColorTrunk       0xFF7A5230
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
 0xFFE0473A, // Apples
 0xFFC9D650, // Pears
 0xFF5FB55A, // Cabbage
 0xFFF08A2C, // Carrots
 0xFFE8C66A, // Wheat
 0xFF6FA8DC, // Fish
 0xFF9B3D33, // Meat
 0xFFB98A5E, // Eggs
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

// NOTE: The state of one need of an island, in the same colours as the island status:
//   Waiting : no supplier heard of yet
//   Short   : the whole world does not make enough of it
//   Ok      : a supplier is known and there is enough
internal u32
W3NeedStatus(app_state *App, u32 IslandIdx, product_kind Kind)
{
 u32 Result = IslandStatus_Ok;
 if(!W3KnowsSupplier(App, IslandIdx, Kind))                          Result = IslandStatus_Waiting;
 else if(App->R3.TotalNeeded[Kind] > App->R3.TotalProduced[Kind])   Result = IslandStatus_Short;
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
 b32 Result = R3->HelpOpen;
 for(u32 Idx = 0; Idx < R3->UIRectCount && !Result; Idx += 1)
 {
  Result = IsInsideRectV2(P, R3->UIRects[Idx]);
 }
 return Result;
}

//~ Island pieces
// NOTE: A terraced garden island: a rock base in its cluster's colours, three garden
// terraces (the lowest holds the crates, the middle one shows what the island makes,
// the top one a glass greenhouse dome) and a small waterfall spilling off one edge.

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

//~ Cluster themes
// NOTE: Every cluster has its own look: rock colours, lawn, trees, how deep and rough
// the rock under the island is, and an extra (hanging vines or crystals).
enum w3_theme_extra
{
 W3Extra_None,
 W3Extra_Vines,
 W3Extra_Crystals,
};

typedef struct w3_theme w3_theme;
struct w3_theme
{
 u32 Stone[4];   // rock from top to bottom
 u32 Lawn;
 u32 Wall;       // terrace walls
 u32 Leaves;
 u32 Fruit;
 f32 Depth;      // how deep the rock goes (1 = normal)
 f32 Rough;      // how much the rock's outline varies (0 = round)
 u32 Extra;      // w3_theme_extra
};

// NOTE: In the same order as WorldGenClusterNames.
global_variable w3_theme W3ClusterThemes[] =
{
 // Verdant Reach: pale stone, lush lawn, hanging vines
 {{0xFFF6E8CC, 0xFFEBD6B2, 0xFFDCC19A, 0xFFC9A883}, 0xFF96D36E, 0xFFF2E2C4, 0xFF4F9D5A, 0xFFF08A5D, 1.f,   0.f,  W3Extra_Vines},
 // Sunward Isles: golden sandstone, flat mesas
 {{0xFFF7DFA0, 0xFFEBC77A, 0xFFD9AA5C, 0xFFC08A45}, 0xFFB9D26A, 0xFFF5E3B0, 0xFF86B84A, 0xFFF2C14E, .7f,   .03f, W3Extra_None},
 // Driftwood Shoals: grey-blue sea rock, rough
 {{0xFFD5DCE0, 0xFFB4C0C8, 0xFF94A3AE, 0xFF76858F}, 0xFF7CC7A0, 0xFFE2E7EA, 0xFF3E8E6E, 0xFFF5F5F5, 1.15f, .12f, W3Extra_None},
 // Ember Archipelago: red rock, autumn trees
 {{0xFFE9A27C, 0xFFD5825C, 0xFFB8623F, 0xFF8F4A30}, 0xFFA8C060, 0xFFF0C3A6, 0xFFE0752E, 0xFFF2D04E, 1.05f, .07f, W3Extra_None},
 // Crystal Spires: lavender stone, crystals under the rock
 {{0xFFE6DDF2, 0xFFCDBFE6, 0xFFB3A1D6, 0xFF937FC0}, 0xFF9FE0B8, 0xFFEFE8F8, 0xFF5AAE8A, 0xFFB58CF0, 1.f,   .03f, W3Extra_Crystals},
 // Mistral Keys: white chalk, tall cliffs
 {{0xFFFBFBF6, 0xFFECECE4, 0xFFD9D9CE, 0xFFBDBDB0}, 0xFF8ED67A, 0xFFFFFFFF, 0xFF4E9A7A, 0xFF7FB3E8, 1.45f, .05f, W3Extra_None},
 // Saltglass Atolls: coral pink stone, blossom trees
 {{0xFFF8D9D0, 0xFFEFBFB2, 0xFFE0A396, 0xFFC9867A}, 0xFF8CD3B5, 0xFFFBE7E1, 0xFFF29BB8, 0xFFFFFFFF, .85f,  .05f, W3Extra_Vines},
};
StaticAssert(ArrayCount(W3ClusterThemes) == WorldGen_ClusterCount, W3ClusterThemesMatch);

internal w3_theme *
W3ThemeFor(app_state *App, u32 IslandIdx, u32 IslandCount)
{
 u32 Cluster = App->HasMapLayout ? WorldGenClusterOf(IslandIdx, IslandCount) : 0;
 w3_theme *Result = &W3ClusterThemes[Cluster % ArrayCount(W3ClusterThemes)];
 return Result;
}

//~ Island body
internal void
W3PushIslandBody(r3_mesh *Opaque, r3_mesh *Translucent, w3_theme *Theme, v3 Top, f32 Radius, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor)
{
 random_series Series = {0};
 RandomSeed(&Series, W3_Seed*7 + 101*(u64)Index);

 //- Rock base: a rounded bowl under the lawn; rough themes vary its outline
 f32 RingRadius[5] = {1.f, .93f, .74f, .45f, .12f};
 f32 RingDepth[5]  = {.35f, 1.2f, 2.3f, 3.3f, 4.f};
 for(u32 Ring = 1; Ring < 5; Ring += 1) RingDepth[Ring] *= Theme->Depth;

 f32 Rough[5][W3_IslandSides];
 for(u32 Ring = 0; Ring < 5; Ring += 1)
 {
  for(u32 Side = 0; Side < W3_IslandSides; Side += 1)
  {
   // The top ring stays round, so it meets the lawn.
   Rough[Ring][Side] = (Ring == 0) ? 1.f : 1.f + Theme->Rough*RandomBilateral(&Series);
  }
 }

 for(u32 Side = 0; Side < W3_IslandSides; Side += 1)
 {
  u32 Next = (Side + 1) % W3_IslandSides;
  f32 Angle0 = 2.f*Pi32*(f32)Side/(f32)W3_IslandSides;
  f32 Angle1 = 2.f*Pi32*(f32)(Side + 1)/(f32)W3_IslandSides;
  for(u32 Ring = 0; Ring < 4; Ring += 1)
  {
   f32 R00 = Radius*RingRadius[Ring]*Rough[Ring][Side];
   f32 R01 = Radius*RingRadius[Ring]*Rough[Ring][Next];
   f32 R10 = Radius*RingRadius[Ring + 1]*Rough[Ring + 1][Side];
   f32 R11 = Radius*RingRadius[Ring + 1]*Rough[Ring + 1][Next];
   f32 Y0 = Top.Y - RingDepth[Ring];
   f32 Y1 = Top.Y - RingDepth[Ring + 1];
   v3 P0 = V3(Top.X + R00*R3Cos(Angle0), Y0, Top.Z + R00*R3Sin(Angle0));
   v3 P1 = V3(Top.X + R01*R3Cos(Angle1), Y0, Top.Z + R01*R3Sin(Angle1));
   v3 P2 = V3(Top.X + R11*R3Cos(Angle1), Y1, Top.Z + R11*R3Sin(Angle1));
   v3 P3 = V3(Top.X + R10*R3Cos(Angle0), Y1, Top.Z + R10*R3Sin(Angle0));
   R3PushQuad(Opaque, P0, P1, P2, P3, V3(Top.X, .5f*(Y0 + Y1), Top.Z), W3Shade(R3Color(Theme->Stone[Ring]), &Series));
  }

  // Close the bottom.
  f32 TipR0 = Radius*RingRadius[4]*Rough[4][Side];
  f32 TipR1 = Radius*RingRadius[4]*Rough[4][Next];
  f32 TipY = Top.Y - RingDepth[4];
  v3 TipCenter = V3(Top.X, TipY - .15f, Top.Z);
  v3 P0 = V3(Top.X + TipR0*R3Cos(Angle0), TipY, Top.Z + TipR0*R3Sin(Angle0));
  v3 P1 = V3(Top.X + TipR1*R3Cos(Angle1), TipY, Top.Z + TipR1*R3Sin(Angle1));
  R3PushTri(Opaque, P0, P1, TipCenter, V3(Top.X, TipY + 1.f, Top.Z), R3Color(Theme->Stone[3]));
 }

 //- Theme extras under the rock
 if(Theme->Extra == W3Extra_Vines)
 {
  for(u32 Vine = 0; Vine < 7; Vine += 1)
  {
   f32 Angle = 2.f*Pi32*((f32)Vine + .6f*RandomUnilateral(&Series))/7.f;
   f32 Length = (.8f + 1.4f*RandomUnilateral(&Series))*Theme->Depth;
   v3 Start = V3(Top.X + .97f*Radius*R3Cos(Angle), Top.Y - .3f, Top.Z + .97f*Radius*R3Sin(Angle));
   v3 End = V3(Start.X, Start.Y - Length, Start.Z);
   R3PushCylinder(Opaque, Start, End, .045f, 4, R3Color(Theme->Leaves));
   R3PushEllipsoid(Opaque, End, V3(.16f, .2f, .16f), 3, 5, W3Shade(R3Color(Theme->Leaves), &Series), false);
  }
 }
 else if(Theme->Extra == W3Extra_Crystals)
 {
  for(u32 Crystal = 0; Crystal < 5; Crystal += 1)
  {
   f32 Angle = 2.f*Pi32*RandomUnilateral(&Series);
   f32 Out = .25f*Radius*RandomUnilateral(&Series);
   f32 Length = 1.2f + 1.6f*RandomUnilateral(&Series);
   v3 Base = V3(Top.X + Out*R3Cos(Angle), Top.Y - RingDepth[3] + .3f, Top.Z + Out*R3Sin(Angle));
   v3 Tip = V3(Base.X + .3f*RandomBilateral(&Series), Base.Y - Length, Base.Z + .3f*RandomBilateral(&Series));
   R3PushCone(Translucent, Base, Tip, .32f, 5, R3WithAlpha(R3Color(0xFF9FD8FF), .65f));
  }
 }

 //- Lawn and the top terrace with the greenhouse
 W3PushTerrace(Opaque, Top, Radius, .35f, W3_IslandSides, R3Color(Theme->Lawn), R3Color(Theme->Stone[0]));

 v3 Terrace2 = R3Add(Top, W3_Terrace2Offset);
 W3PushTerrace(Opaque, Terrace2, W3_Terrace2Radius, W3_TerraceWall + .05f, 14, R3Color(Theme->Lawn), R3Color(Theme->Wall));

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
  f32 FallDepth = 3.2f*Theme->Depth;

  v3 StreamStart = R3Add(Top, R3Scale(Out, .55f*Radius));
  v3 Edge = R3Add(Top, R3Scale(Out, .98f*Radius));
  StreamStart.Y += .02f;
  Edge.Y += .02f;
  R3PushTriUnlit(Opaque, R3Add(StreamStart, R3Scale(Side, Half)), R3Add(Edge, R3Scale(Side, Half)), R3Add(Edge, R3Scale(Side, -Half)), Water);
  R3PushTriUnlit(Opaque, R3Add(StreamStart, R3Scale(Side, Half)), R3Add(Edge, R3Scale(Side, -Half)), R3Add(StreamStart, R3Scale(Side, -Half)), Water);

  v3 FallTop = R3Add(Edge, R3Scale(Out, .08f));
  v3 FallBottom = R3Add(R3Add(Top, R3Scale(Out, 1.12f*Radius)), V3(0.f, -FallDepth, 0.f));
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

//~ What an island makes, shown on its terraces
// NOTE: The island's main food (the one it makes most of) fills the middle terrace;
// a second food gets something small on the back corners of the lawn. An island that
// catches fish also gets a dock with a sky boat.
#define W3_ColorDarkWood  0xFF6E4A2C
#define W3_ColorSail      0xFFFFF8EC
#define W3_ColorBarn      0xFFB5523B
#define W3_ColorRoof      0xFF6B4A3A
#define W3_ColorCow       0xFFF4EFE6
#define W3_ColorCowSpot   0xFF4A3A30
#define W3_ColorHen       0xFFFFFFFF
#define W3_ColorComb      0xFFE0453A
#define W3_ColorFarmSoil  0xFF8A6A4A
#define W3_ColorPasture   0xFF9BD27A

// NOTE: The greenhouse terrace covers the back of the middle terrace, so things go on
// the visible crescent: angles from about -30 to 210 degrees (0 = +X, 90 = front).
internal v3
W3TerracePoint(v3 T, f32 Degrees, f32 Distance)
{
 f32 Angle = Degrees*Pi32/180.f;
 v3 Result = R3Add(T, V3(Distance*R3Cos(Angle), 0.f, Distance*R3Sin(Angle)));
 return Result;
}

// NOTE: A round fruit tree; Pear makes the fruits pear-shaped.
internal void
W3PushFruitTree(r3_mesh *Mesh, v3 Base, f32 Scale, u32 LeafColor, product_kind Fruit, random_series *Series)
{
 R3PushCylinder(Mesh, Base, R3Add(Base, V3(0.f, .55f*Scale, 0.f)), .07f*Scale, 6, R3Color(W3_ColorTrunk));
 R3PushEllipsoid(Mesh, R3Add(Base, V3(0.f, .8f*Scale, 0.f)), V3(.4f*Scale, .36f*Scale, .4f*Scale), 5, 8,
                 W3Shade(R3Color(LeafColor), Series), false);
 for(u32 Idx = 0; Idx < 5; Idx += 1)
 {
  f32 Angle = 2.f*Pi32*RandomUnilateral(Series);
  v3 P = R3Add(Base, V3(.36f*Scale*R3Cos(Angle), (.7f + .25f*RandomUnilateral(Series))*Scale, .36f*Scale*R3Sin(Angle)));
  v3 Radii = (Fruit == ProductKind_Pears) ? V3(.06f, .09f, .06f) : V3(.075f, .075f, .075f);
  R3PushEllipsoid(Mesh, P, Radii, 3, 5, R3Color(W3ProductColors[Fruit]), false);
 }
}

internal void
W3PushCabbage(r3_mesh *Mesh, v3 Base, random_series *Series)
{
 R3PushEllipsoid(Mesh, R3Add(Base, V3(0.f, .1f, 0.f)), V3(.15f, .12f, .15f), 4, 7, W3Shade(R3Color(W3ProductColors[ProductKind_Cabbage]), Series), false);
 R3PushEllipsoid(Mesh, R3Add(Base, V3(0.f, .13f, 0.f)), V3(.09f, .09f, .09f), 3, 6, R3Color(0xFFA8DC7E), false);
}

internal void
W3PushCarrot(r3_mesh *Mesh, v3 Base)
{
 R3PushCone(Mesh, R3Add(Base, V3(0.f, .08f, 0.f)), R3Add(Base, V3(0.f, -.1f, 0.f)), .06f, 5, R3Color(W3ProductColors[ProductKind_Carrots]));
 R3PushCone(Mesh, R3Add(Base, V3(-.03f, .07f, 0.f)), R3Add(Base, V3(-.06f, .28f, 0.f)), .03f, 4, R3Color(0xFF4F9D3A));
 R3PushCone(Mesh, R3Add(Base, V3(.03f, .07f, 0.f)), R3Add(Base, V3(.06f, .26f, .02f)), .03f, 4, R3Color(0xFF4F9D3A));
}

// NOTE: A cow, facing +X (or -X with Facing = -1).
internal void
W3PushCow(r3_mesh *Mesh, v3 Base, f32 Facing)
{
 v3 Body = R3Add(Base, V3(0.f, .3f, 0.f));
 R3PushBox(Mesh, Body, V3(.24f, .12f, .12f), R3Color(W3_ColorCow));
 R3PushBox(Mesh, R3Add(Body, V3(-.05f*Facing, .07f, 0.f)), V3(.08f, .06f, .125f), R3Color(W3_ColorCowSpot));
 R3PushBox(Mesh, R3Add(Body, V3(.3f*Facing, .06f, 0.f)), V3(.08f, .08f, .08f), R3Color(W3_ColorCow));
 R3PushBox(Mesh, R3Add(Body, V3(.37f*Facing, .03f, 0.f)), V3(.025f, .045f, .06f), R3Color(0xFFE8B4A6));
 for(u32 Leg = 0; Leg < 4; Leg += 1)
 {
  f32 DX = (Leg & 1) ? .17f : -.17f;
  f32 DZ = (Leg & 2) ? .08f : -.08f;
  R3PushBox(Mesh, R3Add(Base, V3(DX, .09f, DZ)), V3(.03f, .09f, .03f), R3Color(W3_ColorCowSpot));
 }
}

internal void
W3PushHen(r3_mesh *Mesh, v3 Base, f32 Facing)
{
 R3PushEllipsoid(Mesh, R3Add(Base, V3(0.f, .1f, 0.f)), V3(.11f, .09f, .08f), 4, 6, R3Color(W3_ColorHen), false);
 R3PushEllipsoid(Mesh, R3Add(Base, V3(.09f*Facing, .2f, 0.f)), V3(.05f, .05f, .05f), 3, 5, R3Color(W3_ColorHen), false);
 R3PushBox(Mesh, R3Add(Base, V3(.09f*Facing, .26f, 0.f)), V3(.02f, .02f, .01f), R3Color(W3_ColorComb));
 R3PushCone(Mesh, R3Add(Base, V3(.13f*Facing, .2f, 0.f)), R3Add(Base, V3(.18f*Facing, .19f, 0.f)), .02f, 4, R3Color(0xFFF2B33D));
}

// NOTE: A little house: walls and a pitched roof, ridge along X.
internal void
W3PushHut(r3_mesh *Mesh, v3 Base, v3 Half, u32 WallColor)
{
 v3 Center = R3Add(Base, V3(0.f, Half.Y, 0.f));
 R3PushBox(Mesh, Center, Half, R3Color(WallColor));
 f32 Y0 = Base.Y + 2.f*Half.Y;
 f32 Y1 = Y0 + .8f*Half.Z;
 v3 A0 = V3(Base.X - Half.X - .04f, Y0, Base.Z - Half.Z - .05f);
 v3 A1 = V3(Base.X + Half.X + .04f, Y0, Base.Z - Half.Z - .05f);
 v3 B0 = V3(Base.X - Half.X - .04f, Y0, Base.Z + Half.Z + .05f);
 v3 B1 = V3(Base.X + Half.X + .04f, Y0, Base.Z + Half.Z + .05f);
 v3 R0 = V3(Base.X - Half.X - .04f, Y1, Base.Z);
 v3 R1 = V3(Base.X + Half.X + .04f, Y1, Base.Z);
 v3 Inside = V3(Base.X, Y0, Base.Z);
 R3PushQuad(Mesh, A0, A1, R1, R0, Inside, R3Color(W3_ColorRoof));
 R3PushQuad(Mesh, B0, B1, R1, R0, Inside, R3Color(W3_ColorRoof));
 R3PushTri(Mesh, A0, B0, R0, R3Add(Inside, V3(1.f, 0.f, 0.f)), R3Color(WallColor));
 R3PushTri(Mesh, A1, B1, R1, R3Add(Inside, V3(-1.f, 0.f, 0.f)), R3Color(WallColor));
}

// NOTE: The middle terrace, filled for the island's main food. T is the centre of the
// terrace's top.
internal void
W3PushMainProduct(r3_mesh *Opaque, w3_theme *Theme, product_kind Kind, v3 T, random_series *Series)
{
 f32 R = W3_Terrace1Radius;
 f32 Wall = W3_TerraceWall + .05f;
 v4 WallColor = R3Color(Theme->Wall);

 switch(Kind)
 {
  case ProductKind_Apples:
  case ProductKind_Pears:
  {
   // An orchard.
   W3PushTerrace(Opaque, T, R, Wall, 16, R3Color(Theme->Lawn), WallColor);
   u32 Leaves = (Kind == ProductKind_Pears) ? 0xFF6FAE4A : 0xFF3F8E4A;
   for(u32 Tree = 0; Tree < 5; Tree += 1)
   {
    W3PushFruitTree(Opaque, W3TerracePoint(T, -15.f + 52.f*(f32)Tree, (Tree & 1) ? .7f : 1.05f), 1.f + .25f*RandomUnilateral(Series), Leaves, Kind, Series);
   }
  } break;

  case ProductKind_Cabbage:
  {
   W3PushTerrace(Opaque, T, R, Wall, 16, R3Color(W3_ColorFarmSoil), WallColor);
   for(u32 Ring = 0; Ring < 2; Ring += 1)
   {
    for(u32 Head = 0; Head < 8; Head += 1)
    {
     W3PushCabbage(Opaque, W3TerracePoint(T, -20.f + 31.f*(f32)Head + 15.f*(f32)Ring, Ring ? .75f : 1.1f), Series);
    }
   }
  } break;

  case ProductKind_Carrots:
  {
   W3PushTerrace(Opaque, T, R, Wall, 16, R3Color(W3_ColorFarmSoil), WallColor);
   for(u32 Ring = 0; Ring < 3; Ring += 1)
   {
    for(u32 Carrot = 0; Carrot < 11; Carrot += 1)
    {
     W3PushCarrot(Opaque, W3TerracePoint(T, -20.f + 22.f*(f32)Carrot + 8.f*(f32)Ring, .7f + .2f*(f32)Ring));
    }
   }
  } break;

  case ProductKind_Fish:
  {
   // A fish pond, with fish.
   W3PushTerrace(Opaque, T, R, Wall, 16, R3Color(W3_ColorWater), WallColor);
   for(u32 Fish = 0; Fish < 7; Fish += 1)
   {
    v3 P = W3TerracePoint(T, -20.f + 220.f*RandomUnilateral(Series), .85f + .3f*RandomUnilateral(Series));
    P.Y += .04f;
    R3PushEllipsoid(Opaque, P, V3(.16f, .05f, .07f), 3, 6, R3Color(W3ProductColors[ProductKind_Fish]), false);
   }
  } break;

  case ProductKind_Meat:
  {
   // A pasture with cows and a barn.
   W3PushTerrace(Opaque, T, R, Wall, 16, R3Color(W3_ColorPasture), WallColor);
   W3PushHut(Opaque, W3TerracePoint(T, 170.f, .95f), V3(.32f, .22f, .26f), W3_ColorBarn);
   W3PushCow(Opaque, W3TerracePoint(T, 10.f, .95f), -1.f);
   W3PushCow(Opaque, W3TerracePoint(T, 60.f, 1.f), 1.f);
   W3PushCow(Opaque, W3TerracePoint(T, 110.f, .9f), -1.f);
  } break;

  case ProductKind_Eggs:
  {
   // A hen house with hens around it.
   W3PushTerrace(Opaque, T, R, Wall, 16, R3Color(W3_ColorPasture), WallColor);
   W3PushHut(Opaque, W3TerracePoint(T, 15.f, .95f), V3(.25f, .18f, .2f), 0xFFE9C98F);
   for(u32 Hen = 0; Hen < 6; Hen += 1)
   {
    W3PushHen(Opaque, W3TerracePoint(T, 50.f + 28.f*(f32)Hen, (Hen & 1) ? .8f : 1.1f), (Hen & 1) ? 1.f : -1.f);
   }
  } break;

  case ProductKind_Wheat:
  default:
  {
   // A wheat field with a windmill.
   W3PushTerrace(Opaque, T, R, Wall, 16, R3Color(W3_ColorFarmSoil), WallColor);
   for(u32 Row = 0; Row < 4; Row += 1)
   {
    f32 Z = T.Z - .78f + .52f*(f32)Row;
    f32 HalfLength = .8f*R3Sqrt(Max(R*R - (Z - T.Z)*(Z - T.Z), .01f));
    R3PushBox(Opaque, V3(T.X, T.Y + .1f, Z), V3(HalfLength, .1f, .14f), W3Shade(R3Color(W3ProductColors[ProductKind_Wheat]), Series));
   }
   v3 Mill = W3TerracePoint(T, 170.f, 1.f);
   R3PushCone(Opaque, Mill, R3Add(Mill, V3(0.f, 1.3f, 0.f)), .28f, 8, R3Color(0xFFF4EBDD));
   v3 Hub = R3Add(Mill, V3(0.f, 1.05f, .25f));
   for(u32 Blade = 0; Blade < 4; Blade += 1)
   {
    f32 Angle = .4f + .5f*Pi32*(f32)Blade;
    v3 Dir = V3(R3Cos(Angle), R3Sin(Angle), 0.f);
    R3PushOrientedBox(Opaque, R3Add(Hub, R3Scale(Dir, .32f)), R3Scale(Dir, .3f), V3(-Dir.Y*.07f, Dir.X*.07f, 0.f), V3(0.f, 0.f, .015f), R3Color(0xFFD9C4A0));
   }
  } break;
 }
}

// NOTE: Something small for a second food (or a bush when there is none), on one of
// the back corners of the lawn.
internal void
W3PushSmallProduct(r3_mesh *Opaque, w3_theme *Theme, product_kind Kind, v3 Base, random_series *Series)
{
 switch(Kind)
 {
  case ProductKind_Apples:  W3PushFruitTree(Opaque, Base, 1.f, 0xFF3F8E4A, ProductKind_Apples, Series); break;
  case ProductKind_Pears:   W3PushFruitTree(Opaque, Base, 1.f, 0xFF6FAE4A, ProductKind_Pears, Series); break;
  case ProductKind_Cabbage:
  {
   for(u32 Head = 0; Head < 3; Head += 1) W3PushCabbage(Opaque, R3Add(Base, V3(-.3f + .3f*(f32)Head, 0.f, .1f*(f32)(Head & 1))), Series);
  } break;
  case ProductKind_Carrots:
  {
   for(u32 Carrot = 0; Carrot < 6; Carrot += 1) W3PushCarrot(Opaque, R3Add(Base, V3(-.3f + .12f*(f32)Carrot, 0.f, (Carrot & 1) ? .1f : -.1f)));
  } break;
  case ProductKind_Wheat:
  {
   // Hay bales.
   R3PushCylinder(Opaque, R3Add(Base, V3(-.2f, .17f, -.15f)), R3Add(Base, V3(-.2f, .17f, .15f)), .17f, 8, R3Color(W3ProductColors[ProductKind_Wheat]));
   R3PushCylinder(Opaque, R3Add(Base, V3(.2f, .17f, -.15f)), R3Add(Base, V3(.2f, .17f, .15f)), .17f, 8, W3Shade(R3Color(W3ProductColors[ProductKind_Wheat]), Series));
  } break;
  case ProductKind_Fish:
  {
   // A drying rack with fish.
   R3PushCylinder(Opaque, R3Add(Base, V3(-.3f, 0.f, 0.f)), R3Add(Base, V3(-.3f, .6f, 0.f)), .03f, 4, R3Color(W3_ColorDarkWood));
   R3PushCylinder(Opaque, R3Add(Base, V3(.3f, 0.f, 0.f)), R3Add(Base, V3(.3f, .6f, 0.f)), .03f, 4, R3Color(W3_ColorDarkWood));
   R3PushCylinder(Opaque, R3Add(Base, V3(-.32f, .58f, 0.f)), R3Add(Base, V3(.32f, .58f, 0.f)), .025f, 4, R3Color(W3_ColorDarkWood));
   for(u32 Fish = 0; Fish < 3; Fish += 1)
   {
    R3PushEllipsoid(Opaque, R3Add(Base, V3(-.18f + .18f*(f32)Fish, .42f, 0.f)), V3(.05f, .13f, .03f), 3, 5, R3Color(W3ProductColors[ProductKind_Fish]), false);
   }
  } break;
  case ProductKind_Meat:  W3PushCow(Opaque, Base, 1.f); break;
  case ProductKind_Eggs:
  {
   W3PushHen(Opaque, R3Add(Base, V3(-.15f, 0.f, 0.f)), 1.f);
   W3PushHen(Opaque, R3Add(Base, V3(.18f, 0.f, .1f)), -1.f);
  } break;
  default:
  {
   // A flowering bush.
   R3PushEllipsoid(Opaque, R3Add(Base, V3(0.f, .2f, 0.f)), V3(.3f, .24f, .3f), 4, 7, W3Shade(R3Color(Theme->Leaves), Series), false);
   for(u32 Flower = 0; Flower < 4; Flower += 1)
   {
    f32 Angle = 2.f*Pi32*RandomUnilateral(Series);
    R3PushEllipsoid(Opaque, R3Add(Base, V3(.26f*R3Cos(Angle), .28f, .26f*R3Sin(Angle))), V3(.06f, .06f, .06f), 3, 5, R3Color(Theme->Fruit), false);
   }
  } break;
 }
}

// NOTE: A wooden dock out from the island's side, with a little sky boat moored at it.
internal void
W3PushDock(r3_mesh *Mesh, v3 Top, f32 Radius, f32 SideSign)
{
 v3 Out = V3(SideSign, 0.f, 0.f);
 v3 Start = R3Add(Top, V3(SideSign*.85f*Radius, -.1f, .35f*Radius));
 v3 End = R3Add(Start, R3Scale(Out, 1.6f));
 v3 Mid = R3Scale(R3Add(Start, End), .5f);
 R3PushBox(Mesh, Mid, V3(.82f, .05f, .2f), R3Color(W3_ColorDarkWood));
 R3PushCylinder(Mesh, R3Add(End, V3(0.f, -.6f, -.2f)), R3Add(End, V3(0.f, .25f, -.2f)), .05f, 5, R3Color(W3_ColorDarkWood));
 R3PushCylinder(Mesh, R3Add(End, V3(0.f, -.6f, .2f)), R3Add(End, V3(0.f, .25f, .2f)), .05f, 5, R3Color(W3_ColorDarkWood));

 // Boat beside the end of the dock: a hull, a mast and a sail.
 v3 Boat = R3Add(End, V3(-SideSign*.3f, -.15f, .65f));
 R3PushEllipsoid(Mesh, Boat, V3(.7f, .2f, .26f), 4, 8, R3Color(0xFF9C6B43), false);
 R3PushBox(Mesh, R3Add(Boat, V3(0.f, .1f, 0.f)), V3(.55f, .03f, .18f), R3Color(0xFFD9B98F));
 v3 MastTop = R3Add(Boat, V3(0.f, 1.f, 0.f));
 R3PushCylinder(Mesh, R3Add(Boat, V3(0.f, .1f, 0.f)), MastTop, .03f, 4, R3Color(W3_ColorDarkWood));
 R3PushTri(Mesh, R3Add(Boat, V3(.05f, .25f, 0.f)), R3Add(MastTop, V3(.05f, -.05f, 0.f)), R3Add(Boat, V3(.55f, .25f, 0.f)),
           R3Add(Boat, V3(.2f, .4f, -1.f)), R3Color(W3_ColorSail));
 R3PushTri(Mesh, R3Add(Boat, V3(.05f, .25f, 0.f)), R3Add(MastTop, V3(.05f, -.05f, 0.f)), R3Add(Boat, V3(.55f, .25f, 0.f)),
           R3Add(Boat, V3(.2f, .4f, 1.f)), R3Color(W3_ColorSail));
}

// NOTE: Trees on both sides (in the cluster's colours), what the island makes, and a
// dock when it catches fish. The front of the lawn holds the crates.
internal void
W3PushDecorations(r3_mesh *Opaque, r3_mesh *Translucent, w3_theme *Theme, island *Island, v3 Top, f32 Radius, u32 Index)
{
 random_series Series = {0};
 RandomSeed(&Series, W3_Seed*13 + 977*(u64)Index);

 //- Main and second product (main = the one it makes most of)
 product_kind Main = ProductKind_None;
 product_kind Second = ProductKind_None;
 u64 MainAmount = 0;
 for(product *Product = Island->FirstProduct; Product; Product = Product->Next)
 {
  if(Main == ProductKind_None || Product->Amount > MainAmount)
  {
   Second = Main;
   Main = Product->Kind;
   MainAmount = Product->Amount;
  }
  else if(Second == ProductKind_None)
  {
   Second = Product->Kind;
  }
 }

 W3PushMainProduct(Opaque, Theme, Main, R3Add(Top, W3_Terrace1Offset), &Series);

 b32 MakesFish = (Main == ProductKind_Fish || Second == ProductKind_Fish);
 for(u32 Corner = 0; Corner < 2; Corner += 1)
 {
  f32 SideSign = (Corner == 0) ? -1.f : 1.f;
  v3 Base = V3(Top.X + SideSign*.62f*Radius, Top.Y, Top.Z - .6f*Radius);
  W3PushSmallProduct(Opaque, Theme, (Second == ProductKind_Fish) ? ProductKind_None : Second, Base, &Series);
 }
 if(MakesFish)
 {
  W3PushDock(Opaque, Top, Radius, (Index & 1) ? 1.f : -1.f);
 }

 //- Shade trees on both sides, in the cluster's colours (no fruit, so they are not
 // mistaken for an orchard)
 for(u32 Tree = 0; Tree < 2; Tree += 1)
 {
  f32 SideSign = (Tree == 0) ? -1.f : 1.f;
  f32 Scale = .85f + .35f*RandomUnilateral(&Series);
  v3 Base = V3(Top.X + SideSign*.72f*Radius, Top.Y, Top.Z + .1f + .35f*RandomBilateral(&Series));
  R3PushCylinder(Opaque, Base, R3Add(Base, V3(0.f, .75f*Scale, 0.f)), .09f*Scale, 6, R3Color(W3_ColorTrunk));
  R3PushEllipsoid(Opaque, R3Add(Base, V3(0.f, 1.05f*Scale, 0.f)), V3(.5f*Scale, .45f*Scale, .5f*Scale), 5, 8,
                  W3Shade(R3Color(Theme->Leaves), &Series), false);
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
  v3 Center = V3(130.f*RandomBilateral(&Series),
                 -14.f - 3.f*RandomUnilateral(&Series),
                 130.f*RandomBilateral(&Series));
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
// NOTE: The hand-built hero island lives in solarpunk_hero_island.c.
#define Hero_Radius 3.f   // used for cables, hovering and the camera
#define W3_HeroNeighbours 8
#define W3Style_Normal 0
#define W3Style_Hero   1
#define W3Style_Themed 2
#define W3Style_Forest 3   // Verdant Reach (solarpunk_forest_island.c)
#define W3_ForestCluster 0
#define W3Style_Fishing 4   // Driftwood Shoals (solarpunk_fishing_island.c)
#define W3_FishingCluster 2
#define W3Style_Beach 5     // Sunward Isles (solarpunk_beach_island.c)
#define W3_BeachCluster 1
#define W3Style_Ruins 6     // Ember Archipelago (solarpunk_ruins_island.c)
#define W3_RuinsCluster 3
#define W3Style_Ice 7       // Saltglass Atolls (solarpunk_ice_island.c)
#define W3_IceCluster 6
#define W3Style_Crystal 8   // Mistral Keys (solarpunk_crystal_island.c)
#define W3_CrystalCluster 5
internal void W3PushCrystalIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction);
internal void W3PushIceIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction);
internal void W3PushRuinsIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction);
internal void W3PushBeachIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction);
internal void W3PushFishingIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction);
internal void W3PushForestIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction);
internal void HeroReplayCommands(app_state *App);

// NOTE: Identifies the hand-built islands and everything they are built from (which
// islands, where, what they make and need), so their cached geometry is rebuilt exactly
// when one of these changes.
internal u64
W3HeroCacheKey(app_state *App)
{
 r3_state *R3 = &App->R3;
 u64 Key = 1469598103934665603ull;
#define W3HeroMix(Value) Key = (Key ^ (u64)(Value))*1099511628211ull
 u32 Idx = 0;
 for(island *Island = App->FirstIsland; Island && Idx < R3_MaxIslands; Island = Island->Next)
 {
  if(R3->IslandStyle[Idx] != W3Style_Normal)
  {
   u32 BitsX, BitsY, BitsZ;
   MemoryCopy(&BitsX, &R3->IslandTop[Idx].X, sizeof(BitsX));
   MemoryCopy(&BitsY, &R3->IslandTop[Idx].Y, sizeof(BitsY));
   MemoryCopy(&BitsZ, &R3->IslandTop[Idx].Z, sizeof(BitsZ));
   W3HeroMix(Idx);
   W3HeroMix(R3->IslandStyle[Idx]);
   W3HeroMix(BitsX);
   W3HeroMix(BitsY);
   W3HeroMix(BitsZ);
   for(product *Product = Island->FirstProduct; Product; Product = Product->Next) { W3HeroMix(Product->Kind); W3HeroMix(Product->Amount); }
   W3HeroMix(0xFFFF);
   for(product *Req = Island->FirstRequirement; Req; Req = Req->Next) { W3HeroMix(Req->Kind); W3HeroMix(Req->Amount); }
  }
  Idx += 1;
 }
 W3HeroMix(Idx);
#undef W3HeroMix
 return Key;
}
internal void W3PushHeroIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction);
internal void W3PushThemedIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction);

// NOTE: Positions, screen projections and the hovered island, before building geometry.
internal void
W3PrepareIslands(app_state *App, app_input *Input)
{
 r3_state *R3 = &App->R3;

 R3->AnimTime += Input->dtForFrame;
 if(R3->AnimTime > 3600.f) R3->AnimTime -= 3600.f;

 //- The hero island: the one nearest the map centre
 R3->HeroIdx = -1;
 {
  f32 BestDistanceSq = 1e9f;
  u32 Idx = 0;
  for(island *Island = App->FirstIsland; Island && Idx < R3_MaxIslands; Island = Island->Next)
  {
   f32 DX = Island->MapPos.X - .5f;
   f32 DY = Island->MapPos.Y - .5f;
   if(DX*DX + DY*DY < BestDistanceSq)
   {
    BestDistanceSq = DX*DX + DY*DY;
    R3->HeroIdx = (s32)Idx;
   }
   Idx += 1;
  }
 }

 //- Its nearest neighbours on the map are built the same way, each with its own contents
 MemorySet(R3->IslandStyle, 0, sizeof(R3->IslandStyle));
 if(R3->HeroIdx >= 0)
 {
  R3->IslandStyle[R3->HeroIdx] = W3Style_Hero;
  v2 HeroPos = V2(.5f, .5f);
  {
   u32 Idx = 0;
   for(island *Island = App->FirstIsland; Island && Idx < R3_MaxIslands; Island = Island->Next)
   {
    if(Idx == (u32)R3->HeroIdx) HeroPos = Island->MapPos;
    Idx += 1;
   }
  }
  for(u32 Pick = 0; Pick < W3_HeroNeighbours; Pick += 1)
  {
   s32 Best = -1;
   f32 BestDistanceSq = 1e9f;
   u32 Idx = 0;
   for(island *Island = App->FirstIsland; Island && Idx < R3_MaxIslands; Island = Island->Next)
   {
    f32 DX = Island->MapPos.X - HeroPos.X;
    f32 DY = Island->MapPos.Y - HeroPos.Y;
    if(R3->IslandStyle[Idx] == W3Style_Normal && DX*DX + DY*DY < BestDistanceSq)
    {
     BestDistanceSq = DX*DX + DY*DY;
     Best = (s32)Idx;
    }
    Idx += 1;
   }
   if(Best >= 0) R3->IslandStyle[Best] = W3Style_Themed;
  }
 }

 //- Verdant Reach is a forest cluster: all its islands are forest islands
 if(App->HasMapLayout)
 {
  u32 Count = 0;
  for(island *Island = App->FirstIsland; Island && Count < R3_MaxIslands; Island = Island->Next) Count += 1;
  for(u32 Idx = 0; Idx < Count; Idx += 1)
  {
   if(R3->IslandStyle[Idx] == W3Style_Normal && WorldGenClusterOf(Idx, Count) == W3_ForestCluster)
   {
    R3->IslandStyle[Idx] = W3Style_Forest;
   }
   // Driftwood Shoals is a fishing cluster: all its islands are fishing villages
   if(R3->IslandStyle[Idx] == W3Style_Normal && WorldGenClusterOf(Idx, Count) == W3_FishingCluster)
   {
    R3->IslandStyle[Idx] = W3Style_Fishing;
   }
   // Sunward Isles is a beach cluster
   if(R3->IslandStyle[Idx] == W3Style_Normal && WorldGenClusterOf(Idx, Count) == W3_BeachCluster)
   {
    R3->IslandStyle[Idx] = W3Style_Beach;
   }
   // Ember Archipelago is an overgrown ruined city
   if(R3->IslandStyle[Idx] == W3Style_Normal && WorldGenClusterOf(Idx, Count) == W3_RuinsCluster)
   {
    R3->IslandStyle[Idx] = W3Style_Ruins;
   }
   // Saltglass Atolls is frozen: an icy working yard
   if(R3->IslandStyle[Idx] == W3Style_Normal && WorldGenClusterOf(Idx, Count) == W3_IceCluster)
   {
    R3->IslandStyle[Idx] = W3Style_Ice;
   }
   // Mistral Keys: crystals and shimmering water
   if(R3->IslandStyle[Idx] == W3Style_Normal && WorldGenClusterOf(Idx, Count) == W3_CrystalCluster)
   {
    R3->IslandStyle[Idx] = W3Style_Crystal;
   }
  }
 }


 u32 Index = 0;
 for(island *Island = App->FirstIsland; Island && Index < R3_MaxIslands; Island = Island->Next)
 {
  v3 Top = W3IslandTopFromMap(Island->MapPos, Index);
  R3->IslandTop[Index] = Top;
  R3->IslandRadius[Index] = (R3->IslandStyle[Index] != W3Style_Normal) ? Hero_Radius : W3_IslandRadius;

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

 //- The hand-built islands are cached on the GPU and rebuilt only when they change;
 // their moving parts are replayed every frame (HeroReplayCommands, below).
 u64 HeroKey = W3HeroCacheKey(App);
 b32 RebuildHero = (!R3->HeroCacheValid || R3->HeroCacheKey != HeroKey);
 r3_mesh HeroOpaque = {0};
 r3_mesh HeroTranslucent = {0};
 if(RebuildHero)
 {
  HeroOpaque = R3MeshAlloc(FrameArena, R3_MaxHeroOpaqueVertices);
  HeroTranslucent = R3MeshAlloc(FrameArena, R3_MaxHeroTranslucentVertices);
  R3->HeroCommandCount = 0;
  R3->HeroOpaque = &HeroOpaque;
  R3->HeroTranslucent = &HeroTranslucent;
  R3->HeroRecording = true;
 }

 //- Islands
 u32 Index = 0;
 for(island *Island = App->FirstIsland; Island && Index < R3->IslandCount; Island = Island->Next)
 {
  v3 Top = R3->IslandTop[Index];
  f32 Radius = R3->IslandRadius[Index];

  b32 Hovered = (R3->HoveredIdx == (s32)Index);
  b32 Selected = (App->Panel.SelectedPlusOne == (s32)Index + 1);
  f32 KnownFraction = 0.f;
  if(Broadcast->IslandCount > 1)
  {
   KnownFraction = ((f32)BroadcastKnownCount(App, Island) - 1.f)/((f32)Broadcast->IslandCount - 1.f);
  }
  KnownFraction = Clamp(0.f, KnownFraction, 1.f);
  R3->IslandKnown[Index] = KnownFraction;

  if(R3->IslandStyle[Index] != W3Style_Normal && !RebuildHero)
  {
   // Cached: nothing to build this frame.
  }
  else if(R3->IslandStyle[Index] == W3Style_Hero)
  {
   W3PushHeroIsland(App, Island, Top, Index, W3StatusColor(R3->IslandStatus[Index]),
                    Hovered || Selected, Hovered ? W3_ColorHover : W3_ColorSelected, KnownFraction);
  }
  else if(R3->IslandStyle[Index] == W3Style_Crystal)
  {
   W3PushCrystalIsland(App, Island, Top, Index, W3StatusColor(R3->IslandStatus[Index]),
                       Hovered || Selected, Hovered ? W3_ColorHover : W3_ColorSelected, KnownFraction);
  }
  else if(R3->IslandStyle[Index] == W3Style_Ice)
  {
   W3PushIceIsland(App, Island, Top, Index, W3StatusColor(R3->IslandStatus[Index]),
                   Hovered || Selected, Hovered ? W3_ColorHover : W3_ColorSelected, KnownFraction);
  }
  else if(R3->IslandStyle[Index] == W3Style_Ruins)
  {
   W3PushRuinsIsland(App, Island, Top, Index, W3StatusColor(R3->IslandStatus[Index]),
                     Hovered || Selected, Hovered ? W3_ColorHover : W3_ColorSelected, KnownFraction);
  }
  else if(R3->IslandStyle[Index] == W3Style_Beach)
  {
   W3PushBeachIsland(App, Island, Top, Index, W3StatusColor(R3->IslandStatus[Index]),
                     Hovered || Selected, Hovered ? W3_ColorHover : W3_ColorSelected, KnownFraction);
  }
  else if(R3->IslandStyle[Index] == W3Style_Fishing)
  {
   W3PushFishingIsland(App, Island, Top, Index, W3StatusColor(R3->IslandStatus[Index]),
                       Hovered || Selected, Hovered ? W3_ColorHover : W3_ColorSelected, KnownFraction);
  }
  else if(R3->IslandStyle[Index] == W3Style_Forest)
  {
   W3PushForestIsland(App, Island, Top, Index, W3StatusColor(R3->IslandStatus[Index]),
                      Hovered || Selected, Hovered ? W3_ColorHover : W3_ColorSelected, KnownFraction);
  }
  else if(R3->IslandStyle[Index] == W3Style_Themed)
  {
   W3PushThemedIsland(App, Island, Top, Index, W3StatusColor(R3->IslandStatus[Index]),
                      Hovered || Selected, Hovered ? W3_ColorHover : W3_ColorSelected, KnownFraction);
  }
  else
  {
   w3_theme *Theme = W3ThemeFor(App, Index, R3->IslandCount);
   W3PushIslandBody(&R3->Opaque, &R3->Translucent, Theme, Top, Radius, Index, W3StatusColor(R3->IslandStatus[Index]),
                    Hovered || Selected, Hovered ? W3_ColorHover : W3_ColorSelected);
   W3PushDecorations(&R3->Opaque, &R3->Translucent, Theme, Island, Top, Radius, Index);
   W3PushProductsAndNeeds(&R3->Opaque, &R3->Translucent, Island, Top);
   W3PushKnowledgeCrystal(&R3->Opaque, Top, KnownFraction);
  }

  Index += 1;
 }

 if(RebuildHero)
 {
  R3UploadHeroCache(R3, &HeroOpaque, &HeroTranslucent);
  R3->HeroCacheValid = true;
  R3->HeroCacheKey = HeroKey;
  R3->HeroOpaque = 0;
  R3->HeroTranslucent = 0;
  R3->HeroRecording = false;
 }
 HeroReplayCommands(App);

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
