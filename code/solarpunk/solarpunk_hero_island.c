//-
// SolarPunk 3D hero island: one hand-built island in the style of the reference art.
// A square slab floating over the clouds (grass, a dirt band, stacked stone and a
// rugged underside with hanging roots), a terraced hill of timber houses with solar
// roofs, wooden windmills, a stream that cascades down the terraces into a harbour
// pool cut into the front (docks, boats, a water wheel), market stalls, gardens,
// greenhouses, pine trees, animals and villagers.
//
// Local coordinates: X right, Y up, Z towards the front (the default camera side).
// Y = 0 is the island's top (R3->IslandTop).
//-

//~ Constants
#define Hero_Half        2.3f   // half size of the square slab
#define Hero_Cells       40     // terrain grid cells per side
#define Hero_Step        .4f    // height of one hill terrace
#define Hero_DirtBottom  -.85f  // the dirt band ends and the stacked stone starts
#define Hero_StoneBottom -2.1f
#define Hero_StoneRows   5
#define Hero_PoolX0      -.6f
#define Hero_PoolX1      1.4f
#define Hero_PoolZ0      .9f
#define Hero_PoolFloor   -1.6f
#define Hero_WaterLevel  -.8f
#define Hero_FrontLevel  -.4f
#define Hero_UnderPoints 32

//- Colours
#define Hero_ColorGrassA     0xFF6E9E3C
#define Hero_ColorGrassB     0xFF93BD54
#define Hero_ColorPath       0xFFB8946A
#define Hero_ColorStreamTop  0xFF6BCFE0
#define Hero_ColorPoolFloor  0xFFB9A47C
#define Hero_ColorPoolWater  0xFF3CC3CC
#define Hero_ColorPoolDeep   0xFF1E9AAE
#define Hero_ColorFall       0xFFE4F8FF
#define Hero_ColorWall       0xFF9C9384
#define Hero_ColorDirt       0xFF6E4C31
#define Hero_ColorCore       0xFF5F5850
#define Hero_ColorRoot       0xFF4B3727
#define Hero_ColorWood       0xFF8A6240
#define Hero_ColorWoodDark   0xFF5C4330
#define Hero_ColorWoodLight  0xFFB08458
#define Hero_ColorStone      0xFF9A9184
#define Hero_ColorWindow     0xFFFFC66E
#define Hero_ColorPanel      0xFF2C4F8F
#define Hero_ColorPanelFrame 0xFFC9CDD3
#define Hero_ColorPanelLine  0xFF6F8FC4
#define Hero_ColorVine       0xFF4E8B36
#define Hero_ColorLeaf       0xFF5E9E3E
#define Hero_ColorPine       0xFF2F6B3D
#define Hero_ColorGlass      0xFFCFEFEA
#define Hero_ColorCloth      0xFFF1E6CF
#define Hero_ColorCloth2     0xFFE5CFA8
#define Hero_ColorTin        0xFF9A6A45
#define Hero_ColorTank       0xFF6E9FB0
#define Hero_ColorSoil       0xFF5B4130
#define Hero_ColorSkin       0xFFE2B48E
#define Hero_ColorPants      0xFF3E4A5C
#define Hero_ColorFire       0xFFFF9A3A
#define Hero_ColorFireCore   0xFFFFE07A
#define Hero_ColorCloud      0xFFFFF1E2

global_variable u32 HeroStoneColors[] = {0xFF8E8678, 0xFFA79D8C, 0xFF7B7369, 0xFFB8AC97, 0xFF6E665D, 0xFF9A8670};
global_variable u32 HeroWoodColors[]  = {0xFF8A6240, 0xFF7A5434, 0xFF9C7350};
global_variable u32 HeroRoofColors[]  = {0xFF6A4E3A, 0xFF8E5A3A, 0xFF5E5348};
global_variable u32 HeroShirtColors[] = {0xFFD9653B, 0xFF4F7DB8, 0xFFE8C65A, 0xFF6FA35A, 0xFFB8584F, 0xFFEFE6D6};

// NOTE: The stream and the dirt paths, as polylines over the top (X, Z).
global_variable f32 HeroStreamPoints[][2] = {{.45f, -1.8f}, {.6f, -1.05f}, {.8f, -.35f}, {.85f, 1.f}};
global_variable f32 HeroPathPoints[][2]   = {{-.85f, .95f}, {-.6f, .3f}, {.1f, .15f}, {.55f, .2f}};
global_variable f32 HeroPath2Points[][2]  = {{.1f, .15f}, {-.15f, -.3f}, {-.1f, -.75f}};

// NOTE: Ground taken by buildings (X0, Z0, X1, Z1): no bushes are scattered there.
global_variable f32 HeroTakenRects[][4] =
{
 {-.5f, -1.85f, .2f, -1.25f},   {-1.55f, -1.7f, -.85f, -1.1f}, {-2.15f, -.8f, -1.35f, -.1f},
 {-1.6f, .1f, -1.f, .6f},       {-.85f, -.9f, -.15f, -.3f},    {-.1f, -1.2f, .4f, -.7f},
 {1.2f, -.85f, 2.f, -.15f},     {1.25f, -2.f, 1.95f, -1.4f},   {.8f, -1.4f, 1.3f, -.9f},
 {1.f, -.05f, 1.8f, .75f},      {1.75f, .15f, 2.25f, .75f},    {-2.25f, .3f, -1.65f, .8f},
 {-1.f, .35f, -.6f, .75f},      {-.25f, .25f, .45f, .7f},      {-2.3f, -2.3f, -.6f, -1.75f},
 {1.7f, -2.3f, 2.3f, -.95f},    {0.f, -2.2f, .6f, -1.6f},
};

enum hero_ground
{
 HeroGround_Grass,
 HeroGround_Path,
 HeroGround_Stream,
 HeroGround_Pool,
};

typedef struct hero_ctx hero_ctx;
struct hero_ctx
{
 r3_mesh *Opaque;
 r3_mesh *Translucent;
 v3 Top;
 f32 Time;
 random_series Series;
};

//~ Helpers
internal v3
HeroP(hero_ctx *H, f32 X, f32 Y, f32 Z)
{
 v3 Result = V3(H->Top.X + X, H->Top.Y + Y, H->Top.Z + Z);
 return Result;
}

internal v4
HeroShade(hero_ctx *H, u32 Color)
{
 v4 Result = W3Shade(R3Color(Color), &H->Series);
 return Result;
}

internal u32
HeroPick(hero_ctx *H, u32 *Colors, u32 Count)
{
 u32 Result = Colors[RandomNext(&H->Series) % Count];
 return Result;
}

internal f32
HeroSegmentDistance(f32 PX, f32 PZ, f32 AX, f32 AZ, f32 BX, f32 BZ)
{
 f32 DX = BX - AX;
 f32 DZ = BZ - AZ;
 f32 LengthSq = DX*DX + DZ*DZ;
 f32 T = (LengthSq > 0.f) ? ((PX - AX)*DX + (PZ - AZ)*DZ)/LengthSq : 0.f;
 T = Clamp(0.f, T, 1.f);
 f32 EX = AX + T*DX - PX;
 f32 EZ = AZ + T*DZ - PZ;
 f32 Result = R3Sqrt(EX*EX + EZ*EZ);
 return Result;
}

internal f32
HeroPolylineDistance(f32 PX, f32 PZ, f32 (*Points)[2], u32 Count)
{
 f32 Result = 1e9f;
 for(u32 Idx = 0; Idx + 1 < Count; Idx += 1)
 {
  f32 Distance = HeroSegmentDistance(PX, PZ, Points[Idx][0], Points[Idx][1], Points[Idx + 1][0], Points[Idx + 1][1]);
  Result = Min(Result, Distance);
 }
 return Result;
}

// NOTE: The height of the ground at (X, Z) and what covers it. Heights come in
// terraces: the front harbour sits low, the hill at the back rises in Hero_Step steps.
internal f32
HeroTerrain(f32 X, f32 Z, u32 *Type)
{
 f32 Result = 0.f;
 *Type = HeroGround_Grass;
 if(Z > Hero_PoolZ0)
 {
  if(X > Hero_PoolX0 && X < Hero_PoolX1)
  {
   *Type = HeroGround_Pool;
   Result = Hero_PoolFloor;
  }
  else
  {
   Result = Hero_FrontLevel;
   *Type = (X < Hero_PoolX0) ? HeroGround_Path : HeroGround_Grass;
  }
 }
 else
 {
  f32 DX = (X + .15f)/1.3f;
  f32 DZ = Z + 1.45f;
  f32 Raw = 2.f - .8f*R3Sqrt(DX*DX + DZ*DZ);
  Result = (Raw > 0.f) ? Hero_Step*floorf(Raw/Hero_Step) : 0.f;
  if(X > .95f && Z < .75f)  Result = Max(Result, Hero_Step);
  if(X < -1.15f && Z < .2f) Result = Max(Result, Hero_Step);

  if(HeroPolylineDistance(X, Z, HeroStreamPoints, ArrayCount(HeroStreamPoints)) < .15f)
  {
   *Type = HeroGround_Stream;
  }
  else if(HeroPolylineDistance(X, Z, HeroPathPoints, ArrayCount(HeroPathPoints)) < .09f ||
          HeroPolylineDistance(X, Z, HeroPath2Points, ArrayCount(HeroPath2Points)) < .08f)
  {
   *Type = HeroGround_Path;
  }
 }
 return Result;
}

internal f32
HeroCellSize(void)
{
 f32 Result = 2.f*Hero_Half/(f32)Hero_Cells;
 return Result;
}

// NOTE: Ground height of the terrain cell under (X, Z), so objects sit exactly on it.
internal f32
HeroGroundY(f32 X, f32 Z, u32 *Type)
{
 f32 Cell = HeroCellSize();
 s32 I = Clamp(0, (s32)floorf((X + Hero_Half)/Cell), Hero_Cells - 1);
 s32 J = Clamp(0, (s32)floorf((Z + Hero_Half)/Cell), Hero_Cells - 1);
 f32 CX = -Hero_Half + ((f32)I + .5f)*Cell;
 f32 CZ = -Hero_Half + ((f32)J + .5f)*Cell;
 f32 Result = HeroTerrain(CX, CZ, Type);
 return Result;
}

internal v3
HeroOn(hero_ctx *H, f32 X, f32 Z)
{
 u32 Type;
 v3 Result = HeroP(H, X, HeroGroundY(X, Z, &Type), Z);
 return Result;
}

// NOTE: A glowing (unlit) rectangle: Center +- U +- V.
internal void
HeroGlowRect(r3_mesh *Mesh, v3 Center, v3 U, v3 V, v4 Color)
{
 v3 P0 = R3Sub(R3Sub(Center, U), V);
 v3 P1 = R3Sub(R3Add(Center, U), V);
 v3 P2 = R3Add(R3Add(Center, U), V);
 v3 P3 = R3Add(R3Sub(Center, U), V);
 R3PushTriUnlit(Mesh, P0, P1, P2, Color);
 R3PushTriUnlit(Mesh, P0, P2, P3, Color);
}

