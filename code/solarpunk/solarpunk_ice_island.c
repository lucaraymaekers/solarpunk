//-
// SolarPunk 3D frozen islands (Saltglass Atolls): a square slab locked in snow and ice,
// wrapped in wooden scaffolding with deck ledges and hanging chains, icicles everywhere
// and a frozen waterfall spilling from an iced-over pool at the front. A working yard:
// cranes on the edges lifting cargo over the drop, tracked crane vehicles and trucks on
// a snowy plaza, a boiler with a gear wheel and a smoking chimney, pipes, grain silos and
// a docked airship; snow-roofed cabins with solar panels, snowy pines, windmills.
// What the island makes, sized by the amounts: fish -> ice-fishing holes and a crane
// hoisting a boat; carrots -> fields and greenhouses; wheat -> fields and grain silos.
//
// Built on the hand-built island toolkit in solarpunk_hero_island.c.
//-

//~ Constants
#define Ice_Cluster 6   // Saltglass Atolls, see WorldGenClusterFoods

#define Ice_ColorSnow      0xFFF2F6FA
#define Ice_ColorIce       0xFFD8F0FA
#define Ice_ColorFrostWood 0xFF9A9088
#define Ice_ColorRust      0xFF9A5A3A
#define Ice_ColorMetal     0xFF7C8690
#define Ice_ColorDarkMetal 0xFF4E565E
#define Ice_ColorYellow    0xFFD9A640

//~ Snow and ice
internal void
IcePushIcicles(hero_ctx *H, v3 A, v3 B, u32 Count, f32 MaxLength)
{
 for(u32 Icicle = 0; Icicle < Count; Icicle += 1)
 {
  f32 T = ((f32)Icicle + .5f + .4f*RandomBilateral(&H->Series))/(f32)Count;
  v3 P = R3Lerp(A, B, Clamp(0.f, T, 1.f));
  f32 Length = MaxLength*(.25f + .75f*RandomUnilateral(&H->Series));
  R3PushCone(H->Opaque, P, R3Sub(P, V3(0.f, Length, 0.f)), .012f + .014f*RandomUnilateral(&H->Series), 4, R3Color(Ice_ColorIce));
 }
}

internal void
IcePushSnowyPine(hero_ctx *H, f32 X, f32 Z, f32 Height)
{
 HeroPushPine(H, X, Z, Height);
 v3 B = HeroOn(H, X, Z);
 for(u32 Tier = 0; Tier < 5; Tier += 1)
 {
  f32 Y = Height*(.2f + .15f*(f32)Tier) + .12f*Height;
  f32 R = .24f*Height*(1.f - .17f*(f32)Tier)/1.4f;
  R3PushEllipsoid(H->Opaque, R3Add(B, V3(0.f, Y, 0.f)), V3(R, .035f, R), 3, 8, R3Color(Ice_ColorSnow), false);
 }
}

// NOTE: Smoke puffs rising from a chimney top.
internal void
IcePushSmoke(hero_ctx *H, v3 Top)
{
 for(u32 Puff = 0; Puff < 4; Puff += 1)
 {
  f32 T = (f32)Puff;
  v3 P = R3Add(Top, V3(.06f*T, .12f + .18f*T, .02f*T));
  f32 R = .06f + .035f*T;
  R3PushEllipsoid(H->Translucent, P, V3(R, .8f*R, R), 4, 7, R3WithAlpha(R3Color(0xFFE8E8EA), .55f - .1f*T), true);
 }
}

