//-
// SolarPunk 3D forest islands (Verdant Reach): round, mossy islands held together by
// gnarled roots and hanging vines. A giant tree stands in the middle (flared roots, a
// spiral stair, a platform with a lookout hut, the knowledge crystal above its crown),
// two treehouses are joined to it by rope bridges, redwoods and oaks fill the rest, and
// a stream falls in small waterfalls into a pond at the front. What the island makes
// (orchards, vegetable fields, ...) grows in the clearings, sized by the amounts.
//
// Built on the hand-built island toolkit in solarpunk_hero_island.c.
//-

//~ Constants
#define Forest_Radius     2.3f
#define Forest_RimPoints  64
#define Forest_Cluster    0      // Verdant Reach, see WorldGenClusterFoods

#define Forest_ColorBark      0xFF6B4A32
#define Forest_ColorBarkDark  0xFF4E3524
#define Forest_ColorRedwood   0xFF7A4530
#define Forest_ColorLeaf1     0xFF3F7A35
#define Forest_ColorLeaf2     0xFF6E9E3C
#define Forest_ColorLeafDark  0xFF2E5A34
#define Forest_ColorAutumn1   0xFFC9A33A
#define Forest_ColorAutumn2   0xFFD9862E
#define Forest_ColorSoil      0xFF5A4030
#define Forest_ColorSoilDark  0xFF45311F
#define Forest_ColorMoss      0xFF557F33
#define Forest_ColorRoot      0xFF5E4029

//~ Helpers
// NOTE: A cone with its tip cut off (radius RadiusA at A, RadiusB at B), no caps.
internal void
ForestPushFrustum(r3_mesh *Mesh, v3 A, v3 B, f32 RadiusA, f32 RadiusB, u32 Sides, v4 Color)
{
 v3 Direction = R3Normalize(R3Sub(B, A));
 v3 U, V;
 R3Perpendiculars(Direction, &U, &V);
 v3 Mid = R3Lerp(A, B, .5f);
 for(u32 Side = 0; Side < Sides; Side += 1)
 {
  f32 Angle0 = 2.f*Pi32*(f32)Side/(f32)Sides;
  f32 Angle1 = 2.f*Pi32*(f32)(Side + 1)/(f32)Sides;
  v3 Offset0 = R3Add(R3Scale(U, R3Cos(Angle0)), R3Scale(V, R3Sin(Angle0)));
  v3 Offset1 = R3Add(R3Scale(U, R3Cos(Angle1)), R3Scale(V, R3Sin(Angle1)));
  R3PushQuad(Mesh, R3Add(A, R3Scale(Offset0, RadiusA)), R3Add(A, R3Scale(Offset1, RadiusA)),
             R3Add(B, R3Scale(Offset1, RadiusB)), R3Add(B, R3Scale(Offset0, RadiusB)), Mid, Color);
 }
}

// NOTE: A leafy crown: overlapping blobs between two colours.
internal void
ForestPushCanopy(hero_ctx *H, v3 Center, f32 Radius, u32 Blobs, u32 ColorA, u32 ColorB)
{
 for(u32 Blob = 0; Blob < Blobs; Blob += 1)
 {
  v3 Offset = V3(RandomBilateral(&H->Series), .6f*RandomBilateral(&H->Series), RandomBilateral(&H->Series));
  Offset = R3Scale(Offset, .6f*Radius);
  f32 Size = Radius*(.35f + .2f*RandomUnilateral(&H->Series));
  v4 Color = R3LerpColor(R3Color(ColorA), R3Color(ColorB), RandomUnilateral(&H->Series));
  R3PushEllipsoid(H->Opaque, R3Add(Center, Offset), V3(Size, .8f*Size, Size), 4, 7, W3Shade(Color, &H->Series), false);
 }
}

internal void
ForestPushRootFlare(hero_ctx *H, v3 Base, f32 Reach, f32 Thickness, u32 Count)
{
 for(u32 Root = 0; Root < Count; Root += 1)
 {
  f32 A = 2.f*Pi32*(f32)Root/(f32)Count + .4f*RandomBilateral(&H->Series);
  R3PushCone(H->Opaque, R3Add(Base, V3(0.f, 1.5f*Thickness, 0.f)), R3Add(Base, V3(Reach*R3Cos(A), -.02f, Reach*R3Sin(A))),
             Thickness, 4, HeroShade(H, Forest_ColorBarkDark));
 }
}

