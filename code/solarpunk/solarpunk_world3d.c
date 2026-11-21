//-
// SolarPunk 3D world: floating sky islands, product crates, cables and news pulses.
//-

//~ Constants
#define W3_WorldSize        80.f   // map 0..1 is spread over this many world units
#define W3_IslandRadius     2.2f
#define W3_IslandHeightVary 1.4f   // islands float at slightly different heights
#define W3_IslandSides      20
#define W3_Seed             7919

#define W3_CableRadius      .07f
#define W3_CableSegments    16
#define W3_CableSag         .06f   // sag as a fraction of the cable length
#define W3_LabelHeight      3.4f   // label anchor above the island top

#define W3_CrateHalf        .24f
#define W3_CrateUnitsPerBox 70     // one stacked crate per this many units produced
#define W3_MaxCrateStack    3

#define W3_CloudCount       40

//- Colours
#define W3_ColorGrassTop    0xFF7CB46B
#define W3_ColorGrassLip    0xFF5E9452
#define W3_ColorRockTop     0xFF9C8466
#define W3_ColorRockMid     0xFF7E6A55
#define W3_ColorRockBottom  0xFF5F5045
#define W3_ColorTrunk       0xFF7A5230
#define W3_ColorLeaves      0xFF3E8E4E
#define W3_ColorLeavesLight 0xFF5BAA5F
#define W3_ColorPanel       0xFF24406E
#define W3_ColorPost        0xFFB0B7C3
#define W3_ColorProducePad  0xFF8FD18C
#define W3_ColorNeedPad     0xFFF2A65A
#define W3_ColorCable       0xFF3B4252
#define W3_ColorCableLit    0xFFEBCB8B
#define W3_ColorCableHover  0xFFFFE08A
#define W3_ColorPulse       0xFFFFF3B0
#define W3_ColorCrystalLow  0xFF8892A0
#define W3_ColorCrystalHigh 0xFF7FE3F0
#define W3_ColorHover       0xFFFFE08A
#define W3_ColorCloud       0xFFF7FAFC

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

//~ Island pieces
internal void
W3PushIslandBody(r3_mesh *Mesh, v3 Top, f32 Radius, u32 Index, b32 Hovered)
{
 random_series Series = {0};
 RandomSeed(&Series, W3_Seed*7 + 101*(u64)Index);

 f32 Noise[W3_IslandSides];
 for(u32 Side = 0; Side < W3_IslandSides; Side += 1)
 {
  Noise[Side] = .88f + .22f*RandomUnilateral(&Series);
 }

 // Rings from the grass top down to the rocky tip: radius factor and depth below the top.
 f32 RingRadius[4] = {1.f, 1.03f, .86f, .52f};
 f32 RingDepth[4]  = {0.f, .35f, 1.3f, 2.6f};
 u32 RingColor[4]  = {W3_ColorGrassTop, W3_ColorGrassLip, W3_ColorRockTop, W3_ColorRockMid};

 v3 Ring[4][W3_IslandSides];
 for(u32 RingIdx = 0; RingIdx < 4; RingIdx += 1)
 {
  for(u32 Side = 0; Side < W3_IslandSides; Side += 1)
  {
   f32 Angle = 2.f*Pi32*(f32)Side/(f32)W3_IslandSides;
   f32 R = Radius*Noise[Side]*RingRadius[RingIdx];
   Ring[RingIdx][Side] = V3(Top.X + R*R3Cos(Angle), Top.Y - RingDepth[RingIdx], Top.Z + R*R3Sin(Angle));
  }
 }

 v3 Tip = V3(Top.X + .3f*RandomBilateral(&Series),
             Top.Y - 4.2f - .8f*RandomUnilateral(&Series),
             Top.Z + .3f*RandomBilateral(&Series));

 for(u32 Side = 0; Side < W3_IslandSides; Side += 1)
 {
  u32 Next = (Side + 1) % W3_IslandSides;

  // Grass top
  R3PushTri(Mesh, Top, Ring[0][Side], Ring[0][Next], V3(Top.X, Top.Y - 2.f, Top.Z),
            W3Shade(R3Color(W3_ColorGrassTop), &Series));

  // Sides
  for(u32 RingIdx = 0; RingIdx < 3; RingIdx += 1)
  {
   f32 MidY = Top.Y - .5f*(RingDepth[RingIdx] + RingDepth[RingIdx + 1]);
   R3PushQuad(Mesh, Ring[RingIdx][Side], Ring[RingIdx][Next], Ring[RingIdx + 1][Next], Ring[RingIdx + 1][Side],
              V3(Top.X, MidY, Top.Z), W3Shade(R3Color(RingColor[RingIdx + 1]), &Series));
  }

  // Rocky tip
  R3PushTri(Mesh, Ring[3][Side], Ring[3][Next], Tip, V3(Top.X, Top.Y - 2.9f, Top.Z),
            W3Shade(R3Color(W3_ColorRockBottom), &Series));
 }

 // Hovered: a glowing ring around the rim.
 if(Hovered)
 {
  v4 Glow = R3Color(W3_ColorHover);
  for(u32 Side = 0; Side < W3_IslandSides; Side += 1)
  {
   u32 Next = (Side + 1) % W3_IslandSides;
   f32 Angle0 = 2.f*Pi32*(f32)Side/(f32)W3_IslandSides;
   f32 Angle1 = 2.f*Pi32*(f32)Next/(f32)W3_IslandSides;
   f32 Inner = 1.07f*Radius;
   f32 Outer = 1.22f*Radius;
   f32 Y = Top.Y + .05f;
   v3 P0 = V3(Top.X + Inner*R3Cos(Angle0), Y, Top.Z + Inner*R3Sin(Angle0));
   v3 P1 = V3(Top.X + Outer*R3Cos(Angle0), Y, Top.Z + Outer*R3Sin(Angle0));
   v3 P2 = V3(Top.X + Outer*R3Cos(Angle1), Y, Top.Z + Outer*R3Sin(Angle1));
   v3 P3 = V3(Top.X + Inner*R3Cos(Angle1), Y, Top.Z + Inner*R3Sin(Angle1));
   R3PushTriUnlit(Mesh, P0, P1, P2, Glow);
   R3PushTriUnlit(Mesh, P0, P2, P3, Glow);
  }
 }
}

