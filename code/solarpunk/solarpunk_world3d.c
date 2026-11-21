//-
// SolarPunk 3D world: floating sky islands, product crates, cables and news pulses.
//-

//~ Constants
#define W3_WorldSize        92.f   // map 0..1 is spread over this many world units
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

//~ What an island makes, shown by its buildings
// NOTE: The island's main product (the one it makes most of) fills the middle terrace;
// a second product gets small buildings on the back corners of the lawn. An island
// that makes fish also gets a dock with a sky boat.
#define W3_ColorPaving    0xFFE8E2D6
#define W3_ColorMetal     0xFF6B7280
#define W3_ColorDarkWood  0xFF6E4A2C
#define W3_ColorPine      0xFF2F7D4F
#define W3_ColorRock      0xFF9A8F86
#define W3_ColorSail      0xFFFFF8EC
#define W3_ColorBattery   0xFF3B3F4A

internal void
W3PushConifer(r3_mesh *Mesh, v3 Base, f32 Scale, random_series *Series)
{
 v4 Green = W3Shade(R3Color(W3_ColorPine), Series);
 R3PushCylinder(Mesh, Base, R3Add(Base, V3(0.f, .35f*Scale, 0.f)), .07f*Scale, 5, R3Color(W3_ColorTrunk));
 R3PushCone(Mesh, R3Add(Base, V3(0.f, .3f*Scale, 0.f)), R3Add(Base, V3(0.f, 1.f*Scale, 0.f)), .38f*Scale, 7, Green);
 R3PushCone(Mesh, R3Add(Base, V3(0.f, .65f*Scale, 0.f)), R3Add(Base, V3(0.f, 1.35f*Scale, 0.f)), .28f*Scale, 7, Green);
}

internal void
W3PushTube(r3_mesh *Opaque, r3_mesh *Translucent, v3 Base, f32 Height, f32 Radius)
{
 v3 TopP = R3Add(Base, V3(0.f, Height, 0.f));
 R3PushCylinder(Translucent, Base, TopP, Radius, 8, R3WithAlpha(R3Color(W3ProductColors[ProductKind_Algae]), .6f));
 R3PushCylinder(Opaque, R3Add(TopP, V3(0.f, -.02f, 0.f)), R3Add(TopP, V3(0.f, .06f, 0.f)), Radius + .02f, 8, R3Color(W3_ColorPost));
 R3PushCylinder(Opaque, Base, R3Add(Base, V3(0.f, .06f, 0.f)), Radius + .02f, 8, R3Color(W3_ColorPost));
}

internal void
W3PushBatteryCell(r3_mesh *Mesh, v3 Base, f32 Scale)
{
 f32 Half = .16f*Scale;
 f32 Height = .55f*Scale;
 R3PushBox(Mesh, R3Add(Base, V3(0.f, .5f*Height, 0.f)), V3(Half, .5f*Height, Half), R3Color(W3_ColorBattery));
 R3PushBox(Mesh, R3Add(Base, V3(0.f, .7f*Height, 0.f)), V3(Half + .01f, .08f*Scale, Half + .01f), R3Color(W3ProductColors[ProductKind_Batteries]));
 R3PushCylinder(Mesh, R3Add(Base, V3(0.f, Height, 0.f)), R3Add(Base, V3(0.f, Height + .07f*Scale, 0.f)), .06f*Scale, 6, R3Color(W3_ColorPost));
}

internal void
W3PushSolarPanel(r3_mesh *Mesh, v3 Base, f32 Scale)
{
 v3 PanelCenter = R3Add(Base, V3(0.f, .45f*Scale, 0.f));
 R3PushCylinder(Mesh, Base, PanelCenter, .04f*Scale, 5, R3Color(W3_ColorPost));
 R3PushOrientedBox(Mesh, PanelCenter, V3(.36f*Scale, 0.f, 0.f), V3(0.f, .028f, -.016f), V3(0.f, .14f*Scale, .25f*Scale),
                   R3Color(W3ProductColors[ProductKind_SolarPanels]));
}

