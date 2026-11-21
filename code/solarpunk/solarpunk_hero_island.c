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
 {1.75f, .15f, 2.25f, .75f},    {-2.25f, .3f, -1.65f, .8f},
 {-1.f, .35f, -.6f, .75f},      {-2.3f, -2.3f, -.6f, -1.75f},  {-1.05f, -.15f, -.4f, .05f},
 {1.7f, -2.3f, 2.3f, -.95f},    {0.f, -2.2f, .6f, -1.6f},
};

enum hero_ground
{
 HeroGround_Grass,
 HeroGround_Path,
 HeroGround_Stream,
 HeroGround_Pool,
 HeroGround_Sand,
};

// NOTE: The shape of one island's top: an optional harbour pool cut into the front,
// a terraced hill, optional raised plateaus, a stream and dirt paths.
typedef struct hero_layout hero_layout;
struct hero_layout
{
 b32 Pool;
 f32 PoolX0;
 f32 PoolX1;
 f32 PoolZ0;
 f32 HillX;
 f32 HillZ;
 f32 HillStretch;   // > 1 = wider along X
 f32 HillPeak;      // raw height at the hill centre
 f32 HillCap;       // > 0: the hill is flat-topped at this height
 f32 FrontLevel;    // height of the ground beside the harbour pool
 b32 Round;         // a round island of this Radius (else the square slab)
 f32 Radius;
 b32 Lagoon;        // an oval harbour (centre, radii), a channel from it to the front
 f32 LagoonX;       // edge, and a low shore (FrontLevel) out to ShoreScale x the radii
 f32 LagoonZ;
 f32 LagoonRX;
 f32 LagoonRZ;
 f32 ShoreScale;
 f32 ChannelX;
 f32 ChannelHalf;
 b32 SandyShore;    // the shore is a sand beach, sloping down to the water
 b32 Snowy;         // snow on the ground, ice on the water (Saltglass Atolls)
 b32 PlateauRight;
 b32 PlateauLeft;
 u32 StreamCount;
 f32 Stream[4][2];
 u32 PathCount;
 f32 Path[4][2];
 u32 Path2Count;
 f32 Path2[4][2];
};