//~ Terrain
// NOTE: A vertical wall on one edge of a terrain cell. Dir: 0 = +X, 1 = -X, 2 = +Z, 3 = -Z.
internal void
HeroEdgeWall(hero_ctx *H, r3_mesh *Mesh, u32 Dir, f32 X0, f32 X1, f32 Z0, f32 Z1, f32 YLow, f32 YHigh, f32 Push, v4 Color)
{
 v3 A, B, Out;
 switch(Dir)
 {
  case 0:  { A = V3(X1, 0.f, Z0); B = V3(X1, 0.f, Z1); Out = V3(1.f, 0.f, 0.f); } break;
  case 1:  { A = V3(X0, 0.f, Z0); B = V3(X0, 0.f, Z1); Out = V3(-1.f, 0.f, 0.f); } break;
  case 2:  { A = V3(X0, 0.f, Z1); B = V3(X1, 0.f, Z1); Out = V3(0.f, 0.f, 1.f); } break;
  default: { A = V3(X0, 0.f, Z0); B = V3(X1, 0.f, Z0); Out = V3(0.f, 0.f, -1.f); } break;
 }
 A = R3Add(A, R3Scale(Out, Push));
 B = R3Add(B, R3Scale(Out, Push));
 v3 P0 = HeroP(H, A.X, YLow, A.Z);
 v3 P1 = HeroP(H, B.X, YLow, B.Z);
 v3 P2 = HeroP(H, B.X, YHigh, B.Z);
 v3 P3 = HeroP(H, A.X, YHigh, A.Z);
 v3 Inside = R3Sub(R3Scale(R3Add(P0, P2), .5f), Out);
 R3PushQuad(Mesh, P0, P1, P2, P3, Inside, Color);
}

internal void
HeroPushTerrain(hero_ctx *H)
{
 f32 Cell = HeroCellSize();
 s32 DI[4] = {1, -1, 0, 0};
 s32 DJ[4] = {0, 0, 1, -1};

 for(s32 J = 0; J < Hero_Cells; J += 1)
 {
  for(s32 I = 0; I < Hero_Cells; I += 1)
  {
   f32 X0 = -Hero_Half + (f32)I*Cell;
   f32 Z0 = -Hero_Half + (f32)J*Cell;
   f32 X1 = X0 + Cell;
   f32 Z1 = Z0 + Cell;
   u32 Type;
   f32 Level = HeroTerrain(X0 + .5f*Cell, Z0 + .5f*Cell, &Type);

   //- Top
   v4 TopColor;
   switch(Type)
   {
    case HeroGround_Path:   TopColor = HeroShade(H, Hero_ColorPath); break;
    case HeroGround_Stream: TopColor = R3Color(Hero_ColorStreamTop); break;
    case HeroGround_Pool:   TopColor = HeroShade(H, Hero_ColorPoolFloor); break;
    default:
    {
     TopColor = R3LerpColor(R3Color(Hero_ColorGrassA), R3Color(Hero_ColorGrassB), RandomUnilateral(&H->Series));
    } break;
   }
   R3PushQuad(H->Opaque, HeroP(H, X0, Level, Z0), HeroP(H, X1, Level, Z0), HeroP(H, X1, Level, Z1), HeroP(H, X0, Level, Z1),
              HeroP(H, X0 + .5f*Cell, Level - 1.f, Z0 + .5f*Cell), TopColor);

   //- Walls down to lower neighbours, and the outside of the slab
   for(u32 Dir = 0; Dir < 4; Dir += 1)
   {
    s32 NI = I + DI[Dir];
    s32 NJ = J + DJ[Dir];
    if(NI >= 0 && NI < Hero_Cells && NJ >= 0 && NJ < Hero_Cells)
    {
     u32 NeighbourType;
     f32 NeighbourLevel = HeroTerrain(-Hero_Half + ((f32)NI + .5f)*Cell, -Hero_Half + ((f32)NJ + .5f)*Cell, &NeighbourType);
     if(NeighbourLevel < Level - .001f)
     {
      HeroEdgeWall(H, H->Opaque, Dir, X0, X1, Z0, Z1, NeighbourLevel, Level, 0.f, HeroShade(H, Hero_ColorWall));
      if(Type == HeroGround_Stream)
      {
       // Water falling over the terrace edge.
       f32 Bottom = (NeighbourType == HeroGround_Pool) ? Hero_WaterLevel : NeighbourLevel;
       HeroEdgeWall(H, H->Translucent, Dir, X0, X1, Z0, Z1, Bottom, Level + .01f, .025f, R3WithAlpha(R3Color(Hero_ColorFall), .72f));
      }
     }
    }
    else if(Type != HeroGround_Pool)
    {
     if(Level > 0.f)
     {
      HeroEdgeWall(H, H->Opaque, Dir, X0, X1, Z0, Z1, 0.f, Level, 0.f, HeroShade(H, Hero_ColorWall));
     }
     HeroEdgeWall(H, H->Opaque, Dir, X0, X1, Z0, Z1, Hero_DirtBottom, Min(Level, 0.f), 0.f, HeroShade(H, Hero_ColorDirt));
    }
   }
  }
 }

 //- Harbour water: the surface and the see-through cut at the front of the slab
 v4 Water = R3WithAlpha(R3Color(Hero_ColorPoolWater), .62f);
 v4 Deep = R3WithAlpha(R3Color(Hero_ColorPoolDeep), .7f);
 v3 W0 = HeroP(H, Hero_PoolX0, Hero_WaterLevel, Hero_PoolZ0);
 v3 W1 = HeroP(H, Hero_PoolX1, Hero_WaterLevel, Hero_PoolZ0);
 v3 W2 = HeroP(H, Hero_PoolX1, Hero_WaterLevel, Hero_Half);
 v3 W3 = HeroP(H, Hero_PoolX0, Hero_WaterLevel, Hero_Half);
 R3PushTriUnlit(H->Translucent, W0, W1, W2, Water);
 R3PushTriUnlit(H->Translucent, W0, W2, W3, Water);
 v3 F0 = HeroP(H, Hero_PoolX0, Hero_PoolFloor, Hero_Half);
 v3 F1 = HeroP(H, Hero_PoolX1, Hero_PoolFloor, Hero_Half);
 R3PushTriUnlit(H->Translucent, W3, W2, F1, Deep);
 R3PushTriUnlit(H->Translucent, W3, F1, F0, Deep);
}