internal void
W3PushOreRocks(r3_mesh *Mesh, v3 Base, f32 Scale, random_series *Series)
{
 R3PushEllipsoid(Mesh, R3Add(Base, V3(0.f, .12f*Scale, 0.f)), V3(.38f*Scale, .3f*Scale, .32f*Scale), 4, 7, W3Shade(R3Color(W3_ColorRock), Series), false);
 for(u32 Nugget = 0; Nugget < 4; Nugget += 1)
 {
  f32 Angle = 2.f*Pi32*RandomUnilateral(Series);
  v3 P = R3Add(Base, V3(.3f*Scale*R3Cos(Angle), (.15f + .15f*RandomUnilateral(Series))*Scale, .26f*Scale*R3Sin(Angle)));
  R3PushBox(Mesh, P, V3(.07f*Scale, .07f*Scale, .07f*Scale), R3Color(W3ProductColors[ProductKind_Copper]));
 }
}

// NOTE: The greenhouse terrace covers the back of the middle terrace, so buildings go
// on the visible crescent: angles from about -30 to 210 degrees (0 = +X, 90 = front).
internal v3
W3TerracePoint(v3 T, f32 Degrees, f32 Distance)
{
 f32 Angle = Degrees*Pi32/180.f;
 v3 Result = R3Add(T, V3(Distance*R3Cos(Angle), 0.f, Distance*R3Sin(Angle)));
 return Result;
}

