//-
// SolarPunk 3D beach islands (Sunward Isles): pale limestone islands round a turquoise
// lagoon with a sand beach, overflowing off the front edge in a waterfall. Palm trees,
// colourful beach huts with surfboards, parasols, towels, a volleyball net and kayaks
// on the beach; thatched houses with solar roofs, a couple of piers, windmills, and a
// hill at the back with a stream, a water wheel and the knowledge crystal.
// What the island makes (wheat fields and mills, hen yards, ...) grows on the grassy
// ground above the beach, sized by the amounts. No fishing boats: these islands do not fish.
//
// Built on the hand-built island toolkit (solarpunk_hero_island.c) and the lagoon
// pieces of solarpunk_fishing_island.c.
//-

//~ Constants
#define Beach_Cluster 1   // Sunward Isles, see WorldGenClusterFoods

global_variable u32 BeachBrightColors[] = {0xFFE86A7A, 0xFF4FC1C9, 0xFFF2C94C, 0xFF7BC86C, 0xFF5B8DEF, 0xFFF29E4C};

//~ Beach pieces
// NOTE: A palm: a leaning, ringed trunk, a crown of drooping fronds and coconuts.
internal void
BeachPushPalm(hero_ctx *H, f32 X, f32 Z, f32 Height)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 f32 LeanAngle = 2.f*Pi32*RandomUnilateral(&H->Series);
 v3 LeanDir = V3(R3Cos(LeanAngle), 0.f, R3Sin(LeanAngle));
 f32 Lean = .28f*Height;
 v3 Prev = B;
 u32 Pieces = 6;
 for(u32 Piece = 1; Piece <= Pieces; Piece += 1)
 {
  f32 T = (f32)Piece/(f32)Pieces;
  v3 P = R3Add(B, R3Add(R3Scale(LeanDir, Lean*T*T), V3(0.f, Height*T, 0.f)));
  f32 R0 = .06f - .025f*((f32)(Piece - 1)/(f32)Pieces);
  f32 R1 = .06f - .025f*T;
  ForestPushFrustum(O, Prev, P, R0, R1, 7, HeroShade(H, (Piece & 1) ? 0xFF8A6E4E : 0xFF7A5E40));
  Prev = P;
 }
 v3 Top = Prev;
 u32 Fronds = 8;
 for(u32 Frond = 0; Frond < Fronds; Frond += 1)
 {
  f32 A = 2.f*Pi32*((f32)Frond + .3f*RandomBilateral(&H->Series))/(f32)Fronds;
  v3 Out = V3(R3Cos(A), 0.f, R3Sin(A));
  v3 Side = V3(-Out.Z, 0.f, Out.X);
  f32 Reach = .55f + .2f*RandomUnilateral(&H->Series);
  v3 Points[4];
  Points[0] = Top;
  Points[1] = R3Add(Top, R3Add(R3Scale(Out, .35f*Reach), V3(0.f, .1f, 0.f)));
  Points[2] = R3Add(Top, R3Add(R3Scale(Out, .7f*Reach), V3(0.f, .0f, 0.f)));
  Points[3] = R3Add(Top, R3Add(R3Scale(Out, Reach), V3(0.f, -.22f, 0.f)));
  f32 Widths[3] = {.06f, .075f, .045f};
  v4 Green = HeroShade(H, (Frond & 1) ? 0xFF4E8A3A : 0xFF6A9E45);
  for(u32 Part = 0; Part < 3; Part += 1)
  {
   v3 Mid = R3Lerp(Points[Part], Points[Part + 1], .5f);
   R3PushOrientedBox(O, Mid, R3Scale(R3Sub(Points[Part + 1], Points[Part]), .5f), V3(0.f, .006f, 0.f), R3Scale(Side, Widths[Part]), Green);
  }
 }
 for(u32 Nut = 0; Nut < 3; Nut += 1)
 {
  f32 A = 2.1f*(f32)Nut;
  R3PushEllipsoid(O, R3Add(Top, V3(.05f*R3Cos(A), -.05f, .05f*R3Sin(A))), V3(.03f, .03f, .03f), 3, 5, R3Color(0xFF6B4A2C), false);
 }
}

