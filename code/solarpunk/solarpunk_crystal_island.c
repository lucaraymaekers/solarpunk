//-
// SolarPunk 3D crystal islands (Crystal Spires): a square slab of warm stone in wooden
// scaffolding, with lavender and teal crystals bursting out of the ground and the
// underside. A shimmering stream runs down through the middle and spills off the front
// edge; the houses wear iridescent glass roofs; cranes, crystal carts, a boiler and
// sail-airships work round a sandy yard; sparkles hang in the air.
// What the island makes, sized by the amounts: fish -> boats on the pool, drying racks
// and a crane landing a net of fish; eggs -> hen yards.
//
// Built on the hand-built island toolkit (solarpunk_hero_island.c) and the machines of
// solarpunk_ice_island.c.
//-

//~ Constants
#define Crystal_Cluster 5   // Crystal Spires, see SupplyClimateFoods

global_variable u32 CrystalColors[] = {0xFFC58AF0, 0xFF6FF0D8, 0xFFF2A0E0, 0xFF9FB8FF};

//~ Crystals
// NOTE: A cluster of six-sided crystals growing from Base, mostly along Dir, with a soft
// glow round it.
internal void
CrystalPushCluster(hero_ctx *H, v3 Base, v3 Dir, f32 Size, u32 Color)
{
 r3_mesh *O = H->Opaque;
 u32 Count = 4 + RandomNext(&H->Series) % 3;
 for(u32 Crystal = 0; Crystal < Count; Crystal += 1)
 {
  v3 Jitter = V3(RandomBilateral(&H->Series), RandomBilateral(&H->Series), RandomBilateral(&H->Series));
  v3 D = R3Normalize(R3Add(Dir, R3Scale(Jitter, (Crystal == 0) ? .15f : .65f)));
  f32 Length = Size*((Crystal == 0) ? 1.2f : (.45f + .55f*RandomUnilateral(&H->Series)));
  f32 Radius = .17f*Length;
  v3 Shoulder = R3Add(Base, R3Scale(D, .72f*Length));
  v4 Body = R3LerpColor(R3Color(Color), R3Color(0xFFFFFFFF), .15f*RandomUnilateral(&H->Series));
  R3PushCylinder(O, Base, Shoulder, Radius, 6, Body);
  R3PushCone(O, Shoulder, R3Add(Base, R3Scale(D, Length)), Radius, 6, R3LerpColor(Body, R3Color(0xFFFFFFFF), .3f));
 }
 R3PushEllipsoid(H->Translucent, R3Add(Base, R3Scale(Dir, .45f*Size)), V3(.6f*Size, .6f*Size, .6f*Size), 4, 8, R3WithAlpha(R3Color(Color), .16f), true);
}

// NOTE: Floating sparkles round the island.
internal void
CrystalPushSparkles(hero_ctx *H, u32 Count)
{
 for(u32 Sparkle = 0; Sparkle < Count; Sparkle += 1)
 {
  v3 P = HeroP(H, 3.f*RandomBilateral(&H->Series), -3.f + 5.5f*RandomUnilateral(&H->Series), 3.f*RandomBilateral(&H->Series));
  f32 R = .012f + .02f*RandomUnilateral(&H->Series);
  u32 Color = (Sparkle % 3 == 0) ? 0xFFFFFFFF : CrystalColors[Sparkle % ArrayCount(CrystalColors)];
  R3PushEllipsoid(H->Opaque, P, V3(R, R, R), 2, 4, R3Color(Color), true);
 }
}

internal void
CrystalPushPennant(hero_ctx *H, f32 X, f32 Z, u32 Color)
{
 v3 B = HeroOn(H, X, Z);
 v3 Top = R3Add(B, V3(0.f, .7f, 0.f));
 R3PushCylinder(H->Opaque, B, Top, .008f, 4, R3Color(Hero_ColorWoodDark));
 R3PushTriUnlit(H->Opaque, Top, R3Add(Top, V3(0.f, -.1f, 0.f)), R3Add(Top, V3(.22f, -.05f, .02f)), R3Color(Color));
}