// NOTE: The middle terrace, filled for the island's main product. T is the centre of
// the terrace's top.
internal void
W3PushMainProduct(r3_mesh *Opaque, r3_mesh *Translucent, w3_theme *Theme, product_kind Kind, v3 T, random_series *Series)
{
 f32 R = W3_Terrace1Radius;
 f32 Wall = W3_TerraceWall + .05f;
 v4 WallColor = R3Color(Theme->Wall);
 v4 Water = R3Color(W3_ColorWater);

 switch(Kind)
 {
  case ProductKind_FreshWater:
  {
   // A pond and a water tower.
   W3PushTerrace(Opaque, T, R, Wall, 16, Water, WallColor);
   v3 Tower = W3TerracePoint(T, 10.f, 1.f);
   for(u32 Leg = 0; Leg < 4; Leg += 1)
   {
    f32 DX = (Leg & 1) ? .2f : -.2f;
    f32 DZ = (Leg & 2) ? .2f : -.2f;
    R3PushCylinder(Opaque, R3Add(Tower, V3(DX, 0.f, DZ)), R3Add(Tower, V3(.6f*DX, .8f, .6f*DZ)), .035f, 4, R3Color(W3_ColorPost));
   }
   R3PushCylinder(Opaque, R3Add(Tower, V3(0.f, .8f, 0.f)), R3Add(Tower, V3(0.f, 1.25f, 0.f)), .3f, 10, R3Color(0xFF8FC9E8));
   R3PushCone(Opaque, R3Add(Tower, V3(0.f, 1.25f, 0.f)), R3Add(Tower, V3(0.f, 1.5f, 0.f)), .34f, 10, R3Color(W3_ColorDomeRim));
  } break;

  case ProductKind_SolarPanels:
  {
   W3PushTerrace(Opaque, T, R, Wall, 16, R3Color(W3_ColorPaving), WallColor);
   for(u32 Panel = 0; Panel < 5; Panel += 1)
   {
    W3PushSolarPanel(Opaque, W3TerracePoint(T, -10.f + 50.f*(f32)Panel, .95f), .8f);
   }
  } break;

  case ProductKind_Batteries:
  {
   W3PushTerrace(Opaque, T, R, Wall, 16, R3Color(W3_ColorPaving), WallColor);
   for(u32 Cell = 0; Cell < 7; Cell += 1)
   {
    W3PushBatteryCell(Opaque, W3TerracePoint(T, -15.f + 35.f*(f32)Cell, (Cell & 1) ? .75f : 1.05f), 1.15f);
   }
  } break;

  case ProductKind_Fish:
  {
   // A fish pond, with fish.
   W3PushTerrace(Opaque, T, R, Wall, 16, Water, WallColor);
   for(u32 Fish = 0; Fish < 6; Fish += 1)
   {
    v3 P = W3TerracePoint(T, -20.f + 220.f*RandomUnilateral(Series), .85f + .3f*RandomUnilateral(Series));
    P.Y += .04f;
    R3PushEllipsoid(Opaque, P, V3(.16f, .05f, .07f), 3, 6, R3Color(W3ProductColors[ProductKind_Fish]), false);
   }
  } break;

  case ProductKind_Algae:
  {
   // A teal pool with glass algae tubes.
   W3PushTerrace(Opaque, T, R, Wall, 16, R3Color(0xFF7FD6C0), WallColor);
   for(u32 Tube = 0; Tube < 5; Tube += 1)
   {
    W3PushTube(Opaque, Translucent, W3TerracePoint(T, -10.f + 50.f*(f32)Tube, 1.f), .9f + .25f*(f32)(Tube & 1), .14f);
   }
  } break;

  case ProductKind_Copper:
  {
   // A mine: a rock heap with copper ore, a headframe and a cart.
   W3PushTerrace(Opaque, T, R, Wall, 16, R3Color(W3_ColorRock), WallColor);
   v3 Heap = W3TerracePoint(T, 90.f, .8f);
   R3PushCone(Opaque, Heap, R3Add(Heap, V3(0.f, .65f, 0.f)), .5f, 7, W3Shade(R3Color(W3_ColorRock), Series));
   W3PushOreRocks(Opaque, W3TerracePoint(T, 15.f, 1.f), 1.f, Series);
   v3 Frame = W3TerracePoint(T, 165.f, 1.f);
   R3PushCylinder(Opaque, R3Add(Frame, V3(-.25f, 0.f, 0.f)), R3Add(Frame, V3(0.f, 1.1f, 0.f)), .04f, 4, R3Color(W3_ColorDarkWood));
   R3PushCylinder(Opaque, R3Add(Frame, V3(.25f, 0.f, 0.f)), R3Add(Frame, V3(0.f, 1.1f, 0.f)), .04f, 4, R3Color(W3_ColorDarkWood));
   R3PushCylinder(Opaque, R3Add(Frame, V3(-.15f, 1.05f, 0.f)), R3Add(Frame, V3(.15f, 1.05f, 0.f)), .12f, 8, R3Color(W3ProductColors[ProductKind_Copper]));
   R3PushBox(Opaque, R3Add(W3TerracePoint(T, 125.f, 1.05f), V3(0.f, .14f, 0.f)), V3(.22f, .12f, .15f), R3Color(W3_ColorMetal));
  } break;

  case ProductKind_Timber:
  {
   // A pine grove and a log pile.
   W3PushTerrace(Opaque, T, R, Wall, 16, R3Color(Theme->Lawn), WallColor);
   f32 TreeAngles[6] = {-20.f, 20.f, 55.f, 125.f, 160.f, 200.f};
   for(u32 Tree = 0; Tree < 6; Tree += 1)
   {
    W3PushConifer(Opaque, W3TerracePoint(T, TreeAngles[Tree], 1.05f), .9f + .4f*RandomUnilateral(Series), Series);
   }
   for(u32 Log = 0; Log < 3; Log += 1)
   {
    f32 Y = (Log == 2) ? .26f : .1f;
    f32 Z = (Log == 2) ? 0.f : ((Log == 0) ? -.1f : .1f);
    v3 P = R3Add(T, V3(0.f, Y, .85f + Z));
    R3PushCylinder(Opaque, R3Add(P, V3(-.35f, 0.f, 0.f)), R3Add(P, V3(.35f, 0.f, 0.f)), .09f, 7, R3Color(W3ProductColors[ProductKind_Timber]));
   }
  } break;

  case ProductKind_Seeds:
  default:
  {
   // Crop rows, alternating wheat and greens.
   W3PushTerrace(Opaque, T, R, Wall, 16, R3Color(W3_ColorCropSoil), WallColor);
   for(u32 Row = 0; Row < 4; Row += 1)
   {
    f32 Z = T.Z - .78f + .52f*(f32)Row;
    f32 HalfLength = .8f*R3Sqrt(Max(R*R - (Z - T.Z)*(Z - T.Z), .01f));
    u32 Color = (Row & 1) ? W3_ColorCropGreen : W3_ColorCropWheat;
    R3PushBox(Opaque, V3(T.X, T.Y + .07f, Z), V3(HalfLength, .07f, .12f), R3Color(Color));
   }
  } break;
 }
}