//~ Trees
internal void
ForestPushBroadTree(hero_ctx *H, f32 X, f32 Z, f32 Height, b32 Autumn)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 f32 Lean = .08f*RandomBilateral(&H->Series);
 v3 Mid = R3Add(B, V3(Lean, .45f*Height, .5f*Lean));
 v3 Top = R3Add(B, V3(1.5f*Lean, .7f*Height, Lean));
 ForestPushFrustum(O, B, Mid, .085f*Height, .06f*Height, 7, HeroShade(H, Forest_ColorBark));
 ForestPushFrustum(O, Mid, Top, .06f*Height, .035f*Height, 6, HeroShade(H, Forest_ColorBark));
 ForestPushRootFlare(H, B, .17f*Height, .04f*Height, 4);
 for(u32 Branch = 0; Branch < 3; Branch += 1)
 {
  f32 A = 2.1f*(f32)Branch + RandomUnilateral(&H->Series);
  R3PushCylinder(O, Mid, R3Add(Top, V3(.3f*Height*R3Cos(A), .05f*Height, .3f*Height*R3Sin(A))), .022f*Height, 4, HeroShade(H, Forest_ColorBark));
 }
 u32 LeafA = Autumn ? Forest_ColorAutumn1 : Forest_ColorLeaf1;
 u32 LeafB = Autumn ? Forest_ColorAutumn2 : Forest_ColorLeaf2;
 ForestPushCanopy(H, R3Add(Top, V3(0.f, .12f*Height, 0.f)), .4f*Height, 9, LeafA, LeafB);
}

internal void
ForestPushRedwood(hero_ctx *H, f32 X, f32 Z, f32 Height)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 ForestPushFrustum(O, B, R3Add(B, V3(0.f, .5f*Height, 0.f)), .15f, .11f, 8, HeroShade(H, Forest_ColorRedwood));
 ForestPushFrustum(O, R3Add(B, V3(0.f, .5f*Height, 0.f)), R3Add(B, V3(0.f, Height, 0.f)), .11f, .04f, 7, HeroShade(H, Forest_ColorRedwood));
 ForestPushRootFlare(H, B, .32f, .07f, 5);
 for(u32 Tier = 0; Tier < 6; Tier += 1)
 {
  f32 T = (f32)Tier/5.f;
  f32 Y = (.5f + .46f*T)*Height;
  f32 R = .55f - .3f*T;
  v3 P = R3Add(B, V3(.05f*RandomBilateral(&H->Series), Y, .05f*RandomBilateral(&H->Series)));
  R3PushEllipsoid(O, P, V3(R, .2f, R), 3, 8, HeroShade(H, (Tier & 1) ? Forest_ColorLeafDark : Hero_ColorPine), false);
 }
 R3PushCone(O, R3Add(B, V3(0.f, .96f*Height, 0.f)), R3Add(B, V3(0.f, 1.08f*Height, 0.f)), .2f, 7, HeroShade(H, Forest_ColorLeafDark));
}