//~ Cliff: dirt band, stacked stone and the rugged underside
internal void
HeroPushCliff(hero_ctx *H)
{
 r3_mesh *O = H->Opaque;
 f32 In = Hero_Half - .02f;
 v4 Core = R3Color(Hero_ColorCore);

 //- Core, so there are no holes between the stones. The front part leaves room for
 // the harbour's water cut.
 f32 MidY = .5f*(Hero_DirtBottom + Hero_StoneBottom);
 f32 HalfY = .5f*(Hero_DirtBottom - Hero_StoneBottom);
 R3PushBox(O, HeroP(H, 0.f, MidY, .5f*(-In + Hero_PoolZ0)), V3(In, HalfY, .5f*(Hero_PoolZ0 + In)), Core);
 R3PushBox(O, HeroP(H, 0.f, .5f*(Hero_PoolFloor + Hero_StoneBottom), .5f*(Hero_PoolZ0 + In)),
           V3(In, .5f*(Hero_PoolFloor - Hero_StoneBottom), .5f*(In - Hero_PoolZ0)), Core);
 R3PushBox(O, HeroP(H, .5f*(-In + Hero_PoolX0), .5f*(Hero_DirtBottom + Hero_PoolFloor), .5f*(Hero_PoolZ0 + In)),
           V3(.5f*(Hero_PoolX0 + In), .5f*(Hero_DirtBottom - Hero_PoolFloor), .5f*(In - Hero_PoolZ0)), Core);
 R3PushBox(O, HeroP(H, .5f*(In + Hero_PoolX1), .5f*(Hero_DirtBottom + Hero_PoolFloor), .5f*(Hero_PoolZ0 + In)),
           V3(.5f*(In - Hero_PoolX1), .5f*(Hero_DirtBottom - Hero_PoolFloor), .5f*(In - Hero_PoolZ0)), Core);

 //- Stacked stone blocks in staggered rows on all four sides
 f32 RowHeight = (Hero_DirtBottom - Hero_StoneBottom)/(f32)Hero_StoneRows;
 for(u32 Side = 0; Side < 4; Side += 1)
 {
  for(u32 Row = 0; Row < Hero_StoneRows; Row += 1)
  {
   f32 YTop = Hero_DirtBottom - RowHeight*(f32)Row;
   f32 YMid = YTop - .5f*RowHeight;
   f32 S = -Hero_Half - ((Row & 1) ? .17f : 0.f);
   while(S < Hero_Half)
   {
    f32 Length = .26f + .3f*RandomUnilateral(&H->Series);
    f32 S0 = Max(S, -Hero_Half);
    f32 S1 = Min(S + Length, Hero_Half);
    S += Length;
    f32 Out = .03f + .08f*RandomUnilateral(&H->Series);
    b32 Skip = (S1 - S0 < .08f);
    if(Side == 0 && S1 > Hero_PoolX0 && S0 < Hero_PoolX1 && YTop > Hero_PoolFloor + .01f) Skip = true;
    if(!Skip)
    {
     f32 Along = .5f*(S0 + S1);
     f32 HalfAlong = .5f*(S1 - S0) - .014f;
     f32 Depth = Hero_Half + .5f*(Out - .06f);
     f32 HalfDepth = .5f*(Out + .06f);
     f32 HalfHeight = .5f*RowHeight - .014f;
     v3 Center, Half;
     switch(Side)
     {
      case 0:  { Center = HeroP(H, Along, YMid, Depth);  Half = V3(HalfAlong, HalfHeight, HalfDepth); } break;
      case 1:  { Center = HeroP(H, Along, YMid, -Depth); Half = V3(HalfAlong, HalfHeight, HalfDepth); } break;
      case 2:  { Center = HeroP(H, Depth, YMid, Along);  Half = V3(HalfDepth, HalfHeight, HalfAlong); } break;
      default: { Center = HeroP(H, -Depth, YMid, Along); Half = V3(HalfDepth, HalfHeight, HalfAlong); } break;
     }
     R3PushBox(O, Center, Half, HeroShade(H, HeroPick(H, HeroStoneColors, ArrayCount(HeroStoneColors))));
    }
   }
  }
 }

 //- Underside: rings from the square slab to a rough rocky point
 f32 RingY[4] = {Hero_StoneBottom, -2.9f, -3.6f, -4.15f};
 f32 RingScale[4] = {1.f, .8f, .52f, .22f};
 v3 Ring[4][Hero_UnderPoints];
 for(u32 R = 0; R < 4; R += 1)
 {
  for(u32 K = 0; K < Hero_UnderPoints; K += 1)
  {
   f32 Angle = 2.f*Pi32*(f32)K/(f32)Hero_UnderPoints;
   f32 C = R3Cos(Angle);
   f32 S = R3Sin(Angle);
   f32 M = Max(AbsF32(C), AbsF32(S));
   f32 Round = (f32)R/3.f;
   f32 UX = (1.f - Round)*(C/M) + Round*C;
   f32 UZ = (1.f - Round)*(S/M) + Round*S;
   f32 Jitter = (R == 0) ? 1.f : 1.f + .14f*RandomBilateral(&H->Series);
   f32 Scale = Hero_Half*RingScale[R]*Jitter;
   Ring[R][K] = HeroP(H, UX*Scale + .08f*(f32)R, RingY[R] + ((R == 0) ? 0.f : .15f*RandomBilateral(&H->Series)), UZ*Scale - .05f*(f32)R);
  }
 }
 v3 UnderCenter = HeroP(H, .2f, -3.f, -.1f);
 for(u32 R = 0; R < 3; R += 1)
 {
  for(u32 K = 0; K < Hero_UnderPoints; K += 1)
  {
   u32 Next = (K + 1) % Hero_UnderPoints;
   R3PushQuad(O, Ring[R][K], Ring[R][Next], Ring[R + 1][Next], Ring[R + 1][K], UnderCenter,
              HeroShade(H, HeroPick(H, HeroStoneColors, ArrayCount(HeroStoneColors))));
  }
 }
 v3 Tip = HeroP(H, .3f, -4.6f, -.2f);
 for(u32 K = 0; K < Hero_UnderPoints; K += 1)
 {
  u32 Next = (K + 1) % Hero_UnderPoints;
  R3PushTri(O, Ring[3][K], Ring[3][Next], Tip, UnderCenter, HeroShade(H, HeroStoneColors[4]));
 }

 //- Boulders sticking out of the underside
 for(u32 Idx = 0; Idx < 16; Idx += 1)
 {
  u32 K = RandomNext(&H->Series) % Hero_UnderPoints;
  u32 R = RandomNext(&H->Series) % 2;
  v3 P = R3Lerp(Ring[R][K], Ring[R + 1][K], .3f + .5f*RandomUnilateral(&H->Series));
  f32 Size = .2f + .25f*RandomUnilateral(&H->Series);
  R3PushEllipsoid(O, P, V3(Size*1.3f, Size*.8f, Size*1.2f), 4, 7, HeroShade(H, HeroPick(H, HeroStoneColors, ArrayCount(HeroStoneColors))), false);
 }

 //- Hanging roots
 for(u32 Idx = 0; Idx < 14; Idx += 1)
 {
  u32 K = RandomNext(&H->Series) % Hero_UnderPoints;
  u32 R = 1 + RandomNext(&H->Series) % 2;
  v3 Start = R3Lerp(Ring[R][K], Ring[R + 1][K], .5f*RandomUnilateral(&H->Series));
  f32 Length = .4f + .8f*RandomUnilateral(&H->Series);
  v3 Kink = R3Add(Start, V3(.12f*RandomBilateral(&H->Series), -.5f*Length, .12f*RandomBilateral(&H->Series)));
  v3 End = R3Add(Kink, V3(.15f*RandomBilateral(&H->Series), -.5f*Length, .15f*RandomBilateral(&H->Series)));
  R3PushCylinder(O, Start, Kink, .03f, 4, R3Color(Hero_ColorRoot));
  R3PushCylinder(O, Kink, End, .018f, 4, R3Color(Hero_ColorRoot));
  R3PushCone(O, End, R3Add(End, V3(0.f, -.2f, 0.f)), .018f, 4, R3Color(Hero_ColorRoot));
 }

 //- Grass tufts over the edge and vines down the dirt band
 for(u32 Side = 0; Side < 4; Side += 1)
 {
  for(f32 S = -Hero_Half + .08f; S < Hero_Half; S += .17f)
  {
   if(Side == 0 && S > Hero_PoolX0 - .05f && S < Hero_PoolX1 + .05f) continue;
   f32 X = (Side == 0 || Side == 1) ? S : ((Side == 2) ? Hero_Half : -Hero_Half);
   f32 Z = (Side == 0) ? Hero_Half : ((Side == 1) ? -Hero_Half : S);
   u32 Type;
   f32 Level = HeroGroundY(X*.995f, Z*.995f, &Type);
   f32 Roll = RandomUnilateral(&H->Series);
   if(Roll < .55f)
   {
    v3 P = HeroP(H, X*1.01f, Level, Z*1.01f);
    R3PushEllipsoid(O, P, V3(.1f, .07f, .1f), 3, 6, HeroShade(H, Hero_ColorLeaf), false);
   }
   if(Roll > .72f)
   {
    f32 Length = .25f + .5f*RandomUnilateral(&H->Series);
    v3 Start = HeroP(H, X*1.012f, Min(Level, 0.f), Z*1.012f);
    v3 End = R3Add(Start, V3(0.f, -Length, 0.f));
    R3PushCylinder(O, Start, End, .018f, 4, R3Color(Hero_ColorVine));
    for(u32 Leaf = 0; Leaf < 3; Leaf += 1)
    {
     R3PushEllipsoid(O, R3Lerp(Start, End, .3f + .33f*(f32)Leaf), V3(.05f, .06f, .05f), 3, 5, HeroShade(H, Hero_ColorVine), false);
    }
   }
  }
 }
}

//~ Buildings
// NOTE: A solar panel lying on a roof slope: Along = across the slope, Up = up the
// slope, Normal = out of the roof (all unit vectors).
internal void
HeroPushPanel(hero_ctx *H, v3 Center, v3 Along, v3 Up, v3 Normal, f32 HalfW, f32 HalfL)
{
 v3 FrameCenter = R3Add(Center, R3Scale(Normal, .012f));
 R3PushOrientedBox(H->Opaque, FrameCenter, R3Scale(Along, HalfW + .012f), R3Scale(Normal, .01f), R3Scale(Up, HalfL + .012f), R3Color(Hero_ColorPanelFrame));
 v3 PanelCenter = R3Add(Center, R3Scale(Normal, .024f));
 R3PushOrientedBox(H->Opaque, PanelCenter, R3Scale(Along, HalfW), R3Scale(Normal, .006f), R3Scale(Up, HalfL), R3Color(Hero_ColorPanel));
 // Cell lines
 for(u32 Line = 1; Line < 3; Line += 1)
 {
  f32 T = -HalfW + 2.f*HalfW*(f32)Line/3.f;
  R3PushOrientedBox(H->Opaque, R3Add(PanelCenter, R3Add(R3Scale(Along, T), R3Scale(Normal, .004f))),
                    R3Scale(Along, .004f), R3Scale(Normal, .004f), R3Scale(Up, HalfL), R3Color(Hero_ColorPanelLine));
 }
 R3PushOrientedBox(H->Opaque, R3Add(PanelCenter, R3Scale(Normal, .004f)), R3Scale(Along, HalfW), R3Scale(Normal, .004f), R3Scale(Up, .004f), R3Color(Hero_ColorPanelLine));
}

// NOTE: A window on a wall facing Out: a dark frame with warm light inside.
internal void
HeroPushWindow(hero_ctx *H, v3 Center, v3 Out, v3 Along, f32 HalfW, f32 HalfH)
{
 v3 Up = V3(0.f, 1.f, 0.f);
 R3PushOrientedBox(H->Opaque, Center, R3Scale(Along, HalfW + .014f), R3Scale(Up, HalfH + .014f), R3Scale(Out, .008f), R3Color(Hero_ColorWoodDark));
 HeroGlowRect(H->Opaque, R3Add(Center, R3Scale(Out, .011f)), R3Scale(Along, HalfW), R3Scale(Up, HalfH), R3Color(Hero_ColorWindow));
 // Cross bars
 R3PushOrientedBox(H->Opaque, R3Add(Center, R3Scale(Out, .013f)), R3Scale(Along, .005f), R3Scale(Up, HalfH), R3Scale(Out, .003f), R3Color(Hero_ColorWoodDark));
 R3PushOrientedBox(H->Opaque, R3Add(Center, R3Scale(Out, .013f)), R3Scale(Along, HalfW), R3Scale(Up, .005f), R3Scale(Out, .003f), R3Color(Hero_ColorWoodDark));
}