// NOTE: Small building for a second product (or a bush when there is none), on one of
// the back corners of the lawn.
internal void
W3PushSmallProduct(r3_mesh *Opaque, r3_mesh *Translucent, w3_theme *Theme, product_kind Kind, v3 Base, random_series *Series)
{
 switch(Kind)
 {
  case ProductKind_Seeds:
  {
   for(u32 Row = 0; Row < 3; Row += 1)
   {
    u32 Color = (Row & 1) ? W3_ColorCropGreen : W3_ColorCropWheat;
    R3PushBox(Opaque, R3Add(Base, V3(0.f, .06f, -.25f + .25f*(f32)Row)), V3(.3f, .06f, .08f), R3Color(Color));
   }
  } break;
  case ProductKind_FreshWater:
  {
   R3PushCylinder(Opaque, Base, R3Add(Base, V3(0.f, .55f, 0.f)), .25f, 10, R3Color(0xFF8FC9E8));
   R3PushCone(Opaque, R3Add(Base, V3(0.f, .55f, 0.f)), R3Add(Base, V3(0.f, .8f, 0.f)), .29f, 10, R3Color(W3_ColorDomeRim));
  } break;
  case ProductKind_SolarPanels: W3PushSolarPanel(Opaque, Base, 1.f); break;
  case ProductKind_Batteries:
  {
   W3PushBatteryCell(Opaque, R3Add(Base, V3(-.18f, 0.f, 0.f)), 1.f);
   W3PushBatteryCell(Opaque, R3Add(Base, V3(.18f, 0.f, 0.f)), 1.f);
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
  case ProductKind_Algae:
  {
   W3PushTube(Opaque, Translucent, R3Add(Base, V3(-.15f, 0.f, 0.f)), .7f, .11f);
   W3PushTube(Opaque, Translucent, R3Add(Base, V3(.15f, 0.f, .05f)), .55f, .11f);
  } break;
  case ProductKind_Copper: W3PushOreRocks(Opaque, Base, .9f, Series); break;
  case ProductKind_Timber: W3PushConifer(Opaque, Base, 1.f, Series); break;
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

// NOTE: Trees on both sides (in the cluster's colours), the product buildings, and a
// dock when the island makes fish. The front of the lawn holds the crates.
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

 W3PushMainProduct(Opaque, Translucent, Theme, Main, R3Add(Top, W3_Terrace1Offset), &Series);

 b32 MakesFish = (Main == ProductKind_Fish || Second == ProductKind_Fish);
 for(u32 Corner = 0; Corner < 2; Corner += 1)
 {
  f32 SideSign = (Corner == 0) ? -1.f : 1.f;
  v3 Base = V3(Top.X + SideSign*.62f*Radius, Top.Y, Top.Z - .6f*Radius);
  W3PushSmallProduct(Opaque, Translucent, Theme, (Second == ProductKind_Fish) ? ProductKind_None : Second, Base, &Series);
 }
 if(MakesFish)
 {
  W3PushDock(Opaque, Top, Radius, (Index & 1) ? 1.f : -1.f);
 }

 //- Round trees on both sides
 for(u32 Tree = 0; Tree < 2; Tree += 1)
 {
  f32 SideSign = (Tree == 0) ? -1.f : 1.f;
  f32 Scale = .85f + .35f*RandomUnilateral(&Series);
  v3 Base = V3(Top.X + SideSign*.72f*Radius, Top.Y, Top.Z + .1f + .35f*RandomBilateral(&Series));
  R3PushCylinder(Opaque, Base, R3Add(Base, V3(0.f, .75f*Scale, 0.f)), .09f*Scale, 6, R3Color(W3_ColorTrunk));
  R3PushEllipsoid(Opaque, R3Add(Base, V3(0.f, 1.05f*Scale, 0.f)), V3(.5f*Scale, .45f*Scale, .5f*Scale), 5, 8,
                  W3Shade(R3Color(Theme->Leaves), &Series), false);

  for(u32 Fruit = 0; Fruit < 3; Fruit += 1)
  {
   f32 Angle = 2.f*Pi32*RandomUnilateral(&Series);
   v3 P = R3Add(Base, V3(.42f*Scale*R3Cos(Angle), (.95f + .25f*RandomUnilateral(&Series))*Scale, .42f*Scale*R3Sin(Angle)));
   R3PushEllipsoid(Opaque, P, V3(.07f, .07f, .07f), 3, 5, R3Color(Theme->Fruit), false);
  }
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
  v3 Center = V3(80.f*RandomBilateral(&Series),
                 -14.f - 3.f*RandomUnilateral(&Series),
                 80.f*RandomBilateral(&Series));
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
  w3_theme *Theme = W3ThemeFor(App, Index, R3->IslandCount);
  W3PushIslandBody(&R3->Opaque, &R3->Translucent, Theme, Top, Radius, Index, W3StatusColor(R3->IslandStatus[Index]),
                   Hovered || Selected, Hovered ? W3_ColorHover : W3_ColorSelected);
  W3PushDecorations(&R3->Opaque, &R3->Translucent, Theme, Island, Top, Radius, Index);
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
