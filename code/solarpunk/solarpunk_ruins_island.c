//-
// SolarPunk 3D ruin islands (Ember Archipelago): an old stone city reclaimed by plants.
// Crumbling towers, arches, broken columns and half-fallen walls stand on a plateau at
// the back, smothered in moss and vines under great banyan trees with hanging roots. A
// village with mossy roofs and solar panels lives round a lagoon below, which overflows
// off the front edge. The knowledge crystal sits on top of the tallest tower.
// What the island makes (pastures with cows, pear orchards, ...) is sized by the amounts.
// No fishing boats: these islands do not fish.
//
// Built on the hand-built island toolkit (solarpunk_hero_island.c) and the lagoon pieces
// of solarpunk_fishing_island.c.
//-

//~ Constants
#define Ruins_Cluster 3   // Ember Archipelago, see SupplyClimateFoods
#define Ruins_RowH    .11f

global_variable u32 RuinsStoneColors[] = {0xFFA8A08C, 0xFFB8AE96, 0xFF9A9280, 0xFF8E8A78, 0xFFC2B79C, 0xFF8F947C};

//~ Overgrowth
internal v4
RuinsStone(hero_ctx *H)
{
 v4 Result = HeroShade(H, HeroPick(H, RuinsStoneColors, ArrayCount(RuinsStoneColors)));
 return Result;
}

// NOTE: Moss on top of a stone edge and vines hanging down from it.
internal void
RuinsPushOvergrowth(hero_ctx *H, v3 Top, f32 Width, f32 MaxLength, u32 Vines)
{
 r3_mesh *O = H->Opaque;
 R3PushEllipsoid(O, Top, V3(.6f*Width, .05f, .6f*Width), 3, 7, HeroShade(H, 0xFF557F33), false);
 for(u32 Vine = 0; Vine < Vines; Vine += 1)
 {
  f32 A = 2.f*Pi32*RandomUnilateral(&H->Series);
  v3 Start = R3Add(Top, V3(.55f*Width*R3Cos(A), -.01f, .55f*Width*R3Sin(A)));
  f32 Length = MaxLength*(.35f + .65f*RandomUnilateral(&H->Series));
  v3 End = R3Add(Start, V3(0.f, -Length, 0.f));
  R3PushCylinder(O, Start, End, .01f, 3, R3Color(Hero_ColorVine));
  u32 Leaves = 2 + (u32)(Length*5.f);
  for(u32 Leaf = 0; Leaf < Leaves; Leaf += 1)
  {
   R3PushEllipsoid(O, R3Lerp(Start, End, ((f32)Leaf + .5f)/(f32)Leaves), V3(.035f, .045f, .035f), 3, 5, HeroShade(H, (Leaf & 1) ? Hero_ColorVine : Hero_ColorLeaf), false);
  }
 }
}