//~ Machines
// NOTE: A crane on the island's edge: a lattice tower, a cab, a jib reaching out over the
// drop and a load on its hook (0 = pallet of crates, 1 = boat, 2 = net of fish).
internal void
IcePushCrane(hero_ctx *H, f32 X, f32 Z, v3 Out, u32 Load)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 v4 Rust = HeroShade(H, Ice_ColorRust);
 R3PushBox(O, R3Add(B, V3(0.f, .05f, 0.f)), V3(.17f, .05f, .17f), R3Color(Ice_ColorDarkMetal));
 f32 Height = .95f;
 for(u32 Leg = 0; Leg < 4; Leg += 1)
 {
  f32 SX = (Leg & 1) ? 1.f : -1.f;
  f32 SZ = (Leg & 2) ? 1.f : -1.f;
  R3PushCylinder(O, R3Add(B, V3(.12f*SX, .1f, .12f*SZ)), R3Add(B, V3(.07f*SX, Height, .07f*SZ)), .014f, 4, Rust);
 }
 for(u32 Brace = 0; Brace < 3; Brace += 1)
 {
  f32 Y = .3f + .25f*(f32)Brace;
  R3PushCylinder(O, R3Add(B, V3(-.1f, Y, -.1f)), R3Add(B, V3(.1f, Y + .2f, .1f)), .008f, 3, Rust);
  R3PushCylinder(O, R3Add(B, V3(.1f, Y, -.1f)), R3Add(B, V3(-.1f, Y + .2f, .1f)), .008f, 3, Rust);
 }
 v3 Cab = R3Add(B, V3(0.f, Height + .08f, 0.f));
 R3PushBox(O, Cab, V3(.1f, .08f, .1f), HeroShade(H, Ice_ColorYellow));
 R3PushBox(O, R3Add(Cab, V3(0.f, .09f, 0.f)), V3(.11f, .015f, .11f), R3Color(Ice_ColorSnow));
 HeroGlowRect(O, R3Add(Cab, R3Add(R3Scale(Out, .101f), V3(0.f, .02f, 0.f))), R3Scale(V3(-Out.Z, 0.f, Out.X), .06f), V3(0.f, .03f, 0.f), R3Color(Hero_ColorWindow));

 //- Jib: two chords and zigzag braces, out over the edge
 v3 JibStart = R3Add(Cab, V3(0.f, .04f, 0.f));
 v3 JibEnd = R3Add(R3Add(JibStart, R3Scale(Out, 1.35f)), V3(0.f, .45f, 0.f));
 v3 Side = R3Scale(V3(-Out.Z, 0.f, Out.X), .04f);
 R3PushCylinder(O, R3Add(JibStart, Side), R3Add(JibEnd, Side), .01f, 4, Rust);
 R3PushCylinder(O, R3Sub(JibStart, Side), R3Sub(JibEnd, Side), .01f, 4, Rust);
 for(u32 Zig = 0; Zig < 6; Zig += 1)
 {
  v3 P0 = R3Lerp(JibStart, JibEnd, (f32)Zig/6.f);
  v3 P1 = R3Lerp(JibStart, JibEnd, ((f32)Zig + 1.f)/6.f);
  R3PushCylinder(O, R3Add(P0, (Zig & 1) ? Side : R3Scale(Side, -1.f)), R3Add(P1, (Zig & 1) ? R3Scale(Side, -1.f) : Side), .006f, 3, Rust);
 }
 R3PushCylinder(O, R3Add(Cab, V3(0.f, .3f, 0.f)), JibEnd, .004f, 3, R3Color(Ice_ColorDarkMetal));
 R3PushCylinder(O, R3Add(Cab, V3(0.f, .08f, 0.f)), R3Add(Cab, V3(0.f, .32f, 0.f)), .012f, 4, Rust);
 IcePushIcicles(H, R3Add(JibStart, V3(0.f, -.01f, 0.f)), R3Add(JibEnd, V3(0.f, -.01f, 0.f)), 8, .08f);

 //- Cable and load
 v3 Hook = R3Sub(JibEnd, V3(0.f, .85f, 0.f));
 R3PushCylinder(O, JibEnd, Hook, .004f, 3, R3Color(Ice_ColorDarkMetal));
 if(Load == 1)
 {
  R3PushCylinder(O, Hook, R3Add(Hook, V3(-.15f, -.1f, 0.f)), .003f, 3, R3Color(Ice_ColorDarkMetal));
  R3PushCylinder(O, Hook, R3Add(Hook, V3(.15f, -.1f, 0.f)), .003f, 3, R3Color(Ice_ColorDarkMetal));
  R3PushEllipsoid(O, R3Add(Hook, V3(0.f, -.13f, 0.f)), V3(.2f, .055f, .08f), 4, 9, R3Color(0xFF8A5A36), false);
  R3PushBox(O, R3Add(Hook, V3(0.f, -.1f, 0.f)), V3(.15f, .01f, .06f), R3Color(Ice_ColorSnow));
 }
 else if(Load == 2)
 {
  R3PushEllipsoid(O, R3Add(Hook, V3(0.f, -.1f, 0.f)), V3(.09f, .1f, .09f), 4, 7, R3Color(0xFF3E6B66), false);
  for(u32 Fish = 0; Fish < 4; Fish += 1)
  {
   f32 A = 1.6f*(f32)Fish;
   R3PushEllipsoid(O, R3Add(Hook, V3(.08f*R3Cos(A), -.1f + .03f*(f32)(Fish & 1), .08f*R3Sin(A))), V3(.03f, .012f, .014f), 3, 5, R3Color(W3ProductColors[ProductKind_Fish]), false);
  }
 }
 else
 {
  R3PushBox(O, R3Add(Hook, V3(0.f, -.13f, 0.f)), V3(.13f, .012f, .1f), R3Color(Hero_ColorWoodLight));
  R3PushBox(O, R3Add(Hook, V3(-.05f, -.08f, 0.f)), V3(.045f, .04f, .045f), HeroShade(H, Hero_ColorWoodLight));
  R3PushBox(O, R3Add(Hook, V3(.05f, -.08f, .02f)), V3(.045f, .04f, .045f), HeroShade(H, Hero_ColorWoodLight));
  R3PushBox(O, R3Add(Hook, V3(-.05f, -.03f, 0.f)), V3(.046f, .006f, .046f), R3Color(Ice_ColorSnow));
 }
}