// NOTE: A small cart heaped with crystals.
internal void
CrystalPushCart(hero_ctx *H, f32 X, f32 Z)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 for(u32 Wheel = 0; Wheel < 2; Wheel += 1)
 {
  f32 WZ = (Wheel == 0) ? -.08f : .08f;
  R3PushCylinder(O, R3Add(B, V3(0.f, .05f, WZ - .012f)), R3Add(B, V3(0.f, .05f, WZ + .012f)), .05f, 8, R3Color(Hero_ColorWoodDark));
 }
 R3PushBox(O, R3Add(B, V3(0.f, .1f, 0.f)), V3(.12f, .04f, .08f), HeroShade(H, Hero_ColorWood));
 CrystalPushCluster(H, R3Add(B, V3(0.f, .14f, 0.f)), V3(0.f, 1.f, 0.f), .14f, CrystalColors[RandomNext(&H->Series) % ArrayCount(CrystalColors)]);
 R3PushCylinder(O, R3Add(B, V3(-.12f, .1f, 0.f)), R3Add(B, V3(-.3f, .06f, 0.f)), .008f, 3, R3Color(Hero_ColorWoodDark));
}

// NOTE: A sail-airship: the moored airship with a shimmering sail on top.
internal void
CrystalPushSailship(hero_ctx *H, v3 Mast, v3 Out)
{
 IcePushAirship(H, Mast, Out);
 v3 Body = R3Add(R3Add(Mast, R3Scale(Out, 1.f)), V3(0.f, .55f, 0.f));
 v3 SailFoot = R3Add(Body, V3(0.f, .18f, 0.f));
 R3PushCylinder(H->Opaque, SailFoot, R3Add(SailFoot, V3(0.f, .55f, 0.f)), .008f, 4, R3Color(Hero_ColorWoodDark));
 v4 Sail = R3WithAlpha(R3Color(0xFFE8D8FA), .7f);
 R3PushTriUnlit(H->Translucent, R3Add(SailFoot, V3(.02f, .05f, 0.f)), R3Add(SailFoot, V3(.02f, .55f, 0.f)), R3Add(SailFoot, V3(.4f, .08f, 0.f)), Sail);
 R3PushTriUnlit(H->Translucent, R3Add(SailFoot, V3(-.02f, .05f, 0.f)), R3Add(SailFoot, V3(-.02f, .5f, 0.f)), R3Add(SailFoot, V3(-.35f, .08f, 0.f)), Sail);
}

