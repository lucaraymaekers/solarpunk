//-
// SolarPunk 3D fishing islands (Driftwood Shoals): round rocky islands with kelp and moss
// hanging off them, built around a lagoon harbour that overflows off the front edge in a
// waterfall. A boardwalk on stilts rings the lagoon, with piers, rowing boats and
// trawlers, weathered grey shacks with tin roofs and solar panels, drying racks, nets,
// rope, lobster pots and fish crates. A rocky hill at the back carries pines and the
// knowledge crystal; a stream falls from it into the lagoon past a water wheel.
// The catch (fish amount) sets the lagoon's size, the boats, the racks and the sheds.
//
// Built on the hand-built island toolkit in solarpunk_hero_island.c.
//-

//~ Constants
#define Fishing_Radius   2.3f
#define Fishing_Cluster  2      // Driftwood Shoals, see WorldGenClusterFoods
#define Fishing_ShoreY   -.4f   // the low shore round the lagoon
#define Fishing_RimPoints 64

#define Fishing_ColorNet     0xFF3E6B66
#define Fishing_ColorNet2    0xFF4E7A5A
#define Fishing_ColorFloat   0xFFF07A2E
#define Fishing_ColorRope    0xFFCDB58A
#define Fishing_ColorKelp    0xFF8A7A2E
#define Fishing_ColorKelp2   0xFF6E5A24
#define Fishing_ColorDeck    0xFFA89A86   // sun-bleached boards
#define Fishing_ColorHullRed 0xFFB0463A
#define Fishing_ColorHullTop 0xFFEDE6DA

//~ Boats
// NOTE: A small trawler with a cabin, a mast and a boom, bobbing (a moving part).
internal void
FishingPushTrawlerAt(hero_ctx *H, v3 Base, f32 Facing, f32 Phase)
{
 if(HeroDefer(H, HeroCmd_Trawler, Base, Facing, Phase, 0.f)) return;
 r3_mesh *O = H->Opaque;
 f32 Bob = .015f*R3Sin(1.1f*H->Time + Phase);
 f32 Roll = .02f*R3Sin(.8f*H->Time + 1.7f*Phase);
 v3 C = R3Add(Base, V3(0.f, Bob, 0.f));
 R3PushEllipsoid(O, R3Add(C, V3(0.f, -.01f, 0.f)), V3(.34f, .085f, .12f), 4, 10, R3Color(Fishing_ColorHullRed), false);
 R3PushBox(O, R3Add(C, V3(0.f, .045f, 0.f)), V3(.29f, .022f, .1f), R3Color(Fishing_ColorHullTop));
 R3PushBox(O, R3Add(C, V3(0.f, .07f, 0.f)), V3(.26f, .006f, .085f), R3Color(Fishing_ColorDeck));
 // Cabin with lit windows
 v3 Cabin = R3Add(C, V3(-.06f*Facing, .15f, Roll));
 R3PushBox(O, Cabin, V3(.08f, .075f, .065f), R3Color(Fishing_ColorHullTop));
 R3PushBox(O, R3Add(Cabin, V3(0.f, .085f, 0.f)), V3(.095f, .012f, .08f), R3Color(Fishing_ColorHullRed));
 HeroGlowRect(O, R3Add(Cabin, V3(0.f, .02f, .066f)), V3(.05f, 0.f, 0.f), V3(0.f, .022f, 0.f), R3Color(Hero_ColorWindow));
 HeroGlowRect(O, R3Add(Cabin, V3(.081f*Facing, .02f, 0.f)), V3(0.f, 0.f, .035f), V3(0.f, .022f, 0.f), R3Color(Hero_ColorWindow));
 // Mast, boom and a bundle of net
 v3 MastFoot = R3Add(C, V3(.14f*Facing, .07f, 0.f));
 v3 MastTop = R3Add(MastFoot, V3(0.f, .5f, Roll));
 R3PushCylinder(O, MastFoot, MastTop, .01f, 4, R3Color(Hero_ColorWoodDark));
 R3PushCylinder(O, R3Add(MastFoot, V3(0.f, .3f, 0.f)), R3Add(MastFoot, V3(.22f*Facing, .18f, 0.f)), .006f, 3, R3Color(Hero_ColorWoodDark));
 R3PushCylinder(O, MastTop, R3Add(C, V3(-.3f*Facing, .08f, 0.f)), .003f, 3, R3Color(Fishing_ColorRope));
 R3PushEllipsoid(O, R3Add(C, V3(.22f*Facing, .1f, 0.f)), V3(.06f, .035f, .05f), 3, 6, R3Color(Fishing_ColorNet), false);
 R3PushEllipsoid(O, R3Add(C, V3(.26f*Facing, .12f, .03f)), V3(.015f, .015f, .015f), 3, 4, R3Color(Fishing_ColorFloat), false);
 R3PushEllipsoid(H->Translucent, R3Add(C, V3(0.f, -.03f, 0.f)), V3(.42f, .004f, .17f), 2, 10, R3WithAlpha(R3Color(0xFFFFFFFF), .3f), true);
}