// NOTE: The giant tree in the middle. Returns the centre of its platform.
internal v3
ForestPushGiantTree(hero_ctx *H, f32 X, f32 Z, f32 KnownFraction)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 f32 DeckY = 1.35f;

 //- Trunk in tapering pieces, with moss
 f32 Radii[4] = {.5f, .43f, .35f, .28f};
 f32 Heights[4] = {0.f, .8f, 1.7f, 2.5f};
 for(u32 Piece = 0; Piece < 3; Piece += 1)
 {
  ForestPushFrustum(O, R3Add(B, V3(0.f, Heights[Piece], 0.f)), R3Add(B, V3(0.f, Heights[Piece + 1], 0.f)), Radii[Piece], Radii[Piece + 1], 14, HeroShade(H, Forest_ColorBark));
 }
 for(u32 Patch = 0; Patch < 12; Patch += 1)
 {
  f32 A = 2.f*Pi32*RandomUnilateral(&H->Series);
  f32 Y = 2.2f*RandomUnilateral(&H->Series);
  f32 R = .5f - .085f*Y;
  R3PushEllipsoid(O, R3Add(B, V3(R*R3Cos(A), Y, R*R3Sin(A))), V3(.1f, .18f, .1f), 3, 6, HeroShade(H, Forest_ColorMoss), false);
 }

 //- Flared roots spreading over the ground and down the terrace
 for(u32 Root = 0; Root < 9; Root += 1)
 {
  f32 A = 2.f*Pi32*(f32)Root/9.f + .25f*RandomBilateral(&H->Series);
  f32 C = R3Cos(A);
  f32 S = R3Sin(A);
  v3 P0 = R3Add(B, V3(.38f*C, .6f, .38f*S));
  v3 P1 = R3Add(B, V3(.72f*C, .16f, .72f*S));
  v3 P2 = R3Add(B, V3(1.02f*C, -.06f, 1.02f*S));
  v3 P3 = R3Add(B, V3(1.32f*C, -.45f, 1.32f*S));
  v4 Bark = HeroShade(H, Forest_ColorBarkDark);
  ForestPushFrustum(O, P0, P1, .17f, .12f, 6, Bark);
  ForestPushFrustum(O, P1, P2, .12f, .07f, 6, Bark);
  ForestPushFrustum(O, P2, P3, .07f, .03f, 5, Bark);
 }

 //- Branches with their own leafy clumps, and the great crown
 for(u32 Branch = 0; Branch < 7; Branch += 1)
 {
  f32 A = 2.f*Pi32*(f32)Branch/7.f + .3f*RandomBilateral(&H->Series);
  v3 Start = R3Add(B, V3(0.f, 2.f + .35f*RandomUnilateral(&H->Series), 0.f));
  v3 End = R3Add(B, V3(1.25f*R3Cos(A), 2.9f + .35f*RandomUnilateral(&H->Series), 1.25f*R3Sin(A)));
  ForestPushFrustum(O, Start, End, .15f, .06f, 6, HeroShade(H, Forest_ColorBark));
  ForestPushCanopy(H, R3Add(End, V3(0.f, .25f, 0.f)), .7f, 5, Forest_ColorLeaf1, Forest_ColorLeaf2);
 }
 ForestPushCanopy(H, R3Add(B, V3(0.f, 3.55f, 0.f)), 1.6f, 22, Forest_ColorLeaf1, Forest_ColorLeaf2);
 ForestPushCanopy(H, R3Add(B, V3(.6f, 3.85f, -.4f)), .8f, 4, Forest_ColorAutumn1, Forest_ColorLeaf2);

 //- Spiral stair up the trunk to the platform
 v4 Plank = R3Color(Hero_ColorWoodLight);
 u32 Steps = 26;
 v3 PrevRail = B;
 for(u32 Step = 0; Step < Steps; Step += 1)
 {
  f32 T = ((f32)Step + .5f)/(f32)Steps;
  f32 A = .9f + T*2.f*Pi32*.95f;
  v3 Radial = V3(R3Cos(A), 0.f, R3Sin(A));
  v3 Tangent = V3(-R3Sin(A), 0.f, R3Cos(A));
  v3 P = R3Add(B, V3(.68f*Radial.X, T*DeckY + .02f, .68f*Radial.Z));
  R3PushOrientedBox(O, P, R3Scale(Radial, .14f), V3(0.f, .014f, 0.f), R3Scale(Tangent, .06f), W3Shade(Plank, &H->Series));
  v3 Rail = R3Add(B, V3(.83f*Radial.X, T*DeckY + .16f, .83f*Radial.Z));
  if(Step % 2 == 0) R3PushCylinder(O, R3Sub(Rail, V3(0.f, .14f, 0.f)), Rail, .008f, 4, R3Color(Hero_ColorWoodDark));
  if(Step > 0) R3PushCylinder(O, PrevRail, Rail, .007f, 4, R3Color(Hero_ColorWoodDark));
  PrevRail = Rail;
 }

 //- Platform around the trunk, with a railing and a lookout hut
 u32 Segments = 16;
 for(u32 Seg = 0; Seg < Segments; Seg += 1)
 {
  f32 A0 = 2.f*Pi32*(f32)Seg/(f32)Segments;
  f32 A1 = 2.f*Pi32*(f32)(Seg + 1)/(f32)Segments;
  f32 AM = .5f*(A0 + A1);
  v3 Radial = V3(R3Cos(AM), 0.f, R3Sin(AM));
  v3 Tangent = V3(-R3Sin(AM), 0.f, R3Cos(AM));
  R3PushOrientedBox(O, R3Add(B, V3(.64f*Radial.X, DeckY, .64f*Radial.Z)), R3Scale(Radial, .27f), V3(0.f, .025f, 0.f), R3Scale(Tangent, .135f), W3Shade(Plank, &H->Series));
  v3 R0 = R3Add(B, V3(.88f*R3Cos(A0), DeckY + .13f, .88f*R3Sin(A0)));
  v3 R1 = R3Add(B, V3(.88f*R3Cos(A1), DeckY + .13f, .88f*R3Sin(A1)));
  R3PushCylinder(O, R3Sub(R0, V3(0.f, .13f, 0.f)), R0, .01f, 4, R3Color(Hero_ColorWoodDark));
  R3PushCylinder(O, R0, R1, .008f, 4, R3Color(Hero_ColorWoodDark));
  // Beams under the platform
  if(Seg % 4 == 0) R3PushCylinder(O, R3Add(B, V3(.4f*Radial.X, DeckY - .5f, .4f*Radial.Z)), R3Add(B, V3(.82f*Radial.X, DeckY - .02f, .82f*Radial.Z)), .025f, 4, R3Color(Hero_ColorWoodDark));
 }
 HeroPushHouseAt(H, R3Add(B, V3(0.f, DeckY + .025f, .64f)), .2f, .13f, 1, 4|16|32);

 //- Knowledge crystal on a mast above the crown
 v3 MastBase = R3Add(B, V3(0.f, 4.3f, 0.f));
 v3 Crystal = R3Add(B, V3(0.f, 5.f, 0.f));
 R3PushCylinder(O, MastBase, Crystal, .025f, 5, R3Color(W3_ColorPost));
 HeroPushCrystal(H, Crystal, .2f);

 HeroMarkRect(H, X - 1.25f, Z - 1.2f, X + 1.25f, Z + 1.05f);
 v3 Result = R3Add(B, V3(0.f, DeckY, 0.f));
 return Result;
}