//~ Ruins
// NOTE: A crumbling square tower of stacked blocks: windows and a doorway are left open
// (a dark core shows through), the top rows break off unevenly. Returns its top centre.
internal v3
RuinsPushTower(hero_ctx *H, f32 X, f32 Z, f32 Half, f32 Height)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 u32 Rows = (u32)(Height/Ruins_RowH);
 R3PushBox(O, R3Add(B, V3(0.f, .5f*Height - .2f, 0.f)), V3(Half - .03f, .5f*Height + .2f - .3f, Half - .03f), R3Color(0xFF3E3A34));
 for(u32 Side = 0; Side < 4; Side += 1)
 {
  v3 Out = (Side == 0) ? V3(0.f, 0.f, 1.f) : ((Side == 1) ? V3(1.f, 0.f, 0.f) : ((Side == 2) ? V3(0.f, 0.f, -1.f) : V3(-1.f, 0.f, 0.f)));
  v3 Along = V3(-Out.Z, 0.f, Out.X);
  for(u32 Row = 0; Row < Rows; Row += 1)
  {
   f32 Y = B.Y + Ruins_RowH*((f32)Row + .5f);
   f32 Edge = (Row + 3 >= Rows) ? .35f*RandomUnilateral(&H->Series) : 0.f;   // crumbled top
   u32 Blocks = 3;
   for(u32 Block = 0; Block < Blocks; Block += 1)
   {
    f32 T = -1.f + 2.f*((f32)Block + .5f)/(f32)Blocks + ((Row & 1) ? .15f : 0.f);
    if(T > 1.f) T -= 2.f;
    b32 Window = (Block == 1 && (Row % 5 == 2 || Row % 5 == 3) && Row > 3 && Row + 3 < Rows);
    b32 Door = (Side == 0 && Block == 1 && Row < 4);
    b32 Fallen = (Row + 2 >= Rows) && RandomUnilateral(&H->Series) < .45f;
    if(Window || Door || Fallen || AbsF32(T) > 1.f - Edge) continue;
    v3 P = R3Add(R3Add(V3(B.X, Y, B.Z), R3Scale(Out, Half - .02f + .01f*RandomBilateral(&H->Series))), R3Scale(Along, T*Half*.68f));
    R3PushOrientedBox(O, P, R3Scale(Along, .34f*Half - .006f), V3(0.f, .5f*Ruins_RowH - .006f, 0.f), R3Scale(Out, .04f), RuinsStone(H));
   }
  }
  // Ledge under the windows
  R3PushOrientedBox(O, R3Add(V3(B.X, B.Y + Ruins_RowH*4.f, B.Z), R3Scale(Out, Half + .01f)), R3Scale(Along, Half + .02f), V3(0.f, .015f, 0.f), R3Scale(Out, .03f), RuinsStone(H));
 }
 v3 Result = V3(B.X, B.Y + Ruins_RowH*(f32)Rows - .1f, B.Z);
 RuinsPushOvergrowth(H, Result, 2.f*Half, .7f*Height, 7);
 // A few blocks fallen at its feet
 for(u32 Rubble = 0; Rubble < 3; Rubble += 1)
 {
  f32 A = 2.f*Pi32*RandomUnilateral(&H->Series);
  R3PushBox(O, R3Add(B, V3((Half + .12f)*R3Cos(A), .04f, (Half + .12f)*R3Sin(A))), V3(.06f, .04f, .05f), RuinsStone(H));
 }
 return Result;
}

// NOTE: A stone arch: two block pillars joined by a round arch of wedge stones.
internal void
RuinsPushArch(hero_ctx *H, f32 X, f32 Z, f32 Span, f32 Height)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 u32 Rows = (u32)(Height/Ruins_RowH);
 for(u32 Pillar = 0; Pillar < 2; Pillar += 1)
 {
  f32 PX = (Pillar == 0) ? -Span : Span;
  for(u32 Row = 0; Row < Rows; Row += 1)
  {
   R3PushBox(O, R3Add(B, V3(PX + .008f*RandomBilateral(&H->Series), Ruins_RowH*((f32)Row + .5f) - .1f*(f32)(Row == 0), 0.f)), V3(.1f, .5f*Ruins_RowH - .005f + .05f*(f32)(Row == 0), .1f), RuinsStone(H));
  }
 }
 f32 Spring = B.Y + Ruins_RowH*(f32)Rows;
 u32 Wedges = 9;
 for(u32 Wedge = 0; Wedge < Wedges; Wedge += 1)
 {
  f32 A = Pi32*((f32)Wedge + .5f)/(f32)Wedges;
  v3 Radial = V3(R3Cos(A), R3Sin(A), 0.f);
  v3 Tangent = V3(-R3Sin(A), R3Cos(A), 0.f);
  v3 P = V3(B.X + Span*Radial.X, Spring + Span*Radial.Y, B.Z);
  R3PushOrientedBox(O, P, R3Scale(Radial, .1f), R3Scale(Tangent, .5f*Pi32*Span/(f32)Wedges - .006f), V3(0.f, 0.f, .1f), RuinsStone(H));
 }
 RuinsPushOvergrowth(H, V3(B.X, Spring + Span + .1f, B.Z), .35f, .8f*Height, 6);
}

// NOTE: A crumbling wall along X, its top broken into steps.
internal void
RuinsPushWall(hero_ctx *H, f32 X, f32 Z, f32 HalfLength)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 u32 Blocks = (u32)(2.f*HalfLength/.16f);
 for(u32 Block = 0; Block < Blocks; Block += 1)
 {
  f32 BX = -HalfLength + .16f*((f32)Block + .5f);
  u32 Rows = 1 + RandomNext(&H->Series) % 5;
  for(u32 Row = 0; Row < Rows; Row += 1)
  {
   f32 Shift = (Row & 1) ? .04f : 0.f;
   R3PushBox(O, R3Add(B, V3(BX + Shift, Ruins_RowH*((f32)Row + .5f) - .05f*(f32)(Row == 0), 0.f)), V3(.075f, .5f*Ruins_RowH - .005f + .025f*(f32)(Row == 0), .07f), RuinsStone(H));
  }
  if(Block % 3 == 0) RuinsPushOvergrowth(H, R3Add(B, V3(BX, Ruins_RowH*(f32)Rows, 0.f)), .2f, .3f, 2);
 }
}