//~ Gear on the boardwalks
internal void
FishingPushNets(hero_ctx *H, v3 B)
{
 r3_mesh *O = H->Opaque;
 for(u32 Lump = 0; Lump < 5; Lump += 1)
 {
  v3 P = R3Add(B, V3(.09f*RandomBilateral(&H->Series), .04f + .02f*(f32)(Lump & 1), .07f*RandomBilateral(&H->Series)));
  R3PushEllipsoid(O, P, V3(.07f, .045f, .06f), 3, 6, HeroShade(H, (Lump & 1) ? Fishing_ColorNet : Fishing_ColorNet2), false);
 }
 for(u32 Float = 0; Float < 4; Float += 1)
 {
  v3 P = R3Add(B, V3(.1f*RandomBilateral(&H->Series), .07f + .02f*RandomUnilateral(&H->Series), .08f*RandomBilateral(&H->Series)));
  R3PushEllipsoid(O, P, V3(.018f, .018f, .018f), 3, 5, R3Color(Fishing_ColorFloat), false);
 }
}

internal void
FishingPushRope(hero_ctx *H, v3 B)
{
 for(u32 Turn = 0; Turn < 3; Turn += 1)
 {
  f32 R = .045f + .015f*(f32)Turn;
  for(u32 Seg = 0; Seg < 10; Seg += 1)
  {
   f32 A0 = 2.f*Pi32*(f32)Seg/10.f;
   f32 A1 = 2.f*Pi32*(f32)(Seg + 1)/10.f;
   R3PushCylinder(H->Opaque, R3Add(B, V3(R*R3Cos(A0), .012f + .008f*(f32)Turn, R*R3Sin(A0))), R3Add(B, V3(R*R3Cos(A1), .012f + .008f*(f32)Turn, R*R3Sin(A1))), .008f, 3, R3Color(Fishing_ColorRope));
  }
 }
}

internal void
FishingPushLobsterPot(hero_ctx *H, v3 B)
{
 R3PushBox(H->Opaque, R3Add(B, V3(0.f, .035f, 0.f)), V3(.05f, .035f, .04f), R3Color(Hero_ColorWoodDark));
 R3PushBox(H->Opaque, R3Add(B, V3(0.f, .072f, 0.f)), V3(.052f, .006f, .042f), R3Color(Hero_ColorWoodLight));
 R3PushBox(H->Translucent, R3Add(B, V3(0.f, .035f, 0.f)), V3(.051f, .036f, .041f), R3WithAlpha(R3Color(Fishing_ColorNet), .5f));
}

internal void
FishingPushFishCrate(hero_ctx *H, v3 B)
{
 R3PushBox(H->Opaque, R3Add(B, V3(0.f, .035f, 0.f)), V3(.06f, .035f, .045f), HeroShade(H, Hero_ColorWoodLight));
 for(u32 Fish = 0; Fish < 3; Fish += 1)
 {
  R3PushEllipsoid(H->Opaque, R3Add(B, V3(-.03f + .03f*(f32)Fish, .075f, ((Fish & 1) ? .01f : -.01f))), V3(.025f, .01f, .012f), 3, 5, R3Color(W3ProductColors[ProductKind_Fish]), false);
 }
}