// NOTE: A treehouse: a tree with a deck around its trunk, a cabin on the deck and stairs
// down to the ground (on StairSide, -1 = left, 1 = right). Returns the deck centre.
internal v3
ForestPushTreehouse(hero_ctx *H, f32 X, f32 Z, f32 DeckY, f32 StairSide, u32 Variant, b32 Autumn)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 v4 Wood = R3Color(Hero_ColorWoodDark);

 //- Tree
 ForestPushFrustum(O, B, R3Add(B, V3(0.f, DeckY + 1.5f, 0.f)), .22f, .11f, 9, HeroShade(H, Forest_ColorBark));
 ForestPushRootFlare(H, B, .4f, .09f, 5);
 for(u32 Branch = 0; Branch < 4; Branch += 1)
 {
  f32 A = 1.6f*(f32)Branch + RandomUnilateral(&H->Series);
  R3PushCylinder(O, R3Add(B, V3(0.f, DeckY + 1.f, 0.f)), R3Add(B, V3(.55f*R3Cos(A), DeckY + 1.5f, .55f*R3Sin(A))), .04f, 4, HeroShade(H, Forest_ColorBark));
 }
 ForestPushCanopy(H, R3Add(B, V3(0.f, DeckY + 1.6f, 0.f)), .9f, 12, Autumn ? Forest_ColorAutumn1 : Forest_ColorLeaf1, Autumn ? Forest_ColorAutumn2 : Forest_ColorLeaf2);

 //- Deck, beams and railing
 v3 Deck = R3Add(B, V3(0.f, DeckY, 0.f));
 f32 DX = .5f;
 f32 DZ = .45f;
 R3PushBox(O, Deck, V3(DX, .03f, DZ), HeroShade(H, Hero_ColorWoodLight));
 for(u32 Corner = 0; Corner < 4; Corner += 1)
 {
  f32 SX = (Corner & 1) ? 1.f : -1.f;
  f32 SZ = (Corner & 2) ? 1.f : -1.f;
  R3PushCylinder(O, R3Add(B, V3(.12f*SX, DeckY - .55f, .1f*SZ)), R3Add(Deck, V3(.9f*DX*SX, -.03f, .9f*DZ*SZ)), .022f, 4, Wood);
 }
 f32 RailY = .14f;
 v3 C00 = R3Add(Deck, V3(-DX, RailY, -DZ));
 v3 C10 = R3Add(Deck, V3(DX, RailY, -DZ));
 v3 C01 = R3Add(Deck, V3(-DX, RailY, DZ));
 v3 C11 = R3Add(Deck, V3(DX, RailY, DZ));
 R3PushCylinder(O, C00, C10, .008f, 4, Wood);
 R3PushCylinder(O, C00, C01, .008f, 4, Wood);
 R3PushCylinder(O, C10, C11, .008f, 4, Wood);
 // Front rail with a gap where the stairs arrive
 if(StairSide > 0) R3PushCylinder(O, C01, R3Add(Deck, V3(.15f, RailY, DZ)), .008f, 4, Wood);
 else              R3PushCylinder(O, R3Add(Deck, V3(-.15f, RailY, DZ)), C11, .008f, 4, Wood);
 for(u32 Post = 0; Post < 4; Post += 1)
 {
  v3 P = (Post == 0) ? C00 : ((Post == 1) ? C10 : ((Post == 2) ? C01 : C11));
  R3PushCylinder(O, R3Sub(P, V3(0.f, RailY, 0.f)), P, .012f, 4, Wood);
 }

 //- Cabin on the deck (the trunk goes up through its back)
 HeroPushHouseAt(H, R3Add(Deck, V3(-.04f*StairSide, .03f, -.1f)), .28f, .2f, 1, Variant|16|32);

 //- Stairs down to the ground
 v3 StairTop = R3Add(Deck, V3(.34f*StairSide, .01f, DZ - .02f));
 v3 StairBottom = R3Add(HeroOn(H, X + .34f*StairSide, Z + 1.2f), V3(0.f, .01f, 0.f));
 HeroPushStairs(H, StairTop, StairBottom, .08f);

 HeroMarkRect(H, X - .62f, Z - .55f, X + .62f, Z + 1.3f);
 return Deck;
}

//~ Small forest things
internal void
ForestPushMushroom(hero_ctx *H, v3 B)
{
 f32 Size = .7f + .6f*RandomUnilateral(&H->Series);
 R3PushCylinder(H->Opaque, B, R3Add(B, V3(0.f, .045f*Size, 0.f)), .009f*Size, 4, R3Color(0xFFF2EBDD));
 u32 Cap = (RandomUnilateral(&H->Series) < .6f) ? 0xFFD8453A : 0xFFC9945A;
 R3PushEllipsoid(H->Opaque, R3Add(B, V3(0.f, .048f*Size, 0.f)), V3(.03f*Size, .016f*Size, .03f*Size), 3, 6, R3Color(Cap), false);
}