// NOTE: A timber house on a stone plinth: Floors storeys, a pitched roof with its ridge
// along X, solar panels on the front slope, glowing windows and climbing vines.
// Variant bits: 1 = chimney, 2 = balcony, 4 = two panels.
internal void
HeroPushHouse(hero_ctx *H, f32 X, f32 Z, f32 HX, f32 HZ, u32 Floors, u32 Variant)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 f32 Plinth = .14f;
 f32 FloorH = .34f;
 f32 WallH = FloorH*(f32)Floors;
 u32 Wood = HeroWoodColors[Variant % ArrayCount(HeroWoodColors)];
 u32 Roof = HeroRoofColors[(Variant/2) % ArrayCount(HeroRoofColors)];

 //- Stone plinth (reaches down so houses on terrace edges do not float)
 R3PushBox(O, R3Add(B, V3(0.f, .5f*Plinth - .25f, 0.f)), V3(HX + .035f, .5f*Plinth + .25f, HZ + .035f), HeroShade(H, Hero_ColorStone));

 //- Walls, corner posts and floor beams
 f32 Y0 = B.Y + Plinth;
 R3PushBox(O, V3(B.X, Y0 + .5f*WallH, B.Z), V3(HX, .5f*WallH, HZ), HeroShade(H, Wood));
 for(u32 Corner = 0; Corner < 4; Corner += 1)
 {
  f32 SX = (Corner & 1) ? 1.f : -1.f;
  f32 SZ = (Corner & 2) ? 1.f : -1.f;
  R3PushBox(O, V3(B.X + SX*HX, Y0 + .5f*WallH, B.Z + SZ*HZ), V3(.024f, .5f*WallH, .024f), R3Color(Hero_ColorWoodDark));
 }
 for(u32 Floor = 1; Floor <= Floors; Floor += 1)
 {
  R3PushBox(O, V3(B.X, Y0 + FloorH*(f32)Floor - .01f, B.Z), V3(HX + .012f, .016f, HZ + .012f), R3Color(Hero_ColorWoodDark));
 }

 //- Windows and door
 v3 Front = V3(0.f, 0.f, 1.f);
 v3 Right = V3(1.f, 0.f, 0.f);
 v3 Back = V3(0.f, 0.f, -1.f);
 for(u32 Floor = 0; Floor < Floors; Floor += 1)
 {
  f32 WY = Y0 + FloorH*((f32)Floor + .55f);
  for(u32 Side = 0; Side < 2; Side += 1)
  {
   f32 SX = (Side == 0) ? -.5f : .5f;
   if(Floor == 0 && Side == 1)
   {
    // Door
    R3PushBox(O, V3(B.X + SX*HX, Y0 + .11f, B.Z + HZ + .006f), V3(.055f, .11f, .008f), R3Color(Hero_ColorWoodDark));
    HeroGlowRect(O, V3(B.X + SX*HX, Y0 + .2f, B.Z + HZ + .016f), V3(.03f, 0.f, 0.f), V3(0.f, .012f, 0.f), R3Color(Hero_ColorWindow));
   }
   else
   {
    HeroPushWindow(H, V3(B.X + SX*HX, WY, B.Z + HZ), Front, Right, .05f, .055f);
   }
  }
  HeroPushWindow(H, V3(B.X + HX, WY, B.Z), Right, Back, .045f, .05f);
  HeroPushWindow(H, V3(B.X - HX, WY, B.Z), V3(-1.f, 0.f, 0.f), Front, .045f, .05f);
 }

 //- Pitched roof
 f32 Over = .07f;
 f32 Rise = .5f*HZ + .07f;
 f32 RY0 = Y0 + WallH;
 f32 RY1 = RY0 + Rise;
 v3 A0 = V3(B.X - HX - Over, RY0, B.Z - HZ - Over);
 v3 A1 = V3(B.X + HX + Over, RY0, B.Z - HZ - Over);
 v3 F0 = V3(B.X - HX - Over, RY0, B.Z + HZ + Over);
 v3 F1 = V3(B.X + HX + Over, RY0, B.Z + HZ + Over);
 v3 R0 = V3(B.X - HX - Over, RY1, B.Z);
 v3 R1 = V3(B.X + HX + Over, RY1, B.Z);
 v3 Inside = V3(B.X, RY0, B.Z);
 v4 RoofColor = HeroShade(H, Roof);
 R3PushQuad(O, A0, A1, R1, R0, Inside, RoofColor);
 R3PushQuad(O, F0, F1, R1, R0, Inside, RoofColor);
 R3PushTri(O, V3(B.X - HX, RY0, B.Z - HZ), V3(B.X - HX, RY0, B.Z + HZ), V3(B.X - HX, RY1 - .02f, B.Z), R3Add(Inside, V3(1.f, 0.f, 0.f)), HeroShade(H, Wood));
 R3PushTri(O, V3(B.X + HX, RY0, B.Z - HZ), V3(B.X + HX, RY0, B.Z + HZ), V3(B.X + HX, RY1 - .02f, B.Z), R3Add(Inside, V3(-1.f, 0.f, 0.f)), HeroShade(H, Wood));
 R3PushCylinder(O, R3Add(R0, V3(0.f, .01f, 0.f)), R3Add(R1, V3(0.f, .01f, 0.f)), .018f, 4, R3Color(Hero_ColorWoodDark));

 //- Solar panels on the front slope
 v3 SlopeUp = R3Normalize(R3Sub(R0, F0));
 v3 SlopeNormal = R3Normalize(V3(0.f, HZ + Over, Rise));
 v3 SlopeMid = R3Lerp(V3(B.X, RY0, B.Z + HZ + Over), V3(B.X, RY1, B.Z), .45f);
 f32 PanelHalfL = .3f*(HZ + Over);
 if((Variant & 4) && HX > .26f)
 {
  HeroPushPanel(H, R3Add(SlopeMid, V3(-.45f*HX, 0.f, 0.f)), Right, SlopeUp, SlopeNormal, .38f*HX, PanelHalfL);
  HeroPushPanel(H, R3Add(SlopeMid, V3(.45f*HX, 0.f, 0.f)), Right, SlopeUp, SlopeNormal, .38f*HX, PanelHalfL);
 }
 else
 {
  HeroPushPanel(H, SlopeMid, Right, SlopeUp, SlopeNormal, .55f*HX, PanelHalfL);
 }

 //- Chimney on the back slope
 if(Variant & 1)
 {
  v3 C = V3(B.X + .5f*HX, RY0 + .7f*Rise, B.Z - .35f*HZ);
  R3PushBox(O, C, V3(.04f, .14f, .04f), HeroShade(H, Hero_ColorStone));
  R3PushBox(O, R3Add(C, V3(0.f, .15f, 0.f)), V3(.05f, .015f, .05f), R3Color(Hero_ColorWoodDark));
 }

 //- Balcony with stilts on the upper floor
 if((Variant & 2) && Floors >= 2)
 {
  f32 BY = Y0 + FloorH;
  f32 BZ = B.Z + HZ + .11f;
  R3PushBox(O, V3(B.X, BY, BZ), V3(.75f*HX, .015f, .11f), R3Color(Hero_ColorWoodLight));
  R3PushBox(O, V3(B.X, BY + .07f, BZ + .1f), V3(.75f*HX, .008f, .008f), R3Color(Hero_ColorWoodDark));
  for(u32 Post = 0; Post < 5; Post += 1)
  {
   f32 PX = B.X - .75f*HX + 1.5f*HX*(f32)Post/4.f;
   R3PushBox(O, V3(PX, BY + .035f, BZ + .1f), V3(.006f, .035f, .006f), R3Color(Hero_ColorWoodDark));
  }
  R3PushCylinder(O, V3(B.X - .7f*HX, B.Y - .2f, BZ + .08f), V3(B.X - .7f*HX, BY, BZ + .08f), .015f, 4, R3Color(Hero_ColorWoodDark));
  R3PushCylinder(O, V3(B.X + .7f*HX, B.Y - .2f, BZ + .08f), V3(B.X + .7f*HX, BY, BZ + .08f), .015f, 4, R3Color(Hero_ColorWoodDark));
 }

 //- Vines climbing the walls and spilling over the roof
 for(u32 Vine = 0; Vine < 5; Vine += 1)
 {
  f32 SX = (RandomUnilateral(&H->Series) < .5f) ? -1.f : 1.f;
  f32 T = RandomUnilateral(&H->Series);
  v3 P = V3(B.X + SX*(HX + .02f), Y0 + T*WallH, B.Z + HZ*(.6f*RandomBilateral(&H->Series)));
  if(Vine >= 3) P = V3(B.X + HX*RandomBilateral(&H->Series), Y0 + T*WallH, B.Z + HZ + .02f);
  R3PushEllipsoid(O, P, V3(.06f, .11f, .06f), 3, 6, HeroShade(H, Hero_ColorVine), false);
 }
 R3PushEllipsoid(O, V3(B.X - HX - Over*.5f, RY0 + .03f, B.Z + .4f*HZ), V3(.09f, .06f, .14f), 3, 6, HeroShade(H, Hero_ColorLeaf), false);
}

internal void
HeroPushGreenhouse(hero_ctx *H, f32 X, f32 Z, f32 HX, f32 HZ)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 f32 WallH = .3f;
 R3PushBox(O, R3Add(B, V3(0.f, .04f - .1f, 0.f)), V3(HX, .14f, HZ), R3Color(Hero_ColorWood));

 //- Plants inside
 for(u32 Plant = 0; Plant < 10; Plant += 1)
 {
  v3 P = R3Add(B, V3(HX*.8f*RandomBilateral(&H->Series), .12f, HZ*.75f*RandomBilateral(&H->Series)));
  R3PushEllipsoid(O, P, V3(.06f, .07f, .06f), 3, 5, HeroShade(H, Hero_ColorLeaf), false);
  if(Plant & 1) R3PushEllipsoid(O, R3Add(P, V3(.03f, .03f, .03f)), V3(.022f, .022f, .022f), 3, 4, R3Color(0xFFE0533A), false);
 }

 //- Frame
 for(u32 Corner = 0; Corner < 4; Corner += 1)
 {
  f32 SX = (Corner & 1) ? 1.f : -1.f;
  f32 SZ = (Corner & 2) ? 1.f : -1.f;
  R3PushBox(O, R3Add(B, V3(SX*HX, .08f + .5f*WallH, SZ*HZ)), V3(.012f, .5f*WallH, .012f), R3Color(Hero_ColorWoodDark));
 }
 f32 Y1 = B.Y + .08f + WallH;
 f32 Y2 = Y1 + .5f*HZ;
 R3PushCylinder(O, V3(B.X - HX, Y2, B.Z), V3(B.X + HX, Y2, B.Z), .014f, 4, R3Color(Hero_ColorWoodDark));
 R3PushCylinder(O, V3(B.X - HX, Y1, B.Z + HZ), V3(B.X + HX, Y1, B.Z + HZ), .012f, 4, R3Color(Hero_ColorWoodDark));
 R3PushCylinder(O, V3(B.X - HX, Y1, B.Z - HZ), V3(B.X + HX, Y1, B.Z - HZ), .012f, 4, R3Color(Hero_ColorWoodDark));
 for(u32 Rib = 0; Rib < 4; Rib += 1)
 {
  f32 RX = B.X - HX + 2.f*HX*(f32)Rib/3.f;
  R3PushCylinder(O, V3(RX, Y1, B.Z + HZ), V3(RX, Y2, B.Z), .008f, 4, R3Color(Hero_ColorWoodDark));
  R3PushCylinder(O, V3(RX, Y1, B.Z - HZ), V3(RX, Y2, B.Z), .008f, 4, R3Color(Hero_ColorWoodDark));
 }

 //- Glass
 v4 Glass = R3WithAlpha(R3Color(Hero_ColorGlass), .32f);
 R3PushBox(H->Translucent, R3Add(B, V3(0.f, .08f + .5f*WallH, 0.f)), V3(HX, .5f*WallH, HZ), Glass);
 v3 Inside = V3(B.X, Y1, B.Z);
 R3PushQuad(H->Translucent, V3(B.X - HX, Y1, B.Z + HZ), V3(B.X + HX, Y1, B.Z + HZ), V3(B.X + HX, Y2, B.Z), V3(B.X - HX, Y2, B.Z), Inside, Glass);
 R3PushQuad(H->Translucent, V3(B.X - HX, Y1, B.Z - HZ), V3(B.X + HX, Y1, B.Z - HZ), V3(B.X + HX, Y2, B.Z), V3(B.X - HX, Y2, B.Z), Inside, Glass);
 R3PushTriUnlit(H->Translucent, V3(B.X - HX, Y1, B.Z - HZ), V3(B.X - HX, Y1, B.Z + HZ), V3(B.X - HX, Y2, B.Z), Glass);
 R3PushTriUnlit(H->Translucent, V3(B.X + HX, Y1, B.Z - HZ), V3(B.X + HX, Y1, B.Z + HZ), V3(B.X + HX, Y2, B.Z), Glass);
}