internal void
RuinsPushColumns(hero_ctx *H, f32 X, f32 Z)
{
 r3_mesh *O = H->Opaque;
 for(u32 Column = 0; Column < 3; Column += 1)
 {
  v3 B = HeroOn(H, X + .22f*((f32)Column - 1.f), Z);
  f32 Height = (Column == 1) ? .25f : .45f + .35f*RandomUnilateral(&H->Series);
  R3PushBox(O, R3Add(B, V3(0.f, .03f, 0.f)), V3(.07f, .03f, .07f), RuinsStone(H));
  ForestPushFrustum(O, R3Add(B, V3(0.f, .06f, 0.f)), R3Add(B, V3(0.f, Height, 0.f)), .05f, .045f, 8, RuinsStone(H));
  if(Height > .4f)
  {
   R3PushBox(O, R3Add(B, V3(0.f, Height + .02f, 0.f)), V3(.07f, .02f, .07f), RuinsStone(H));
   RuinsPushOvergrowth(H, R3Add(B, V3(0.f, Height + .05f, 0.f)), .14f, .35f, 2);
  }
 }
 // A fallen drum lying in the grass
 v3 Drum = HeroOn(H, X + .1f, Z + .25f);
 R3PushCylinder(O, R3Add(Drum, V3(-.15f, .05f, 0.f)), R3Add(Drum, V3(.15f, .05f, .02f)), .05f, 8, RuinsStone(H));
 R3PushEllipsoid(O, R3Add(Drum, V3(0.f, .1f, 0.f)), V3(.1f, .02f, .05f), 3, 6, HeroShade(H, 0xFF557F33), false);
}

// NOTE: A banyan: a massive trunk on flared roots, a wide flat crown, and aerial roots
// hanging from the branches down to the ground.
internal void
RuinsPushBanyan(hero_ctx *H, f32 X, f32 Z, f32 Height)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 ForestPushFrustum(O, B, R3Add(B, V3(0.f, .6f*Height, 0.f)), .26f, .18f, 10, HeroShade(H, 0xFF7A6248));
 ForestPushRootFlare(H, B, .5f, .1f, 7);
 v3 Fork = R3Add(B, V3(0.f, .6f*Height, 0.f));
 for(u32 Branch = 0; Branch < 6; Branch += 1)
 {
  f32 A = 2.f*Pi32*(f32)Branch/6.f + .3f*RandomBilateral(&H->Series);
  v3 End = R3Add(Fork, V3(.75f*Height*R3Cos(A), .25f*Height, .75f*Height*R3Sin(A)));
  ForestPushFrustum(O, Fork, End, .1f, .04f, 6, HeroShade(H, 0xFF7A6248));
  ForestPushCanopy(H, R3Add(End, V3(0.f, .12f, 0.f)), .45f*Height, 5, Forest_ColorLeaf1, Forest_ColorLeaf2);
  // Aerial roots from the branch down to the ground
  for(u32 Root = 0; Root < 2; Root += 1)
  {
   v3 Start = R3Lerp(Fork, End, .5f + .4f*(f32)Root);
   v3 Foot = V3(Start.X + .03f*RandomBilateral(&H->Series), B.Y - .02f, Start.Z + .03f*RandomBilateral(&H->Series));
   R3PushCylinder(O, Start, Foot, .012f, 4, HeroShade(H, 0xFF8A7458));
  }
 }
 ForestPushCanopy(H, R3Add(Fork, V3(0.f, .35f*Height, 0.f)), .6f*Height, 10, Forest_ColorLeaf1, Forest_ColorLeaf2);
 HeroMarkRect(H, X - .4f, Z - .4f, X + .4f, Z + .4f);
}