// NOTE: Trees and a solar panel on the back half of the island (the front holds the crates).
internal void
W3PushDecorations(r3_mesh *Mesh, v3 Top, f32 Radius, u32 Index)
{
 random_series Series = {0};
 RandomSeed(&Series, W3_Seed*13 + 977*(u64)Index);

 u32 TreeCount = 2 + RandomNext(&Series) % 3;
 for(u32 Tree = 0; Tree < TreeCount; Tree += 1)
 {
  f32 Angle = Pi32 + .3f + (Pi32 - .6f)*RandomUnilateral(&Series);
  f32 Distance = Radius*(.45f + .35f*RandomUnilateral(&Series));
  f32 Scale = .8f + .5f*RandomUnilateral(&Series);
  v3 Base = V3(Top.X + Distance*R3Cos(Angle), Top.Y, Top.Z + Distance*R3Sin(Angle));

  R3PushCylinder(Mesh, Base, R3Add(Base, V3(0.f, .6f*Scale, 0.f)), .1f*Scale, 6, R3Color(W3_ColorTrunk));
  R3PushCone(Mesh, R3Add(Base, V3(0.f, .45f*Scale, 0.f)), R3Add(Base, V3(0.f, 1.5f*Scale, 0.f)),
             .55f*Scale, 7, W3Shade(R3Color(W3_ColorLeaves), &Series));
  R3PushCone(Mesh, R3Add(Base, V3(0.f, 1.05f*Scale, 0.f)), R3Add(Base, V3(0.f, 2.f*Scale, 0.f)),
             .4f*Scale, 7, W3Shade(R3Color(W3_ColorLeavesLight), &Series));
 }

 // Solar panel on a post, tilted towards the sun.
 {
  f32 Angle = 1.5f*Pi32 + .4f*RandomBilateral(&Series);
  f32 Distance = .55f*Radius;
  v3 Base = V3(Top.X + Distance*R3Cos(Angle), Top.Y, Top.Z + Distance*R3Sin(Angle));
  v3 PanelCenter = R3Add(Base, V3(0.f, .7f, 0.f));
  R3PushCylinder(Mesh, Base, PanelCenter, .05f, 5, R3Color(W3_ColorPost));
  R3PushOrientedBox(Mesh, PanelCenter,
                    V3(.5f, 0.f, 0.f),
                    V3(0.f, .028f, -.016f),
                    V3(0.f, .16f, .28f),
                    R3Color(W3_ColorPanel));
 }
}

// NOTE: Green pad with solid crates = what the island produces.
//       Orange pad with empty crate frames = what the island needs.
internal void
W3PushProductsAndNeeds(r3_mesh *Opaque, r3_mesh *Translucent, island *Island, v3 Top)
{
 v3 ProducePad = R3Add(Top, V3(-.75f, .04f, .55f));
 v3 NeedPad = R3Add(Top, V3(.75f, .04f, .55f));
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
 v3 Base = R3Add(Top, V3(0.f, 0.f, -.35f));
 v3 Center = R3Add(Base, V3(0.f, 2.3f, 0.f));
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
  R3->IslandVisible[Index] = R3ProjectToScreen(R3, R3Add(Top, V3(0.f, W3_LabelHeight, -.35f)), &Screen);
  R3->IslandScreen[Index] = Screen;

  // NOTE: The hover tooltip (DrawIslandTooltip) looks at Island->Pos.
  Island->Pos = (R3->IslandVisible[Index] ? Screen : V2(-1000.f, -1000.f));

  Index += 1;
 }
 R3->IslandCount = Index;

 //- Hovered island: closest label anchor or island top to the mouse
 R3->HoveredIdx = -1;
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

  W3PushIslandBody(&R3->Opaque, Top, Radius, Index, (R3->HoveredIdx == (s32)Index));
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