// NOTE: A wooden lattice windmill whose rotor faces the front and turns with time.
internal void
HeroPushWindmill(hero_ctx *H, f32 X, f32 Z, f32 Height, f32 Phase)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 v4 Wood = R3Color(Hero_ColorWoodDark);
 f32 Spread = .15f;
 f32 TopSpread = .035f;

 v3 Legs[4][3];
 for(u32 Leg = 0; Leg < 4; Leg += 1)
 {
  f32 SX = (Leg == 0 || Leg == 3) ? -1.f : 1.f;
  f32 SZ = (Leg < 2) ? -1.f : 1.f;
  v3 Foot = R3Add(B, V3(SX*Spread, 0.f, SZ*Spread));
  v3 Head = R3Add(B, V3(SX*TopSpread, Height, SZ*TopSpread));
  Legs[Leg][0] = Foot;
  Legs[Leg][1] = R3Lerp(Foot, Head, .35f);
  Legs[Leg][2] = R3Lerp(Foot, Head, .68f);
  R3PushCylinder(O, Foot, Head, .016f, 4, Wood);
 }
 for(u32 Level = 1; Level < 3; Level += 1)
 {
  for(u32 Leg = 0; Leg < 4; Leg += 1)
  {
   u32 Next = (Leg + 1) % 4;
   R3PushCylinder(O, Legs[Leg][Level], Legs[Next][Level], .009f, 4, Wood);
   R3PushCylinder(O, Legs[Leg][Level - 1], Legs[Next][Level], .007f, 4, Wood);
  }
 }

 //- Nacelle, tail vane and rotor
 v3 Head = R3Add(B, V3(0.f, Height + .03f, 0.f));
 R3PushBox(O, Head, V3(.045f, .04f, .09f), R3Color(0xFF6B5340));
 R3PushOrientedBox(O, R3Add(Head, V3(0.f, .02f, -.25f)), V3(.006f, 0.f, 0.f), V3(0.f, .07f, 0.f), V3(0.f, 0.f, .14f), R3Color(Hero_ColorWoodLight));
 R3PushCylinder(O, R3Add(Head, V3(0.f, 0.f, -.08f)), R3Add(Head, V3(0.f, .02f, -.14f)), .008f, 4, Wood);

 v3 Hub = R3Add(Head, V3(0.f, 0.f, .11f));
 R3PushEllipsoid(O, Hub, V3(.03f, .03f, .03f), 3, 6, R3Color(0xFF8A7A6A), false);
 f32 Spin = 1.6f*H->Time + Phase;
 for(u32 Blade = 0; Blade < 6; Blade += 1)
 {
  f32 Angle = Spin + 2.f*Pi32*(f32)Blade/6.f;
  v3 Dir = V3(R3Cos(Angle), R3Sin(Angle), 0.f);
  v3 Side = V3(-Dir.Y, Dir.X, 0.f);
  u32 Color = (Blade & 1) ? 0xFFC79A6A : 0xFFE6D6BC;
  R3PushOrientedBox(O, R3Add(Hub, R3Scale(Dir, .2f)), R3Scale(Dir, .17f), R3Scale(Side, .04f), V3(0.f, 0.f, .006f), R3Color(Color));
  R3PushCylinder(O, Hub, R3Add(Hub, R3Scale(Dir, .36f)), .006f, 3, Wood);
 }
}

internal void
HeroPushWaterTower(hero_ctx *H, f32 X, f32 Z)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 f32 StiltH = .75f;
 for(u32 Leg = 0; Leg < 4; Leg += 1)
 {
  f32 SX = (Leg & 1) ? 1.f : -1.f;
  f32 SZ = (Leg & 2) ? 1.f : -1.f;
  R3PushCylinder(O, R3Add(B, V3(SX*.17f, 0.f, SZ*.17f)), R3Add(B, V3(SX*.14f, StiltH, SZ*.14f)), .02f, 4, R3Color(Hero_ColorWoodDark));
 }
 R3PushCylinder(O, R3Add(B, V3(-.16f, .3f, -.16f)), R3Add(B, V3(.16f, .55f, .16f)), .01f, 4, R3Color(Hero_ColorWoodDark));
 R3PushCylinder(O, R3Add(B, V3(.16f, .3f, -.16f)), R3Add(B, V3(-.16f, .55f, .16f)), .01f, 4, R3Color(Hero_ColorWoodDark));
 R3PushBox(O, R3Add(B, V3(0.f, StiltH, 0.f)), V3(.2f, .015f, .2f), R3Color(Hero_ColorWood));
 v3 TankTop = R3Add(B, V3(0.f, StiltH + .4f, 0.f));
 W3PushTerrace(O, TankTop, .19f, .39f, 14, R3Color(Hero_ColorTank), HeroShade(H, Hero_ColorTank));
 R3PushCylinder(O, R3Add(B, V3(0.f, StiltH + .12f, 0.f)), R3Add(B, V3(0.f, StiltH + .135f, 0.f)), .195f, 14, R3Color(0xFF4E7584));
 R3PushCylinder(O, R3Add(B, V3(0.f, StiltH + .28f, 0.f)), R3Add(B, V3(0.f, StiltH + .295f, 0.f)), .195f, 14, R3Color(0xFF4E7584));
 R3PushCone(O, TankTop, R3Add(TankTop, V3(0.f, .1f, 0.f)), .21f, 14, R3Color(0xFF5E7F8C));
 // Pipe down to the stream
 R3PushCylinder(O, R3Add(B, V3(-.12f, StiltH, .1f)), R3Add(B, V3(-.12f, 0.f, .1f)), .018f, 5, R3Color(0xFF7A8A90));
}

internal void
HeroPushPine(hero_ctx *H, f32 X, f32 Z, f32 Height)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 R3PushCylinder(O, B, R3Add(B, V3(0.f, .3f*Height, 0.f)), .045f, 6, R3Color(0xFF5A3D26));
 for(u32 Tier = 0; Tier < 5; Tier += 1)
 {
  f32 Base = Height*(.18f + .15f*(f32)Tier);
  f32 Radius = .3f*Height*(1.f - .17f*(f32)Tier)/1.4f;
  R3PushCone(O, R3Add(B, V3(0.f, Base, 0.f)), R3Add(B, V3(0.f, Base + .34f*Height, 0.f)), Radius, 9, HeroShade(H, (Tier & 1) ? Hero_ColorPine : 0xFF3A7A45));
 }
}

internal void
HeroPushBed(hero_ctx *H, f32 X, f32 Z, f32 HX, f32 HZ)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 R3PushBox(O, R3Add(B, V3(0.f, .05f, 0.f)), V3(HX, .05f, HZ), HeroShade(H, Hero_ColorWood));
 R3PushBox(O, R3Add(B, V3(0.f, .056f, 0.f)), V3(HX - .02f, .05f, HZ - .02f), R3Color(Hero_ColorSoil));
 u32 Count = (u32)(2.f*HX/.075f);
 for(u32 Row = 0; Row < 2; Row += 1)
 {
  for(u32 Plant = 0; Plant < Count; Plant += 1)
  {
   f32 PX = B.X - HX + .05f + (2.f*HX - .1f)*((f32)Plant + .5f*(f32)Row)/(f32)Count;
   v3 P = V3(PX, B.Y + .13f, B.Z + ((Row == 0) ? -.45f : .45f)*HZ);
   R3PushEllipsoid(O, P, V3(.04f, .045f, .04f), 3, 5, HeroShade(H, Hero_ColorLeaf), false);
   f32 Roll = RandomUnilateral(&H->Series);
   if(Roll < .2f)      R3PushEllipsoid(O, R3Add(P, V3(0.f, -.01f, .03f)), V3(.03f, .028f, .03f), 3, 5, R3Color(0xFFF08A2C), false);
   else if(Roll < .35f) R3PushEllipsoid(O, R3Add(P, V3(.02f, .01f, .02f)), V3(.016f, .016f, .016f), 3, 4, R3Color(0xFFE0453A), false);
  }
 }
}

// NOTE: A market stall: posts, a counter with piles of goods (in the island's product
// colours) and a cloth awning, or a tin roof with a solar panel.
internal void
HeroPushStall(hero_ctx *H, f32 X, f32 Z, u32 *Goods, u32 GoodsCount, b32 TinRoof)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 v4 Wood = R3Color(Hero_ColorWoodDark);
 R3PushCylinder(O, R3Add(B, V3(-.27f, 0.f, .22f)), R3Add(B, V3(-.27f, .44f, .22f)), .014f, 4, Wood);
 R3PushCylinder(O, R3Add(B, V3(.27f, 0.f, .22f)), R3Add(B, V3(.27f, .44f, .22f)), .014f, 4, Wood);
 R3PushCylinder(O, R3Add(B, V3(-.27f, 0.f, -.16f)), R3Add(B, V3(-.27f, .52f, -.16f)), .014f, 4, Wood);
 R3PushCylinder(O, R3Add(B, V3(.27f, 0.f, -.16f)), R3Add(B, V3(.27f, .52f, -.16f)), .014f, 4, Wood);
 R3PushBox(O, R3Add(B, V3(0.f, .1f, .1f)), V3(.26f, .1f, .08f), HeroShade(H, Hero_ColorWood));
 R3PushBox(O, R3Add(B, V3(0.f, .2f, -.1f)), V3(.24f, .015f, .05f), HeroShade(H, Hero_ColorWoodLight));

 //- Goods
 for(u32 Pile = 0; Pile < 6; Pile += 1)
 {
  u32 Color = (GoodsCount > 0) ? Goods[Pile % GoodsCount] : HeroShirtColors[Pile % 3];
  v3 P = R3Add(B, V3(-.2f + .08f*(f32)Pile, .22f, .1f));
  R3PushEllipsoid(O, P, V3(.03f, .025f, .03f), 3, 5, R3Color(Color), false);
  R3PushEllipsoid(O, R3Add(P, V3(.02f, 0.f, .025f)), V3(.026f, .022f, .026f), 3, 5, R3Color(Color), false);
  R3PushEllipsoid(O, R3Add(P, V3(.01f, .03f, .012f)), V3(.024f, .02f, .024f), 3, 5, R3Color(Color), false);
  if(Pile < 3)
  {
   // Baskets on the back shelf
   R3PushBox(O, R3Add(B, V3(-.15f + .15f*(f32)Pile, .24f, -.1f)), V3(.04f, .025f, .035f), R3Color(Hero_ColorWoodLight));
   R3PushEllipsoid(O, R3Add(B, V3(-.15f + .15f*(f32)Pile, .27f, -.1f)), V3(.03f, .015f, .025f), 3, 5, R3Color(Color), false);
  }
 }
 // Crates beside the stall
 R3PushBox(O, R3Add(B, V3(.36f, .05f, .12f)), V3(.05f, .05f, .05f), HeroShade(H, Hero_ColorWoodLight));
 R3PushBox(O, R3Add(B, V3(.36f, .15f, .12f)), V3(.045f, .045f, .045f), HeroShade(H, Hero_ColorWoodLight));

 //- Roof
 v3 Back = R3Add(B, V3(0.f, .54f, -.22f));
 v3 Front = R3Add(B, V3(0.f, .44f, .3f));
 v3 Inside = R3Add(B, V3(0.f, 0.f, 0.f));
 if(TinRoof)
 {
  R3PushQuad(O, R3Add(Back, V3(-.33f, 0.f, 0.f)), R3Add(Back, V3(.33f, 0.f, 0.f)), R3Add(Front, V3(.33f, 0.f, 0.f)), R3Add(Front, V3(-.33f, 0.f, 0.f)), Inside, HeroShade(H, Hero_ColorTin));
  v3 Up = R3Normalize(R3Sub(Back, Front));
  v3 Normal = R3Normalize(R3Cross(V3(1.f, 0.f, 0.f), Up));
  if(Normal.Y < 0.f) Normal = R3Scale(Normal, -1.f);
  HeroPushPanel(H, R3Lerp(Front, Back, .5f), V3(1.f, 0.f, 0.f), Up, Normal, .2f, .16f);
 }
 else
 {
  for(u32 Strip = 0; Strip < 6; Strip += 1)
  {
   f32 SX0 = -.33f + .11f*(f32)Strip;
   f32 SX1 = SX0 + .11f;
   v4 Color = R3Color((Strip & 1) ? Hero_ColorCloth2 : Hero_ColorCloth);
   R3PushQuad(O, R3Add(Back, V3(SX0, 0.f, 0.f)), R3Add(Back, V3(SX1, 0.f, 0.f)), R3Add(Front, V3(SX1, 0.f, 0.f)), R3Add(Front, V3(SX0, 0.f, 0.f)), Inside, Color);
   // Valance hanging at the front
   R3PushQuad(O, R3Add(Front, V3(SX0, 0.f, 0.f)), R3Add(Front, V3(SX1, 0.f, 0.f)), R3Add(Front, V3(SX1, -.05f, .005f)), R3Add(Front, V3(SX0, -.05f, .005f)), Inside, Color);
  }
 }
}