// NOTE: A fish shed: open at the front, barrels and crates inside, fish hanging under the eave.
internal void
FishingPushShed(hero_ctx *H, f32 X, f32 Z)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 v4 Grey = HeroShade(H, 0xFF8C8478);
 R3PushBox(O, R3Add(B, V3(0.f, .02f, 0.f)), V3(.3f, .02f, .22f), HeroShade(H, Fishing_ColorDeck));
 R3PushBox(O, R3Add(B, V3(0.f, .2f, -.2f)), V3(.3f, .18f, .02f), Grey);
 R3PushBox(O, R3Add(B, V3(-.28f, .2f, 0.f)), V3(.02f, .18f, .22f), Grey);
 R3PushBox(O, R3Add(B, V3(.28f, .2f, 0.f)), V3(.02f, .18f, .22f), Grey);
 R3PushOrientedBox(O, R3Add(B, V3(0.f, .42f, 0.f)), V3(.34f, 0.f, 0.f), V3(0.f, .012f, 0.f), R3Normalize(V3(0.f, -.1f, .28f)), HeroShade(H, 0xFF8E6A4E));
 for(u32 Barrel = 0; Barrel < 3; Barrel += 1)
 {
  W3PushTerrace(O, R3Add(B, V3(-.18f + .12f*(f32)Barrel, .17f, -.1f)), .05f, .13f, 8, R3Color(0xFF6E4A2C), HeroShade(H, Hero_ColorWood));
 }
 FishingPushFishCrate(H, R3Add(B, V3(.15f, .04f, .08f)));
 for(u32 Fish = 0; Fish < 4; Fish += 1)
 {
  v3 P = R3Add(B, V3(-.2f + .13f*(f32)Fish, .3f, .2f));
  R3PushCylinder(O, R3Add(P, V3(0.f, .06f, 0.f)), P, .003f, 3, R3Color(Fishing_ColorRope));
  R3PushEllipsoid(O, R3Add(P, V3(0.f, -.03f, 0.f)), V3(.014f, .04f, .01f), 3, 5, R3Color(W3ProductColors[ProductKind_Fish]), false);
 }
}

//~ Rim: rock, moss and kelp; the lagoon overflowing off the front edge
global_variable u32 BeachStoneColors[] = {0xFFE3D7C1, 0xFFD2C3A8, 0xFFC4B293, 0xFFB8A684, 0xFFA89878};