// NOTE: A tracked vehicle: a yellow body on caterpillar tracks, a lit cab, and either a
// small crane arm or a dozer blade.
internal void
IcePushTracked(hero_ctx *H, f32 X, f32 Z, f32 Facing, b32 Arm)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 for(u32 Track = 0; Track < 2; Track += 1)
 {
  f32 TZ = (Track == 0) ? -.075f : .075f;
  R3PushBox(O, R3Add(B, V3(0.f, .035f, TZ)), V3(.16f, .035f, .025f), R3Color(0xFF2E3236));
 }
 R3PushBox(O, R3Add(B, V3(0.f, .1f, 0.f)), V3(.14f, .035f, .09f), HeroShade(H, Ice_ColorYellow));
 v3 Cab = R3Add(B, V3(-.04f*Facing, .17f, 0.f));
 R3PushBox(O, Cab, V3(.06f, .045f, .06f), HeroShade(H, Ice_ColorYellow));
 HeroGlowRect(O, R3Add(Cab, V3(.061f*Facing, .01f, 0.f)), V3(0.f, 0.f, .04f), V3(0.f, .025f, 0.f), R3Color(Hero_ColorWindow));
 R3PushBox(O, R3Add(Cab, V3(0.f, .05f, 0.f)), V3(.065f, .008f, .065f), R3Color(Ice_ColorSnow));
 if(Arm)
 {
  v3 Pivot = R3Add(B, V3(.06f*Facing, .14f, 0.f));
  v3 Elbow = R3Add(Pivot, V3(.15f*Facing, .25f, 0.f));
  v3 Tip = R3Add(Elbow, V3(.14f*Facing, -.12f, 0.f));
  R3PushCylinder(O, Pivot, Elbow, .016f, 4, HeroShade(H, Ice_ColorYellow));
  R3PushCylinder(O, Elbow, Tip, .012f, 4, HeroShade(H, Ice_ColorYellow));
  R3PushBox(O, R3Add(Tip, V3(0.f, -.04f, 0.f)), V3(.035f, .03f, .04f), R3Color(Ice_ColorDarkMetal));
 }
 else
 {
  R3PushBox(O, R3Add(B, V3(.19f*Facing, .06f, 0.f)), V3(.012f, .05f, .12f), R3Color(Ice_ColorDarkMetal));
 }
}

internal void
IcePushTruck(hero_ctx *H, f32 X, f32 Z, f32 Facing)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 for(u32 Wheel = 0; Wheel < 4; Wheel += 1)
 {
  f32 WX = (Wheel & 1) ? .11f : -.11f;
  f32 WZ = (Wheel & 2) ? .08f : -.08f;
  R3PushCylinder(O, R3Add(B, V3(WX, .045f, WZ - .015f)), R3Add(B, V3(WX, .045f, WZ + .015f)), .045f, 8, R3Color(0xFF2E3236));
 }
 R3PushBox(O, R3Add(B, V3(0.f, .09f, 0.f)), V3(.17f, .025f, .07f), R3Color(Ice_ColorDarkMetal));
 v3 Cab = R3Add(B, V3(.1f*Facing, .16f, 0.f));
 R3PushBox(O, Cab, V3(.06f, .05f, .07f), HeroShade(H, 0xFFB0463A));
 HeroGlowRect(O, R3Add(Cab, V3(.061f*Facing, .015f, 0.f)), V3(0.f, 0.f, .05f), V3(0.f, .022f, 0.f), R3Color(Hero_ColorWindow));
 R3PushBox(O, R3Add(B, V3(-.05f*Facing, .15f, 0.f)), V3(.1f, .035f, .07f), HeroShade(H, Ice_ColorFrostWood));
 R3PushBox(O, R3Add(B, V3(-.05f*Facing, .19f, 0.f)), V3(.1f, .008f, .07f), R3Color(Ice_ColorSnow));
}