internal void
ForestPushLog(hero_ctx *H, f32 X, f32 Z)
{
 v3 B = HeroOn(H, X, Z);
 f32 A = Pi32*RandomUnilateral(&H->Series);
 v3 Dir = V3(.22f*R3Cos(A), 0.f, .22f*R3Sin(A));
 R3PushCylinder(H->Opaque, R3Add(R3Sub(B, Dir), V3(0.f, .05f, 0.f)), R3Add(R3Add(B, Dir), V3(0.f, .05f, 0.f)), .05f, 7, HeroShade(H, Forest_ColorBark));
 R3PushEllipsoid(H->Opaque, R3Add(B, V3(0.f, .1f, 0.f)), V3(.1f, .025f, .06f), 3, 6, HeroShade(H, Forest_ColorMoss), false);
}

//~ Rim: earth, roots and vines under a round island
internal void
ForestPushRim(hero_ctx *H)
{
 r3_mesh *O = H->Opaque;
 hero_layout *L = H->Layout;
 f32 R = L->Radius;
 f32 RingScale[6] = {1.f, .98f, .88f, .66f, .4f, .15f};
 f32 RingDepth[6] = {0.f, -.6f, -1.45f, -2.35f, -3.15f, -3.65f};
 v3 Ring[6][Forest_RimPoints];
 b32 PoolAt[Forest_RimPoints];
 for(u32 K = 0; K < Forest_RimPoints; K += 1)
 {
  f32 Angle = 2.f*Pi32*(f32)K/(f32)Forest_RimPoints;
  f32 C = R3Cos(Angle);
  f32 S = R3Sin(Angle);
  u32 Type;
  f32 TopY = HeroGroundY(L, .985f*R*C, .985f*R*S, &Type);
  PoolAt[K] = (Type == HeroGround_Pool);
  for(u32 Level = 0; Level < 6; Level += 1)
  {
   f32 Jitter = (Level <= 1) ? 1.f : 1.f + .12f*RandomBilateral(&H->Series);
   f32 Y = TopY;
   if(Level > 0)
   {
    Y = Min(RingDepth[Level], TopY - .3f*(f32)Level);
    if(Level >= 2) Y += .15f*RandomBilateral(&H->Series);
   }
   Ring[Level][K] = HeroP(H, R*RingScale[Level]*Jitter*C, Y, R*RingScale[Level]*Jitter*S);
  }
 }

 //- Earth: a mossy band at the top, dark soil below
 v3 Center = HeroP(H, 0.f, -2.f, 0.f);
 for(u32 Level = 0; Level < 5; Level += 1)
 {
  for(u32 K = 0; K < Forest_RimPoints; K += 1)
  {
   u32 Next = (K + 1) % Forest_RimPoints;
   f32 Roll = RandomUnilateral(&H->Series);
   u32 Color = (Roll < .5f) ? Forest_ColorSoil : Forest_ColorSoilDark;
   if(Roll > ((Level == 0) ? .62f : .9f)) Color = Forest_ColorMoss;
   R3PushQuad(O, Ring[Level][K], Ring[Level][Next], Ring[Level + 1][Next], Ring[Level + 1][K], Center, HeroShade(H, Color));
  }
 }
 v3 Tip = HeroP(H, .1f, -4.1f, -.05f);
 for(u32 K = 0; K < Forest_RimPoints; K += 1)
 {
  u32 Next = (K + 1) % Forest_RimPoints;
  R3PushTri(O, Ring[5][K], Ring[5][Next], Tip, Center, HeroShade(H, Forest_ColorSoilDark));
 }

 //- The pond seen through the edge
 v4 Deep = R3WithAlpha(R3Color(Hero_ColorPoolDeep), .7f);
 for(u32 K = 0; K < Forest_RimPoints; K += 1)
 {
  u32 Next = (K + 1) % Forest_RimPoints;
  if(PoolAt[K] && PoolAt[Next])
  {
   v3 A0 = Ring[0][K];
   v3 A1 = Ring[0][Next];
   v3 W0 = V3(A0.X, H->Top.Y + Hero_WaterLevel, A0.Z);
   v3 W1 = V3(A1.X, H->Top.Y + Hero_WaterLevel, A1.Z);
   v3 F0 = V3(A0.X, H->Top.Y + Hero_PoolFloor, A0.Z);
   v3 F1 = V3(A1.X, H->Top.Y + Hero_PoolFloor, A1.Z);
   R3PushTriUnlit(H->Translucent, W0, W1, F1, Deep);
   R3PushTriUnlit(H->Translucent, W0, F1, F0, Deep);
  }
 }

 //- Gnarled roots wrapping the underside, then hanging free
 for(u32 Root = 0; Root < 34; Root += 1)
 {
  u32 K = RandomNext(&H->Series) % Forest_RimPoints;
  s32 Drift = (s32)(RandomNext(&H->Series) % 5) - 2;
  v3 Prev = V3(0.f, 0.f, 0.f);
  for(u32 Level = 0; Level < 4; Level += 1)
  {
   u32 Idx = (u32)(((s32)K + Drift*(s32)Level + 4*Forest_RimPoints) % Forest_RimPoints);
   v3 P = Ring[Level][Idx];
   v3 Out = R3Normalize(V3(P.X - H->Top.X, 0.f, P.Z - H->Top.Z));
   P = R3Add(P, R3Scale(Out, .06f));
   if(Level == 0) P.Y -= .12f;
   if(Level > 0) R3PushCylinder(O, Prev, P, .08f - .016f*(f32)Level, 5, HeroShade(H, Forest_ColorRoot));
   Prev = P;
  }
  f32 Length = .3f + .6f*RandomUnilateral(&H->Series);
  v3 Mid = R3Add(Prev, V3(.12f*RandomBilateral(&H->Series), -Length, .12f*RandomBilateral(&H->Series)));
  v3 End = R3Add(Mid, V3(.15f*RandomBilateral(&H->Series), -.7f*Length, .15f*RandomBilateral(&H->Series)));
  R3PushCylinder(O, Prev, Mid, .028f, 4, HeroShade(H, Forest_ColorRoot));
  R3PushCylinder(O, Mid, End, .016f, 4, HeroShade(H, Forest_ColorRoot));
 }

 //- Hanging vines and a mossy lip
 for(u32 Vine = 0; Vine < 30; Vine += 1)
 {
  u32 K = RandomNext(&H->Series) % Forest_RimPoints;
  if(PoolAt[K]) continue;
  v3 Out = R3Normalize(V3(Ring[0][K].X - H->Top.X, 0.f, Ring[0][K].Z - H->Top.Z));
  v3 Start = R3Add(R3Add(Ring[0][K], R3Scale(Out, .04f)), V3(0.f, -.05f, 0.f));
  f32 Length = .5f + 1.5f*RandomUnilateral(&H->Series);
  v3 End = R3Add(Start, V3(.1f*RandomBilateral(&H->Series), -Length, .1f*RandomBilateral(&H->Series)));
  R3PushCylinder(O, Start, End, .014f, 4, R3Color(Hero_ColorVine));
  u32 Leaves = 3 + (u32)(Length*3.f);
  for(u32 Leaf = 0; Leaf < Leaves; Leaf += 1)
  {
   v3 P = R3Lerp(Start, End, ((f32)Leaf + .5f)/(f32)Leaves);
   R3PushEllipsoid(O, P, V3(.055f, .07f, .055f), 3, 5, HeroShade(H, (Leaf & 1) ? Hero_ColorVine : Hero_ColorLeaf), false);
  }
  if((Vine % 5) == 0) R3PushEllipsoid(O, R3Lerp(Start, End, .5f), V3(.025f, .025f, .025f), 3, 4, R3Color(0xFFF2D04E), false);
 }
 for(u32 K = 0; K < Forest_RimPoints; K += 2)
 {
  if(!PoolAt[K])
  {
   R3PushEllipsoid(O, Ring[0][K], V3(.15f, .08f, .15f), 3, 6, HeroShade(H, (K & 2) ? Forest_ColorMoss : Hero_ColorLeaf), false);
  }
 }

 //- Stones set in the earth band
 for(u32 Stone = 0; Stone < 9; Stone += 1)
 {
  u32 K = RandomNext(&H->Series) % Forest_RimPoints;
  v3 P = R3Lerp(Ring[0][K], Ring[1][K], .5f);
  HeroPushBoulder(H, P, .1f + .08f*RandomUnilateral(&H->Series));
 }
}