// NOTE: Beach = pale limestone with a sandy lip and green vines (Sunward Isles),
// otherwise grey rock with moss and kelp.
internal void
FishingPushRim(hero_ctx *H, b32 Beach)
{
 r3_mesh *O = H->Opaque;
 hero_layout *L = H->Layout;
 f32 R = L->Radius;
 f32 RingScale[6] = {1.f, .98f, .86f, .64f, .38f, .14f};
 f32 RingDepth[6] = {0.f, -.7f, -1.6f, -2.5f, -3.25f, -3.75f};
 v3 Ring[6][Fishing_RimPoints];
 b32 PoolAt[Fishing_RimPoints];
 for(u32 K = 0; K < Fishing_RimPoints; K += 1)
 {
  f32 Angle = 2.f*Pi32*(f32)K/(f32)Fishing_RimPoints;
  f32 C = R3Cos(Angle);
  f32 S = R3Sin(Angle);
  u32 Type;
  f32 TopY = HeroGroundY(L, .985f*R*C, .985f*R*S, &Type);
  PoolAt[K] = (Type == HeroGround_Pool);
  for(u32 Level = 0; Level < 6; Level += 1)
  {
   f32 Jitter = (Level <= 1) ? 1.f : 1.f + .13f*RandomBilateral(&H->Series);
   f32 Y = TopY;
   if(Level > 0)
   {
    Y = Min(RingDepth[Level], TopY - .3f*(f32)Level);
    if(Level >= 2) Y += .15f*RandomBilateral(&H->Series);
   }
   Ring[Level][K] = HeroP(H, R*RingScale[Level]*Jitter*C, Y, R*RingScale[Level]*Jitter*S);
  }
 }

 //- Rock, mossy near the top
 v3 Center = HeroP(H, 0.f, -2.f, 0.f);
 for(u32 Level = 0; Level < 5; Level += 1)
 {
  for(u32 K = 0; K < Fishing_RimPoints; K += 1)
  {
   u32 Next = (K + 1) % Fishing_RimPoints;
   u32 Color = Beach ? HeroPick(H, BeachStoneColors, ArrayCount(BeachStoneColors)) : HeroPick(H, HeroStoneColors, ArrayCount(HeroStoneColors));
   f32 Roll = RandomUnilateral(&H->Series);
   if(Roll > ((Level == 0) ? .55f : .88f)) Color = 0xFF557F33;
   if(Beach && Level == 0 && Roll < .4f) Color = 0xFFE6D3A3;
   R3PushQuad(O, Ring[Level][K], Ring[Level][Next], Ring[Level + 1][Next], Ring[Level + 1][K], Center, HeroShade(H, Color));
  }
 }
 v3 Tip = HeroP(H, -.1f, -4.2f, .05f);
 for(u32 K = 0; K < Fishing_RimPoints; K += 1)
 {
  u32 Next = (K + 1) % Fishing_RimPoints;
  R3PushTri(O, Ring[5][K], Ring[5][Next], Tip, Center, HeroShade(H, HeroStoneColors[4]));
 }

 //- Boulders round the top edge
 for(u32 K = 0; K < Fishing_RimPoints; K += 1)
 {
  if(!PoolAt[K] && (K % 2 == 0 || RandomUnilateral(&H->Series) < .3f))
  {
   v3 P = R3Lerp(Ring[0][K], Ring[1][K], .15f);
   HeroPushBoulder(H, P, .1f + .1f*RandomUnilateral(&H->Series));
  }
 }

 //- The lagoon seen through the edge, and overflowing off it
 v4 Deep = R3WithAlpha(R3Color(Hero_ColorPoolDeep), .7f);
 v4 Fall = R3WithAlpha(R3Color(Hero_ColorFall), .7f);
 for(u32 K = 0; K < Fishing_RimPoints; K += 1)
 {
  u32 Next = (K + 1) % Fishing_RimPoints;
  if(PoolAt[K] && PoolAt[Next])
  {
   v3 A0 = Ring[0][K];
   v3 A1 = Ring[0][Next];
   f32 WaterY = H->Top.Y + Hero_WaterLevel;
   v3 W0 = V3(A0.X, WaterY, A0.Z);
   v3 W1 = V3(A1.X, WaterY, A1.Z);
   v3 F0 = V3(A0.X, H->Top.Y + Hero_PoolFloor, A0.Z);
   v3 F1 = V3(A1.X, H->Top.Y + Hero_PoolFloor, A1.Z);
   R3PushTriUnlit(H->Translucent, W0, W1, F1, Deep);
   R3PushTriUnlit(H->Translucent, W0, F1, F0, Deep);

   // Waterfall: from the lip, curving out and down past the underside
   v3 Out0 = R3Normalize(V3(A0.X - H->Top.X, 0.f, A0.Z - H->Top.Z));
   v3 Out1 = R3Normalize(V3(A1.X - H->Top.X, 0.f, A1.Z - H->Top.Z));
   f32 Drops[4] = {0.f, .6f, 2.f, 4.4f};
   f32 Outs[4] = {.02f, .2f, .38f, .5f};
   for(u32 Step = 0; Step < 3; Step += 1)
   {
    v3 P0 = R3Add(V3(A0.X, WaterY + .01f - Drops[Step], A0.Z), R3Scale(Out0, Outs[Step]));
    v3 P1 = R3Add(V3(A1.X, WaterY + .01f - Drops[Step], A1.Z), R3Scale(Out1, Outs[Step]));
    v3 Q0 = R3Add(V3(A0.X, WaterY + .01f - Drops[Step + 1], A0.Z), R3Scale(Out0, Outs[Step + 1]));
    v3 Q1 = R3Add(V3(A1.X, WaterY + .01f - Drops[Step + 1], A1.Z), R3Scale(Out1, Outs[Step + 1]));
    R3PushTriUnlit(H->Translucent, P0, P1, Q1, Fall);
    R3PushTriUnlit(H->Translucent, P0, Q1, Q0, Fall);
   }
   v3 Mid = R3Lerp(A0, A1, .5f);
   v3 Out = R3Normalize(V3(Mid.X - H->Top.X, 0.f, Mid.Z - H->Top.Z));
   R3PushEllipsoid(H->Translucent, R3Add(V3(Mid.X, WaterY - 4.3f, Mid.Z), R3Scale(Out, .5f)), V3(.35f, .25f, .35f), 4, 8, R3WithAlpha(R3Color(0xFFFFFFFF), .35f), true);
  }
 }

 //- Kelp and weed hanging off the rock
 for(u32 Strand = 0; Strand < (Beach ? 18u : 40u); Strand += 1)
 {
  u32 K = RandomNext(&H->Series) % Fishing_RimPoints;
  if(PoolAt[K]) continue;
  v3 Out = R3Normalize(V3(Ring[0][K].X - H->Top.X, 0.f, Ring[0][K].Z - H->Top.Z));
  v3 Side = V3(-Out.Z, 0.f, Out.X);
  v3 P = R3Add(R3Lerp(Ring[0][K], Ring[1][K], .3f*RandomUnilateral(&H->Series)), R3Scale(Out, .05f));
  f32 Length = (Beach ? .3f : .5f) + (Beach ? .9f : 1.6f)*RandomUnilateral(&H->Series);
  u32 Pieces = 3 + (u32)(Length*2.f);
  u32 Color = (Strand & 1) ? Fishing_ColorKelp : Fishing_ColorKelp2;
  if(Beach) Color = (Strand & 1) ? Hero_ColorVine : Hero_ColorLeaf;
  f32 Phase = 6.f*RandomUnilateral(&H->Series);
  for(u32 Piece = 0; Piece < Pieces; Piece += 1)
  {
   f32 Step = Length/(f32)Pieces;
   v3 Next = R3Add(P, V3(0.f, -Step, 0.f));
   Next = R3Add(Next, R3Add(R3Scale(Out, .03f*R3Sin(Phase + (f32)Piece)), R3Scale(Side, .04f*R3Cos(Phase + .7f*(f32)Piece))));
   v3 Mid = R3Lerp(P, Next, .5f);
   R3PushOrientedBox(O, Mid, R3Scale(Side, .035f), R3Scale(R3Sub(Next, P), .5f), R3Scale(Out, .004f), HeroShade(H, Color));
   P = Next;
  }
 }
}