// NOTE: A small painted beach hut with a surfboard leaning beside it.
internal void
BeachPushHut(hero_ctx *H, f32 X, f32 Z, u32 Color)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 R3PushBox(O, R3Add(B, V3(0.f, .015f, 0.f)), V3(.15f, .015f, .13f), HeroShade(H, 0xFFA89A86));
 W3PushHut(O, R3Add(B, V3(0.f, .03f, 0.f)), V3(.12f, .12f, .1f), Color);
 R3PushBox(O, R3Add(B, V3(0.f, .13f, .101f)), V3(.035f, .085f, .006f), R3Color(0xFFF5F0E6));
 R3PushBox(O, R3Add(B, V3(0.f, .12f, .106f)), V3(.025f, .075f, .004f), R3Color(Hero_ColorWoodDark));
 u32 Board = BeachBrightColors[(Color + 3) % ArrayCount(BeachBrightColors)];
 R3PushEllipsoid(O, R3Add(B, V3(.18f, .17f, .06f)), V3(.03f, .17f, .012f), 4, 8, R3Color(Board), false);
 R3PushBox(O, R3Add(B, V3(.18f, .17f, .073f)), V3(.004f, .14f, .002f), R3Color(0xFFFFFFFF));
}

internal void
BeachPushParasol(hero_ctx *H, v3 B, u32 Color)
{
 R3PushCylinder(H->Opaque, B, R3Add(B, V3(0.f, .32f, 0.f)), .008f, 4, R3Color(0xFFF5F0E6));
 R3PushCone(H->Opaque, R3Add(B, V3(0.f, .28f, 0.f)), R3Add(B, V3(0.f, .36f, 0.f)), .2f, 10, R3Color(Color));
}

internal void
BeachPushTowel(hero_ctx *H, v3 B, u32 Color)
{
 f32 A = Pi32*RandomUnilateral(&H->Series);
 v3 Along = V3(.12f*R3Cos(A), 0.f, .12f*R3Sin(A));
 v3 Across = V3(-.06f*R3Sin(A), 0.f, .06f*R3Cos(A));
 R3PushOrientedBox(H->Opaque, R3Add(B, V3(0.f, .005f, 0.f)), Along, V3(0.f, .004f, 0.f), Across, R3Color(Color));
 R3PushOrientedBox(H->Opaque, R3Add(B, V3(0.f, .01f, 0.f)), R3Scale(Along, .25f), V3(0.f, .002f, 0.f), Across, R3Color(0xFFFFFFFF));
}

internal void
BeachPushVolleyball(hero_ctx *H, f32 X, f32 Z)
{
 v3 B = HeroOn(H, X, Z);
 R3PushCylinder(H->Opaque, R3Add(B, V3(-.36f, 0.f, 0.f)), R3Add(B, V3(-.36f, .3f, 0.f)), .01f, 4, R3Color(Hero_ColorWoodDark));
 R3PushCylinder(H->Opaque, R3Add(B, V3(.36f, 0.f, 0.f)), R3Add(B, V3(.36f, .3f, 0.f)), .01f, 4, R3Color(Hero_ColorWoodDark));
 R3PushBox(H->Translucent, R3Add(B, V3(0.f, .24f, 0.f)), V3(.36f, .05f, .003f), R3WithAlpha(R3Color(0xFFFFFFFF), .55f));
 R3PushBox(H->Opaque, R3Add(B, V3(0.f, .295f, 0.f)), V3(.36f, .006f, .004f), R3Color(0xFFFFFFFF));
 R3PushEllipsoid(H->Opaque, R3Add(B, V3(.15f, .03f, .18f)), V3(.03f, .03f, .03f), 3, 6, R3Color(0xFFF2C94C), false);
}