//~ The whole forest island
internal void
W3PushForestIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction)
{
 hero_layout Layout = {0};
 hero_ctx Ctx;
 hero_ctx *H = &Ctx;
 HeroBegin(H, App, &Layout, Top, W3_Seed*29 + 811*(u64)Index, Index);
 random_series *S = &H->Series;
 f32 Mirror = (RandomUnilateral(S) < .5f) ? 1.f : -1.f;   // which side the stream runs

 product *Products[4];
 u32 ProductCount = HeroSortedProducts(Island, Products, ArrayCount(Products));
 u64 FishAmount = HeroAmountOf(Island, ProductKind_Fish);

 //- Layout: round, a flat-topped mound for the giant tree, a pond at the front fed by
 // a stream from the mound
 Layout.Round = true;
 Layout.Radius = Forest_Radius;
 Layout.FrontLevel = 0.f;
 Layout.Pool = true;
 f32 PondCenter = -.1f*Mirror + .2f*RandomBilateral(S);
 Layout.PoolX0 = PondCenter - .85f;
 Layout.PoolX1 = PondCenter + .8f;
 Layout.PoolZ0 = 1.f + .15f*RandomUnilateral(S);
 Layout.HillX = .1f*RandomBilateral(S);
 Layout.HillZ = -.85f;
 Layout.HillStretch = 1.25f;
 Layout.HillPeak = 1.6f;
 Layout.HillCap = .8f;
 Layout.Stream[0][0] = Layout.HillX + .55f*Mirror;  Layout.Stream[0][1] = Layout.HillZ + .45f;
 Layout.Stream[1][0] = Layout.HillX + .8f*Mirror;   Layout.Stream[1][1] = .15f;
 Layout.Stream[2][0] = PondCenter + .4f*Mirror;     Layout.Stream[2][1] = .6f;
 Layout.Stream[3][0] = PondCenter + .15f*Mirror;    Layout.Stream[3][1] = Layout.PoolZ0 + .1f;
 Layout.StreamCount = 4;
 Layout.Path[0][0] = -1.15f*Mirror;  Layout.Path[0][1] = 1.55f;
 Layout.Path[1][0] = -.75f*Mirror;   Layout.Path[1][1] = .6f;
 Layout.Path[2][0] = Layout.HillX - .45f*Mirror; Layout.Path[2][1] = -.05f;
 Layout.PathCount = 3;

 HeroPushTerrain(H);
 ForestPushRim(H);

 //- Giant tree and the treehouses, joined by rope bridges
 v3 Platform = ForestPushGiantTree(H, Layout.HillX, Layout.HillZ - .1f, KnownFraction);
 v3 Decks[2];
 Decks[0] = ForestPushTreehouse(H, -1.55f, -.45f, 1.15f, 1.f, RandomNext(S) % 4, Mirror > 0.f);
 Decks[1] = ForestPushTreehouse(H, 1.55f, -.45f, 1.05f, -1.f, 1 + RandomNext(S) % 4, Mirror < 0.f);
 for(u32 Deck = 0; Deck < 2; Deck += 1)
 {
  v3 Flat = R3Normalize(V3(Decks[Deck].X - Platform.X, 0.f, Decks[Deck].Z - Platform.Z));
  v3 A = R3Add(R3Add(Platform, R3Scale(Flat, .86f)), V3(0.f, .03f, 0.f));
  v3 B = R3Add(R3Sub(Decks[Deck], R3Scale(Flat, .5f)), V3(0.f, .03f, 0.f));
  HeroPushRopeBridge(H, A, B);
 }

 //- Laundry strung between the treehouse stairs and a post
 f32 X, Z;
 if(HeroPlace(H, .3f, .06f, -.2f, .9f, &X, &Z)) HeroPushLaundry(H, X - .25f, X + .25f, Z);

 //- What the island makes, in the clearings (first, they need the most room)
 for(u32 Idx = 0; Idx < ProductCount; Idx += 1)
 {
  HeroPushProduct(H, Products[Idx]->Kind, Products[Idx]->Amount, 0);
 }

 //- Cottages with greenhouses
 for(u32 Cottage = 0; Cottage < 2; Cottage += 1)
 {
  if(HeroPlace(H, .36f, .32f, -.6f, 1.7f, &X, &Z))
  {
   HeroPushHouse(H, X, Z - .04f, .27f, .21f, 1, (RandomNext(S) % 4)|4|32);
   f32 GX, GZ;
   f32 HX = .22f, HZ = .2f;
   if(HeroPlaceNear(H, X + .62f*((Cottage == 0) ? 1.f : -1.f), Z + .05f, .3f, &HX, &HZ, &GX, &GZ)) HeroPushGreenhouse(H, GX, GZ, HX - .02f, HZ - .02f);
  }
 }

 //- Windmills and redwoods at the back
 for(u32 Mill = 0; Mill < 4; Mill += 1)
 {
  if(HeroPlace(H, .17f, .17f, -2.1f, -.5f, &X, &Z)) HeroPushWindmill(H, X, Z, 1.3f + .5f*RandomUnilateral(S), 6.f*RandomUnilateral(S));
 }
 for(u32 Redwood = 0; Redwood < 4; Redwood += 1)
 {
  if(HeroPlace(H, .22f, .22f, -2.2f, -1.f, &X, &Z)) ForestPushRedwood(H, X, Z, 3.6f + 1.f*RandomUnilateral(S));
 }

 //- The pond: a harbour only for an island that fishes, otherwise lily pads and stones
 if(FishAmount > 0)
 {
  HeroPushDock(H, Layout.PoolX0, Layout.PoolX0 + .6f, Layout.PoolZ0 + .5f, .1f);
  u32 Boats = Min(4, 1 + (u32)(FishAmount/40));
  for(u32 Boat = 0; Boat < Boats; Boat += 1)
  {
   HeroPushBoat(H, Layout.PoolX0 + .4f + .35f*(f32)Boat, Layout.PoolZ0 + .75f, (Boat & 1) ? 1.f : -1.f, 2.f*(f32)Boat, true);
  }
 }
 else
 {
  for(u32 Pad = 0; Pad < 10; Pad += 1)
  {
   f32 PX = Layout.PoolX0 + .15f + (Layout.PoolX1 - Layout.PoolX0 - .3f)*RandomUnilateral(S);
   f32 PZ = Layout.PoolZ0 + .15f + 1.f*RandomUnilateral(S);
   u32 Type;
   HeroGroundY(&Layout, PX, PZ, &Type);
   if(Type == HeroGround_Pool && PX*PX + PZ*PZ < 2.f*2.f)
   {
    R3PushEllipsoid(H->Opaque, HeroP(H, PX, Hero_WaterLevel + .01f, PZ), V3(.07f, .006f, .07f), 2, 7, HeroShade(H, 0xFF5E9E3E), false);
    if(Pad % 3 == 0) R3PushEllipsoid(H->Opaque, HeroP(H, PX + .02f, Hero_WaterLevel + .025f, PZ), V3(.02f, .015f, .02f), 3, 5, R3Color(0xFFF7D9E6), false);
   }
  }
 }
 for(u32 Stone = 0; Stone < 12; Stone += 1)
 {
  f32 T = RandomUnilateral(S);
  f32 PX, PZ;
  if(Stone % 3 == 0)      { PX = Layout.PoolX0 - .06f; PZ = Layout.PoolZ0 + 1.f*T; }
  else if(Stone % 3 == 1) { PX = Layout.PoolX1 + .06f; PZ = Layout.PoolZ0 + 1.f*T; }
  else                    { PX = Layout.PoolX0 + (Layout.PoolX1 - Layout.PoolX0)*T; PZ = Layout.PoolZ0 - .06f; }
  if(PX*PX + PZ*PZ < 2.15f*2.15f) HeroPushBoulder(H, R3Add(HeroOn(H, PX, PZ), V3(0.f, .04f, 0.f)), .06f + .06f*RandomUnilateral(S));
 }
 R3PushEllipsoid(H->Translucent, HeroP(H, Layout.Stream[3][0], Hero_WaterLevel + .03f, Layout.PoolZ0 + .12f), V3(.25f, .07f, .15f), 4, 8, R3WithAlpha(R3Color(0xFFFFFFFF), .5f), true);

 //- Market stall, crates, campfire, barrels, logs, mushrooms
 u32 Goods[3];
 u32 GoodsCount = 0;
 for(product *Product = Island->FirstProduct; Product && GoodsCount < ArrayCount(Goods); Product = Product->Next)
 {
  Goods[GoodsCount++] = W3ProductColors[Product->Kind];
 }
 if(HeroPlace(H, .42f, .32f, .3f, 1.9f, &X, &Z)) HeroPushStall(H, X, Z - .02f, Goods, GoodsCount, false);
 {
  f32 PX = 0.f, PZ = 0.f, NX = 0.f, NZ = 0.f;
  b32 HaveProduce = HeroPlace(H, .13f, .4f, -.8f, 1.9f, &PX, &PZ);
  b32 HaveNeeds = HeroPlace(H, .4f, .13f, -.8f, 1.9f, &NX, &NZ);
  if(HaveProduce && HaveNeeds) HeroPushCrates(H, Island, PX, PZ - .26f, NX - .26f, NZ);
 }
 if(HeroPlace(H, .15f, .15f, -.5f, 1.6f, &X, &Z)) HeroPushCampfire(H, X, Z);
 for(u32 Barrel = 0; Barrel < 4; Barrel += 1)
 {
  if(HeroPlace(H, .07f, .07f, -1.8f, 1.9f, &X, &Z)) HeroPushBarrel(H, X, Z);
 }
 for(u32 Log = 0; Log < 3; Log += 1)
 {
  if(HeroPlace(H, .24f, .24f, -1.9f, 1.9f, &X, &Z)) ForestPushLog(H, X, Z);
 }

 //- Woods: fill the free ground with oaks, some turning gold
 for(u32 Tree = 0; Tree < 40; Tree += 1)
 {
  f32 Size = .22f + .06f*RandomUnilateral(S);
  if(HeroPlace(H, Size, Size, -2.2f, 2.2f, &X, &Z))
  {
   ForestPushBroadTree(H, X, Z, 1.f + .7f*RandomUnilateral(S), RandomUnilateral(S) < .28f);
  }
 }
 for(u32 Mushroom = 0; Mushroom < 24; Mushroom += 1)
 {
  f32 PX = 2.f*RandomBilateral(S);
  f32 PZ = 2.f*RandomBilateral(S);
  u32 Type;
  HeroGroundY(&Layout, PX, PZ, &Type);
  if(Type == HeroGround_Grass && PX*PX + PZ*PZ < 2.1f*2.1f) ForestPushMushroom(H, HeroOn(H, PX, PZ));
 }

 //- Villagers
 u32 People = 7 + RandomNext(S) % 4;
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
 // Someone on each treehouse deck
 HeroPushPerson(H, R3Add(Decks[0], V3(.3f, .03f, .25f)), HeroShirtColors[1]);
 HeroPushPerson(H, R3Add(Decks[1], V3(-.3f, .03f, .25f)), HeroShirtColors[3]);

 HeroFinish(H, App, StatusColor, Highlight, HighlightColor);
}