//~ The lagoon's boardwalk, piers and boats
// NOTE: A point on the oval round the lagoon (Scale 1 = the water's edge).
internal v2
FishingOval(hero_layout *L, f32 Angle, f32 Scale)
{
 v2 Result = V2(L->LagoonX + Scale*L->LagoonRX*R3Cos(Angle), L->LagoonZ + Scale*L->LagoonRZ*R3Sin(Angle));
 return Result;
}

// NOTE: True for angles where the boardwalk must not go: the front channel and the
// mouth of the stream.
internal b32
FishingGap(hero_layout *L, v2 P)
{
 b32 Result = (P.Y > L->LagoonZ && AbsF32(P.X - L->ChannelX) < L->ChannelHalf + .18f);
 if(L->StreamCount)
 {
  f32 DX = P.X - L->Stream[L->StreamCount - 1][0];
  f32 DZ = P.Y - L->Stream[L->StreamCount - 1][1];
  Result = Result || (DX*DX + DZ*DZ < .3f*.3f);
 }
 return Result;
}

internal void
FishingPushBoardwalk(hero_ctx *H)
{
 r3_mesh *O = H->Opaque;
 hero_layout *L = H->Layout;
 u32 Segments = 44;
 f32 DeckY = Fishing_ShoreY + .035f;
 for(u32 Seg = 0; Seg < Segments; Seg += 1)
 {
  f32 A0 = 2.f*Pi32*(f32)Seg/(f32)Segments;
  f32 A1 = 2.f*Pi32*(f32)(Seg + 1)/(f32)Segments;
  v2 P0 = FishingOval(L, A0, 1.06f);
  v2 P1 = FishingOval(L, A1, 1.06f);
  if(FishingGap(L, P0) || FishingGap(L, P1)) continue;
  v3 Q0 = HeroP(H, P0.X, DeckY, P0.Y);
  v3 Q1 = HeroP(H, P1.X, DeckY, P1.Y);
  v3 Along = R3Sub(Q1, Q0);
  v3 Normal = R3Normalize(V3(-Along.Z, 0.f, Along.X));
  v3 Mid = R3Lerp(Q0, Q1, .5f);
  R3PushOrientedBox(O, Mid, R3Scale(Along, .52f), V3(0.f, .018f, 0.f), R3Scale(Normal, .13f), HeroShade(H, Fishing_ColorDeck));
  // Posts on the water side, down into the lagoon
  v2 Toward = V2(L->LagoonX - P0.X, L->LagoonZ - P0.Y);
  f32 TowardSign = (Toward.X*Normal.X + Toward.Y*Normal.Z > 0.f) ? 1.f : -1.f;
  v3 Inner = R3Add(Mid, R3Scale(Normal, .12f*TowardSign));
  if(Seg % 2 == 0)
  {
   R3PushCylinder(O, V3(Inner.X, H->Top.Y + Hero_PoolFloor, Inner.Z), R3Add(Inner, V3(0.f, .06f, 0.f)), .022f, 5, R3Color(Hero_ColorWoodDark));
  }
 }
}