// NOTE: A grain silo: a rusty tank with bands, a cone roof capped with snow, a ladder.
internal void
IcePushSilo(hero_ctx *H, f32 X, f32 Z, f32 Height)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 W3PushTerrace(O, R3Add(B, V3(0.f, Height, 0.f)), .16f, Height, 12, HeroShade(H, Ice_ColorRust), HeroShade(H, Ice_ColorRust));
 for(u32 Band = 1; Band < 4; Band += 1)
 {
  f32 Y = Height*(f32)Band/4.f;
  R3PushCylinder(O, R3Add(B, V3(0.f, Y, 0.f)), R3Add(B, V3(0.f, Y + .015f, 0.f)), .165f, 12, R3Color(0xFF6E4A30));
 }
 R3PushCone(O, R3Add(B, V3(0.f, Height, 0.f)), R3Add(B, V3(0.f, Height + .14f, 0.f)), .18f, 12, R3Color(Ice_ColorSnow));
 R3PushCylinder(O, R3Add(B, V3(.165f, 0.f, .03f)), R3Add(B, V3(.165f, Height, .03f)), .006f, 3, R3Color(Ice_ColorDarkMetal));
 R3PushCylinder(O, R3Add(B, V3(.165f, 0.f, -.03f)), R3Add(B, V3(.165f, Height, -.03f)), .006f, 3, R3Color(Ice_ColorDarkMetal));
 IcePushIcicles(H, R3Add(B, V3(-.15f, Height - .01f, .07f)), R3Add(B, V3(.15f, Height - .01f, .07f)), 5, .1f);
}

// NOTE: A boiler: a big horizontal drum with a gear wheel, a smoking chimney and pipes.
internal void
IcePushBoiler(hero_ctx *H, f32 X, f32 Z)
{
 r3_mesh *O = H->Opaque;
 v3 B = HeroOn(H, X, Z);
 v3 Drum = R3Add(B, V3(0.f, .24f, 0.f));
 R3PushBox(O, R3Add(B, V3(0.f, .04f, 0.f)), V3(.32f, .04f, .14f), R3Color(Ice_ColorDarkMetal));
 R3PushCylinder(O, R3Add(Drum, V3(-.3f, 0.f, 0.f)), R3Add(Drum, V3(.3f, 0.f, 0.f)), .2f, 14, HeroShade(H, Ice_ColorMetal));
 R3PushEllipsoid(O, R3Add(Drum, V3(-.3f, 0.f, 0.f)), V3(.05f, .2f, .2f), 4, 12, HeroShade(H, Ice_ColorMetal), false);
 R3PushEllipsoid(O, R3Add(Drum, V3(.3f, 0.f, 0.f)), V3(.05f, .2f, .2f), 4, 12, HeroShade(H, Ice_ColorMetal), false);
 R3PushEllipsoid(O, R3Add(Drum, V3(0.f, .17f, 0.f)), V3(.3f, .05f, .15f), 3, 10, R3Color(Ice_ColorSnow), false);
 // Gear wheel on the side
 v3 Gear = R3Add(Drum, V3(.37f, -.02f, 0.f));
 R3PushCylinder(O, R3Add(Gear, V3(-.015f, 0.f, 0.f)), R3Add(Gear, V3(.015f, 0.f, 0.f)), .16f, 14, R3Color(Ice_ColorRust));
 for(u32 Tooth = 0; Tooth < 12; Tooth += 1)
 {
  f32 A = 2.f*Pi32*(f32)Tooth/12.f;
  R3PushBox(O, R3Add(Gear, V3(0.f, .18f*R3Cos(A), .18f*R3Sin(A))), V3(.016f, .02f, .02f), R3Color(Ice_ColorRust));
 }
 // Chimney with smoke
 v3 StackBase = R3Add(Drum, V3(-.15f, .15f, 0.f));
 v3 StackTop = R3Add(StackBase, V3(0.f, .8f, 0.f));
 R3PushCylinder(O, StackBase, StackTop, .045f, 8, HeroShade(H, 0xFF5E5650));
 R3PushCylinder(O, StackTop, R3Add(StackTop, V3(0.f, .04f, 0.f)), .055f, 8, R3Color(Ice_ColorDarkMetal));
 IcePushSmoke(H, R3Add(StackTop, V3(0.f, .04f, 0.f)));
 // Pipes down into the ground
 R3PushCylinder(O, R3Add(Drum, V3(.2f, .15f, .08f)), R3Add(Drum, V3(.2f, .35f, .08f)), .025f, 6, R3Color(Ice_ColorMetal));
 R3PushEllipsoid(O, R3Add(Drum, V3(.2f, .35f, .08f)), V3(.03f, .03f, .03f), 3, 6, R3Color(Ice_ColorMetal), false);
 R3PushCylinder(O, R3Add(Drum, V3(.2f, .35f, .08f)), R3Add(Drum, V3(.55f, .35f, .08f)), .025f, 6, R3Color(Ice_ColorMetal));
 R3PushCylinder(O, R3Add(Drum, V3(.55f, .35f, .08f)), R3Add(B, V3(.55f + .3f, -.05f, .08f)), .025f, 6, R3Color(Ice_ColorMetal));
 IcePushIcicles(H, R3Add(Drum, V3(.22f, .33f, .1f)), R3Add(Drum, V3(.55f, .33f, .1f)), 5, .07f);
}