//~ Harbour
internal void
HeroPushDock(hero_ctx *H, f32 XFrom, f32 XTo, f32 Z, f32 HalfWidth)
{
 r3_mesh *O = H->Opaque;
 f32 Y = Hero_WaterLevel + .09f;
 f32 X0 = Min(XFrom, XTo);
 f32 X1 = Max(XFrom, XTo);
 u32 Planks = (u32)((X1 - X0)/.07f);
 for(u32 Plank = 0; Plank < Planks; Plank += 1)
 {
  f32 PX = X0 + (X1 - X0)*((f32)Plank + .5f)/(f32)Planks;
  R3PushBox(O, HeroP(H, PX, Y, Z), V3(.03f, .014f, HalfWidth), HeroShade(H, Hero_ColorWoodLight));
 }
 R3PushBox(O, HeroP(H, .5f*(X0 + X1), Y - .025f, Z), V3(.5f*(X1 - X0), .012f, HalfWidth - .02f), R3Color(Hero_ColorWoodDark));
 for(u32 Post = 0; Post < 3; Post += 1)
 {
  f32 PX = X0 + (X1 - X0)*(f32)Post/2.f;
  for(u32 Side = 0; Side < 2; Side += 1)
  {
   f32 PZ = Z + ((Side == 0) ? -HalfWidth : HalfWidth);
   R3PushCylinder(O, HeroP(H, PX, Hero_PoolFloor, PZ), HeroP(H, PX, Y + .1f, PZ), .022f, 5, R3Color(Hero_ColorWoodDark));
  }
 }
}

internal void
HeroPushBoat(hero_ctx *H, f32 X, f32 Z, f32 Facing, f32 Phase, b32 Rower)
{
 r3_mesh *O = H->Opaque;
 f32 Bob = .012f*R3Sin(1.3f*H->Time + Phase);
 v3 C = HeroP(H, X, Hero_WaterLevel + .02f + Bob, Z);
 R3PushEllipsoid(O, C, V3(.2f, .06f, .085f), 4, 9, R3Color(0xFF8A5A36), false);
 R3PushBox(O, R3Add(C, V3(0.f, .045f, 0.f)), V3(.15f, .01f, .06f), R3Color(0xFFC79A6A));
 R3PushBox(O, R3Add(C, V3(.03f*Facing, .065f, 0.f)), V3(.02f, .01f, .065f), R3Color(Hero_ColorWoodDark));
 if(Rower)
 {
  v3 Seat = R3Add(C, V3(.03f*Facing, .07f, 0.f));
  R3PushBox(O, R3Add(Seat, V3(0.f, .04f, 0.f)), V3(.025f, .035f, .02f), R3Color(HeroShirtColors[(u32)(Phase*10.f) % ArrayCount(HeroShirtColors)]));
  R3PushEllipsoid(O, R3Add(Seat, V3(0.f, .095f, 0.f)), V3(.022f, .022f, .022f), 3, 5, R3Color(Hero_ColorSkin), false);
  f32 Stroke = .3f*R3Sin(2.f*H->Time + Phase);
  R3PushCylinder(O, R3Add(Seat, V3(-.05f*Facing, .04f, -.11f)), R3Add(Seat, V3((.08f + Stroke*.2f)*Facing, -.07f, -.17f)), .006f, 3, R3Color(Hero_ColorWoodLight));
  R3PushCylinder(O, R3Add(Seat, V3(-.05f*Facing, .04f, .11f)), R3Add(Seat, V3((.08f + Stroke*.2f)*Facing, -.07f, .17f)), .006f, 3, R3Color(Hero_ColorWoodLight));
 }
 // Ripples
 R3PushEllipsoid(H->Translucent, R3Add(C, V3(0.f, -.015f, 0.f)), V3(.26f, .004f, .12f), 2, 10, R3WithAlpha(R3Color(0xFFFFFFFF), .35f), true);
}

// NOTE: A water wheel with its axle along X, turning with time.
internal void
HeroPushWaterWheel(hero_ctx *H, v3 C, f32 Radius)
{
 r3_mesh *O = H->Opaque;
 v4 Wood = R3Color(Hero_ColorWoodDark);
 f32 Spin = -1.2f*H->Time;
 u32 Segments = 14;
 for(u32 Side = 0; Side < 2; Side += 1)
 {
  f32 SX = (Side == 0) ? -.055f : .055f;
  for(u32 Seg = 0; Seg < Segments; Seg += 1)
  {
   f32 A0 = Spin + 2.f*Pi32*(f32)Seg/(f32)Segments;
   f32 A1 = Spin + 2.f*Pi32*(f32)(Seg + 1)/(f32)Segments;
   R3PushCylinder(O, R3Add(C, V3(SX, Radius*R3Cos(A0), Radius*R3Sin(A0))), R3Add(C, V3(SX, Radius*R3Cos(A1), Radius*R3Sin(A1))), .014f, 4, Wood);
  }
  for(u32 Spoke = 0; Spoke < 6; Spoke += 1)
  {
   f32 A = Spin + Pi32*(f32)Spoke/3.f;
   R3PushCylinder(O, R3Add(C, V3(SX, 0.f, 0.f)), R3Add(C, V3(SX, Radius*R3Cos(A), Radius*R3Sin(A))), .01f, 4, Wood);
  }
 }
 for(u32 Paddle = 0; Paddle < 12; Paddle += 1)
 {
  f32 A = Spin + 2.f*Pi32*(f32)Paddle/12.f;
  v3 Radial = V3(0.f, R3Cos(A), R3Sin(A));
  v3 Tangent = V3(0.f, -R3Sin(A), R3Cos(A));
  R3PushOrientedBox(O, R3Add(C, R3Scale(Radial, .9f*Radius)), V3(.065f, 0.f, 0.f), R3Scale(Radial, .08f), R3Scale(Tangent, .01f), R3Color(Hero_ColorWoodLight));
 }
 R3PushCylinder(O, R3Add(C, V3(-.13f, 0.f, 0.f)), R3Add(C, V3(.13f, 0.f, 0.f)), .025f, 6, R3Color(0xFF4A4038));
}

//~ Small things
internal void
HeroPushPerson(hero_ctx *H, v3 B, u32 Shirt)
{
 r3_mesh *O = H->Opaque;
 R3PushBox(O, R3Add(B, V3(0.f, .045f, 0.f)), V3(.02f, .045f, .015f), R3Color(Hero_ColorPants));
 R3PushBox(O, R3Add(B, V3(0.f, .125f, 0.f)), V3(.028f, .038f, .019f), R3Color(Shirt));
 R3PushEllipsoid(O, R3Add(B, V3(0.f, .188f, 0.f)), V3(.023f, .025f, .023f), 3, 6, R3Color(Hero_ColorSkin), false);
 R3PushEllipsoid(O, R3Add(B, V3(0.f, .205f, 0.f)), V3(.024f, .012f, .024f), 3, 6, R3Color(0xFF4A3426), false);
}

internal void
HeroPushAnimal(hero_ctx *H, f32 X, f32 Z, f32 Scale, u32 Body, u32 Spot, f32 Facing)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 v3 C = R3Add(B, V3(0.f, .17f*Scale, 0.f));
 R3PushBox(O, C, V3(.13f*Scale, .065f*Scale, .065f*Scale), R3Color(Body));
 R3PushBox(O, R3Add(C, V3(-.03f*Scale*Facing, .04f*Scale, 0.f)), V3(.05f*Scale, .035f*Scale, .068f*Scale), R3Color(Spot));
 R3PushBox(O, R3Add(C, V3(.16f*Scale*Facing, .04f*Scale, 0.f)), V3(.045f*Scale, .045f*Scale, .045f*Scale), R3Color(Body));
 R3PushBox(O, R3Add(C, V3(.2f*Scale*Facing, .02f*Scale, 0.f)), V3(.015f*Scale, .025f*Scale, .035f*Scale), R3Color(0xFFE8B4A6));
 for(u32 Leg = 0; Leg < 4; Leg += 1)
 {
  f32 DX = (Leg & 1) ? .09f : -.09f;
  f32 DZ = (Leg & 2) ? .045f : -.045f;
  R3PushBox(O, R3Add(B, V3(DX*Scale, .05f*Scale, DZ*Scale)), V3(.017f*Scale, .05f*Scale, .017f*Scale), R3Color(Spot));
 }
}

internal void
HeroPushBarrel(hero_ctx *H, f32 X, f32 Z)
{
 v3 B = HeroOn(H, X, Z);
 W3PushTerrace(H->Opaque, R3Add(B, V3(0.f, .13f, 0.f)), .055f, .13f, 8, R3Color(0xFF6E4A2C), HeroShade(H, Hero_ColorWood));
 R3PushCylinder(H->Opaque, R3Add(B, V3(0.f, .03f, 0.f)), R3Add(B, V3(0.f, .042f, 0.f)), .058f, 8, R3Color(0xFF4A4038));
 R3PushCylinder(H->Opaque, R3Add(B, V3(0.f, .09f, 0.f)), R3Add(B, V3(0.f, .102f, 0.f)), .058f, 8, R3Color(0xFF4A4038));
}