// NOTE: A pier from the boardwalk into the lagoon. Returns the water point beside its end.
internal v3
FishingPushPier(hero_ctx *H, f32 Angle, f32 Length)
{
 r3_mesh *O = H->Opaque;
 hero_layout *L = H->Layout;
 v2 Start = FishingOval(L, Angle, 1.f);
 v2 Center = V2(L->LagoonX, L->LagoonZ);
 v2 Dir = V2(Center.X - Start.X, Center.Y - Start.Y);
 f32 DirLength = R3Sqrt(Dir.X*Dir.X + Dir.Y*Dir.Y);
 Dir = V2(Dir.X/DirLength, Dir.Y/DirLength);
 f32 DeckY = Fishing_ShoreY + .03f;
 v3 A = HeroP(H, Start.X, DeckY, Start.Y);
 v3 B = HeroP(H, Start.X + Length*Dir.X, DeckY, Start.Y + Length*Dir.Y);
 v3 Along = R3Sub(B, A);
 v3 Side = R3Normalize(V3(-Along.Z, 0.f, Along.X));
 R3PushOrientedBox(O, R3Lerp(A, B, .5f), R3Scale(Along, .5f), V3(0.f, .016f, 0.f), R3Scale(Side, .09f), HeroShade(H, Fishing_ColorDeck));
 for(u32 Post = 0; Post < 3; Post += 1)
 {
  v3 P = R3Lerp(A, B, .2f + .4f*(f32)Post);
  for(u32 S = 0; S < 2; S += 1)
  {
   v3 Q = R3Add(P, R3Scale(Side, (S == 0) ? -.09f : .09f));
   R3PushCylinder(O, V3(Q.X, H->Top.Y + Hero_PoolFloor, Q.Z), R3Add(Q, V3(0.f, .08f, 0.f)), .018f, 5, R3Color(Hero_ColorWoodDark));
  }
 }
 // Gear at the end
 FishingPushRope(H, R3Add(B, V3(0.f, .016f, 0.f)));
 v3 Result = R3Add(B, R3Add(R3Scale(Side, .28f), V3(0.f, 0.f, 0.f)));
 Result.Y = H->Top.Y + Hero_WaterLevel + .03f;
 return Result;
}