// NOTE: A small airship moored beside the island on a rope.
internal void
IcePushAirship(hero_ctx *H, v3 Mast, v3 Out)
{
 r3_mesh *O = H->Opaque;
 v3 Body = R3Add(R3Add(Mast, R3Scale(Out, 1.f)), V3(0.f, .55f, 0.f));
 R3PushEllipsoid(O, Body, V3(.55f, .2f, .2f), 6, 12, R3Color(0xFFC9CDD2), false);
 for(u32 Band = 0; Band < 3; Band += 1)
 {
  R3PushEllipsoid(O, R3Add(Body, V3(-.3f + .3f*(f32)Band, 0.f, 0.f)), V3(.02f, .205f, .205f), 4, 12, R3Color(Ice_ColorRust), false);
 }
 R3PushOrientedBox(O, R3Add(Body, V3(-.52f, .08f, 0.f)), V3(.07f, 0.f, 0.f), V3(0.f, .1f, 0.f), V3(0.f, 0.f, .006f), R3Color(Ice_ColorRust));
 v3 Gondola = R3Add(Body, V3(0.f, -.26f, 0.f));
 R3PushBox(O, Gondola, V3(.16f, .045f, .06f), HeroShade(H, Hero_ColorWood));
 HeroGlowRect(O, R3Add(Gondola, V3(0.f, .005f, .061f)), V3(.1f, 0.f, 0.f), V3(0.f, .018f, 0.f), R3Color(Hero_ColorWindow));
 R3PushCylinder(O, R3Add(Gondola, V3(-.12f, .04f, 0.f)), R3Add(Body, V3(-.12f, -.18f, 0.f)), .004f, 3, R3Color(Ice_ColorDarkMetal));
 R3PushCylinder(O, R3Add(Gondola, V3(.12f, .04f, 0.f)), R3Add(Body, V3(.12f, -.18f, 0.f)), .004f, 3, R3Color(Ice_ColorDarkMetal));
 R3PushCylinder(O, R3Add(Body, V3(.55f, 0.f, 0.f)), Mast, .004f, 3, R3Color(Fishing_ColorRope));
 R3PushCylinder(O, R3Sub(Mast, V3(0.f, .5f, 0.f)), Mast, .015f, 4, R3Color(Hero_ColorWoodDark));
}