internal void
HeroPushCampfire(hero_ctx *H, f32 X, f32 Z)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 for(u32 Stone = 0; Stone < 8; Stone += 1)
 {
  f32 A = 2.f*Pi32*(f32)Stone/8.f;
  R3PushEllipsoid(O, R3Add(B, V3(.09f*R3Cos(A), .015f, .09f*R3Sin(A))), V3(.025f, .018f, .025f), 3, 5, HeroShade(H, Hero_ColorStone), false);
 }
 for(u32 Log = 0; Log < 3; Log += 1)
 {
  f32 A = Pi32*(f32)Log/3.f;
  v3 Dir = V3(R3Cos(A), 0.f, R3Sin(A));
  R3PushOrientedBox(O, R3Add(B, V3(0.f, .02f, 0.f)), R3Scale(Dir, .07f), V3(0.f, .012f, 0.f), R3Scale(V3(-Dir.Z, 0.f, Dir.X), .012f), R3Color(0xFF5A3D26));
 }
 f32 Flicker = 1.f + .15f*R3Sin(9.f*H->Time) + .08f*R3Sin(23.f*H->Time);
 R3PushEllipsoid(O, R3Add(B, V3(0.f, .07f*Flicker, 0.f)), V3(.045f, .07f*Flicker, .045f), 4, 6, R3Color(Hero_ColorFire), true);
 R3PushEllipsoid(O, R3Add(B, V3(0.f, .055f*Flicker, 0.f)), V3(.025f, .045f*Flicker, .025f), 3, 6, R3Color(Hero_ColorFireCore), true);
 R3PushEllipsoid(H->Translucent, R3Add(B, V3(0.f, .08f, 0.f)), V3(.22f, .16f, .22f), 4, 8, R3WithAlpha(R3Color(Hero_ColorFire), .18f), true);
}

internal void
HeroPushLaundry(hero_ctx *H, f32 XA, f32 XB, f32 Z)
{
 r3_mesh *O = H->Opaque;
 v3 A = HeroOn(H, XA, Z);
 v3 B = HeroOn(H, XB, Z);
 v3 TopA = R3Add(A, V3(0.f, .3f, 0.f));
 v3 TopB = R3Add(B, V3(0.f, .3f, 0.f));
 R3PushCylinder(O, A, TopA, .01f, 4, R3Color(Hero_ColorWoodDark));
 R3PushCylinder(O, B, TopB, .01f, 4, R3Color(Hero_ColorWoodDark));
 R3PushCylinder(O, TopA, TopB, .003f, 3, R3Color(0xFFE8E0D0));
 for(u32 Cloth = 0; Cloth < 5; Cloth += 1)
 {
  v3 P = R3Lerp(TopA, TopB, .15f + .17f*(f32)Cloth);
  R3PushBox(O, R3Add(P, V3(0.f, -.045f, 0.f)), V3(.03f, .045f, .004f), R3Color(HeroShirtColors[(Cloth + 1) % ArrayCount(HeroShirtColors)]));
 }
}

internal void
HeroPushRopeBridge(hero_ctx *H, v3 A, v3 B)
{
 r3_mesh *O = H->Opaque;
 u32 Planks = 12;
 v3 Dir = R3Normalize(R3Sub(B, A));
 v3 Side = R3Normalize(R3Cross(Dir, V3(0.f, 1.f, 0.f)));
 f32 Length = R3Length(R3Sub(B, A));
 v3 Prev[2];
 Prev[0] = A;
 Prev[1] = B;
 for(u32 Plank = 0; Plank <= Planks; Plank += 1)
 {
  f32 T = (f32)Plank/(f32)Planks;
  v3 P = R3Lerp(A, B, T);
  P.Y -= .5f*Length*.12f*4.f*T*(1.f - T)*.5f;
  if(Plank < Planks)
  {
   R3PushOrientedBox(O, P, R3Scale(Side, .08f), V3(0.f, .007f, 0.f), R3Scale(Dir, .022f), HeroShade(H, Hero_ColorWoodLight));
  }
  for(u32 Rope = 0; Rope < 2; Rope += 1)
  {
   v3 R = R3Add(R3Add(P, R3Scale(Side, (Rope == 0) ? -.085f : .085f)), V3(0.f, .09f, 0.f));
   if(Plank > 0) R3PushCylinder(O, Prev[Rope], R, .005f, 3, R3Color(0xFFCDB58A));
   if(Plank % 3 == 0) R3PushCylinder(O, R, R3Sub(R, V3(0.f, .09f, 0.f)), .004f, 3, R3Color(0xFFCDB58A));
   Prev[Rope] = R;
  }
 }
 for(u32 End = 0; End < 2; End += 1)
 {
  v3 P = (End == 0) ? A : B;
  R3PushCylinder(O, R3Add(P, R3Scale(Side, -.09f)), R3Add(R3Add(P, R3Scale(Side, -.09f)), V3(0.f, .14f, 0.f)), .012f, 4, R3Color(Hero_ColorWoodDark));
  R3PushCylinder(O, R3Add(P, R3Scale(Side, .09f)), R3Add(R3Add(P, R3Scale(Side, .09f)), V3(0.f, .14f, 0.f)), .012f, 4, R3Color(Hero_ColorWoodDark));
 }
}

// NOTE: Wooden stairs from A (top) down to B (bottom).
internal void
HeroPushStairs(hero_ctx *H, v3 A, v3 B, f32 HalfWidth)
{
 r3_mesh *O = H->Opaque;
 v3 Flat = V3(B.X - A.X, 0.f, B.Z - A.Z);
 f32 Run = R3Length(Flat);
 v3 Dir = R3Normalize(Flat);
 v3 Side = V3(-Dir.Z, 0.f, Dir.X);
 u32 Steps = Max(2, (u32)((A.Y - B.Y)/.065f));
 for(u32 Step = 0; Step < Steps; Step += 1)
 {
  f32 T = ((f32)Step + .5f)/(f32)Steps;
  v3 P = R3Lerp(A, B, T);
  R3PushOrientedBox(O, P, R3Scale(Side, HalfWidth), V3(0.f, .014f, 0.f), R3Scale(Dir, .5f*Run/(f32)Steps), HeroShade(H, Hero_ColorWoodLight));
 }
 for(u32 Rail = 0; Rail < 2; Rail += 1)
 {
  v3 Offset = R3Scale(Side, (Rail == 0) ? -HalfWidth : HalfWidth);
  R3PushCylinder(O, R3Add(A, Offset), R3Add(B, Offset), .012f, 4, R3Color(Hero_ColorWoodDark));
  R3PushCylinder(O, R3Add(R3Add(A, Offset), V3(0.f, .14f, 0.f)), R3Add(R3Add(B, Offset), V3(0.f, .14f, 0.f)), .007f, 4, R3Color(Hero_ColorWoodDark));
 }
}

internal void
HeroPushBoulder(hero_ctx *H, v3 P, f32 Size)
{
 R3PushEllipsoid(H->Opaque, P, V3(Size*1.2f, Size*.85f, Size), 4, 7, HeroShade(H, HeroPick(H, HeroStoneColors, ArrayCount(HeroStoneColors))), false);
}

internal b32
HeroSpotTaken(f32 X, f32 Z)
{
 b32 Result = false;
 for(u32 Idx = 0; Idx < ArrayCount(HeroTakenRects) && !Result; Idx += 1)
 {
  Result = (X > HeroTakenRects[Idx][0] && X < HeroTakenRects[Idx][2] && Z > HeroTakenRects[Idx][1] && Z < HeroTakenRects[Idx][3]);
 }
 return Result;
}

// NOTE: A square glowing ring around the slab at the island's top height.
internal void
HeroPushSquareRing(hero_ctx *H, f32 Inner, f32 Outer, v4 Color)
{
 f32 CX[4] = {1.f, -1.f, -1.f, 1.f};
 f32 CZ[4] = {1.f, 1.f, -1.f, -1.f};
 f32 Y = .05f;
 for(u32 Side = 0; Side < 4; Side += 1)
 {
  u32 Next = (Side + 1) % 4;
  v3 P0 = HeroP(H, CX[Side]*Outer, Y, CZ[Side]*Outer);
  v3 P1 = HeroP(H, CX[Next]*Outer, Y, CZ[Next]*Outer);
  v3 P2 = HeroP(H, CX[Next]*Inner, Y, CZ[Next]*Inner);
  v3 P3 = HeroP(H, CX[Side]*Inner, Y, CZ[Side]*Inner);
  R3PushTriUnlit(H->Opaque, P0, P1, P2, Color);
  R3PushTriUnlit(H->Opaque, P0, P2, P3, Color);
 }
}

//~ Products and needs
// NOTE: The same meaning as the crates on the other islands, sized for this island:
// stacked solid crates (produced) beside the market, crate frames (needed) on the
// right harbour terrace.
internal void
HeroPushCrates(hero_ctx *H, island *Island)
{
 r3_mesh *O = H->Opaque;
 f32 Half = .1f;

 u32 Slot = 0;
 for(product *Product = Island->FirstProduct; Product && Slot < 3; Product = Product->Next)
 {
  v3 B = HeroOn(H, -.78f, 1.2f + .26f*(f32)Slot);
  u32 Stack = Clamp(1, (u32)(Product->Amount/W3_CrateUnitsPerBox) + 1, W3_MaxCrateStack);
  for(u32 Level = 0; Level < Stack; Level += 1)
  {
   R3PushBox(O, R3Add(B, V3(0.f, Half + 2.02f*Half*(f32)Level, 0.f)), V3(Half, Half, Half), R3Color(W3ProductColors[Product->Kind]));
  }
  Slot += 1;
 }

 Slot = 0;
 for(product *Req = Island->FirstRequirement; Req && Slot < 3; Req = Req->Next)
 {
  v3 Center = R3Add(HeroOn(H, 1.62f + .26f*(f32)Slot, 2.02f), V3(0.f, Half, 0.f));
  v4 Color = R3Color(W3ProductColors[Req->Kind]);
  R3PushBox(H->Translucent, Center, V3(Half, Half, Half), R3WithAlpha(Color, .38f));
  f32 Edge = .014f;
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
    R3PushBox(O, R3Add(Center, Offset), HalfSize, Color);
   }
  }
  Slot += 1;
 }
}