//~ The whole fishing island
internal void
W3PushFishingIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction)
{
 hero_layout Layout = {0};
 hero_ctx Ctx;
 hero_ctx *H = &Ctx;
 HeroBegin(H, App, &Layout, Top, W3_Seed*31 + 907*(u64)Index, Index);
 random_series *S = &H->Series;
 f32 Mirror = (RandomUnilateral(S) < .5f) ? 1.f : -1.f;   // which side the hill stands

 u64 Fish = HeroAmountOf(Island, ProductKind_Fish);
 product *Products[4];
 u32 ProductCount = HeroSortedProducts(Island, Products, ArrayCount(Products));

 //- Layout: a lagoon (bigger for a bigger catch) on a low shore, a channel to the front
 // edge, a rocky hill at the back on one side
 Layout.Round = true;
 Layout.Radius = Fishing_Radius;
 Layout.FrontLevel = Fishing_ShoreY;
 Layout.Lagoon = true;
 Layout.LagoonX = -.1f*Mirror;
 Layout.LagoonZ = .35f;
 Layout.LagoonRX = Min(1.35f, .95f + (f32)Fish/400.f);
 Layout.LagoonRZ = .78f*Layout.LagoonRX;
 Layout.ShoreScale = 1.9f;
 Layout.ChannelX = Layout.LagoonX + .25f*RandomBilateral(S);
 Layout.ChannelHalf = .22f;
 Layout.HillX = 1.15f*Mirror;
 Layout.HillZ = -1.25f;
 Layout.HillStretch = 1.f;
 Layout.HillPeak = 1.7f;
 Layout.HillCap = 1.2f;
 Layout.Stream[0][0] = Layout.HillX - .1f*Mirror;   Layout.Stream[0][1] = Layout.HillZ + .1f;
 Layout.Stream[1][0] = Layout.HillX - .45f*Mirror;  Layout.Stream[1][1] = Layout.HillZ + .45f;
 Layout.Stream[2][0] = Layout.LagoonX + .55f*Mirror; Layout.Stream[2][1] = Layout.LagoonZ - Layout.LagoonRZ - .25f;
 Layout.Stream[3][0] = Layout.LagoonX + .4f*Mirror;  Layout.Stream[3][1] = Layout.LagoonZ - Layout.LagoonRZ + .05f;
 Layout.StreamCount = 4;

 HeroPushTerrain(H);
 FishingPushRim(H, false);

 //- The lagoon is taken (claims only look at land, but keep the boardwalk ring free too)
 {
  f32 RX = 1.18f*Layout.LagoonRX;
  f32 RZ = 1.18f*Layout.LagoonRZ;
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

 //- Boardwalk and piers
 FishingPushBoardwalk(H);
 u32 Piers = 3 + (u32)(Fish/60);
 v3 Moorings[8];
 u32 MooringCount = 0;
 for(u32 Pier = 0; Pier < Piers && MooringCount < ArrayCount(Moorings); Pier += 1)
 {
  f32 Angle = Pi32*(1.15f + .7f*(f32)Pier/(f32)Max(1, Piers - 1)) + .15f*RandomBilateral(S);   // round the back
  if(Pier == Piers - 1) Angle = .5f*Pi32 + .9f*Mirror;                                          // one at the front
  if(FishingGap(&Layout, FishingOval(&Layout, Angle, 1.06f))) continue;
  Moorings[MooringCount++] = FishingPushPier(H, Angle, .45f + .2f*RandomUnilateral(S));
 }

 //- Boats: trawlers at the piers, rowing boats on the lagoon (more for a bigger catch)
 u32 Trawlers = (Fish >= 100) ? 2 : 1;
 for(u32 Boat = 0; Boat < Trawlers && Boat < MooringCount; Boat += 1)
 {
  FishingPushTrawlerAt(H, Moorings[Boat], (Boat & 1) ? -1.f : 1.f, 2.f*(f32)Boat);
 }
 u32 Rowboats = 2 + (u32)(Fish/40);
 for(u32 Boat = 0; Boat < Rowboats; Boat += 1)
 {
  f32 A = 2.f*Pi32*RandomUnilateral(S);
  f32 Spread = .35f + .4f*RandomUnilateral(S);
  v2 P = FishingOval(&Layout, A, Spread);
  HeroPushBoat(H, P.X, P.Y, (Boat & 1) ? 1.f : -1.f, 6.f*RandomUnilateral(S), (Boat % 3) != 2);
 }

 //- Shacks on the shore round the lagoon, with solar roofs
 f32 X, Z;
 u32 Shacks = 4 + RandomNext(S) % 2;
 for(u32 Shack = 0; Shack < Shacks; Shack += 1)
 {
  for(u32 Try = 0; Try < 12; Try += 1)
  {
   f32 Angle = 2.f*Pi32*RandomUnilateral(S);
   v2 P = FishingOval(&Layout, Angle, 1.5f + .1f*RandomUnilateral(S));
   f32 HX = .26f + .06f*RandomUnilateral(S);
   f32 HZ = .2f + .04f*RandomUnilateral(S);
   if(HeroClaim(H, P.X, P.Y, HX + .05f, HZ + .1f))
   {
    HeroPushHouse(H, P.X, P.Y - .04f, HX, HZ, 1 + (RandomNext(S) % 2), 64 | (RandomNext(S) % 8 & 7) | 4);
    break;
   }
  }
 }

 //- What the catch needs: fish sheds and drying racks (more for a bigger catch)
 u32 Sheds = 1 + (u32)(Fish/70);
 for(u32 Shed = 0; Shed < Sheds; Shed += 1)
 {
  if(HeroPlace(H, .34f, .26f, -1.6f, 2.f, &X, &Z)) FishingPushShed(H, X, Z);
 }
 for(u32 Idx = 0; Idx < ProductCount; Idx += 1)
 {
  HeroPushProduct(H, Products[Idx]->Kind, Products[Idx]->Amount, 0);
 }

 //- Gear along the shore: nets, rope, lobster pots, fish crates, barrels
 for(u32 Gear = 0; Gear < 14; Gear += 1)
 {
  f32 Angle = 2.f*Pi32*RandomUnilateral(S);
  v2 P = FishingOval(&Layout, Angle, 1.2f + .08f*RandomUnilateral(S));
  if(FishingGap(&Layout, P)) continue;
  v3 B = HeroP(H, P.X, Fishing_ShoreY + .04f, P.Y);
  switch(Gear % 5)
  {
   case 0:  FishingPushNets(H, B); break;
   case 1:  FishingPushRope(H, B); break;
   case 2:  FishingPushLobsterPot(H, B); FishingPushLobsterPot(H, R3Add(B, V3(.05f, .075f, .01f))); break;
   case 3:  FishingPushFishCrate(H, B); break;
   default: W3PushTerrace(H->Opaque, R3Add(B, V3(0.f, .13f, 0.f)), .055f, .13f, 8, R3Color(0xFF6E4A2C), HeroShade(H, Hero_ColorWood)); break;
  }
 }

 //- The hill: rocks, pines, the knowledge crystal; the stream with a wheel and a bridge
 HeroPushCrest(H, Layout.HillX + .1f*Mirror, Layout.HillZ - .2f, KnownFraction);
 for(u32 Rock = 0; Rock < 6; Rock += 1)
 {
  if(HeroPlace(H, .12f, .12f, -2.1f, -.4f, &X, &Z)) HeroPushBoulder(H, R3Add(HeroOn(H, X, Z), V3(0.f, .06f, 0.f)), .12f + .1f*RandomUnilateral(S));
 }
 for(u32 Pine = 0; Pine < 6; Pine += 1)
 {
  if(HeroPlace(H, .18f, .18f, -2.2f, -.2f, &X, &Z)) HeroPushPine(H, X, Z, 1.f + .7f*RandomUnilateral(S));
 }
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
 for(u32 Line = 0; Line < 2; Line += 1)
 {
  if(HeroPlace(H, .3f, .06f, -1.8f, 1.9f, &X, &Z)) HeroPushLaundry(H, X - .25f, X + .25f, Z);
 }
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

 //- Villagers on the boardwalk and shore
 u32 People = 8 + RandomNext(S) % 4;
 for(u32 Person = 0; Person < People; Person += 1)
 {
  f32 Angle = 2.f*Pi32*RandomUnilateral(S);
  v2 P = FishingOval(&Layout, Angle, 1.06f + .2f*RandomUnilateral(S));
  if(FishingGap(&Layout, P)) continue;
  HeroPushPerson(H, HeroP(H, P.X, Fishing_ShoreY + .055f, P.Y), HeroShirtColors[RandomNext(S) % ArrayCount(HeroShirtColors)]);
 }

 HeroFinish(H, App, StatusColor, Highlight, HighlightColor);
}