internal void
BeachPushKayak(hero_ctx *H, f32 X, f32 Z, u32 Color)
{
 f32 A = Pi32*RandomUnilateral(&H->Series);
 v3 C = HeroP(H, X, Hero_WaterLevel + .02f, Z);
 v3 Along = V3(R3Cos(A), 0.f, R3Sin(A));
 v3 Side = V3(-Along.Z, 0.f, Along.X);
 R3PushOrientedBox(H->Opaque, C, R3Scale(Along, .17f), V3(0.f, .025f, 0.f), R3Scale(Side, .04f), R3Color(Color));
 R3PushOrientedBox(H->Opaque, R3Add(C, V3(0.f, .02f, 0.f)), R3Scale(Along, .05f), V3(0.f, .012f, 0.f), R3Scale(Side, .028f), R3Color(0xFF2E2E2E));
 R3PushCylinder(H->Opaque, R3Add(C, R3Add(R3Scale(Side, -.14f), V3(0.f, .05f, 0.f))), R3Add(C, R3Add(R3Scale(Side, .14f), V3(0.f, .05f, 0.f))), .005f, 3, R3Color(0xFFF5F0E6));
}

//~ The whole beach island
internal void
W3PushBeachIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction)
{
 hero_layout Layout = {0};
 hero_ctx Ctx;
 hero_ctx *H = &Ctx;
 HeroBegin(H, App, &Layout, Top, W3_Seed*37 + 1013*(u64)Index, Index);
 random_series *S = &H->Series;
 f32 Mirror = (RandomUnilateral(S) < .5f) ? 1.f : -1.f;

 product *Products[4];
 u32 ProductCount = HeroSortedProducts(Island, Products, ArrayCount(Products));
 u64 Fish = HeroAmountOf(Island, ProductKind_Fish);

 //- Layout: a lagoon with a sand beach, a channel to the front edge, grass above the
 // beach for the farms, a hill at the back on one side
 Layout.Round = true;
 Layout.Radius = Fishing_Radius;
 Layout.FrontLevel = Fishing_ShoreY;
 Layout.Lagoon = true;
 Layout.SandyShore = true;
 Layout.LagoonX = -.1f*Mirror;
 Layout.LagoonZ = .4f;
 Layout.LagoonRX = 1.f + .15f*RandomUnilateral(S);
 Layout.LagoonRZ = .78f*Layout.LagoonRX;
 Layout.ShoreScale = 1.5f;
 Layout.ChannelX = Layout.LagoonX + .25f*RandomBilateral(S);
 Layout.ChannelHalf = .22f;
 Layout.HillX = 1.15f*Mirror;
 Layout.HillZ = -1.3f;
 Layout.HillStretch = 1.f;
 Layout.HillPeak = 1.5f;
 Layout.HillCap = .8f;
 Layout.Stream[0][0] = Layout.HillX - .1f*Mirror;    Layout.Stream[0][1] = Layout.HillZ + .1f;
 Layout.Stream[1][0] = Layout.HillX - .45f*Mirror;   Layout.Stream[1][1] = Layout.HillZ + .45f;
 Layout.Stream[2][0] = Layout.LagoonX + .55f*Mirror; Layout.Stream[2][1] = Layout.LagoonZ - Layout.LagoonRZ - .3f;
 Layout.Stream[3][0] = Layout.LagoonX + .4f*Mirror;  Layout.Stream[3][1] = Layout.LagoonZ - Layout.LagoonRZ + .05f;
 Layout.StreamCount = 4;

 HeroPushTerrain(H);
 FishingPushRim(H, true);

 // Keep the waterline free (the beach ring closest to the water)
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

 //- Knowledge crystal on the hill
 HeroPushCrest(H, Layout.HillX + .1f*Mirror, Layout.HillZ - .2f, KnownFraction);

 //- What the island makes, on the grass above the beach (first: needs the most room)
 H->GrassOnly = true;
 for(u32 Idx = 0; Idx < ProductCount; Idx += 1)
 {
  HeroPushProduct(H, Products[Idx]->Kind, Products[Idx]->Amount, 0);
 }
 H->GrassOnly = false;

 //- Piers, and boats only if the island fishes; kayaks for fun
 f32 X, Z;
 u32 Piers = 2;
 v3 Moorings[4];
 u32 MooringCount = 0;
 for(u32 Pier = 0; Pier < Piers; Pier += 1)
 {
  f32 Angle = Pi32*((Pier == 0) ? 1.25f : 1.75f) + .15f*RandomBilateral(S);
  if(FishingGap(&Layout, FishingOval(&Layout, Angle, 1.06f))) continue;
  Moorings[MooringCount++] = FishingPushPier(H, Angle, .45f + .15f*RandomUnilateral(S));
 }
 if(Fish > 0)
 {
  for(u32 Boat = 0; Boat < MooringCount; Boat += 1) FishingPushTrawlerAt(H, Moorings[Boat], (Boat & 1) ? -1.f : 1.f, 2.f*(f32)Boat);
 }
 for(u32 Kayak = 0; Kayak < 3; Kayak += 1)
 {
  v2 P = FishingOval(&Layout, 2.f*Pi32*RandomUnilateral(S), .3f + .45f*RandomUnilateral(S));
  BeachPushKayak(H, P.X, P.Y, BeachBrightColors[RandomNext(S) % ArrayCount(BeachBrightColors)]);
 }

 //- Thatched houses with solar roofs
 u32 Houses = 3 + RandomNext(S) % 2;
 for(u32 House = 0; House < Houses; House += 1)
 {
  f32 HX = .25f + .06f*RandomUnilateral(S);
  f32 HZ = .2f + .04f*RandomUnilateral(S);
  if(HeroPlace(H, HX + .06f, HZ + .12f, -2.f, 1.9f, &X, &Z))
  {
   HeroPushHouse(H, X, Z - .04f, HX, HZ, 1 + RandomNext(S) % 2, 128 | 64 | 4 | (RandomNext(S) % 4));
  }
 }

 //- The beach: huts, parasols, towels, a volleyball net
 u32 Huts = 3 + RandomNext(S) % 2;
 for(u32 Hut = 0; Hut < Huts; Hut += 1)
 {
  if(HeroPlace(H, .24f, .16f, -.6f, 2.f, &X, &Z)) BeachPushHut(H, X, Z, BeachBrightColors[(Hut + Index) % ArrayCount(BeachBrightColors)]);
 }
 for(u32 Spot = 0; Spot < 7; Spot += 1)
 {
  v2 P = FishingOval(&Layout, 2.f*Pi32*RandomUnilateral(S), 1.16f + .1f*RandomUnilateral(S));
  if(FishingGap(&Layout, P)) continue;
  u32 Type;
  f32 Y = HeroGroundY(&Layout, P.X, P.Y, &Type);
  if(Type != HeroGround_Sand) continue;
  v3 B = HeroP(H, P.X, Y, P.Y);
  u32 Color = BeachBrightColors[RandomNext(S) % ArrayCount(BeachBrightColors)];
  BeachPushTowel(H, B, Color);
  if(Spot % 2 == 0) BeachPushParasol(H, R3Add(B, V3(.1f, 0.f, -.06f)), BeachBrightColors[(RandomNext(S)) % ArrayCount(BeachBrightColors)]);
  if(Spot % 3 == 0) HeroPushPerson(H, R3Add(B, V3(-.05f, 0.f, .1f)), HeroShirtColors[RandomNext(S) % ArrayCount(HeroShirtColors)]);
 }
 if(HeroPlace(H, .42f, .2f, -.4f, 2.f, &X, &Z)) BeachPushVolleyball(H, X, Z);

 //- Palms round the beach and on the hill
 for(u32 Palm = 0; Palm < 14; Palm += 1)
 {
  if(HeroPlace(H, .16f, .16f, -2.2f, 2.2f, &X, &Z)) BeachPushPalm(H, X, Z, 1.f + .6f*RandomUnilateral(S));
 }

 //- The stream from the hill: a waterfall into the lagoon, a water wheel and a bridge
 {
  f32 MouthX = Layout.Stream[3][0];
  f32 MouthZ = Layout.Stream[3][1];
  R3PushEllipsoid(H->Translucent, HeroP(H, MouthX, Hero_WaterLevel + .03f, MouthZ + .08f), V3(.22f, .06f, .14f), 4, 8, R3WithAlpha(R3Color(0xFFFFFFFF), .5f), true);
  HeroPushWaterWheel(H, HeroP(H, MouthX + .3f*Mirror, Fishing_ShoreY + .05f, MouthZ - .1f), .22f);
  v3 BA = R3Add(HeroOn(H, Layout.Stream[1][0] - .4f, Layout.Stream[1][1]), V3(0.f, .05f, 0.f));
  v3 BB = R3Add(HeroOn(H, Layout.Stream[1][0] + .4f, Layout.Stream[1][1]), V3(0.f, .05f, 0.f));
  HeroPushRopeBridge(H, BA, BB);
 }

 //- Windmills, laundry, market stall, crates
 u32 Mills = 2 + RandomNext(S) % 2;
 for(u32 Mill = 0; Mill < Mills; Mill += 1)
 {
  if(HeroPlace(H, .17f, .17f, -2.1f, .2f, &X, &Z)) HeroPushWindmill(H, X, Z, 1.3f + .5f*RandomUnilateral(S), 6.f*RandomUnilateral(S));
 }
 if(HeroPlace(H, .3f, .06f, -1.8f, 1.9f, &X, &Z)) HeroPushLaundry(H, X - .25f, X + .25f, Z);
 u32 Goods[3];
 u32 GoodsCount = 0;
 for(product *Product = Island->FirstProduct; Product && GoodsCount < ArrayCount(Goods); Product = Product->Next)
 {
  Goods[GoodsCount++] = W3ProductColors[Product->Kind];
 }
 if(HeroPlace(H, .42f, .32f, -1.f, 2.f, &X, &Z)) HeroPushStall(H, X, Z - .02f, Goods, GoodsCount, false);
 {
  f32 PX = 0.f, PZ = 0.f, NX = 0.f, NZ = 0.f;
  b32 HaveProduce = HeroPlace(H, .13f, .4f, -1.5f, 2.f, &PX, &PZ);
  b32 HaveNeeds = HeroPlace(H, .4f, .13f, -1.5f, 2.f, &NX, &NZ);
  if(HaveProduce && HaveNeeds) HeroPushCrates(H, Island, PX, PZ - .26f, NX - .26f, NZ);
 }
 for(u32 Barrel = 0; Barrel < 3; Barrel += 1)
 {
  if(HeroPlace(H, .07f, .07f, -1.8f, 2.f, &X, &Z)) HeroPushBarrel(H, X, Z);
 }

 //- Villagers on the beach and the paths
 u32 People = 6 + RandomNext(S) % 4;
 for(u32 Person = 0; Person < People; Person += 1)
 {
  for(u32 Try = 0; Try < 20; Try += 1)
  {
   f32 PX = 2.f*RandomBilateral(S);
   f32 PZ = 2.f*RandomBilateral(S);
   u32 Type;
   HeroGroundY(&Layout, PX, PZ, &Type);
   if((Type == HeroGround_Grass || Type == HeroGround_Sand) && !HeroCellTaken(H, PX, PZ) && PX*PX + PZ*PZ < 2.f*2.f)
   {
    HeroPushPerson(H, HeroOn(H, PX, PZ), HeroShirtColors[RandomNext(S) % ArrayCount(HeroShirtColors)]);
    HeroMarkRect(H, PX, PZ, PX, PZ);
    break;
   }
  }
 }

 HeroFinish(H, App, StatusColor, Highlight, HighlightColor);
}