//~ The whole island
internal void
W3PushHeroIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction)
{
 r3_state *R3 = &App->R3;
 hero_ctx Ctx = {0};
 hero_ctx *H = &Ctx;
 H->Opaque = &R3->Opaque;
 H->Translucent = &R3->Translucent;
 H->Top = Top;
 H->Time = R3->AnimTime;
 RandomSeed(&H->Series, W3_Seed*17 + 389*(u64)Index);

 HeroPushTerrain(H);
 HeroPushCliff(H);

 //- Goods for the stalls, in the island's product colours
 u32 Goods[3];
 u32 GoodsCount = 0;
 for(product *Product = Island->FirstProduct; Product && GoodsCount < ArrayCount(Goods); Product = Product->Next)
 {
  Goods[GoodsCount++] = W3ProductColors[Product->Kind];
 }

 //- Houses up the hill (X, Z, half sizes, floors, variant)
 HeroPushHouse(H, -.15f, -1.55f, .3f, .24f, 2, 1|4);
 HeroPushHouse(H, -1.2f, -1.4f, .3f, .24f, 2, 2);
 HeroPushHouse(H, -1.75f, -.45f, .36f, .27f, 2, 3|4);
 HeroPushHouse(H, -1.3f, .35f, .26f, .21f, 1, 0);
 HeroPushHouse(H, -.5f, -.6f, .3f, .24f, 2, 2|4);
 HeroPushHouse(H, .15f, -.95f, .22f, .2f, 1, 1);
 HeroPushHouse(H, 1.6f, -.5f, .34f, .27f, 2, 5|2);
 HeroPushHouse(H, 1.6f, -1.7f, .28f, .23f, 2, 1);

 //- Greenhouses, gardens, water tower
 HeroPushGreenhouse(H, -1.95f, .55f, .26f, .21f);
 HeroPushGreenhouse(H, 2.f, .45f, .22f, .25f);
 HeroPushBed(H, 1.38f, .08f, .27f, .085f);
 HeroPushBed(H, 1.38f, .34f, .27f, .085f);
 HeroPushBed(H, 1.38f, .6f, .27f, .085f);
 HeroPushWaterTower(H, 1.05f, -1.15f);

 //- Windmills along the back, pines on the sides
 HeroPushWindmill(H, -2.f, -1.95f, 1.45f, 0.f);
 HeroPushWindmill(H, -1.45f, -2.08f, 1.65f, 1.3f);
 HeroPushWindmill(H, -.75f, -2.1f, 1.3f, 2.1f);
 HeroPushWindmill(H, 2.05f, -2.1f, 1.55f, .7f);
 HeroPushWindmill(H, 2.12f, -1.2f, 1.35f, 2.8f);
 HeroPushPine(H, -2.12f, .05f, 1.25f);
 HeroPushPine(H, 2.12f, -.05f, 1.15f);
 HeroPushPine(H, -.95f, -1.95f, .8f);

 //- Rocky crest at the top of the hill, with the knowledge crystal on a mast
 {
  v3 Crest = HeroOn(H, .3f, -1.9f);
  HeroPushBoulder(H, R3Add(Crest, V3(0.f, .2f, 0.f)), .32f);
  HeroPushBoulder(H, R3Add(Crest, V3(-.28f, .12f, .1f)), .2f);
  HeroPushBoulder(H, R3Add(Crest, V3(.22f, .1f, -.15f)), .22f);
  HeroPushBoulder(H, R3Add(Crest, V3(.05f, .4f, -.05f)), .18f);
  v3 MastBase = R3Add(Crest, V3(.05f, .5f, -.05f));
  v3 Crystal = R3Add(MastBase, V3(0.f, .65f, 0.f));
  R3PushCylinder(H->Opaque, MastBase, Crystal, .025f, 5, R3Color(W3_ColorPost));
  v4 Color = R3LerpColor(R3Color(W3_ColorCrystalLow), R3Color(W3_ColorCrystalHigh), KnownFraction);
  f32 Size = .2f;
  R3PushCone(H->Opaque, Crystal, R3Add(Crystal, V3(0.f, 1.3f*Size, 0.f)), Size, 4, Color);
  R3PushCone(H->Opaque, Crystal, R3Add(Crystal, V3(0.f, -1.3f*Size, 0.f)), Size, 4, Color);
 }
 HeroPushBoulder(H, R3Add(HeroOn(H, -.45f, .82f), V3(0.f, .05f, 0.f)), .12f);
 HeroPushBoulder(H, R3Add(HeroOn(H, 1.48f, .84f), V3(0.f, .05f, 0.f)), .1f);
 HeroPushBoulder(H, R3Add(HeroOn(H, .62f, -.62f), V3(0.f, .04f, 0.f)), .11f);

 //- Rope bridge over the stream
 {
  v3 A = R3Add(HeroOn(H, .32f, -.68f), V3(0.f, .05f, 0.f));
  v3 B = R3Add(HeroOn(H, 1.15f, -.68f), V3(0.f, .05f, 0.f));
  HeroPushRopeBridge(H, A, B);
 }

 //- Harbour: waterfall mist, water wheel, docks and boats
 R3PushEllipsoid(H->Translucent, HeroP(H, .85f, Hero_WaterLevel + .02f, Hero_PoolZ0 + .12f), V3(.3f, .08f, .18f), 4, 8, R3WithAlpha(R3Color(0xFFFFFFFF), .5f), true);
 R3PushEllipsoid(H->Translucent, HeroP(H, .85f, Hero_WaterLevel + .15f, Hero_PoolZ0 + .1f), V3(.22f, .16f, .14f), 4, 8, R3WithAlpha(R3Color(0xFFFFFFFF), .22f), true);
 HeroPushWaterWheel(H, HeroP(H, 1.16f, Hero_WaterLevel + .26f, Hero_PoolZ0 + .05f), .3f);
 R3PushBox(H->Opaque, HeroP(H, 1.16f, -.2f, Hero_PoolZ0 - .02f), V3(.08f, .3f, .04f), HeroShade(H, Hero_ColorStone));
 HeroPushDock(H, Hero_PoolX0, .2f, 1.88f, .12f);
 HeroPushDock(H, Hero_PoolX1, .75f, 1.45f, .11f);
 HeroPushBoat(H, -.05f, 2.14f, 1.f, 0.f, true);
 HeroPushBoat(H, .45f, 1.72f, -1.f, 1.7f, false);
 HeroPushBoat(H, 1.f, 2.06f, 1.f, 3.1f, true);

 //- Market on the lower terraces
 HeroPushStall(H, -1.75f, 1.5f, Goods, GoodsCount, false);
 HeroPushStall(H, -1.1f, 1.72f, Goods, GoodsCount, false);
 HeroPushStall(H, 1.88f, 1.32f, Goods, GoodsCount, true);
 HeroPushStairs(H, R3Add(HeroOn(H, -.85f, .72f), V3(0.f, .01f, 0.f)), R3Add(HeroOn(H, -.85f, 1.1f), V3(0.f, .01f, 0.f)), .1f);
 HeroPushStairs(H, R3Add(HeroOn(H, 1.62f, .7f), V3(0.f, .01f, 0.f)), R3Add(HeroOn(H, 1.62f, 1.12f), V3(0.f, .01f, 0.f)), .09f);
 HeroPushStairs(H, R3Add(HeroOn(H, -.95f, -.98f), V3(0.f, .01f, 0.f)), R3Add(HeroOn(H, -.95f, -.62f), V3(0.f, .01f, 0.f)), .07f);
 HeroPushCrates(H, Island);

 //- Village life
 HeroPushCampfire(H, -.8f, .55f);
 HeroPushLaundry(H, -1.f, -.5f, -.05f);
 HeroPushAnimal(H, -.05f, .45f, .8f, 0xFFF4EFE6, 0xFF6B4A35, 1.f);
 HeroPushAnimal(H, .3f, .58f, .75f, 0xFFB98A5E, 0xFF5A3D26, -1.f);
 HeroPushAnimal(H, -.2f, -.15f, .55f, 0xFFEDE6DA, 0xFF8A7A6A, 1.f);
 HeroPushBarrel(H, 1.25f, -.12f);
 HeroPushBarrel(H, 1.36f, -.08f);
 HeroPushBarrel(H, -.2f, -.3f);
 HeroPushBarrel(H, 2.05f, 1.75f);
 HeroPushBarrel(H, -1.45f, 1.95f);
 {
  f32 People[][2] =
  {
   {-.92f, .64f}, {-.66f, .44f}, {-1.45f, 1.3f}, {-.9f, 1.45f}, {1.75f, 1.12f},
   {1.25f, .46f}, {-.65f, -.2f}, {1.95f, 1.85f}, {-1.75f, 1.9f}, {.05f, .05f},
  };
  for(u32 Person = 0; Person < ArrayCount(People); Person += 1)
  {
   HeroPushPerson(H, HeroOn(H, People[Person][0], People[Person][1]), HeroShirtColors[Person % ArrayCount(HeroShirtColors)]);
  }
  HeroPushPerson(H, HeroP(H, .05f, Hero_WaterLevel + .105f, 1.85f), HeroShirtColors[2]);
  HeroPushPerson(H, HeroP(H, 1.f, Hero_WaterLevel + .105f, 1.45f), HeroShirtColors[4]);
 }

 //- Bushes and ferns on free grass
 for(u32 Bush = 0; Bush < 70; Bush += 1)
 {
  f32 X = (Hero_Half - .1f)*RandomBilateral(&H->Series);
  f32 Z = (Hero_Half - .1f)*RandomBilateral(&H->Series);
  u32 Type;
  f32 Y = HeroGroundY(X, Z, &Type);
  if(Type == HeroGround_Grass && !HeroSpotTaken(X, Z))
  {
   f32 Size = .05f + .06f*RandomUnilateral(&H->Series);
   R3PushEllipsoid(H->Opaque, HeroP(H, X, Y + .6f*Size, Z), V3(Size*1.2f, Size, Size*1.2f), 3, 6, HeroShade(H, (Bush & 1) ? Hero_ColorLeaf : Hero_ColorVine), false);
   if((Bush % 5) == 0)
   {
    R3PushEllipsoid(H->Opaque, HeroP(H, X + .03f, Y + 1.3f*Size, Z + .02f), V3(.018f, .018f, .018f), 3, 4, R3Color((Bush & 2) ? 0xFFF2D04E : 0xFFE87FA6), false);
   }
  }
 }

 //- Clouds wrapping the base
 for(u32 Puff = 0; Puff < 9; Puff += 1)
 {
  f32 A = 2.f*Pi32*((f32)Puff + .5f*RandomUnilateral(&H->Series))/9.f;
  f32 Out = Hero_Half*(.85f + .35f*RandomUnilateral(&H->Series));
  v3 P = HeroP(H, Out*R3Cos(A), -2.3f - 1.f*RandomUnilateral(&H->Series), Out*R3Sin(A));
  v3 Radii = V3(.8f + .6f*RandomUnilateral(&H->Series), .35f + .2f*RandomUnilateral(&H->Series), .7f + .5f*RandomUnilateral(&H->Series));
  R3PushEllipsoid(H->Translucent, P, Radii, 5, 9, R3WithAlpha(R3Color(Hero_ColorCloud), .5f), true);
 }

 //- Status ring (and hovered/selected ring) around the slab
 HeroPushSquareRing(H, Hero_Half + .12f, Hero_Half + .3f, R3Color(StatusColor));
 if(Highlight)
 {
  HeroPushSquareRing(H, Hero_Half + .36f, Hero_Half + .48f, R3Color(HighlightColor));
 }
}