//~ Scaffolding, chains and the frozen waterfall round the slab
internal void
IcePushSlabDressing(hero_ctx *H, hero_layout *L)
{
 r3_mesh *O = H->Opaque;
 f32 Out = Hero_Half + .07f;
 v4 Wood = R3Color(Ice_ColorFrostWood);
 for(u32 Side = 0; Side < 4; Side += 1)
 {
  // Side 0 = front (+Z), 1 = right (+X), 2 = back, 3 = left
  for(u32 Post = 0; Post <= 8; Post += 1)
  {
   f32 S = -Hero_Half + 2.f*Hero_Half*(f32)Post/8.f;
   b32 InPool = (Side == 0 && L->Pool && S > L->PoolX0 - .05f && S < L->PoolX1 + .05f);
   if(InPool) continue;
   f32 X = (Side == 0 || Side == 2) ? S : ((Side == 1) ? Out : -Out);
   f32 Z = (Side == 0) ? Out : ((Side == 2) ? -Out : S);
   R3PushBox(O, HeroP(H, X, -.85f, Z), V3(.025f, .85f, .025f), Wood);
   // Diagonal brace to the next post
   if(Post < 8)
   {
    f32 S1 = -Hero_Half + 2.f*Hero_Half*(f32)(Post + 1)/8.f;
    b32 NextInPool = (Side == 0 && L->Pool && S1 > L->PoolX0 - .05f && S1 < L->PoolX1 + .05f);
    if(!NextInPool)
    {
     f32 X1 = (Side == 0 || Side == 2) ? S1 : X;
     f32 Z1 = (Side == 1 || Side == 3) ? S1 : Z;
     v3 A = HeroP(H, X, ((Post & 1) ? -.25f : -1.5f), Z);
     v3 B = HeroP(H, X1, ((Post & 1) ? -1.5f : -.25f), Z1);
     R3PushCylinder(O, A, B, .014f, 4, Wood);
     v3 Beam0 = HeroP(H, X, -.3f, Z);
     v3 Beam1 = HeroP(H, X1, -.3f, Z1);
     R3PushCylinder(O, Beam0, Beam1, .018f, 4, Wood);
     IcePushIcicles(H, R3Sub(Beam0, V3(0.f, .02f, 0.f)), R3Sub(Beam1, V3(0.f, .02f, 0.f)), 4, .18f);
     IcePushIcicles(H, HeroP(H, X, 0.f, Z), HeroP(H, X1, 0.f, Z1), 5, .35f);
    }
   }
  }
  // Snow lip along the top edge
  for(u32 Lump = 0; Lump < 10; Lump += 1)
  {
   f32 S = -Hero_Half + 2.f*Hero_Half*((f32)Lump + .5f)/10.f;
   if(Side == 0 && L->Pool && S > L->PoolX0 - .1f && S < L->PoolX1 + .1f) continue;
   f32 X = (Side == 0 || Side == 2) ? S : ((Side == 1) ? Hero_Half : -Hero_Half);
   f32 Z = (Side == 0) ? Hero_Half : ((Side == 2) ? -Hero_Half : S);
   u32 Type;
   f32 Y = HeroGroundY(L, X*.99f, Z*.99f, &Type);
   R3PushEllipsoid(O, HeroP(H, X, Y, Z), V3(.18f, .06f, .18f), 3, 7, R3Color(Ice_ColorSnow), false);
  }
 }

 //- Deck ledges sticking out on the sides, with railings, snow and icicles
 for(u32 Ledge = 0; Ledge < 3; Ledge += 1)
 {
  f32 SideSign = (Ledge == 1) ? 1.f : -1.f;
  f32 Along = (Ledge == 2) ? -1.2f : .9f;
  v3 C = HeroP(H, SideSign*(Hero_Half + .3f), -.08f, Along);
  R3PushBox(O, C, V3(.3f, .025f, .35f), HeroShade(H, Ice_ColorFrostWood));
  R3PushBox(O, R3Add(C, V3(0.f, .03f, 0.f)), V3(.29f, .008f, .34f), R3Color(Ice_ColorSnow));
  R3PushCylinder(O, R3Add(C, V3(SideSign*.28f, .14f, -.33f)), R3Add(C, V3(SideSign*.28f, .14f, .33f)), .008f, 4, R3Color(Hero_ColorWoodDark));
  R3PushCylinder(O, R3Add(C, V3(SideSign*.28f, 0.f, -.33f)), R3Add(C, V3(SideSign*.28f, .14f, -.33f)), .01f, 4, R3Color(Hero_ColorWoodDark));
  R3PushCylinder(O, R3Add(C, V3(SideSign*.28f, 0.f, .33f)), R3Add(C, V3(SideSign*.28f, .14f, .33f)), .01f, 4, R3Color(Hero_ColorWoodDark));
  R3PushCylinder(O, R3Add(C, V3(SideSign*.25f, -.02f, 0.f)), HeroP(H, SideSign*Hero_Half, -.7f, Along), .018f, 4, Wood);
  IcePushIcicles(H, R3Add(C, V3(SideSign*.3f, -.03f, -.33f)), R3Add(C, V3(SideSign*.3f, -.03f, .33f)), 7, .3f);
  FishingPushFishCrate(H, R3Add(C, V3(-SideSign*.05f, .03f, .15f)));
  W3PushTerrace(O, R3Add(C, V3(SideSign*.1f, .155f, -.15f)), .055f, .13f, 8, R3Color(0xFF6E4A2C), HeroShade(H, Hero_ColorWood));
 }

 //- Chains hanging under the island
 for(u32 Chain = 0; Chain < 8; Chain += 1)
 {
  f32 A = 2.f*Pi32*((f32)Chain + .5f)/8.f;
  v3 P = HeroP(H, 1.6f*R3Cos(A), Hero_StoneBottom - .3f, 1.6f*R3Sin(A));
  f32 Length = .6f + 1.f*RandomUnilateral(&H->Series);
  u32 Links = (u32)(Length/.07f);
  for(u32 Link = 0; Link < Links; Link += 1)
  {
   v3 L0 = R3Sub(P, V3(0.f, .07f*(f32)Link, 0.f));
   v3 L1 = R3Sub(L0, V3(0.f, .06f, 0.f));
   v3 Offset = (Link & 1) ? V3(.012f, 0.f, 0.f) : V3(0.f, 0.f, .012f);
   R3PushCylinder(O, R3Add(L0, Offset), R3Add(L1, Offset), .006f, 3, R3Color(Ice_ColorDarkMetal));
   R3PushCylinder(O, R3Sub(L0, Offset), R3Sub(L1, Offset), .006f, 3, R3Color(Ice_ColorDarkMetal));
  }
 }

 //- Frozen waterfall where the iced pool meets the front edge
 if(L->Pool)
 {
  for(u32 Spike = 0; Spike < 34; Spike += 1)
  {
   f32 X = L->PoolX0 + (L->PoolX1 - L->PoolX0)*RandomUnilateral(&H->Series);
   f32 Length = 1.f + 2.8f*RandomUnilateral(&H->Series);
   v3 Top = HeroP(H, X, Hero_WaterLevel + .02f, Hero_Half + .04f + .1f*RandomUnilateral(&H->Series));
   R3PushCone(H->Translucent, Top, R3Sub(Top, V3(0.f, Length, 0.f)), .06f + .08f*RandomUnilateral(&H->Series), 5, R3WithAlpha(R3Color(Ice_ColorIce), .82f));
  }
  for(u32 Mass = 0; Mass < 6; Mass += 1)
  {
   f32 X = L->PoolX0 + (L->PoolX1 - L->PoolX0)*((f32)Mass + .5f)/6.f;
   R3PushEllipsoid(H->Translucent, HeroP(H, X, Hero_WaterLevel - .3f - .5f*RandomUnilateral(&H->Series), Hero_Half + .08f), V3(.2f, .4f, .12f), 4, 8, R3WithAlpha(R3Color(0xFFEAF7FC), .75f), false);
  }
 }
}