typedef struct hero_ctx hero_ctx;
struct hero_ctx
{
 r3_mesh *Opaque;
 r3_mesh *Translucent;
 v3 Top;
 f32 Time;
 random_series Series;
 hero_layout *Layout;
 u8 Taken[Hero_Cells][Hero_Cells];   // terrain cells used by something (themed islands)
 r3_state *R3;
 u32 Island;      // index of the island being built
 b32 Recording;   // building the cache: moving parts are recorded, not drawn
 b32 GrassOnly;   // placement skips sand (fields and yards belong on grass)
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
HeroTerrain(hero_layout *L, f32 X, f32 Z, u32 *Type)
{
 f32 Result = 0.f;
 *Type = HeroGround_Grass;
 if(L->Round)
 {
  // Outside the circle: the ground at the rim.
  f32 RadiusSq = X*X + Z*Z;
  f32 Rim = .995f*L->Radius;
  if(RadiusSq > Rim*Rim)
  {
   f32 Scale = Rim/R3Sqrt(RadiusSq);
   X *= Scale;
   Z *= Scale;
  }
 }
 f32 LagoonDX = L->Lagoon ? (X - L->LagoonX)/L->LagoonRX : 0.f;
 f32 LagoonDZ = L->Lagoon ? (Z - L->LagoonZ)/L->LagoonRZ : 0.f;
 f32 LagoonE = LagoonDX*LagoonDX + LagoonDZ*LagoonDZ;
 b32 InChannel = L->Lagoon && Z > L->LagoonZ && AbsF32(X - L->ChannelX) < L->ChannelHalf;
 if(L->Lagoon && (LagoonE < 1.f || InChannel))
 {
  *Type = HeroGround_Pool;
  Result = Hero_PoolFloor;
 }
 else if(L->Lagoon && LagoonE < L->ShoreScale*L->ShoreScale)
 {
  Result = L->FrontLevel;
  *Type = L->SandyShore ? HeroGround_Sand : HeroGround_Grass;
  if(L->SandyShore && LagoonE < 1.3f*1.3f) Result = L->FrontLevel - .22f;   // the beach by the water
  if(L->StreamCount && HeroPolylineDistance(X, Z, L->Stream, L->StreamCount) < .15f) *Type = HeroGround_Stream;
 }
 else if(L->Pool && Z > L->PoolZ0)
 {
  if(X > L->PoolX0 && X < L->PoolX1)
  {
   *Type = HeroGround_Pool;
   Result = Hero_PoolFloor;
  }
  else
  {
   Result = L->FrontLevel;
   *Type = (X < L->PoolX0) ? HeroGround_Path : HeroGround_Grass;
  }
 }
 else
 {
  f32 DX = (X - L->HillX)/L->HillStretch;
  f32 DZ = Z - L->HillZ;
  f32 Raw = L->HillPeak - .8f*R3Sqrt(DX*DX + DZ*DZ);
  Result = (Raw > 0.f) ? Hero_Step*floorf(Raw/Hero_Step) : 0.f;
  if(L->HillCap > 0.f) Result = Min(Result, L->HillCap);
  if(L->PlateauRight && X > .95f && Z < .75f)  Result = Max(Result, Hero_Step);
  if(L->PlateauLeft && X < -1.15f && Z < .2f)  Result = Max(Result, Hero_Step);

  if(L->StreamCount && HeroPolylineDistance(X, Z, L->Stream, L->StreamCount) < .15f)
  {
   *Type = HeroGround_Stream;
  }
  else if((L->PathCount && HeroPolylineDistance(X, Z, L->Path, L->PathCount) < .09f) ||
          (L->Path2Count && HeroPolylineDistance(X, Z, L->Path2, L->Path2Count) < .08f))
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
internal s32
HeroCellIndex(f32 V)
{
 s32 Result = Clamp(0, (s32)floorf((V + Hero_Half)/HeroCellSize()), Hero_Cells - 1);
 return Result;
}

internal f32
HeroCellCenter(s32 I)
{
 f32 Result = -Hero_Half + ((f32)I + .5f)*HeroCellSize();
 return Result;
}

internal f32
HeroGroundY(hero_layout *L, f32 X, f32 Z, u32 *Type)
{
 f32 Result = HeroTerrain(L, HeroCellCenter(HeroCellIndex(X)), HeroCellCenter(HeroCellIndex(Z)), Type);
 return Result;
}

internal v3
HeroOn(hero_ctx *H, f32 X, f32 Z)
{
 u32 Type;
 v3 Result = HeroP(H, X, HeroGroundY(H->Layout, X, Z, &Type), Z);
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

// NOTE: While the cache is built, a moving part is recorded (to be drawn every frame by
// HeroReplayCommands) instead of drawn. Returns true when it was recorded.
internal b32
HeroDefer(hero_ctx *H, u32 Kind, v3 P, f32 A, f32 B, f32 C)
{
 b32 Result = H->Recording;
 if(Result && H->R3->HeroCommandCount < R3_MaxHeroCommands)
 {
  hero_command *Command = &H->R3->HeroCommands[H->R3->HeroCommandCount++];
  Command->Kind = Kind;
  Command->Island = H->Island;
  Command->P = P;
  Command->A = A;
  Command->B = B;
  Command->C = C;
 }
 return Result;
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
   f32 Level = HeroTerrain(H->Layout, X0 + .5f*Cell, Z0 + .5f*Cell, &Type);

   // Corners of the top; on a round island, corners outside the circle move onto it
   // and cells fully outside are skipped.
   f32 CX[4];
   f32 CZ[4];
   CX[0] = X0; CZ[0] = Z0;
   CX[1] = X1; CZ[1] = Z0;
   CX[2] = X1; CZ[2] = Z1;
   CX[3] = X0; CZ[3] = Z1;
   if(H->Layout->Round)
   {
    f32 Radius = H->Layout->Radius;
    u32 Inside = 0;
    for(u32 Corner = 0; Corner < 4; Corner += 1)
    {
     f32 RadiusSq = CX[Corner]*CX[Corner] + CZ[Corner]*CZ[Corner];
     if(RadiusSq <= Radius*Radius)
     {
      Inside += 1;
     }
     else
     {
      f32 Scale = Radius/R3Sqrt(RadiusSq);
      CX[Corner] *= Scale;
      CZ[Corner] *= Scale;
     }
    }
    if(Inside == 0) continue;
   }

   //- Top
   v4 TopColor;
   switch(Type)
   {
    case HeroGround_Path:   TopColor = HeroShade(H, Hero_ColorPath); break;
    case HeroGround_Stream: TopColor = R3Color(Hero_ColorStreamTop); break;
    case HeroGround_Pool:   TopColor = HeroShade(H, Hero_ColorPoolFloor); break;
    case HeroGround_Sand:   TopColor = HeroShade(H, 0xFFEAD7A8); break;
    default:
    {
     TopColor = R3LerpColor(R3Color(Hero_ColorGrassA), R3Color(Hero_ColorGrassB), RandomUnilateral(&H->Series));
    } break;
   }
   if(H->Layout->Snowy)
   {
    if(Type == HeroGround_Grass)        TopColor = R3LerpColor(R3Color(0xFFEEF3F8), R3Color(0xFFDCE5EE), RandomUnilateral(&H->Series));
    else if(Type == HeroGround_Path)    TopColor = HeroShade(H, 0xFFC4CDD6);
    else if(Type == HeroGround_Stream)  TopColor = R3Color(0xFFCDEBF5);
   }
   R3PushQuad(H->Opaque, HeroP(H, CX[0], Level, CZ[0]), HeroP(H, CX[1], Level, CZ[1]), HeroP(H, CX[2], Level, CZ[2]), HeroP(H, CX[3], Level, CZ[3]),
              HeroP(H, X0 + .5f*Cell, Level - 1.f, Z0 + .5f*Cell), TopColor);
   if(H->Layout->Round && Type == HeroGround_Pool)
   {
    // Round islands get their pond surface cell by cell, so it follows the rim.
    v4 Water = R3WithAlpha(R3Color(Hero_ColorPoolWater), .62f);
    v3 P0 = HeroP(H, CX[0], Hero_WaterLevel, CZ[0]);
    v3 P1 = HeroP(H, CX[1], Hero_WaterLevel, CZ[1]);
    v3 P2 = HeroP(H, CX[2], Hero_WaterLevel, CZ[2]);
    v3 P3 = HeroP(H, CX[3], Hero_WaterLevel, CZ[3]);
    R3PushTriUnlit(H->Translucent, P0, P1, P2, Water);
    R3PushTriUnlit(H->Translucent, P0, P2, P3, Water);
   }

   //- Walls down to lower neighbours, and the outside of the slab
   for(u32 Dir = 0; Dir < 4; Dir += 1)
   {
    s32 NI = I + DI[Dir];
    s32 NJ = J + DJ[Dir];
    if(NI >= 0 && NI < Hero_Cells && NJ >= 0 && NJ < Hero_Cells)
    {
     u32 NeighbourType;
     f32 NeighbourLevel = HeroTerrain(H->Layout, -Hero_Half + ((f32)NI + .5f)*Cell, -Hero_Half + ((f32)NJ + .5f)*Cell, &NeighbourType);
     if(NeighbourLevel < Level - .001f)
     {
      u32 WallColor = (Type == HeroGround_Sand) ? 0xFFD9C495 : (H->Layout->Snowy ? 0xFFB7C1CA : Hero_ColorWall);
      HeroEdgeWall(H, H->Opaque, Dir, X0, X1, Z0, Z1, NeighbourLevel, Level, 0.f, HeroShade(H, WallColor));
      if(Type == HeroGround_Stream)
      {
       // Water falling over the terrace edge.
       f32 Bottom = (NeighbourType == HeroGround_Pool) ? Hero_WaterLevel : NeighbourLevel;
       HeroEdgeWall(H, H->Translucent, Dir, X0, X1, Z0, Z1, Bottom, Level + .01f, .025f, R3WithAlpha(R3Color(H->Layout->Snowy ? 0xFFD6F0FA : Hero_ColorFall), H->Layout->Snowy ? .9f : .72f));
      }
     }
    }
    else if(Type != HeroGround_Pool && !H->Layout->Round)
    {
     if(Level > 0.f)
     {
      HeroEdgeWall(H, H->Opaque, Dir, X0, X1, Z0, Z1, 0.f, Level, 0.f, HeroShade(H, Hero_ColorWall));
     }
     HeroEdgeWall(H, H->Opaque, Dir, X0, X1, Z0, Z1, Hero_DirtBottom, Min(Level, 0.f), 0.f, HeroShade(H, H->Layout->Snowy ? 0xFF7A6E64 : Hero_ColorDirt));
    }
   }
  }
 }

 //- Harbour water: the surface and the see-through cut at the front of the slab
 hero_layout *L = H->Layout;
 if(L->Pool && !L->Round)
 {
  v4 Water = L->Snowy ? R3WithAlpha(R3Color(0xFFD6EEF7), .88f) : R3WithAlpha(R3Color(Hero_ColorPoolWater), .62f);
  v4 Deep = L->Snowy ? R3WithAlpha(R3Color(0xFF9CCFE3), .82f) : R3WithAlpha(R3Color(Hero_ColorPoolDeep), .7f);
  v3 W0 = HeroP(H, L->PoolX0, Hero_WaterLevel, L->PoolZ0);
  v3 W1 = HeroP(H, L->PoolX1, Hero_WaterLevel, L->PoolZ0);
  v3 W2 = HeroP(H, L->PoolX1, Hero_WaterLevel, Hero_Half);
  v3 W3 = HeroP(H, L->PoolX0, Hero_WaterLevel, Hero_Half);
  R3PushTriUnlit(H->Translucent, W0, W1, W2, Water);
  R3PushTriUnlit(H->Translucent, W0, W2, W3, Water);
  v3 F0 = HeroP(H, L->PoolX0, Hero_PoolFloor, Hero_Half);
  v3 F1 = HeroP(H, L->PoolX1, Hero_PoolFloor, Hero_Half);
  R3PushTriUnlit(H->Translucent, W3, W2, F1, Deep);
  R3PushTriUnlit(H->Translucent, W3, F1, F0, Deep);
 }
}

//~ Cliff: dirt band, stacked stone and the rugged underside
internal void
HeroPushCliff(hero_ctx *H)
{
 r3_mesh *O = H->Opaque;
 hero_layout *L = H->Layout;
 f32 In = Hero_Half - .02f;
 v4 Core = R3Color(Hero_ColorCore);

 //- Core, so there are no holes between the stones. With a harbour, the front part
 // leaves room for the water cut.
 f32 MidY = .5f*(Hero_DirtBottom + Hero_StoneBottom);
 f32 HalfY = .5f*(Hero_DirtBottom - Hero_StoneBottom);
 if(L->Pool)
 {
  R3PushBox(O, HeroP(H, 0.f, MidY, .5f*(-In + L->PoolZ0)), V3(In, HalfY, .5f*(L->PoolZ0 + In)), Core);
  R3PushBox(O, HeroP(H, 0.f, .5f*(Hero_PoolFloor + Hero_StoneBottom), .5f*(L->PoolZ0 + In)),
            V3(In, .5f*(Hero_PoolFloor - Hero_StoneBottom), .5f*(In - L->PoolZ0)), Core);
  if(L->PoolX0 > -In)
  {
   R3PushBox(O, HeroP(H, .5f*(-In + L->PoolX0), .5f*(Hero_DirtBottom + Hero_PoolFloor), .5f*(L->PoolZ0 + In)),
             V3(.5f*(L->PoolX0 + In), .5f*(Hero_DirtBottom - Hero_PoolFloor), .5f*(In - L->PoolZ0)), Core);
  }
  if(L->PoolX1 < In)
  {
   R3PushBox(O, HeroP(H, .5f*(In + L->PoolX1), .5f*(Hero_DirtBottom + Hero_PoolFloor), .5f*(L->PoolZ0 + In)),
             V3(.5f*(In - L->PoolX1), .5f*(Hero_DirtBottom - Hero_PoolFloor), .5f*(In - L->PoolZ0)), Core);
  }
 }
 else
 {
  R3PushBox(O, HeroP(H, 0.f, MidY, 0.f), V3(In, HalfY, In), Core);
 }

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
    if(L->Pool && Side == 0 && S1 > L->PoolX0 && S0 < L->PoolX1 && YTop > Hero_PoolFloor + .01f) Skip = true;
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
   if(L->Pool && Side == 0 && S > L->PoolX0 - .05f && S < L->PoolX1 + .05f) continue;
   f32 X = (Side == 0 || Side == 1) ? S : ((Side == 2) ? Hero_Half : -Hero_Half);
   f32 Z = (Side == 0) ? Hero_Half : ((Side == 1) ? -Hero_Half : S);
   u32 Type;
   f32 Level = HeroGroundY(L, X*.995f, Z*.995f, &Type);
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
// Variant bits (continued): 8 = red barn, 16 = on a deck (wooden floor, no stone
// plinth), 32 = mossy roof.
internal void
HeroPushHouseAt(hero_ctx *H, v3 B, f32 HX, f32 HZ, u32 Floors, u32 Variant)
{
 r3_mesh *O = H->Opaque;
 f32 Plinth = (Variant & 16) ? .04f : .14f;
 f32 FloorH = .34f;
 f32 WallH = FloorH*(f32)Floors;
 u32 Wood = HeroWoodColors[Variant % ArrayCount(HeroWoodColors)];
 u32 Roof = HeroRoofColors[(Variant/2) % ArrayCount(HeroRoofColors)];
 if(Variant & 8) Wood = 0xFFB5523B;   // a red barn
 if(Variant & 32) Roof = 0xFF5E8A3A;  // moss
 if(Variant & 64)
 {
  // Weathered grey boards under a rusty tin roof (fishing villages)
  u32 Grey[3] = {0xFF8C8478, 0xFF9C9284, 0xFF7E766B};
  u32 Tin[3] = {0xFF8E6A4E, 0xFF9A8676, 0xFF7C5A44};
  Wood = Grey[Variant % 3];
  Roof = Tin[(Variant/2) % 3];
 }
 if(Variant & 128) Roof = 0xFFC9A86A;  // thatch
 if(Variant & 256) Roof = 0xFFF2F6FA;  // snow

 //- Stone plinth (reaches down so houses on terrace edges do not float), or a
 // wooden floor on a deck
 if(Variant & 16)
 {
  R3PushBox(O, R3Add(B, V3(0.f, .5f*Plinth, 0.f)), V3(HX + .04f, .5f*Plinth, HZ + .04f), HeroShade(H, Hero_ColorWoodLight));
 }
 else
 {
  R3PushBox(O, R3Add(B, V3(0.f, .5f*Plinth - .25f, 0.f)), V3(HX + .035f, .5f*Plinth + .25f, HZ + .035f), HeroShade(H, Hero_ColorStone));
 }

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
 if(Variant & 256)
 {
  // Snow overhanging the eaves, icicles hanging from it
  R3PushBox(O, R3Lerp(F0, F1, .5f), V3(HX + Over, .02f, .025f), R3Color(0xFFF2F6FA));
  for(u32 Icicle = 0; Icicle < 9; Icicle += 1)
  {
   v3 P = R3Lerp(F0, F1, ((f32)Icicle + .5f)/9.f);
   R3PushCone(O, R3Add(P, V3(0.f, -.01f, .01f)), R3Add(P, V3(0.f, -.05f - .07f*RandomUnilateral(&H->Series), .01f)), .012f, 4, R3Color(0xFFD8F0FA));
  }
 }
 if(Variant & 128)
 {
  // Thatch: a shaggy fringe hanging over the eaves
  R3PushBox(O, R3Lerp(F0, F1, .5f), V3(HX + Over, .018f, .02f), HeroShade(H, 0xFFB08E52));
  R3PushBox(O, R3Lerp(A0, A1, .5f), V3(HX + Over, .018f, .02f), HeroShade(H, 0xFFB08E52));
 }
 if(Variant & 32)
 {
  // Moss and ferns growing on the roof
  for(u32 Tuft = 0; Tuft < 7; Tuft += 1)
  {
   f32 TX = RandomUnilateral(&H->Series);
   f32 TY = RandomUnilateral(&H->Series);
   v3 Low = R3Lerp((Tuft & 1) ? F0 : A0, (Tuft & 1) ? F1 : A1, TX);
   v3 High = R3Lerp(R0, R1, TX);
   R3PushEllipsoid(O, R3Lerp(Low, High, .2f + .6f*TY), V3(.07f, .03f, .07f), 3, 6, HeroShade(H, Hero_ColorLeaf), false);
  }
 }

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
HeroPushHouse(hero_ctx *H, f32 X, f32 Z, f32 HX, f32 HZ, u32 Floors, u32 Variant)
{
 HeroPushHouseAt(H, HeroOn(H, X, Z), HX, HZ, Floors, Variant);
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

// NOTE: A windmill's six-bladed rotor, turning with time (a moving part).
internal void
HeroPushRotor(hero_ctx *H, v3 Hub, f32 Phase)
{
 if(HeroDefer(H, HeroCmd_Rotor, Hub, Phase, 0.f, 0.f)) return;
 r3_mesh *O = H->Opaque;
 v4 Wood = R3Color(Hero_ColorWoodDark);
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

 HeroPushRotor(H, R3Add(Head, V3(0.f, 0.f, .11f)), Phase);
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

// NOTE: A raised garden bed. Produce = colour of what grows in it (0 = mixed).
internal void
HeroPushBed(hero_ctx *H, f32 X, f32 Z, f32 HX, f32 HZ, u32 Produce)
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
   if(Produce)
   {
    if(Roll < .6f) R3PushEllipsoid(O, R3Add(P, V3(0.f, -.01f, .03f)), V3(.032f, .03f, .032f), 3, 5, R3Color(Produce), false);
   }
   else if(Roll < .2f)  R3PushEllipsoid(O, R3Add(P, V3(0.f, -.01f, .03f)), V3(.03f, .028f, .03f), 3, 5, R3Color(0xFFF08A2C), false);
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

// NOTE: A rowing boat bobbing on the water (a moving part). Base = hull centre at rest.
internal void
HeroPushBoatAt(hero_ctx *H, v3 Base, f32 Facing, f32 Phase, b32 Rower)
{
 if(HeroDefer(H, HeroCmd_Boat, Base, Facing, Phase, Rower ? 1.f : 0.f)) return;
 r3_mesh *O = H->Opaque;
 f32 Bob = .012f*R3Sin(1.3f*H->Time + Phase);
 v3 C = R3Add(Base, V3(0.f, Bob, 0.f));
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

internal void
HeroPushBoat(hero_ctx *H, f32 X, f32 Z, f32 Facing, f32 Phase, b32 Rower)
{
 HeroPushBoatAt(H, HeroP(H, X, Hero_WaterLevel + .02f, Z), Facing, Phase, Rower);
}

// NOTE: A water wheel with its axle along X, turning with time.
internal void
HeroPushWaterWheel(hero_ctx *H, v3 C, f32 Radius)
{
 if(HeroDefer(H, HeroCmd_WaterWheel, C, Radius, 0.f, 0.f)) return;
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

// NOTE: Flickering campfire flames and their glow (a moving part).
internal void
HeroPushFlames(hero_ctx *H, v3 B)
{
 if(HeroDefer(H, HeroCmd_Flames, B, 0.f, 0.f, 0.f)) return;
 r3_mesh *O = H->Opaque;
 f32 Flicker = 1.f + .15f*R3Sin(9.f*H->Time) + .08f*R3Sin(23.f*H->Time);
 R3PushEllipsoid(O, R3Add(B, V3(0.f, .07f*Flicker, 0.f)), V3(.045f, .07f*Flicker, .045f), 4, 6, R3Color(Hero_ColorFire), true);
 R3PushEllipsoid(O, R3Add(B, V3(0.f, .055f*Flicker, 0.f)), V3(.025f, .045f*Flicker, .025f), 3, 6, R3Color(Hero_ColorFireCore), true);
 R3PushEllipsoid(H->Translucent, R3Add(B, V3(0.f, .08f, 0.f)), V3(.22f, .16f, .22f), 4, 8, R3WithAlpha(R3Color(Hero_ColorFire), .18f), true);
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
 HeroPushFlames(H, B);
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
HeroPushCrates(hero_ctx *H, island *Island, f32 ProduceX, f32 ProduceZ, f32 NeedX, f32 NeedZ)
{
 r3_mesh *O = H->Opaque;
 f32 Half = .1f;

 u32 Slot = 0;
 for(product *Product = Island->FirstProduct; Product && Slot < 3; Product = Product->Next)
 {
  v3 B = HeroOn(H, ProduceX, ProduceZ + .26f*(f32)Slot);
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
  v3 Center = R3Add(HeroOn(H, NeedX + .26f*(f32)Slot, NeedZ), V3(0.f, Half, 0.f));
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

//~ Free ground (themed islands place things at random on flat, unused cells)
internal void
HeroMarkRect(hero_ctx *H, f32 X0, f32 Z0, f32 X1, f32 Z1)
{
 for(s32 J = HeroCellIndex(Z0); J <= HeroCellIndex(Z1); J += 1)
 {
  for(s32 I = HeroCellIndex(X0); I <= HeroCellIndex(X1); I += 1)
  {
   H->Taken[J][I] = 1;
  }
 }
}

internal b32
HeroCellTaken(hero_ctx *H, f32 X, f32 Z)
{
 b32 Result = (H->Taken[HeroCellIndex(Z)][HeroCellIndex(X)] != 0);
 return Result;
}

// NOTE: Claims a flat, free rectangle of grass or path (half sizes HX, HZ) at X, Z.
internal b32
HeroClaim(hero_ctx *H, f32 X, f32 Z, f32 HX, f32 HZ)
{
 b32 Result = (X - HX > -Hero_Half + .12f && X + HX < Hero_Half - .12f &&
               Z - HZ > -Hero_Half + .12f && Z + HZ < Hero_Half - .12f);
 if(H->Layout->Round)
 {
  f32 FarX = AbsF32(X) + HX;
  f32 FarZ = AbsF32(Z) + HZ;
  f32 Limit = H->Layout->Radius - .12f;
  Result = Result && (FarX*FarX + FarZ*FarZ < Limit*Limit);
 }
 u32 CenterType;
 f32 Level = HeroGroundY(H->Layout, X, Z, &CenterType);
 s32 I0 = HeroCellIndex(X - HX);
 s32 I1 = HeroCellIndex(X + HX);
 s32 J0 = HeroCellIndex(Z - HZ);
 s32 J1 = HeroCellIndex(Z + HZ);
 for(s32 J = J0; J <= J1 && Result; J += 1)
 {
  for(s32 I = I0; I <= I1 && Result; I += 1)
  {
   u32 Type;
   f32 CellLevel = HeroTerrain(H->Layout, HeroCellCenter(I), HeroCellCenter(J), &Type);
   b32 GroundOk = (Type == HeroGround_Grass || Type == HeroGround_Path || (Type == HeroGround_Sand && !H->GrassOnly));
   Result = (!H->Taken[J][I] && GroundOk && AbsF32(CellLevel - Level) < .01f);
  }
 }
 if(Result)
 {
  HeroMarkRect(H, X - HX, Z - HZ, X + HX, Z + HZ);
 }
 return Result;
}

// NOTE: Finds a free spot for a rectangle somewhere between ZMin and ZMax.
internal b32
HeroPlace(hero_ctx *H, f32 HX, f32 HZ, f32 ZMin, f32 ZMax, f32 *X, f32 *Z)
{
 b32 Result = false;
 for(u32 Try = 0; Try < 90 && !Result; Try += 1)
 {
  f32 PX = (Hero_Half - HX - .15f)*RandomBilateral(&H->Series);
  f32 PZ = ZMin + (ZMax - ZMin)*RandomUnilateral(&H->Series);
  if(HeroClaim(H, PX, PZ, HX, HZ))
  {
   *X = PX;
   *Z = PZ;
   Result = true;
  }
 }
 return Result;
}

// NOTE: Like HeroPlace, but near a preferred spot (tries the spot itself first). The
// rectangle shrinks a little when nothing fits.
internal b32
HeroPlaceNear(hero_ctx *H, f32 PreferX, f32 PreferZ, f32 Radius, f32 *HX, f32 *HZ, f32 *X, f32 *Z)
{
 b32 Result = false;
 for(u32 Try = 0; Try < 150 && !Result; Try += 1)
 {
  if(Try > 0 && (Try % 30) == 0)
  {
   *HX *= .85f;
   *HZ *= .85f;
  }
  f32 PX = PreferX;
  f32 PZ = PreferZ;
  if(Try > 0)
  {
   PX += Radius*RandomBilateral(&H->Series);
   PZ += Radius*RandomBilateral(&H->Series);
  }
  if(HeroClaim(H, PX, PZ, *HX, *HZ))
  {
   *X = PX;
   *Z = PZ;
   Result = true;
  }
 }
 return Result;
}

// NOTE: Where to put one block of a product: near Prefer (when given) or anywhere free
// between ZMin and ZMax. The size may shrink to fit.
typedef struct hero_spot hero_spot;
struct hero_spot
{
 b32 HasPrefer;
 f32 PreferX;
 f32 PreferZ;
};

internal b32
HeroFindSpot(hero_ctx *H, hero_spot *Spot, f32 *HX, f32 *HZ, f32 ZMin, f32 ZMax, f32 *X, f32 *Z)
{
 b32 Result = false;
 if(Spot && Spot->HasPrefer)
 {
  Result = HeroPlaceNear(H, Spot->PreferX, Spot->PreferZ, .7f, HX, HZ, X, Z);
 }
 for(u32 Shrink = 0; Shrink < 4 && !Result; Shrink += 1)
 {
  Result = HeroPlace(H, *HX, *HZ, ZMin, ZMax, X, Z);
  if(!Result)
  {
   *HX *= .85f;
   *HZ *= .85f;
  }
 }
 return Result;
}

//~ Farm pieces for the themed islands
// NOTE: A vegetable field: a tilled plot with furrows, planted with cabbage heads or
// carrots (orange shoulders under feathery tops), and a crate of the harvest.
internal void
HeroPushVegField(hero_ctx *H, f32 X, f32 Z, f32 HX, f32 HZ, product_kind Kind)
{
 r3_mesh *O = H->Opaque;
 v3 C = HeroOn(H, X, Z);
 R3PushBox(O, R3Add(C, V3(0.f, .02f, 0.f)), V3(HX, .022f, HZ), R3Color(Hero_ColorWoodDark));
 R3PushBox(O, R3Add(C, V3(0.f, .024f, 0.f)), V3(HX - .025f, .022f, HZ - .025f), R3Color(Hero_ColorSoil));

 b32 Cabbage = (Kind == ProductKind_Cabbage);
 f32 RowGap = Cabbage ? .14f : .12f;
 f32 PlantGap = Cabbage ? .11f : .075f;
 u32 Rows = Max(1, (u32)((2.f*HZ - .06f)/RowGap));
 u32 Plants = Max(1, (u32)((2.f*HX - .08f)/PlantGap));
 f32 RowStart = -.5f*RowGap*(f32)(Rows - 1);
 f32 PlantStart = -.5f*PlantGap*(f32)(Plants - 1);
 for(u32 Row = 0; Row < Rows; Row += 1)
 {
  f32 DZ = RowStart + RowGap*(f32)Row;
  R3PushBox(O, R3Add(C, V3(0.f, .05f, DZ)), V3(HX - .04f, .01f, .028f), R3Color(0xFF6E5038));
  for(u32 Plant = 0; Plant < Plants; Plant += 1)
  {
   f32 DX = PlantStart + PlantGap*(f32)Plant + ((Row & 1) ? .25f*PlantGap : 0.f);
   if(DX > HX - .04f) continue;
   v3 P = R3Add(C, V3(DX, .06f, DZ));
   if(Cabbage)
   {
    R3PushEllipsoid(O, R3Add(P, V3(0.f, .025f, 0.f)), V3(.045f, .032f, .045f), 3, 6, HeroShade(H, 0xFF4E9A48), false);
    R3PushEllipsoid(O, R3Add(P, V3(0.f, .04f, 0.f)), V3(.028f, .026f, .028f), 3, 6, R3Color(0xFFA8DC7E), false);
   }
   else
   {
    R3PushEllipsoid(O, R3Add(P, V3(0.f, .006f, 0.f)), V3(.016f, .014f, .016f), 2, 5, R3Color(W3ProductColors[ProductKind_Carrots]), false);
    for(u32 Leaf = 0; Leaf < 3; Leaf += 1)
    {
     f32 A = 2.1f*(f32)Leaf + (f32)Plant;
     R3PushCone(O, R3Add(P, V3(0.f, .012f, 0.f)), R3Add(P, V3(.03f*R3Cos(A), .1f + .02f*(f32)(Leaf & 1), .03f*R3Sin(A))), .012f, 3, R3Color(0xFF4F9D3A));
    }
   }
  }
 }

 // A farmer at work beside the field
 HeroPushPerson(H, R3Add(C, V3(-HX + .06f, 0.f, HZ + .07f)), HeroShirtColors[RandomNext(&H->Series) % ArrayCount(HeroShirtColors)]);

 // Harvest crate at the front corner
 v3 Crate = R3Add(C, V3(HX - .08f, .045f, HZ + .07f));
 R3PushBox(O, R3Add(Crate, V3(0.f, .03f, 0.f)), V3(.055f, .035f, .04f), HeroShade(H, Hero_ColorWoodLight));
 for(u32 Item = 0; Item < 4; Item += 1)
 {
  v3 P = R3Add(Crate, V3(-.03f + .02f*(f32)Item, .07f, ((Item & 1) ? .012f : -.012f)));
  if(Cabbage) R3PushEllipsoid(O, P, V3(.022f, .02f, .022f), 3, 5, R3Color(W3ProductColors[ProductKind_Cabbage]), false);
  else        R3PushEllipsoid(O, P, V3(.03f, .01f, .01f), 2, 5, R3Color(W3ProductColors[ProductKind_Carrots]), false);
 }
}
internal void
HeroPushFence(hero_ctx *H, f32 X, f32 Z, f32 HX, f32 HZ)
{
 r3_mesh *O = H->Opaque;
 v3 C = HeroOn(H, X, Z);
 f32 CX[4] = {-1.f, 1.f, 1.f, -1.f};
 f32 CZ[4] = {1.f, 1.f, -1.f, -1.f};
 v4 Wood = R3Color(Hero_ColorWoodLight);
 for(u32 Side = 0; Side < 4; Side += 1)
 {
  u32 Next = (Side + 1) % 4;
  v3 A = R3Add(C, V3(CX[Side]*HX, 0.f, CZ[Side]*HZ));
  v3 B = R3Add(C, V3(CX[Next]*HX, 0.f, CZ[Next]*HZ));
  f32 Length = R3Length(R3Sub(B, A));
  u32 Posts = Max(2, (u32)(Length/.14f) + 1);
  for(u32 Post = 0; Post < Posts; Post += 1)
  {
   v3 P = R3Lerp(A, B, (f32)Post/(f32)(Posts - 1));
   R3PushBox(O, R3Add(P, V3(0.f, .055f, 0.f)), V3(.01f, .055f, .01f), R3Color(Hero_ColorWoodDark));
  }
  for(u32 Rail = 0; Rail < 2; Rail += 1)
  {
   v3 Up = V3(0.f, (Rail == 0) ? .045f : .09f, 0.f);
   if(Side == 0)
   {
    // A gate gap in the middle of the front.
    v3 Mid = R3Lerp(A, B, .5f);
    v3 Gap = R3Scale(R3Normalize(R3Sub(B, A)), .07f);
    R3PushCylinder(O, R3Add(A, Up), R3Add(R3Sub(Mid, Gap), Up), .007f, 4, Wood);
    R3PushCylinder(O, R3Add(R3Add(Mid, Gap), Up), R3Add(B, Up), .007f, 4, Wood);
   }
   else
   {
    R3PushCylinder(O, R3Add(A, Up), R3Add(B, Up), .007f, 4, Wood);
   }
  }
 }
}

internal void
HeroPushHen(hero_ctx *H, v3 B, f32 Facing)
{
 r3_mesh *O = H->Opaque;
 u32 Body = (RandomUnilateral(&H->Series) < .3f) ? 0xFFB9783E : 0xFFFFFFFF;
 R3PushEllipsoid(O, R3Add(B, V3(0.f, .035f, 0.f)), V3(.035f, .028f, .027f), 3, 6, R3Color(Body), false);
 R3PushEllipsoid(O, R3Add(B, V3(.03f*Facing, .064f, 0.f)), V3(.017f, .017f, .017f), 3, 5, R3Color(Body), false);
 R3PushBox(O, R3Add(B, V3(.03f*Facing, .083f, 0.f)), V3(.007f, .006f, .003f), R3Color(0xFFE0453A));
 R3PushCone(O, R3Add(B, V3(.044f*Facing, .064f, 0.f)), R3Add(B, V3(.06f*Facing, .061f, 0.f)), .007f, 4, R3Color(0xFFF2B33D));
}

internal void
HeroPushOrchard(hero_ctx *H, f32 X, f32 Z, f32 HX, f32 HZ, product_kind Fruit)
{
 v3 C = HeroOn(H, X, Z);
 u32 Leaves = (Fruit == ProductKind_Pears) ? 0xFF6FAE4A : 0xFF3F8E4A;
 for(f32 DZ = -HZ + .15f; DZ <= HZ - .1f; DZ += .3f)
 {
  for(f32 DX = -HX + .15f; DX <= HX - .1f; DX += .3f)
  {
   W3PushFruitTree(H->Opaque, R3Add(C, V3(DX, 0.f, DZ)), .45f + .12f*RandomUnilateral(&H->Series), Leaves, Fruit, &H->Series);
  }
 }
 // A basket of picked fruit
 v3 Basket = R3Add(C, V3(HX - .08f, 0.f, HZ - .02f));
 R3PushBox(H->Opaque, R3Add(Basket, V3(0.f, .03f, 0.f)), V3(.045f, .03f, .035f), R3Color(Hero_ColorWoodLight));
 R3PushEllipsoid(H->Opaque, R3Add(Basket, V3(0.f, .065f, 0.f)), V3(.035f, .018f, .027f), 3, 5, R3Color(W3ProductColors[Fruit]), false);
}

internal void
HeroPushWheatField(hero_ctx *H, f32 X, f32 Z, f32 HX, f32 HZ)
{
 r3_mesh *O = H->Opaque;
 v3 C = HeroOn(H, X, Z);
 R3PushBox(O, R3Add(C, V3(0.f, .008f, 0.f)), V3(HX, .01f, HZ), R3Color(W3_ColorFarmSoil));
 for(f32 DZ = -HZ + .06f; DZ <= HZ - .04f; DZ += .11f)
 {
  R3PushBox(O, R3Add(C, V3(0.f, .055f, DZ)), V3(HX - .03f, .045f, .038f), HeroShade(H, W3ProductColors[ProductKind_Wheat]));
 }
 // Hay bales at the corner
 v3 Bale = R3Add(C, V3(HX + .02f, .06f, HZ + .02f));
 R3PushCylinder(O, R3Add(Bale, V3(-.06f, 0.f, 0.f)), R3Add(Bale, V3(.06f, 0.f, 0.f)), .06f, 8, R3Color(W3ProductColors[ProductKind_Wheat]));
}

internal void HeroPushSails(hero_ctx *H, v3 Hub, f32 Phase);

// NOTE: A white tower mill with four cloth sails, turning with time.
internal void
HeroPushTowerMill(hero_ctx *H, f32 X, f32 Z, f32 Phase)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 R3PushCone(O, B, R3Add(B, V3(0.f, 1.3f, 0.f)), .22f, 10, R3Color(0xFFF4EBDD));
 R3PushCone(O, R3Add(B, V3(0.f, .82f, 0.f)), R3Add(B, V3(0.f, 1.1f, 0.f)), .15f, 10, R3Color(0xFF6A4E3A));
 R3PushBox(O, R3Add(B, V3(0.f, .1f, .2f)), V3(.05f, .1f, .012f), R3Color(Hero_ColorWoodDark));
 HeroGlowRect(O, R3Add(B, V3(0.f, .45f, .155f)), V3(.03f, 0.f, 0.f), V3(0.f, .035f, 0.f), R3Color(Hero_ColorWindow));
 HeroPushSails(H, R3Add(B, V3(0.f, .92f, .17f)), Phase);
}

// NOTE: A tower mill's four cloth sails, turning with time (a moving part).
internal void
HeroPushSails(hero_ctx *H, v3 Hub, f32 Phase)
{
 if(HeroDefer(H, HeroCmd_Sails, Hub, Phase, 0.f, 0.f)) return;
 r3_mesh *O = H->Opaque;
 R3PushEllipsoid(O, Hub, V3(.035f, .035f, .035f), 3, 6, R3Color(Hero_ColorWoodDark), false);
 f32 Spin = 1.1f*H->Time + Phase;
 for(u32 Sail = 0; Sail < 4; Sail += 1)
 {
  f32 Angle = Spin + .5f*Pi32*(f32)Sail;
  v3 Dir = V3(R3Cos(Angle), R3Sin(Angle), 0.f);
  v3 Side = V3(-Dir.Y, Dir.X, 0.f);
  R3PushCylinder(O, Hub, R3Add(Hub, R3Scale(Dir, .5f)), .01f, 4, R3Color(Hero_ColorWoodDark));
  R3PushOrientedBox(O, R3Add(R3Add(Hub, R3Scale(Dir, .3f)), R3Scale(Side, .045f)), R3Scale(Dir, .19f), R3Scale(Side, .045f), V3(0.f, 0.f, .004f), R3Color(Hero_ColorCloth));
 }
}

// NOTE: Everything an island makes of one product, sized by the amount it makes:
//   Cabbage, Carrots : vegetable fields (area grows with the amount), a greenhouse for big harvests
//   Apples, Pears    : orchard trees, about one per 10 units
//   Wheat            : wheat fields (area grows with the amount), a tower mill for big harvests
//   Fish             : drying racks (boats are added in the harbour, see the caller)
//   Meat             : cows in a fenced pasture, about one per 10 units, a barn for a herd
//   Eggs             : hens in fenced yards, about one per 12 units, with coops
// Spot (optional) is a preferred place for the first block.
internal void
HeroPushProduct(hero_ctx *H, product_kind Kind, u64 Amount, hero_spot *Spot)
{
 f32 X, Z;
 switch(Kind)
 {
  case ProductKind_Cabbage:
  case ProductKind_Carrots:
  {
   f32 Area = .0045f*(f32)Amount;
   u32 Fields = (Amount >= 110) ? 2 : 1;
   for(u32 Field = 0; Field < Fields; Field += 1)
   {
    f32 FieldArea = Area/(f32)Fields;
    f32 HX = .5f*R3Sqrt(FieldArea*1.4f);
    f32 HZ = FieldArea/(4.f*HX);
    if(HeroFindSpot(H, (Field == 0) ? Spot : 0, &HX, &HZ, -1.8f, 1.9f, &X, &Z))
    {
     HeroPushVegField(H, X, Z, HX - .02f, HZ - .06f, Kind);
    }
   }
   if(Amount >= 120)
   {
    f32 HX = .27f, HZ = .23f;
    if(HeroFindSpot(H, 0, &HX, &HZ, -1.8f, 1.9f, &X, &Z)) HeroPushGreenhouse(H, X, Z, HX - .03f, HZ - .03f);
   }
  } break;

  case ProductKind_Apples:
  case ProductKind_Pears:
  {
   u32 Trees = Max(2, (u32)(Amount/10));
   b32 First = true;
   while(Trees > 0)
   {
    u32 Block = Min(Trees, 8);
    u32 Columns = Min(Block, 4);
    u32 RowsInBlock = (Block + 3)/4;
    f32 HX = .15f*(f32)Columns;
    f32 HZ = .15f*(f32)RowsInBlock;
    if(HeroFindSpot(H, First ? Spot : 0, &HX, &HZ, -1.9f, 1.9f, &X, &Z))
    {
     HeroPushOrchard(H, X, Z, HX, HZ, Kind);
    }
    Trees -= Block;
    First = false;
   }
  } break;

  case ProductKind_Wheat:
  {
   f32 Area = .005f*(f32)Amount;
   u32 Fields = (Amount >= 110) ? 2 : 1;
   for(u32 Field = 0; Field < Fields; Field += 1)
   {
    f32 FieldArea = Area/(f32)Fields;
    f32 HX = .5f*R3Sqrt(FieldArea*1.5f);
    f32 HZ = FieldArea/(4.f*HX);
    if(HeroFindSpot(H, (Field == 0) ? Spot : 0, &HX, &HZ, -1.8f, 1.9f, &X, &Z))
    {
     HeroPushWheatField(H, X, Z, HX - .1f, HZ - .1f);
    }
   }
   if(Amount >= 100)
   {
    f32 HX = .26f, HZ = .26f;
    if(HeroFindSpot(H, 0, &HX, &HZ, -2.f, 1.f, &X, &Z)) HeroPushTowerMill(H, X, Z, 6.f*RandomUnilateral(&H->Series));
   }
  } break;

  case ProductKind_Fish:
  {
   u32 Racks = 1 + (u32)(Amount/60);
   for(u32 Rack = 0; Rack < Racks; Rack += 1)
   {
    f32 HX = .34f, HZ = .08f;
    if(HeroFindSpot(H, (Rack == 0) ? Spot : 0, &HX, &HZ, -1.5f, 2.f, &X, &Z))
    {
     W3PushSmallProduct(H->Opaque, 0, ProductKind_Fish, HeroOn(H, X, Z), &H->Series);
    }
   }
  } break;

  case ProductKind_Meat:
  {
   u32 Cows = Max(1, (u32)(Amount/10));
   f32 HX = .26f + .07f*(f32)Cows;
   f32 HZ = .22f + .04f*(f32)Cows;
   f32 ClaimHX = HX + .04f, ClaimHZ = HZ + .04f;
   if(HeroFindSpot(H, Spot, &ClaimHX, &ClaimHZ, -1.8f, 1.9f, &X, &Z))
   {
    HX = ClaimHX - .04f;
    HZ = ClaimHZ - .04f;
    HeroPushFence(H, X, Z, HX, HZ);
    for(u32 Cow = 0; Cow < Cows; Cow += 1)
    {
     f32 CX = X + (HX - .2f)*RandomBilateral(&H->Series);
     f32 CZ = Z + (HZ - .12f)*RandomBilateral(&H->Series);
     b32 Brown = (Cow & 1);
     HeroPushAnimal(H, CX, CZ, .75f, Brown ? 0xFFB98A5E : 0xFFF4EFE6, Brown ? 0xFF5A3D26 : 0xFF4A3A30, (RandomUnilateral(&H->Series) < .5f) ? 1.f : -1.f);
    }
    v3 Trough = HeroOn(H, X - HX + .12f, Z - HZ + .08f);
    R3PushBox(H->Opaque, R3Add(Trough, V3(0.f, .03f, 0.f)), V3(.08f, .03f, .03f), R3Color(Hero_ColorWoodDark));
   }
   if(Cows >= 3)
   {
    f32 BX = .38f, BZ = .34f;
    if(HeroFindSpot(H, 0, &BX, &BZ, -1.9f, 1.5f, &X, &Z)) HeroPushHouse(H, X, Z, BX - .08f, BZ - .1f, 1, 8);
   }
  } break;

  case ProductKind_Eggs:
  {
   u32 Hens = Max(2, (u32)(Amount/12));
   u32 Yards = (Hens > 8) ? 2 : 1;
   for(u32 Yard = 0; Yard < Yards; Yard += 1)
   {
    u32 YardHens = Hens/Yards + ((Yard == 0) ? Hens % Yards : 0);
    f32 HX = .25f + .025f*(f32)YardHens;
    f32 HZ = .22f + .012f*(f32)YardHens;
    f32 ClaimHX = HX + .04f, ClaimHZ = HZ + .04f;
    if(HeroFindSpot(H, (Yard == 0) ? Spot : 0, &ClaimHX, &ClaimHZ, -1.8f, 1.9f, &X, &Z))
    {
     HX = ClaimHX - .04f;
     HZ = ClaimHZ - .04f;
     HeroPushFence(H, X, Z, HX, HZ);
     W3PushHut(H->Opaque, HeroOn(H, X - .45f*HX, Z - .3f*HZ), V3(.14f, .1f, .11f), 0xFFE9C98F);
     for(u32 Hen = 0; Hen < YardHens; Hen += 1)
     {
      f32 PX = X + (HX - .07f)*RandomBilateral(&H->Series);
      f32 PZ = Z + .2f*HZ + (.7f*HZ)*RandomBilateral(&H->Series);
      HeroPushHen(H, HeroOn(H, PX, PZ), (Hen & 1) ? 1.f : -1.f);
     }
    }
   }
  } break;

  default: break;
 }
}

// NOTE: An island's products, largest amount first (at most 4).
internal u32
HeroSortedProducts(island *Island, product **Out, u32 MaxCount)
{
 u32 Count = 0;
 for(product *Product = Island->FirstProduct; Product && Count < MaxCount; Product = Product->Next)
 {
  u32 At = Count;
  while(At > 0 && Out[At - 1]->Amount < Product->Amount)
  {
   Out[At] = Out[At - 1];
   At -= 1;
  }
  Out[At] = Product;
  Count += 1;
 }
 return Count;
}

internal u64
HeroAmountOf(island *Island, product_kind Kind)
{
 u64 Result = 0;
 for(product *Product = Island->FirstProduct; Product; Product = Product->Next)
 {
  if(Product->Kind == Kind) Result += Product->Amount;
 }
 return Result;
}

//~ Shared by all hand-built islands
// NOTE: The knowledge crystal: grey -> cyan as the island learns about more islands (a
// moving part: its colour changes as news arrives).
internal void
HeroPushCrystal(hero_ctx *H, v3 Center, f32 Size)
{
 if(HeroDefer(H, HeroCmd_Crystal, Center, Size, 0.f, 0.f)) return;
 f32 Known = (H->Island < R3_MaxIslands) ? H->R3->IslandKnown[H->Island] : 0.f;
 v4 Color = R3LerpColor(R3Color(W3_ColorCrystalLow), R3Color(W3_ColorCrystalHigh), Known);
 R3PushCone(H->Opaque, Center, R3Add(Center, V3(0.f, 1.3f*Size, 0.f)), Size, 4, Color);
 R3PushCone(H->Opaque, Center, R3Add(Center, V3(0.f, -1.3f*Size, 0.f)), Size, 4, Color);
}

// NOTE: The status ring round the island and, when hovered or selected, a second ring
// outside it (a moving part). Radius 0 = the square slab.
internal void
HeroPushRings(hero_ctx *H, app_state *App, f32 Radius)
{
 if(HeroDefer(H, HeroCmd_Ring, H->Top, Radius, 0.f, 0.f)) return;
 r3_state *R3 = &App->R3;
 b32 Hovered = (R3->HoveredIdx == (s32)H->Island);
 b32 Selected = (App->Panel.SelectedPlusOne == (s32)H->Island + 1);
 v4 Status = R3Color(W3StatusColor(R3->IslandStatus[H->Island]));
 v4 Highlight = R3Color(Hovered ? W3_ColorHover : W3_ColorSelected);
 if(Radius > 0.f)
 {
  W3PushRing(H->Opaque, H->Top, Radius + .12f, Radius + .3f, Status);
  if(Hovered || Selected) W3PushRing(H->Opaque, H->Top, Radius + .36f, Radius + .48f, Highlight);
 }
 else
 {
  HeroPushSquareRing(H, Hero_Half + .12f, Hero_Half + .3f, Status);
  if(Hovered || Selected) HeroPushSquareRing(H, Hero_Half + .36f, Hero_Half + .48f, Highlight);
 }
}

internal void FishingPushTrawlerAt(hero_ctx *H, v3 Base, f32 Facing, f32 Phase);   // solarpunk_fishing_island.c

// NOTE: Draws the moving parts of all hand-built islands for this frame.
internal void
HeroReplayCommands(app_state *App)
{
 r3_state *R3 = &App->R3;
 hero_ctx Ctx;
 hero_ctx *H = &Ctx;
 MemorySet(H, 0, sizeof(*H));
 H->Opaque = &R3->Opaque;
 H->Translucent = &R3->Translucent;
 H->Time = R3->AnimTime;
 H->R3 = R3;
 RandomSeed(&H->Series, W3_Seed);
 for(u32 Idx = 0; Idx < R3->HeroCommandCount; Idx += 1)
 {
  hero_command *Command = &R3->HeroCommands[Idx];
  H->Island = Command->Island;
  H->Top = Command->P;
  switch(Command->Kind)
  {
   case HeroCmd_Rotor:      HeroPushRotor(H, Command->P, Command->A); break;
   case HeroCmd_Sails:      HeroPushSails(H, Command->P, Command->A); break;
   case HeroCmd_WaterWheel: HeroPushWaterWheel(H, Command->P, Command->A); break;
   case HeroCmd_Boat:       HeroPushBoatAt(H, Command->P, Command->A, Command->B, Command->C > .5f); break;
   case HeroCmd_Flames:     HeroPushFlames(H, Command->P); break;
   case HeroCmd_Crystal:    HeroPushCrystal(H, Command->P, Command->A); break;
   case HeroCmd_Ring:       HeroPushRings(H, App, Command->A); break;
   case HeroCmd_Trawler:    FishingPushTrawlerAt(H, Command->P, Command->A, Command->B); break;
   default: break;
  }
 }
}
// NOTE: Rocky crest with the knowledge crystal on a mast (grey -> cyan as the island
// learns about more islands).
internal void
HeroPushCrest(hero_ctx *H, f32 X, f32 Z, f32 KnownFraction)
{
 v3 Crest = HeroOn(H, X, Z);
 HeroPushBoulder(H, R3Add(Crest, V3(0.f, .2f, 0.f)), .32f);
 HeroPushBoulder(H, R3Add(Crest, V3(-.28f, .12f, .1f)), .2f);
 HeroPushBoulder(H, R3Add(Crest, V3(.22f, .1f, -.15f)), .22f);
 HeroPushBoulder(H, R3Add(Crest, V3(.05f, .4f, -.05f)), .18f);
 HeroMarkRect(H, X - .4f, Z - .35f, X + .35f, Z + .3f);
 v3 MastBase = R3Add(Crest, V3(.05f, .5f, -.05f));
 v3 Crystal = R3Add(MastBase, V3(0.f, .65f, 0.f));
 R3PushCylinder(H->Opaque, MastBase, Crystal, .025f, 5, R3Color(W3_ColorPost));
 HeroPushCrystal(H, Crystal, .2f);
}

internal void
HeroBegin(hero_ctx *H, app_state *App, hero_layout *Layout, v3 Top, u64 Seed, u32 Index)
{
 r3_state *R3 = &App->R3;
 MemorySet(H, 0, sizeof(*H));
 H->Opaque = R3->HeroOpaque ? R3->HeroOpaque : &R3->Opaque;
 H->Translucent = R3->HeroTranslucent ? R3->HeroTranslucent : &R3->Translucent;
 H->Top = Top;
 H->Time = R3->AnimTime;
 H->Layout = Layout;
 H->R3 = R3;
 H->Island = Index;
 H->Recording = R3->HeroRecording;
 RandomSeed(&H->Series, Seed);
}

// NOTE: Bushes on free grass, clouds around the base and the status rings.
internal void
HeroFinish(hero_ctx *H, app_state *App, u32 StatusColor, b32 Highlight, u32 HighlightColor)
{
 for(u32 Bush = 0; Bush < 70; Bush += 1)
 {
  f32 X = (Hero_Half - .1f)*RandomBilateral(&H->Series);
  f32 Z = (Hero_Half - .1f)*RandomBilateral(&H->Series);
  u32 Type;
  f32 Y = HeroGroundY(H->Layout, X, Z, &Type);
  b32 OnIsland = !H->Layout->Round || (X*X + Z*Z < (H->Layout->Radius - .1f)*(H->Layout->Radius - .1f));
  if(Type == HeroGround_Grass && !HeroCellTaken(H, X, Z) && OnIsland)
  {
   f32 Size = .05f + .06f*RandomUnilateral(&H->Series);
   u32 BushColor = H->Layout->Snowy ? 0xFFF2F6FA : ((Bush & 1) ? Hero_ColorLeaf : Hero_ColorVine);   // snow drifts on snowy islands
   R3PushEllipsoid(H->Opaque, HeroP(H, X, Y + .6f*Size, Z), V3(Size*1.2f, Size, Size*1.2f), 3, 6, HeroShade(H, BushColor), false);
   if((Bush % 5) == 0 && !H->Layout->Snowy)
   {
    R3PushEllipsoid(H->Opaque, HeroP(H, X + .03f, Y + 1.3f*Size, Z + .02f), V3(.018f, .018f, .018f), 3, 4, R3Color((Bush & 2) ? 0xFFF2D04E : 0xFFE87FA6), false);
   }
  }
 }

 for(u32 Puff = 0; Puff < 9; Puff += 1)
 {
  f32 A = 2.f*Pi32*((f32)Puff + .5f*RandomUnilateral(&H->Series))/9.f;
  f32 Out = Hero_Half*(.85f + .35f*RandomUnilateral(&H->Series));
  v3 P = HeroP(H, Out*R3Cos(A), -2.3f - 1.f*RandomUnilateral(&H->Series), Out*R3Sin(A));
  v3 Radii = V3(.8f + .6f*RandomUnilateral(&H->Series), .35f + .2f*RandomUnilateral(&H->Series), .7f + .5f*RandomUnilateral(&H->Series));
  R3PushEllipsoid(H->Translucent, P, Radii, 5, 9, R3WithAlpha(R3Color(Hero_ColorCloud), .5f), true);
 }

 HeroPushRings(H, App, H->Layout->Round ? H->Layout->Radius : 0.f);
}

//~ The hero island
internal void
W3PushHeroIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction)
{
 hero_layout Layout = {0};
 Layout.Pool = true;
 Layout.PoolX0 = Hero_PoolX0;
 Layout.PoolX1 = Hero_PoolX1;
 Layout.PoolZ0 = Hero_PoolZ0;
 Layout.FrontLevel = Hero_FrontLevel;
 Layout.HillX = -.15f;
 Layout.HillZ = -1.45f;
 Layout.HillStretch = 1.3f;
 Layout.HillPeak = 2.f;
 Layout.PlateauRight = true;
 Layout.PlateauLeft = true;
 Layout.StreamCount = ArrayCount(HeroStreamPoints);
 for(u32 Idx = 0; Idx < ArrayCount(HeroStreamPoints); Idx += 1) { Layout.Stream[Idx][0] = HeroStreamPoints[Idx][0]; Layout.Stream[Idx][1] = HeroStreamPoints[Idx][1]; }
 Layout.PathCount = ArrayCount(HeroPathPoints);
 for(u32 Idx = 0; Idx < ArrayCount(HeroPathPoints); Idx += 1) { Layout.Path[Idx][0] = HeroPathPoints[Idx][0]; Layout.Path[Idx][1] = HeroPathPoints[Idx][1]; }
 Layout.Path2Count = ArrayCount(HeroPath2Points);
 for(u32 Idx = 0; Idx < ArrayCount(HeroPath2Points); Idx += 1) { Layout.Path2[Idx][0] = HeroPath2Points[Idx][0]; Layout.Path2[Idx][1] = HeroPath2Points[Idx][1]; }

 hero_ctx Ctx;
 hero_ctx *H = &Ctx;
 HeroBegin(H, App, &Layout, Top, W3_Seed*17 + 389*(u64)Index, Index);
 for(u32 Idx = 0; Idx < ArrayCount(HeroTakenRects); Idx += 1)
 {
  HeroMarkRect(H, HeroTakenRects[Idx][0], HeroTakenRects[Idx][1], HeroTakenRects[Idx][2], HeroTakenRects[Idx][3]);
 }

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

 //- Greenhouses and water tower
 HeroPushGreenhouse(H, -1.95f, .55f, .26f, .21f);
 HeroPushGreenhouse(H, 2.f, .45f, .22f, .25f);
 HeroPushWaterTower(H, 1.05f, -1.15f);

 //- What the island makes, sized by the amounts: the biggest on the right plateau,
 // the next in the middle of the village, any others wherever there is room.
 {
  product *Products[4];
  u32 ProductCount = HeroSortedProducts(Island, Products, ArrayCount(Products));
  for(u32 Idx = 0; Idx < ProductCount; Idx += 1)
  {
   hero_spot Spot = {0};
   if(Idx == 0) { Spot.HasPrefer = true; Spot.PreferX = 1.4f; Spot.PreferZ = .33f; }
   if(Idx == 1) { Spot.HasPrefer = true; Spot.PreferX = .1f;  Spot.PreferZ = .5f; }
   HeroPushProduct(H, Products[Idx]->Kind, Products[Idx]->Amount, &Spot);
  }
 }

 //- Windmills along the back, pines on the sides
 HeroPushWindmill(H, -2.f, -1.95f, 1.45f, 0.f);
 HeroPushWindmill(H, -1.45f, -2.08f, 1.65f, 1.3f);
 HeroPushWindmill(H, -.75f, -2.1f, 1.3f, 2.1f);
 HeroPushWindmill(H, 2.05f, -2.1f, 1.55f, .7f);
 HeroPushWindmill(H, 2.12f, -1.2f, 1.35f, 2.8f);
 HeroPushPine(H, -2.12f, .05f, 1.25f);
 HeroPushPine(H, 2.12f, -.05f, 1.15f);
 HeroPushPine(H, -.95f, -1.95f, .8f);

 //- Rocky crest at the top of the hill, with the knowledge crystal
 HeroPushCrest(H, .3f, -1.9f, KnownFraction);
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
 HeroPushCrates(H, Island, -.78f, 1.2f, 1.62f, 2.02f);

 //- Village life
 HeroPushCampfire(H, -.8f, .55f);
 HeroPushLaundry(H, -1.f, -.5f, -.05f);
 HeroPushBarrel(H, 1.25f, -.12f);
 HeroPushBarrel(H, 1.36f, -.08f);
 HeroPushBarrel(H, -.2f, -.3f);
 HeroPushBarrel(H, 2.05f, 1.75f);
 HeroPushBarrel(H, -1.45f, 1.95f);
 {
  f32 People[][2] =
  {
   {-.92f, .64f}, {-.66f, .44f}, {-1.45f, 1.3f}, {-.9f, 1.45f}, {1.75f, 1.12f},
   {-.65f, -.25f}, {1.95f, 1.85f}, {-1.75f, 1.9f},
  };
  for(u32 Person = 0; Person < ArrayCount(People); Person += 1)
  {
   HeroPushPerson(H, HeroOn(H, People[Person][0], People[Person][1]), HeroShirtColors[Person % ArrayCount(HeroShirtColors)]);
  }
  HeroPushPerson(H, HeroP(H, .05f, Hero_WaterLevel + .105f, 1.85f), HeroShirtColors[2]);
  HeroPushPerson(H, HeroP(H, 1.f, Hero_WaterLevel + .105f, 1.45f), HeroShirtColors[4]);
 }

 HeroFinish(H, App, StatusColor, Highlight, HighlightColor);
}

//~ Themed islands: the hero island's neighbours, built the same way, with contents
// that follow what each island makes (orchards, market gardens, wheat fields and
// mills, fishing harbours, pastures, hen yards) and a layout of their own.
internal void
W3PushThemedIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction)
{
 hero_layout Layout = {0};
 hero_ctx Ctx;
 hero_ctx *H = &Ctx;
 HeroBegin(H, App, &Layout, Top, W3_Seed*23 + 601*(u64)Index, Index);
 random_series *S = &H->Series;

 //- What it makes, largest amount first
 product *Products[4];
 u32 ProductCount = HeroSortedProducts(Island, Products, ArrayCount(Products));
 u64 FishAmount = HeroAmountOf(Island, ProductKind_Fish);

 //- Layout: a harbour only when the island catches fish, bigger for a bigger catch
 Layout.FrontLevel = Hero_FrontLevel;
 Layout.Pool = (FishAmount > 0);
 if(Layout.Pool)
 {
  f32 Width = Min(3.4f, 1.3f + (f32)FishAmount/80.f);
  f32 Center = (Hero_Half - .5f*Width - .25f)*RandomBilateral(S);
  Layout.PoolX0 = Center - .5f*Width;
  Layout.PoolX1 = Center + .5f*Width;
  Layout.PoolZ0 = ((FishAmount >= 100) ? .6f : .85f) + .4f*RandomUnilateral(S);
 }
 Layout.HillX = 1.1f*RandomBilateral(S);
 Layout.HillZ = -1.75f + .6f*RandomUnilateral(S);
 Layout.HillStretch = 1.f + .5f*RandomUnilateral(S);
 Layout.HillPeak = 1.3f + .8f*RandomUnilateral(S);
 Layout.PlateauRight = (RandomUnilateral(S) < .35f);
 Layout.PlateauLeft = !Layout.PlateauRight && (RandomUnilateral(S) < .35f);

 f32 StreamEndX = 0.f;
 if(Layout.Pool && RandomUnilateral(S) < .7f)
 {
  StreamEndX = Clamp(Layout.PoolX0 + .25f, .5f*(Layout.PoolX0 + Layout.PoolX1) + .3f*RandomBilateral(S), Layout.PoolX1 - .45f);
  f32 SX = Layout.HillX + .3f;
  f32 SZ = Layout.HillZ - .05f;
  f32 EZ = Layout.PoolZ0 + .1f;
  Layout.Stream[0][0] = SX;
  Layout.Stream[0][1] = SZ;
  Layout.Stream[1][0] = SX + .35f*(StreamEndX - SX) + .2f*RandomBilateral(S);
  Layout.Stream[1][1] = SZ + .35f*(EZ - SZ);
  Layout.Stream[2][0] = SX + .7f*(StreamEndX - SX) + .2f*RandomBilateral(S);
  Layout.Stream[2][1] = SZ + .7f*(EZ - SZ);
  Layout.Stream[3][0] = StreamEndX;
  Layout.Stream[3][1] = EZ;
  Layout.StreamCount = 4;
 }

 // A dirt path from the front up towards the hill.
 {
  f32 PX = .8f*RandomBilateral(S);
  if(Layout.Pool) PX = (Layout.PoolX0 > -1.5f) ? Layout.PoolX0 - .3f : Layout.PoolX1 + .3f;
  Layout.Path[0][0] = PX;
  Layout.Path[0][1] = Hero_Half - .05f;
  Layout.Path[1][0] = .6f*PX;
  Layout.Path[1][1] = .3f;
  Layout.Path[2][0] = .5f*Layout.HillX;
  Layout.Path[2][1] = -.5f;
  Layout.PathCount = 3;
 }

 HeroPushTerrain(H);
 HeroPushCliff(H);

 //- Crest with the knowledge crystal on the hill
 HeroPushCrest(H, Layout.HillX - .2f, Layout.HillZ, KnownFraction);

 //- Everything it makes, sized by the amounts (these need the most room, so they go first)
 for(u32 Idx = 0; Idx < ProductCount; Idx += 1)
 {
  HeroPushProduct(H, Products[Idx]->Kind, Products[Idx]->Amount, 0);
 }

 //- Houses
 f32 X, Z;
 u32 Houses = 3 + RandomNext(S) % 3;
 for(u32 House = 0; House < Houses; House += 1)
 {
  f32 HX = .22f + .12f*RandomUnilateral(S);
  f32 HZ = .18f + .08f*RandomUnilateral(S);
  if(HeroPlace(H, HX + .08f, HZ + .14f, -2.f, 1.7f, &X, &Z))
  {
   HeroPushHouse(H, X, Z - .04f, HX, HZ, 1 + RandomNext(S) % 2, RandomNext(S) % 8);
  }
 }

 //- Windmills along the back, a water tower, pines
 u32 Mills = 2 + RandomNext(S) % 3;
 for(u32 Mill = 0; Mill < Mills; Mill += 1)
 {
  if(HeroPlace(H, .17f, .17f, -2.1f, -1.f, &X, &Z)) HeroPushWindmill(H, X, Z, 1.2f + .5f*RandomUnilateral(S), 6.f*RandomUnilateral(S));
 }
 if(RandomUnilateral(S) < .5f && HeroPlace(H, .24f, .24f, -1.9f, .6f, &X, &Z)) HeroPushWaterTower(H, X, Z);
 u32 Pines = 2 + RandomNext(S) % 3;
 for(u32 Pine = 0; Pine < Pines; Pine += 1)
 {
  if(HeroPlace(H, .2f, .2f, -2.f, 2.f, &X, &Z)) HeroPushPine(H, X, Z, .9f + .5f*RandomUnilateral(S));
 }

 //- Harbour
 if(Layout.Pool)
 {
  f32 PoolWidth = Layout.PoolX1 - Layout.PoolX0;
  f32 DockZ = .5f*(Layout.PoolZ0 + Hero_Half) + .1f;
  HeroPushDock(H, Layout.PoolX0, Layout.PoolX0 + Min(.8f, .4f*PoolWidth), DockZ, .11f);
  if(PoolWidth > 1.6f) HeroPushDock(H, Layout.PoolX1, Layout.PoolX1 - Min(.8f, .35f*PoolWidth), DockZ - .3f, .11f);
  u32 Boats = Min(5, 1 + (u32)(FishAmount/40));
  for(u32 Boat = 0; Boat < Boats; Boat += 1)
  {
   f32 BX = Layout.PoolX0 + .3f + (PoolWidth - .6f)*((f32)Boat + .5f)/(f32)Boats;
   f32 BZ = Layout.PoolZ0 + .3f + (Hero_Half - Layout.PoolZ0 - .5f)*RandomUnilateral(S);
   HeroPushBoat(H, BX, BZ, (Boat & 1) ? 1.f : -1.f, 6.f*RandomUnilateral(S), (Boat % 2) == 0);
  }
  if(Layout.StreamCount)
  {
   R3PushEllipsoid(H->Translucent, HeroP(H, StreamEndX, Hero_WaterLevel + .02f, Layout.PoolZ0 + .12f), V3(.3f, .08f, .18f), 4, 8, R3WithAlpha(R3Color(0xFFFFFFFF), .5f), true);
   f32 WheelX = StreamEndX + .32f;
   if(WheelX < Layout.PoolX1 - .1f)
   {
    HeroPushWaterWheel(H, HeroP(H, WheelX, Hero_WaterLevel + .26f, Layout.PoolZ0 + .05f), .28f);
   }
  }
 }

 //- Market stall, campfire, crates
 u32 Goods[3];
 u32 GoodsCount = 0;
 for(product *Product = Island->FirstProduct; Product && GoodsCount < ArrayCount(Goods); Product = Product->Next)
 {
  Goods[GoodsCount++] = W3ProductColors[Product->Kind];
 }
 if(HeroPlace(H, .42f, .32f, -.2f, 2.f, &X, &Z)) HeroPushStall(H, X, Z - .02f, Goods, GoodsCount, RandomUnilateral(S) < .4f);
 if(RandomUnilateral(S) < .6f && HeroPlace(H, .15f, .15f, -1.f, 1.8f, &X, &Z)) HeroPushCampfire(H, X, Z);
 {
  f32 PX = 0.f, PZ = 0.f, NX = 0.f, NZ = 0.f;
  b32 HaveProduce = HeroPlace(H, .13f, .4f, -1.2f, 2.f, &PX, &PZ);
  b32 HaveNeeds = HeroPlace(H, .4f, .13f, -1.2f, 2.f, &NX, &NZ);
  if(HaveProduce && HaveNeeds) HeroPushCrates(H, Island, PX, PZ - .26f, NX - .26f, NZ);
 }

 //- Barrels and villagers
 for(u32 Barrel = 0; Barrel < 3; Barrel += 1)
 {
  if(HeroPlace(H, .07f, .07f, -1.8f, 2.f, &X, &Z)) HeroPushBarrel(H, X, Z);
 }
 u32 People = 6 + RandomNext(S) % 4;
 for(u32 Person = 0; Person < People; Person += 1)
 {
  for(u32 Try = 0; Try < 20; Try += 1)
  {
   f32 PX = (Hero_Half - .2f)*RandomBilateral(S);
   f32 PZ = (Hero_Half - .2f)*RandomBilateral(S);
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

 HeroFinish(H, App, StatusColor, Highlight, HighlightColor);
}