//~ The whole ruin island
internal void
W3PushRuinsIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction)
{
 hero_layout Layout = {0};
 hero_ctx Ctx;
 hero_ctx *H = &Ctx;
 HeroBegin(H, App, &Layout, Top, W3_Seed*41 + 1123*(u64)Index, Index);
 random_series *S = &H->Series;
 f32 Mirror = (RandomUnilateral(S) < .5f) ? 1.f : -1.f;

 product *Products[4];
 u32 ProductCount = HeroSortedProducts(Island, Products, ArrayCount(Products));
 u64 Fish = HeroAmountOf(Island, ProductKind_Fish);

 //- Layout: a lagoon in front with a grassy shore, a wide plateau at the back for the
 // ruins, a stream falling from it into the lagoon on one side
 Layout.Round = true;
 Layout.Radius = Fishing_Radius;
 Layout.FrontLevel = Fishing_ShoreY;
 Layout.Lagoon = true;
 Layout.LagoonX = -.05f*Mirror;
 Layout.LagoonZ = .55f;
 Layout.LagoonRX = .95f + .15f*RandomUnilateral(S);
 Layout.LagoonRZ = .7f*Layout.LagoonRX;
 Layout.ShoreScale = 1.45f;
 Layout.ChannelX = Layout.LagoonX + .25f*RandomBilateral(S);
 Layout.ChannelHalf = .22f;
 Layout.HillX = .2f*Mirror;
 Layout.HillZ = -1.35f;
 Layout.HillStretch = 1.7f;
 Layout.HillPeak = 1.5f;
 Layout.HillCap = .8f;
 Layout.Stream[0][0] = Layout.HillX + .9f*Mirror;    Layout.Stream[0][1] = Layout.HillZ + .15f;
 Layout.Stream[1][0] = Layout.HillX + 1.f*Mirror;    Layout.Stream[1][1] = Layout.HillZ + .6f;
 Layout.Stream[2][0] = Layout.LagoonX + .6f*Mirror;  Layout.Stream[2][1] = Layout.LagoonZ - Layout.LagoonRZ - .25f;
 Layout.Stream[3][0] = Layout.LagoonX + .45f*Mirror; Layout.Stream[3][1] = Layout.LagoonZ - Layout.LagoonRZ + .05f;
 Layout.StreamCount = 4;

 HeroPushTerrain(H);
 FishingPushRim(H, FishingRim_Overgrown);

 // Keep the waterline free
 {
  f32 RX = 1.12f*Layout.LagoonRX;
  f32 RZ = 1.12f*Layout.LagoonRZ;
  for(s32 J = 0; J < Hero_Cells; J += 1)
  {
   for(s32 I = 0; I < Hero_Cells; I += 1)
   {
    f32 DX = (HeroCellCenter(I) - Layout.LagoonX)/RX;
    f32 DZ = (HeroCellCenter(J) - Layout.LagoonZ)/RZ;
    if(DX*DX + DZ*DZ < 1.f) H->Taken[J][I] = 1;
   }
  }
 }

 //- The ruined city on the plateau: the tallest tower carries the knowledge crystal
 f32 X, Z;
 {
  f32 TX = Layout.HillX - .55f*Mirror;
  f32 TZ = Layout.HillZ - .15f;
  f32 HX = .3f, HZ = .3f;
  f32 PX = TX, PZ = TZ;
  if(HeroPlaceNear(H, TX, TZ, .4f, &HX, &HZ, &PX, &PZ))
  {
   v3 TowerTop = RuinsPushTower(H, PX, PZ, .27f, 1.65f);
   R3PushCylinder(H->Opaque, TowerTop, R3Add(TowerTop, V3(0.f, .45f, 0.f)), .02f, 5, R3Color(W3_ColorPost));
   HeroPushCrystal(H, R3Add(TowerTop, V3(0.f, .55f, 0.f)), .18f);
  }
  else
  {
   HeroPushCrest(H, Layout.HillX, Layout.HillZ, KnownFraction);
  }
 }
 u32 Towers = 1 + RandomNext(S) % 2;
 for(u32 Tower = 0; Tower < Towers; Tower += 1)
 {
  if(HeroPlace(H, .3f, .3f, -2.1f, -.4f, &X, &Z)) RuinsPushTower(H, X, Z, .22f + .05f*RandomUnilateral(S), .9f + .5f*RandomUnilateral(S));
 }
 for(u32 Arch = 0; Arch < 2; Arch += 1)
 {
  if(HeroPlace(H, .42f, .14f, -2.1f, .2f, &X, &Z)) RuinsPushArch(H, X, Z, .26f + .06f*RandomUnilateral(S), .45f + .25f*RandomUnilateral(S));
 }
 for(u32 Wall = 0; Wall < 3; Wall += 1)
 {
  f32 HalfLength = .3f + .2f*RandomUnilateral(S);
  if(HeroPlace(H, HalfLength + .05f, .1f, -2.1f, 1.8f, &X, &Z)) RuinsPushWall(H, X, Z, HalfLength);
 }
 if(HeroPlace(H, .36f, .36f, -2.f, 1.2f, &X, &Z)) RuinsPushColumns(H, X, Z);

 //- Banyan trees
 u32 Banyans = 2 + RandomNext(S) % 2;
 for(u32 Banyan = 0; Banyan < Banyans; Banyan += 1)
 {
  if(HeroPlace(H, .45f, .45f, -2.1f, 1.5f, &X, &Z)) RuinsPushBanyan(H, X, Z, 1.1f + .4f*RandomUnilateral(S));
 }

 //- What the island makes (pastures, orchards, ...), sized by the amounts
 for(u32 Idx = 0; Idx < ProductCount; Idx += 1)
 {
  HeroPushProduct(H, Products[Idx]->Kind, Products[Idx]->Amount, 0);
 }

 //- The village: mossy grey houses with solar roofs
 u32 Houses = 3 + RandomNext(S) % 2;
 for(u32 House = 0; House < Houses; House += 1)
 {
  f32 HX = .25f + .06f*RandomUnilateral(S);
  f32 HZ = .2f + .04f*RandomUnilateral(S);
  if(HeroPlace(H, HX + .06f, HZ + .12f, -1.6f, 1.9f, &X, &Z))
  {
   HeroPushHouse(H, X, Z - .04f, HX, HZ, 1 + RandomNext(S) % 2, 64 | 32 | 4 | (RandomNext(S) % 4));
  }
 }

 //- The lagoon: piers, lily pads and stepping stones; boats only if the island fishes
 u32 Piers = 2;
 v3 Moorings[4];
 u32 MooringCount = 0;
 for(u32 Pier = 0; Pier < Piers; Pier += 1)
 {
  f32 Angle = Pi32*((Pier == 0) ? 1.2f : 1.8f) + .15f*RandomBilateral(S);
  if(FishingGap(&Layout, FishingOval(&Layout, Angle, 1.06f))) continue;
  Moorings[MooringCount++] = FishingPushPier(H, Angle, .4f + .15f*RandomUnilateral(S));
 }
 if(Fish > 0)
 {
  for(u32 Boat = 0; Boat < MooringCount; Boat += 1) FishingPushTrawlerAt(H, Moorings[Boat], (Boat & 1) ? -1.f : 1.f, 2.f*(f32)Boat);
 }
 for(u32 Pad = 0; Pad < 12; Pad += 1)
 {
  v2 P = FishingOval(&Layout, 2.f*Pi32*RandomUnilateral(S), .25f + .65f*RandomUnilateral(S));
  R3PushEllipsoid(H->Opaque, HeroP(H, P.X, Hero_WaterLevel + .01f, P.Y), V3(.07f, .006f, .07f), 2, 7, HeroShade(H, 0xFF5E9E3E), false);
  if(Pad % 3 == 0) R3PushEllipsoid(H->Opaque, HeroP(H, P.X + .02f, Hero_WaterLevel + .025f, P.Y), V3(.02f, .015f, .02f), 3, 5, R3Color(0xFFF7D9E6), false);
 }
 for(u32 Stone = 0; Stone < 5; Stone += 1)
 {
  v2 P = FishingOval(&Layout, Pi32*(.15f + .14f*(f32)Stone), .85f);
  if(FishingGap(&Layout, P)) continue;
  R3PushBox(H->Opaque, HeroP(H, P.X, Hero_WaterLevel + .02f, P.Y), V3(.07f, .03f, .06f), RuinsStone(H));
 }

 //- The stream from the plateau: waterfall, water wheel, rope bridge
 {
  f32 MouthX = Layout.Stream[3][0];
  f32 MouthZ = Layout.Stream[3][1];
  R3PushEllipsoid(H->Translucent, HeroP(H, MouthX, Hero_WaterLevel + .03f, MouthZ + .08f), V3(.22f, .06f, .14f), 4, 8, R3WithAlpha(R3Color(0xFFFFFFFF), .5f), true);
  HeroPushWaterWheel(H, HeroP(H, MouthX - .3f*Mirror, Fishing_ShoreY + .05f, MouthZ - .1f), .22f);
  v3 BA = R3Add(HeroOn(H, Layout.Stream[1][0] - .4f, Layout.Stream[1][1]), V3(0.f, .05f, 0.f));
  v3 BB = R3Add(HeroOn(H, Layout.Stream[1][0] + .4f, Layout.Stream[1][1]), V3(0.f, .05f, 0.f));
  HeroPushRopeBridge(H, BA, BB);
 }

 //- Windmills, laundry, market stall, crates, campfire, barrels
 u32 Mills = 2 + RandomNext(S) % 2;
 for(u32 Mill = 0; Mill < Mills; Mill += 1)
 {
  if(HeroPlace(H, .17f, .17f, -2.1f, .6f, &X, &Z)) HeroPushWindmill(H, X, Z, 1.3f + .5f*RandomUnilateral(S), 6.f*RandomUnilateral(S));
 }
 if(HeroPlace(H, .3f, .06f, -1.8f, 1.9f, &X, &Z)) HeroPushLaundry(H, X - .25f, X + .25f, Z);
 u32 Goods[3];
 u32 GoodsCount = 0;
 for(product *Product = Island->FirstProduct; Product && GoodsCount < ArrayCount(Goods); Product = Product->Next)
 {
  Goods[GoodsCount++] = W3ProductColors[Product->Kind];
 }
 if(HeroPlace(H, .42f, .32f, -1.f, 2.f, &X, &Z)) HeroPushStall(H, X, Z - .02f, Goods, GoodsCount, true);
 {
  f32 PX = 0.f, PZ = 0.f, NX = 0.f, NZ = 0.f;
  b32 HaveProduce = HeroPlace(H, .13f, .4f, -1.5f, 2.f, &PX, &PZ);
  b32 HaveNeeds = HeroPlace(H, .4f, .13f, -1.5f, 2.f, &NX, &NZ);
  if(HaveProduce && HaveNeeds) HeroPushCrates(H, Island, PX, PZ - .26f, NX - .26f, NZ);
 }
 if(HeroPlace(H, .15f, .15f, -1.f, 1.8f, &X, &Z)) HeroPushCampfire(H, X, Z);
 for(u32 Barrel = 0; Barrel < 3; Barrel += 1)
 {
  if(HeroPlace(H, .07f, .07f, -1.8f, 2.f, &X, &Z)) HeroPushBarrel(H, X, Z);
 }

 //- Jungle: palms and broad trees in the free spots, fallen blocks in the grass
 for(u32 Tree = 0; Tree < 12; Tree += 1)
 {
  if(HeroPlace(H, .18f, .18f, -2.2f, 2.2f, &X, &Z))
  {
   if(Tree % 3 == 0) BeachPushPalm(H, X, Z, 1.f + .5f*RandomUnilateral(S));
   else              ForestPushBroadTree(H, X, Z, .9f + .5f*RandomUnilateral(S), false);
  }
 }
 for(u32 Block = 0; Block < 10; Block += 1)
 {
  if(HeroPlace(H, .08f, .08f, -2.1f, 2.f, &X, &Z))
  {
   v3 P = HeroOn(H, X, Z);
   R3PushBox(H->Opaque, R3Add(P, V3(0.f, .04f, 0.f)), V3(.07f, .04f, .055f), RuinsStone(H));
   R3PushEllipsoid(H->Opaque, R3Add(P, V3(0.f, .085f, 0.f)), V3(.06f, .015f, .045f), 3, 5, HeroShade(H, 0xFF557F33), false);
  }
 }

 //- Villagers
 u32 People = 6 + RandomNext(S) % 4;
 for(u32 Person = 0; Person < People; Person += 1)
 {
  for(u32 Try = 0; Try < 20; Try += 1)
  {
   f32 PX = 2.f*RandomBilateral(S);
   f32 PZ = 2.f*RandomBilateral(S);
   u32 Type;
   HeroGroundY(&Layout, PX, PZ, &Type);
   if((Type == HeroGround_Grass || Type == HeroGround_Path) && !HeroCellTaken(H, PX, PZ) && PX*PX + PZ*PZ < 2.f*2.f)
   {
    HeroPushPerson(H, HeroOn(H, PX, PZ), HeroShirtColors[RandomNext(S) % ArrayCount(HeroShirtColors)]);
    HeroMarkRect(H, PX, PZ, PX, PZ);
    break;
   }
  }
 }

 HeroFinish(H, App, StatusColor, Highlight, HighlightColor);
}