//~ The whole frozen island
internal void
W3PushIceIsland(app_state *App, island *Island, v3 Top, u32 Index, u32 StatusColor, b32 Highlight, u32 HighlightColor, f32 KnownFraction)
{
 hero_layout Layout = {0};
 hero_ctx Ctx;
 hero_ctx *H = &Ctx;
 HeroBegin(H, App, &Layout, Top, W3_Seed*43 + 1229*(u64)Index, Index);
 random_series *S = &H->Series;
 f32 Mirror = (RandomUnilateral(S) < .5f) ? 1.f : -1.f;

 product *Products[4];
 u32 ProductCount = HeroSortedProducts(Island, Products, ArrayCount(Products));
 u64 Fish = HeroAmountOf(Island, ProductKind_Fish);
 u64 Wheat = HeroAmountOf(Island, ProductKind_Wheat);

 //- Layout: the square slab, a frozen pool at the front (wider for a bigger catch), a
 // snowy hill at the back on one side, a flat plaza in the middle for the machines
 Layout.Snowy = true;
 Layout.FrontLevel = 0.f;
 Layout.Pool = true;
 f32 PoolWidth = 1.f + Min(.9f, (f32)Fish/180.f);
 f32 PoolCenter = -.35f*Mirror;
 Layout.PoolX0 = PoolCenter - .5f*PoolWidth;
 Layout.PoolX1 = PoolCenter + .5f*PoolWidth;
 Layout.PoolZ0 = 1.25f;
 Layout.HillX = .9f*Mirror;
 Layout.HillZ = -1.4f;
 Layout.HillStretch = 1.3f;
 Layout.HillPeak = 1.8f;
 Layout.HillCap = 1.2f;
 Layout.Path[0][0] = -1.6f*Mirror; Layout.Path[0][1] = .9f;
 Layout.Path[1][0] = 0.f;          Layout.Path[1][1] = .2f;
 Layout.Path[2][0] = 1.5f*Mirror;  Layout.Path[2][1] = .6f;
 Layout.PathCount = 3;

 HeroPushTerrain(H);
 HeroPushCliff(H);
 IcePushSlabDressing(H, &Layout);

 //- Knowledge crystal on the snowy hill
 HeroPushCrest(H, Layout.HillX, Layout.HillZ - .1f, KnownFraction);

 //- The machine yard: a boiler, vehicles on the plaza
 f32 X, Z;
 if(HeroPlace(H, .55f, .25f, -1.6f, .4f, &X, &Z)) IcePushBoiler(H, X - .1f, Z);
 if(HeroPlace(H, .25f, .16f, -.6f, 1.f, &X, &Z)) IcePushTracked(H, X, Z, Mirror, true);
 if(HeroPlace(H, .25f, .16f, -.6f, 1.1f, &X, &Z)) IcePushTracked(H, X, Z, -Mirror, false);
 if(HeroPlace(H, .22f, .14f, -.6f, 1.1f, &X, &Z)) IcePushTruck(H, X, Z, Mirror);

 //- Cranes on the edges, lifting loads over the drop (a boat when the island fishes)
 {
  f32 CX[3];
  f32 CZ[3];
  v3 Outs[3];
  CX[0] = -2.f; CZ[0] = -.35f; Outs[0] = V3(-1.f, 0.f, 0.f);
  CX[1] = 2.f;  CZ[1] = .2f;   Outs[1] = V3(1.f, 0.f, 0.f);
  CX[2] = 1.f*Mirror; CZ[2] = 2.f; Outs[2] = V3(0.f, 0.f, 1.f);
  for(u32 Crane = 0; Crane < 3; Crane += 1)
  {
   f32 HX = .2f, HZ = .2f;
   f32 PX, PZ;
   if(HeroPlaceNear(H, CX[Crane], CZ[Crane], .3f, &HX, &HZ, &PX, &PZ))
   {
    u32 Load = (Crane == 0 && Fish > 0) ? 1 : ((Crane == 1 && Fish >= 100) ? 2 : 0);
    IcePushCrane(H, PX, PZ, Outs[Crane], Load);
   }
  }
 }

 //- What the island makes, sized by the amounts
 for(u32 Idx = 0; Idx < ProductCount; Idx += 1)
 {
  HeroPushProduct(H, Products[Idx]->Kind, Products[Idx]->Amount, 0);
 }
 // Grain silos store the wheat: one per 50
 u32 Silos = (Wheat > 0) ? 1 + (u32)(Wheat/50) : 0;
 for(u32 Silo = 0; Silo < Silos; Silo += 1)
 {
  if(HeroPlace(H, .2f, .2f, -2.f, 1.f, &X, &Z)) IcePushSilo(H, X, Z, .65f + .25f*RandomUnilateral(S));
 }
 // Ice fishing on the frozen pool: a hole (with a fisher) per 30 fish
 u32 Holes = (u32)(Fish/30);
 for(u32 Hole = 0; Hole < Holes; Hole += 1)
 {
  f32 HX = Layout.PoolX0 + .2f + (Layout.PoolX1 - Layout.PoolX0 - .4f)*RandomUnilateral(S);
  f32 HZ = Layout.PoolZ0 + .2f + (Hero_Half - Layout.PoolZ0 - .35f)*RandomUnilateral(S);
  v3 P = HeroP(H, HX, Hero_WaterLevel + .012f, HZ);
  R3PushEllipsoid(H->Opaque, P, V3(.06f, .004f, .06f), 2, 8, R3Color(0xFF1E5A6E), false);
  HeroPushPerson(H, R3Add(P, V3(.09f, 0.f, .02f)), HeroShirtColors[Hole % ArrayCount(HeroShirtColors)]);
  FishingPushFishCrate(H, R3Add(P, V3(-.1f, 0.f, .03f)));
 }

 //- Snow-roofed cabins with solar panels, smoke from their chimneys
 u32 Houses = 3 + RandomNext(S) % 2;
 for(u32 House = 0; House < Houses; House += 1)
 {
  f32 HX = .25f + .06f*RandomUnilateral(S);
  f32 HZ = .2f + .04f*RandomUnilateral(S);
  if(HeroPlace(H, HX + .06f, HZ + .12f, -2.f, 1.9f, &X, &Z))
  {
   HeroPushHouse(H, X, Z - .04f, HX, HZ, 1 + RandomNext(S) % 2, 256 | 64 | 4 | 1);
   u32 GroundType;
   f32 Ground = HeroGroundY(&Layout, X, Z, &GroundType);
   IcePushSmoke(H, HeroP(H, X + .5f*HX, Ground + .14f + .34f*2.f + .4f*HZ + .3f, Z - .04f - .35f*HZ));
  }
 }

 //- Airship moored off one side
 {
  f32 Side = -Mirror;
  u32 GroundType;
  f32 Ground = HeroGroundY(&Layout, Side*(Hero_Half - .15f), -1.f, &GroundType);
  v3 Mast = HeroP(H, Side*(Hero_Half - .15f), Ground + .5f, -1.f);
  IcePushAirship(H, Mast, V3(Side, 0.f, 0.f));
 }

 //- Windmills, snowy pines, laundry, market stall, crates
 u32 Mills = 2 + RandomNext(S) % 2;
 for(u32 Mill = 0; Mill < Mills; Mill += 1)
 {
  if(HeroPlace(H, .17f, .17f, -2.1f, .3f, &X, &Z)) HeroPushWindmill(H, X, Z, 1.3f + .5f*RandomUnilateral(S), 6.f*RandomUnilateral(S));
 }
 for(u32 Pine = 0; Pine < 7; Pine += 1)
 {
  if(HeroPlace(H, .18f, .18f, -2.2f, 1.6f, &X, &Z)) IcePushSnowyPine(H, X, Z, .9f + .6f*RandomUnilateral(S));
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
 for(u32 Barrel = 0; Barrel < 4; Barrel += 1)
 {
  if(HeroPlace(H, .07f, .07f, -1.8f, 2.f, &X, &Z)) HeroPushBarrel(H, X, Z);
 }

 //- Workers in the yard
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
    HeroPushPerson(H, HeroOn(H, PX, PZ), (Person & 1) ? 0xFFF29E4C : HeroShirtColors[RandomNext(S) % ArrayCount(HeroShirtColors)]);
    HeroMarkRect(H, PX, PZ, PX, PZ);
    break;
   }
  }
 }

 HeroFinish(H, App, StatusColor, Highlight, HighlightColor);
}