//~ The whole crystal island
internal void
W3PushCrystalIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction)
{
 hero_layout Layout = {0};
 hero_ctx Ctx;
 hero_ctx *H = &Ctx;
 HeroBegin(H, App, &Layout, Top, W3_Seed*47 + 1321*(u64)Index, Index);
 random_series *S = &H->Series;
 f32 Mirror = (RandomUnilateral(S) < .5f) ? 1.f : -1.f;

 product *Products[4];
 u32 ProductCount = HeroSortedProducts(Island, Products, ArrayCount(Products));
 u64 Fish = HeroAmountOf(Island, ProductKind_Fish);

 //- Layout: the square slab, a shimmering pool at the front (wider for a bigger catch)
 // fed by a stream that winds down from the hill at the back through the yard
 Layout.Glowing = true;
 Layout.FrontLevel = 0.f;
 Layout.Pool = true;
 f32 PoolWidth = .9f + Min(.8f, (f32)Fish/200.f);
 f32 PoolCenter = -.25f*Mirror;
 Layout.PoolX0 = PoolCenter - .5f*PoolWidth;
 Layout.PoolX1 = PoolCenter + .5f*PoolWidth;
 Layout.PoolZ0 = 1.3f;
 Layout.HillX = -.2f*Mirror;
 Layout.HillZ = -1.45f;
 Layout.HillStretch = 1.6f;
 Layout.HillPeak = 1.8f;
 Layout.HillCap = 1.2f;
 Layout.Stream[0][0] = Layout.HillX + .2f*Mirror; Layout.Stream[0][1] = Layout.HillZ + .1f;
 Layout.Stream[1][0] = .35f*Mirror;               Layout.Stream[1][1] = -.45f;
 Layout.Stream[2][0] = -.1f*Mirror;               Layout.Stream[2][1] = .5f;
 Layout.Stream[3][0] = PoolCenter;                Layout.Stream[3][1] = Layout.PoolZ0 + .1f;
 Layout.StreamCount = 4;

 HeroPushTerrain(H);
 HeroPushCliff(H);
 IcePushSlabDressing(H, &Layout, false);

 //- Crystals bursting out of the sides and the underside
 for(u32 Side = 0; Side < 4; Side += 1)
 {
  for(u32 Burst = 0; Burst < 3; Burst += 1)
  {
   f32 S0 = -1.7f + 1.7f*(f32)Burst + .3f*RandomBilateral(S);
   if(Side == 0 && S0 > Layout.PoolX0 - .2f && S0 < Layout.PoolX1 + .2f) continue;
   v3 Out = (Side == 0) ? V3(0.f, 0.f, 1.f) : ((Side == 1) ? V3(1.f, 0.f, 0.f) : ((Side == 2) ? V3(0.f, 0.f, -1.f) : V3(-1.f, 0.f, 0.f)));
   v3 Along = V3(-Out.Z, 0.f, Out.X);
   v3 Base = R3Add(R3Add(HeroP(H, 0.f, -1.f - .9f*RandomUnilateral(S), 0.f), R3Scale(Out, Hero_Half)), R3Scale(Along, S0));
   v3 Dir = R3Normalize(R3Add(Out, V3(0.f, -.6f*RandomUnilateral(S), 0.f)));
   CrystalPushCluster(H, Base, Dir, .45f + .4f*RandomUnilateral(S), CrystalColors[(Side + Burst) % ArrayCount(CrystalColors)]);
  }
 }
 for(u32 Burst = 0; Burst < 7; Burst += 1)
 {
  f32 A = 2.f*Pi32*(f32)Burst/7.f;
  v3 Base = HeroP(H, 1.1f*R3Cos(A), -2.6f - .6f*RandomUnilateral(S), 1.1f*R3Sin(A));
  v3 Dir = R3Normalize(V3(.5f*R3Cos(A), -1.f, .5f*R3Sin(A)));
  CrystalPushCluster(H, Base, Dir, .6f + .5f*RandomUnilateral(S), CrystalColors[Burst % ArrayCount(CrystalColors)]);
 }

 //- The shimmering stream spilling off the front edge
 {
  v4 Fall = R3WithAlpha(R3Color(0xFFB8F6FF), .75f);
  f32 Half = .2f;
  f32 Drops[4] = {0.f, .5f, 1.8f, 4.2f};
  f32 Outs[4] = {.02f, .15f, .3f, .4f};
  for(u32 Step = 0; Step < 3; Step += 1)
  {
   v3 P0 = HeroP(H, PoolCenter - Half, Hero_WaterLevel - Drops[Step], Hero_Half + Outs[Step]);
   v3 P1 = HeroP(H, PoolCenter + Half, Hero_WaterLevel - Drops[Step], Hero_Half + Outs[Step]);
   v3 Q0 = HeroP(H, PoolCenter - Half, Hero_WaterLevel - Drops[Step + 1], Hero_Half + Outs[Step + 1]);
   v3 Q1 = HeroP(H, PoolCenter + Half, Hero_WaterLevel - Drops[Step + 1], Hero_Half + Outs[Step + 1]);
   R3PushTriUnlit(H->Translucent, P0, P1, Q1, Fall);
   R3PushTriUnlit(H->Translucent, P0, Q1, Q0, Fall);
  }
  for(u32 Sparkle = 0; Sparkle < 20; Sparkle += 1)
  {
   f32 T = RandomUnilateral(S);
   v3 P = HeroP(H, PoolCenter + Half*RandomBilateral(S), Hero_WaterLevel - 4.f*T, Hero_Half + .05f + .35f*T);
   R3PushEllipsoid(H->Opaque, P, V3(.018f, .018f, .018f), 2, 4, R3Color(0xFFFFFFFF), true);
  }
 }

 //- Knowledge crystal on the hill
 HeroPushCrest(H, Layout.HillX - .5f*Mirror, Layout.HillZ - .1f, KnownFraction);

 //- What the island makes, sized by the amounts (first: needs the most room)
 for(u32 Idx = 0; Idx < ProductCount; Idx += 1)
 {
  HeroPushProduct(H, Products[Idx]->Kind, Products[Idx]->Amount, 0);
 }
 if(Fish > 0)
 {
  u32 Boats = 1 + (u32)(Fish/50);
  for(u32 Boat = 0; Boat < Boats; Boat += 1)
  {
   f32 BX = Layout.PoolX0 + .25f + (PoolWidth - .5f)*((f32)Boat + .5f)/(f32)Boats;
   HeroPushBoat(H, BX, Layout.PoolZ0 + .55f, (Boat & 1) ? 1.f : -1.f, 2.f*(f32)Boat, true);
  }
 }

 //- Crystal clusters growing in the yard, and twin crystal pillars by the pool
 f32 X, Z;
 for(u32 Cluster = 0; Cluster < 7; Cluster += 1)
 {
  if(HeroPlace(H, .2f, .2f, -2.1f, 2.f, &X, &Z))
  {
   CrystalPushCluster(H, HeroOn(H, X, Z), V3(0.f, 1.f, 0.f), .3f + .3f*RandomUnilateral(S), CrystalColors[Cluster % ArrayCount(CrystalColors)]);
  }
 }
 {
  f32 HX = .3f, HZ = .18f;
  f32 PX, PZ;
  if(HeroPlaceNear(H, Layout.PoolX1 + .35f, Layout.PoolZ0 + .3f, .4f, &HX, &HZ, &PX, &PZ))
  {
   for(u32 Pillar = 0; Pillar < 2; Pillar += 1)
   {
    v3 B = HeroOn(H, PX + ((Pillar == 0) ? -.12f : .12f), PZ);
    u32 Color = CrystalColors[Pillar];
    R3PushCylinder(H->Opaque, B, R3Add(B, V3(0.f, .55f, 0.f)), .09f, 6, R3Color(Color));
    R3PushCone(H->Opaque, R3Add(B, V3(0.f, .55f, 0.f)), R3Add(B, V3(0.f, .78f, 0.f)), .09f, 6, R3LerpColor(R3Color(Color), R3Color(0xFFFFFFFF), .3f));
    R3PushEllipsoid(H->Translucent, R3Add(B, V3(0.f, .4f, 0.f)), V3(.2f, .45f, .2f), 4, 8, R3WithAlpha(R3Color(Color), .15f), true);
   }
  }
 }

 //- The yard: a boiler, tracked vehicles, a truck and carts hauling crystals
 if(HeroPlace(H, .55f, .25f, -1.7f, .3f, &X, &Z)) IcePushBoiler(H, X - .1f, Z);
 if(HeroPlace(H, .25f, .16f, -.8f, 1.1f, &X, &Z)) IcePushTracked(H, X, Z, Mirror, true);
 if(HeroPlace(H, .22f, .14f, -.8f, 1.1f, &X, &Z))
 {
  IcePushTruck(H, X, Z, -Mirror);
  CrystalPushCluster(H, R3Add(HeroOn(H, X + .05f*Mirror, Z), V3(0.f, .19f, 0.f)), V3(0.f, 1.f, 0.f), .12f, CrystalColors[1]);
 }
 for(u32 Cart = 0; Cart < 2; Cart += 1)
 {
  if(HeroPlace(H, .2f, .12f, -1.f, 1.6f, &X, &Z)) CrystalPushCart(H, X, Z);
 }

 //- Cranes on the edges (one lands a net of fish when the island fishes)
 {
  f32 CX[2];
  f32 CZ[2];
  v3 Outs[2];
  CX[0] = -2.f; CZ[0] = .3f; Outs[0] = V3(-1.f, 0.f, 0.f);
  CX[1] = 2.f;  CZ[1] = -.4f; Outs[1] = V3(1.f, 0.f, 0.f);
  for(u32 Crane = 0; Crane < 2; Crane += 1)
  {
   f32 HX = .2f, HZ = .2f;
   f32 PX, PZ;
   if(HeroPlaceNear(H, CX[Crane], CZ[Crane], .3f, &HX, &HZ, &PX, &PZ))
   {
    IcePushCrane(H, PX, PZ, Outs[Crane], (Crane == 0 && Fish > 0) ? 2 : 0);
   }
  }
 }

 //- Houses with iridescent glass roofs
 u32 Houses = 4 + RandomNext(S) % 2;
 for(u32 House = 0; House < Houses; House += 1)
 {
  f32 HX = .25f + .06f*RandomUnilateral(S);
  f32 HZ = .2f + .04f*RandomUnilateral(S);
  if(HeroPlace(H, HX + .06f, HZ + .12f, -2.f, 1.9f, &X, &Z))
  {
   HeroPushHouse(H, X, Z - .04f, HX, HZ, 1 + RandomNext(S) % 2, 512 | 64 | 2 | (RandomNext(S) & 1));
  }
 }

 //- Sail-airships moored off both sides
 for(u32 Ship = 0; Ship < 2; Ship += 1)
 {
  f32 Side = (Ship == 0) ? -1.f : 1.f;
  f32 MZ = (Ship == 0) ? -1.f : 1.f;
  u32 GroundType;
  f32 Ground = HeroGroundY(&Layout, Side*(Hero_Half - .15f), MZ, &GroundType);
  CrystalPushSailship(H, HeroP(H, Side*(Hero_Half - .15f), Ground + .5f, MZ), V3(Side, 0.f, 0.f));
 }

 //- Windmills, pennants, laundry, market stall, crates, barrels
 for(u32 Mill = 0; Mill < 2; Mill += 1)
 {
  if(HeroPlace(H, .17f, .17f, -2.1f, .3f, &X, &Z)) HeroPushWindmill(H, X, Z, 1.3f + .5f*RandomUnilateral(S), 6.f*RandomUnilateral(S));
 }
 for(u32 Flag = 0; Flag < 4; Flag += 1)
 {
  if(HeroPlace(H, .06f, .06f, -2.1f, 2.f, &X, &Z)) CrystalPushPennant(H, X, Z, (Flag & 1) ? 0xFFD9506A : 0xFF8A6AD8);
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
 for(u32 Barrel = 0; Barrel < 4; Barrel += 1)
 {
  if(HeroPlace(H, .07f, .07f, -1.8f, 2.f, &X, &Z)) HeroPushBarrel(H, X, Z);
 }

 //- Lavender in the free grass, villagers, sparkles in the air
 for(u32 Flower = 0; Flower < 40; Flower += 1)
 {
  f32 PX = 2.2f*RandomBilateral(S);
  f32 PZ = 2.2f*RandomBilateral(S);
  u32 Type;
  f32 Y = HeroGroundY(&Layout, PX, PZ, &Type);
  if(Type == HeroGround_Grass && !HeroCellTaken(H, PX, PZ))
  {
   R3PushEllipsoid(H->Opaque, HeroP(H, PX, Y + .05f, PZ), V3(.06f, .05f, .06f), 3, 6, HeroShade(H, (Flower & 1) ? 0xFFB08AD8 : 0xFF9A7AC8), false);
  }
 }
 u32 People = 7 + RandomNext(S) % 4;
 for(u32 Person = 0; Person < People; Person += 1)
 {
  for(u32 Try = 0; Try < 20; Try += 1)
  {
   f32 PX = 2.f*RandomBilateral(S);
   f32 PZ = 2.f*RandomBilateral(S);
   u32 Type;
   HeroGroundY(&Layout, PX, PZ, &Type);
   if((Type == HeroGround_Grass || Type == HeroGround_Path) && !HeroCellTaken(H, PX, PZ))
   {
    HeroPushPerson(H, HeroOn(H, PX, PZ), HeroShirtColors[RandomNext(S) % ArrayCount(HeroShirtColors)]);
    HeroMarkRect(H, PX, PZ, PX, PZ);
    break;
   }
  }
 }
 CrystalPushSparkles(H, 90);

 HeroFinish(H, App, StatusColor, Highlight, HighlightColor);
}
