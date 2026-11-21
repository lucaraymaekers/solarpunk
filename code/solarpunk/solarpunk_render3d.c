//-
// SolarPunk 3D renderer: a small lit-triangle renderer with its own shader and buffers.
// It does not touch Luca's rectangle renderer, it only restores the GL state that
// renderer relies on after drawing.
//-

#include <stddef.h> // offsetof

//~ Scalar and vector math
internal f32 R3Sin(f32 X) { f32 Result = sinf(X); return Result; }
internal f32 R3Cos(f32 X) { f32 Result = cosf(X); return Result; }
internal f32 R3Tan(f32 X) { f32 Result = tanf(X); return Result; }
internal f32 R3Sqrt(f32 X) { f32 Result = sqrtf(X); return Result; }

internal v3 R3Add(v3 A, v3 B) { v3 Result = V3(A.X + B.X, A.Y + B.Y, A.Z + B.Z); return Result; }
internal v3 R3Sub(v3 A, v3 B) { v3 Result = V3(A.X - B.X, A.Y - B.Y, A.Z - B.Z); return Result; }
internal v3 R3Scale(v3 A, f32 S) { v3 Result = V3(A.X*S, A.Y*S, A.Z*S); return Result; }
internal f32 R3Dot(v3 A, v3 B) { f32 Result = A.X*B.X + A.Y*B.Y + A.Z*B.Z; return Result; }
internal f32 R3Length(v3 A) { f32 Result = R3Sqrt(R3Dot(A, A)); return Result; }

internal v3
R3Cross(v3 A, v3 B)
{
 v3 Result = V3(A.Y*B.Z - A.Z*B.Y,
                A.Z*B.X - A.X*B.Z,
                A.X*B.Y - A.Y*B.X);
 return Result;
}

internal v3
R3Normalize(v3 A)
{
 v3 Result = V3(0.f, 0.f, 0.f);
 f32 Length = R3Length(A);
 if(Length > 1e-6f)
 {
  Result = R3Scale(A, 1.f/Length);
 }
 return Result;
}

internal v3
R3Lerp(v3 A, v3 B, f32 t)
{
 v3 Result = R3Add(A, R3Scale(R3Sub(B, A), t));
 return Result;
}

internal v4
R3LerpColor(v4 A, v4 B, f32 t)
{
 v4 Result = V4(Lerp(A.X, B.X, t), Lerp(A.Y, B.Y, t), Lerp(A.Z, B.Z, t), Lerp(A.W, B.W, t));
 return Result;
}

internal v4
R3Color(u32 ARGB)
{
 v4 Result = V4FromU32(ARGB);
 return Result;
}

internal v4
R3WithAlpha(v4 Color, f32 Alpha)
{
 v4 Result = Color;
 Result.W = Alpha;
 return Result;
}

//~ Matrices
internal m4
M4Identity(void)
{
 m4 Result = {0};
 Result.e[0][0] = 1.f;
 Result.e[1][1] = 1.f;
 Result.e[2][2] = 1.f;
 Result.e[3][3] = 1.f;
 return Result;
}

internal m4
M4Mul(m4 A, m4 B)
{
 m4 Result = {0};
 for(u32 Column = 0; Column < 4; Column += 1)
 {
  for(u32 Row = 0; Row < 4; Row += 1)
  {
   f32 Sum = 0.f;
   for(u32 K = 0; K < 4; K += 1)
   {
    Sum += A.e[K][Row]*B.e[Column][K];
   }
   Result.e[Column][Row] = Sum;
  }
 }
 return Result;
}

internal v4
M4MulV4(m4 M, v4 V)
{
 v4 Result = V4(0.f, 0.f, 0.f, 0.f);
 for(u32 Row = 0; Row < 4; Row += 1)
 {
  Result.e[Row] = (M.e[0][Row]*V.e[0] + M.e[1][Row]*V.e[1] +
                   M.e[2][Row]*V.e[2] + M.e[3][Row]*V.e[3]);
 }
 return Result;
}

internal m4
M4Perspective(f32 FovY, f32 Aspect, f32 Near, f32 Far)
{
 m4 Result = {0};
 f32 F = 1.f/R3Tan(.5f*FovY);
 Result.e[0][0] = F/Aspect;
 Result.e[1][1] = F;
 Result.e[2][2] = (Far + Near)/(Near - Far);
 Result.e[2][3] = -1.f;
 Result.e[3][2] = (2.f*Far*Near)/(Near - Far);
 return Result;
}

internal m4
M4LookAt(v3 Eye, v3 Target, v3 Up)
{
 v3 F = R3Normalize(R3Sub(Target, Eye));
 v3 S = R3Normalize(R3Cross(F, Up));
 v3 U = R3Cross(S, F);

 m4 Result = M4Identity();
 Result.e[0][0] = S.X; Result.e[1][0] = S.Y; Result.e[2][0] = S.Z;
 Result.e[0][1] = U.X; Result.e[1][1] = U.Y; Result.e[2][1] = U.Z;
 Result.e[0][2] = -F.X; Result.e[1][2] = -F.Y; Result.e[2][2] = -F.Z;
 Result.e[3][0] = -R3Dot(S, Eye);
 Result.e[3][1] = -R3Dot(U, Eye);
 Result.e[3][2] = R3Dot(F, Eye);
 return Result;
}

// NOTE: World position to window pixels (top-left origin). False if behind the camera.
internal b32
R3ProjectToScreen(r3_state *R3, v3 P, v2 *ScreenPos)
{
 b32 Result = false;
 v4 Clip = M4MulV4(R3->ViewProj, V4(P.X, P.Y, P.Z, 1.f));
 if(Clip.W > 1e-4f)
 {
  f32 X = Clip.X/Clip.W;
  f32 Y = Clip.Y/Clip.W;
  ScreenPos->X = (.5f*X + .5f)*R3->ViewportDim.X;
  ScreenPos->Y = (1.f - (.5f*Y + .5f))*R3->ViewportDim.Y;
  Result = true;
 }
 return Result;
}

//~ Meshes
internal r3_mesh
R3MeshAlloc(arena *Arena, u32 Capacity)
{
 r3_mesh Result = {0};
 Result.Vertices = PushArray(Arena, r3_vertex, Capacity);
 Result.Capacity = Capacity;
 return Result;
}

internal void
R3PushVertex(r3_mesh *Mesh, v3 Pos, v3 Normal, v4 Color)
{
 if(Mesh->Count < Mesh->Capacity)
 {
  r3_vertex *Vertex = Mesh->Vertices + Mesh->Count;
  Vertex->Pos = Pos;
  Vertex->Normal = Normal;
  Vertex->Color = Color;
  Mesh->Count += 1;
 }
}

// NOTE: Flat-shaded triangle. The normal is flipped to point away from Inside, so the
// winding order of the corners does not matter.
internal void
R3PushTri(r3_mesh *Mesh, v3 P0, v3 P1, v3 P2, v3 Inside, v4 Color)
{
 v3 Normal = R3Normalize(R3Cross(R3Sub(P1, P0), R3Sub(P2, P0)));
 v3 Centroid = R3Scale(R3Add(R3Add(P0, P1), P2), 1.f/3.f);
 if(R3Dot(Normal, R3Sub(Centroid, Inside)) < 0.f)
 {
  Normal = R3Scale(Normal, -1.f);
 }

 if(Mesh->Count + 3 <= Mesh->Capacity)
 {
  R3PushVertex(Mesh, P0, Normal, Color);
  R3PushVertex(Mesh, P1, Normal, Color);
  R3PushVertex(Mesh, P2, Normal, Color);
 }
}

internal void
R3PushQuad(r3_mesh *Mesh, v3 P0, v3 P1, v3 P2, v3 P3, v3 Inside, v4 Color)
{
 R3PushTri(Mesh, P0, P1, P2, Inside, Color);
 R3PushTri(Mesh, P0, P2, P3, Inside, Color);
}

// NOTE: Unlit triangle (glows with its own colour).
internal void
R3PushTriUnlit(r3_mesh *Mesh, v3 P0, v3 P1, v3 P2, v4 Color)
{
 v3 Zero = V3(0.f, 0.f, 0.f);
 if(Mesh->Count + 3 <= Mesh->Capacity)
 {
  R3PushVertex(Mesh, P0, Zero, Color);
  R3PushVertex(Mesh, P1, Zero, Color);
  R3PushVertex(Mesh, P2, Zero, Color);
 }
}

// NOTE: Box from a centre and three half-extent axes (they may be rotated).
internal void
R3PushOrientedBox(r3_mesh *Mesh, v3 Center, v3 AxisX, v3 AxisY, v3 AxisZ, v4 Color)
{
 v3 Corners[8];
 for(u32 Idx = 0; Idx < 8; Idx += 1)
 {
  f32 SX = (Idx & 1) ? 1.f : -1.f;
  f32 SY = (Idx & 2) ? 1.f : -1.f;
  f32 SZ = (Idx & 4) ? 1.f : -1.f;
  Corners[Idx] = R3Add(R3Add(R3Add(Center, R3Scale(AxisX, SX)), R3Scale(AxisY, SY)), R3Scale(AxisZ, SZ));
 }

 R3PushQuad(Mesh, Corners[0], Corners[1], Corners[3], Corners[2], Center, Color); // -Z
 R3PushQuad(Mesh, Corners[4], Corners[5], Corners[7], Corners[6], Center, Color); // +Z
 R3PushQuad(Mesh, Corners[0], Corners[1], Corners[5], Corners[4], Center, Color); // -Y
 R3PushQuad(Mesh, Corners[2], Corners[3], Corners[7], Corners[6], Center, Color); // +Y
 R3PushQuad(Mesh, Corners[0], Corners[2], Corners[6], Corners[4], Center, Color); // -X
 R3PushQuad(Mesh, Corners[1], Corners[3], Corners[7], Corners[5], Center, Color); // +X
}

internal void
R3PushBox(r3_mesh *Mesh, v3 Center, v3 HalfSize, v4 Color)
{
 R3PushOrientedBox(Mesh, Center, V3(HalfSize.X, 0.f, 0.f), V3(0.f, HalfSize.Y, 0.f), V3(0.f, 0.f, HalfSize.Z), Color);
}

// NOTE: Two unit vectors perpendicular to Direction (and to each other).
internal void
R3Perpendiculars(v3 Direction, v3 *U, v3 *V)
{
 v3 Helper = (Direction.Y > .9f || Direction.Y < -.9f) ? V3(1.f, 0.f, 0.f) : V3(0.f, 1.f, 0.f);
 *U = R3Normalize(R3Cross(Direction, Helper));
 *V = R3Cross(Direction, *U);
}

internal void
R3PushCylinder(r3_mesh *Mesh, v3 A, v3 B, f32 Radius, u32 Sides, v4 Color)
{
 v3 Direction = R3Normalize(R3Sub(B, A));
 v3 U, V;
 R3Perpendiculars(Direction, &U, &V);
 v3 Mid = R3Scale(R3Add(A, B), .5f);

 for(u32 Side = 0; Side < Sides; Side += 1)
 {
  f32 Angle0 = 2.f*Pi32*(f32)Side/(f32)Sides;
  f32 Angle1 = 2.f*Pi32*(f32)(Side + 1)/(f32)Sides;
  v3 Offset0 = R3Add(R3Scale(U, Radius*R3Cos(Angle0)), R3Scale(V, Radius*R3Sin(Angle0)));
  v3 Offset1 = R3Add(R3Scale(U, Radius*R3Cos(Angle1)), R3Scale(V, Radius*R3Sin(Angle1)));
  R3PushQuad(Mesh, R3Add(A, Offset0), R3Add(A, Offset1), R3Add(B, Offset1), R3Add(B, Offset0), Mid, Color);
 }
}

internal void
R3PushCone(r3_mesh *Mesh, v3 Base, v3 Tip, f32 Radius, u32 Sides, v4 Color)
{
 v3 Direction = R3Normalize(R3Sub(Tip, Base));
 v3 U, V;
 R3Perpendiculars(Direction, &U, &V);
 v3 Inside = R3Lerp(Base, Tip, .25f);

 for(u32 Side = 0; Side < Sides; Side += 1)
 {
  f32 Angle0 = 2.f*Pi32*(f32)Side/(f32)Sides;
  f32 Angle1 = 2.f*Pi32*(f32)(Side + 1)/(f32)Sides;
  v3 P0 = R3Add(Base, R3Add(R3Scale(U, Radius*R3Cos(Angle0)), R3Scale(V, Radius*R3Sin(Angle0))));
  v3 P1 = R3Add(Base, R3Add(R3Scale(U, Radius*R3Cos(Angle1)), R3Scale(V, Radius*R3Sin(Angle1))));
  R3PushTri(Mesh, P0, P1, Tip, Inside, Color);
  R3PushTri(Mesh, P0, P1, Base, Tip, Color);
 }
}

// NOTE: Ellipsoid with smooth normals. Unlit = glowing (normal zero).
internal void
R3PushEllipsoid(r3_mesh *Mesh, v3 Center, v3 Radii, u32 Rings, u32 Segments, v4 Color, b32 Unlit)
{
 for(u32 Ring = 0; Ring < Rings; Ring += 1)
 {
  f32 Theta0 = Pi32*(f32)Ring/(f32)Rings;
  f32 Theta1 = Pi32*(f32)(Ring + 1)/(f32)Rings;

  for(u32 Segment = 0; Segment < Segments; Segment += 1)
  {
   f32 Phi0 = 2.f*Pi32*(f32)Segment/(f32)Segments;
   f32 Phi1 = 2.f*Pi32*(f32)(Segment + 1)/(f32)Segments;

   v3 Unit[4];
   Unit[0] = V3(R3Sin(Theta0)*R3Cos(Phi0), R3Cos(Theta0), R3Sin(Theta0)*R3Sin(Phi0));
   Unit[1] = V3(R3Sin(Theta0)*R3Cos(Phi1), R3Cos(Theta0), R3Sin(Theta0)*R3Sin(Phi1));
   Unit[2] = V3(R3Sin(Theta1)*R3Cos(Phi1), R3Cos(Theta1), R3Sin(Theta1)*R3Sin(Phi1));
   Unit[3] = V3(R3Sin(Theta1)*R3Cos(Phi0), R3Cos(Theta1), R3Sin(Theta1)*R3Sin(Phi0));

   v3 P[4];
   v3 N[4];
   for(u32 Corner = 0; Corner < 4; Corner += 1)
   {
    P[Corner] = R3Add(Center, V3(Unit[Corner].X*Radii.X, Unit[Corner].Y*Radii.Y, Unit[Corner].Z*Radii.Z));
    N[Corner] = (Unlit ? V3(0.f, 0.f, 0.f) : Unit[Corner]);
   }

   if(Mesh->Count + 6 <= Mesh->Capacity)
   {
    R3PushVertex(Mesh, P[0], N[0], Color);
    R3PushVertex(Mesh, P[1], N[1], Color);
    R3PushVertex(Mesh, P[2], N[2], Color);
    R3PushVertex(Mesh, P[0], N[0], Color);
    R3PushVertex(Mesh, P[2], N[2], Color);
    R3PushVertex(Mesh, P[3], N[3], Color);
   }
  }
 }
}

//~ Camera
#define R3_CameraDefaultYaw      .55f
#define R3_CameraDefaultPitch    .72f
#define R3_CameraDefaultDistance 150.f
#define R3_CameraMinDistance     6.f
#define R3_CameraMaxDistance     340.f
#define R3_CameraMinPitch        .12f
#define R3_CameraMaxPitch        1.45f
#define R3_CameraDragYawSpeed    .008f
#define R3_CameraDragPitchSpeed  .006f
#define R3_CameraZoomStep        1.2f
#define R3_CameraWheelStep       1.12f
#define R3_CameraPanSpeed        .0015f  // world units per pixel, times the distance
#define R3_CameraMaxPan          110.f
#define R3_CameraFlySpeed        4.f     // higher = faster flight to a cluster

internal void
R3CameraReset(r3_camera *Camera)
{
 Camera->Initialized = true;
 Camera->Yaw = R3_CameraDefaultYaw;
 Camera->Pitch = R3_CameraDefaultPitch;
 Camera->Distance = R3_CameraDefaultDistance;
 Camera->Target = V3(0.f, -2.f, 0.f);
 Camera->Rotating = false;
 Camera->Panning = false;
 Camera->Flying = false;
}

// NOTE: Start a smooth flight to look at Target from Distance (any mouse use stops it).
internal void
R3CameraFlyTo(r3_camera *Camera, v3 Target, f32 Distance)
{
 Camera->Flying = true;
 Camera->FlyTarget = Target;
 Camera->FlyDistance = Clamp(R3_CameraMinDistance, Distance, R3_CameraMaxDistance);
}

internal void
R3CameraZoom(r3_camera *Camera, f32 Factor)
{
 Camera->Distance = Clamp(R3_CameraMinDistance, Camera->Distance*Factor, R3_CameraMaxDistance);
}

// NOTE: Mouse wheel notches this frame, positive = away from the user. Windows reports
// the wheel as Mouse.Z; Linux reports it as the ScrollUp/ScrollDown buttons.
internal s32
R3WheelSteps(app_input *Input)
{
 s32 Result = Input->Mouse.Z;
#if !OS_WINDOWS
 if(WasPressed(Input->Mouse.Buttons[PlatformMouseButton_ScrollUp]))   Result += 1;
 if(WasPressed(Input->Mouse.Buttons[PlatformMouseButton_ScrollDown])) Result -= 1;
#endif
 return Result;
}

// NOTE: Mouse controls
//   wheel        : zoom (skipped when the mouse is over the UI, e.g. the panel scrolls instead)
//   right drag   : move the camera over the map
//   middle drag  : rotate around the target
internal void
R3CameraUpdate(r3_camera *Camera, app_input *Input, b32 MouseOverUI)
{
 if(!Camera->Initialized)
 {
  R3CameraReset(Camera);
 }

 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 f32 DX = MousePos.X - Camera->LastMousePos.X;
 f32 DY = MousePos.Y - Camera->LastMousePos.Y;

 //- Zoom
 if(!MouseOverUI)
 {
  s32 Steps = R3WheelSteps(Input);
  if(Steps != 0) Camera->Flying = false;
  for(; Steps > 0; Steps -= 1) R3CameraZoom(Camera, 1.f/R3_CameraWheelStep);
  for(; Steps < 0; Steps += 1) R3CameraZoom(Camera, R3_CameraWheelStep);
 }

 //- Rotate (middle drag)
 b32 MiddleDown = Input->Mouse.Buttons[PlatformMouseButton_Middle].EndedDown;
 if(MiddleDown && Camera->Rotating)
 {
  Camera->Yaw -= DX*R3_CameraDragYawSpeed;
  Camera->Pitch = Clamp(R3_CameraMinPitch, Camera->Pitch + DY*R3_CameraDragPitchSpeed, R3_CameraMaxPitch);
 }
 Camera->Rotating = MiddleDown;

 //- Move (right drag): the map follows the mouse
 b32 RightDown = Input->Mouse.Buttons[PlatformMouseButton_Right].EndedDown;
 if(RightDown && Camera->Panning)
 {
  v3 Forward = V3(-R3Sin(Camera->Yaw), 0.f, -R3Cos(Camera->Yaw));
  v3 Right = V3(R3Cos(Camera->Yaw), 0.f, -R3Sin(Camera->Yaw));
  f32 Scale = R3_CameraPanSpeed*Camera->Distance;
  Camera->Target = R3Sub(Camera->Target, R3Scale(Right, DX*Scale));
  Camera->Target = R3Add(Camera->Target, R3Scale(Forward, DY*Scale));
  Camera->Target.X = Clamp(-R3_CameraMaxPan, Camera->Target.X, R3_CameraMaxPan);
  Camera->Target.Z = Clamp(-R3_CameraMaxPan, Camera->Target.Z, R3_CameraMaxPan);
 }
 Camera->Panning = RightDown;
 
 //- Flight
 if(MiddleDown || RightDown)
 {
  Camera->Flying = false;
 }
 if(Camera->Flying)
 {
  f32 t = 1.f - expf(-R3_CameraFlySpeed*Input->dtForFrame);
  Camera->Target = R3Lerp(Camera->Target, Camera->FlyTarget, t);
  Camera->Distance = Lerp(Camera->Distance, Camera->FlyDistance, t);
  if(R3Length(R3Sub(Camera->Target, Camera->FlyTarget)) < .05f &&
     (Camera->Distance - Camera->FlyDistance)*(Camera->Distance - Camera->FlyDistance) < .01f)
  {
   Camera->Flying = false;
  }
 }
 
 Camera->LastMousePos = MousePos;
}

internal v3
R3CameraEye(r3_camera *Camera)
{
 v3 Offset = V3(R3Cos(Camera->Pitch)*R3Sin(Camera->Yaw),
                R3Sin(Camera->Pitch),
                R3Cos(Camera->Pitch)*R3Cos(Camera->Yaw));
 v3 Result = R3Add(Camera->Target, R3Scale(Offset, Camera->Distance));
 return Result;
}

//~ GL
#define R3_MaxSkyVertices         1024
#define R3_MaxOpaqueVertices      (1024*1024)     // per frame: simple islands, cables, moving parts
#define R3_MaxHeroOpaqueVertices  (6*1024*1024)   // the hand-built islands, cached (~3.5M)
#define R3_MaxHeroTranslucentVertices (512*1024)
#define R3_MaxTranslucentVertices (512*1024)

#define R3_ColorFog        0xFFF0BC8E
#define R3_FogStart        90.f
#define R3_FogEnd          320.f

// NOTE: Warm, low sun from the left; violet-tinted ambient light in the shadows.
// The islands always get this light, day or night, so their colours never change.
#define R3_SunDirX  -.85f
#define R3_SunDirY   .42f
#define R3_SunDirZ   .32f
#define R3_SunColorR 1.f
#define R3_SunColorG .8f
#define R3_SunColorB .58f
#define R3_AmbientR  .66f
#define R3_AmbientG  .6f
#define R3_AmbientB  .68f

//~ Day and night (visual only, L14): only the sky changes
// NOTE: The time of day comes from BroadcastDayOfRound (0 = midnight, .25 = 06:00
// sunrise, .5 = noon, .75 = 18:00 sunset) and moves smoothly through each round.
#define R3_DayEaseRate     3.f     // after a jump: higher = the sky turns faster
#define R3_DaySnap         .02f    // smaller changes (normal running) are followed exactly

// NOTE: Three skies (top, middle, horizon). Everything is blended from the sun's height,
// which changes a little every frame, so the sky never holds still and never jumps:
//   night <-> day follows the height (midnight = night, noon = day),
//   the golden sky glows in around sunrise and sunset (sun near the horizon).
#define R3_ColorNightTop     0xFF151B3E
#define R3_ColorNightMiddle  0xFF263168
#define R3_ColorNightHorizon 0xFF3C4A80
#define R3_ColorDayTop       0xFF4A82C4
#define R3_ColorDayMiddle    0xFF8CBCE6
#define R3_ColorDayHorizon   0xFFD2E8F4
#define R3_ColorGoldTop      0xFF5C4D8C
#define R3_ColorGoldMiddle   0xFFE3906C
#define R3_ColorGoldHorizon  0xFFF7C97E
#define R3_GoldWidth         .55f   // sun height range (0..1) over which the golden glow fades
#define R3_BodyFadeLow       -.3f   // sun/moon height where the disc starts to show
#define R3_BodyFadeHigh      .35f   // ... and is fully visible

// NOTE: Sun and moon travel on an arc across the open sky (the part of the window the
// top bar and the panel leave free, see LensShift): up on the left, down on the right.
// The camera looks down on the islands, so the open sky is the band under the top bar;
// the arc lives there.
#define R3_SkyArcHalfWidth .85f   // of the free width
#define R3_SkyArcTopGap    .2f    // clip space kept free under the top bar (shortage strip)
#define R3_SkyArcDrop      .35f   // clip space from the arc's top down to its horizon
#define R3_SunRadius       .075f
#define R3_MoonRadius      .05f
#define R3_ColorSunCore    0xFFFFF6DC
#define R3_ColorSunGlow    0xFFFFD58A
#define R3_ColorMoonCore   0xFFEEF1FA
#define R3_ColorMoonGlow   0xFFB4C4F0
#define R3_MoonShadowSize   .85f  // of the moon's radius
#define R3_MoonShadowOffset .55f  // of the moon's radius, up and to the right
#define R3_SkyMiddleY       .2f   // where the sky gradient's middle colour sits (clip space)

global_variable char *R3VertexShaderSource =
"#version 330 core\n"
"layout(location = 0) in vec3 A_Pos;\n"
"layout(location = 1) in vec3 A_Normal;\n"
"layout(location = 2) in vec4 A_Color;\n"
"uniform mat4 U_ViewProj;\n"
"out vec3 V_Normal;\n"
"out vec4 V_Color;\n"
"out vec3 V_WorldPos;\n"
"void main()\n"
"{\n"
"  V_Normal = A_Normal;\n"
"  V_Color = A_Color;\n"
"  V_WorldPos = A_Pos;\n"
"  gl_Position = U_ViewProj*vec4(A_Pos, 1.0);\n"
"}\n";

global_variable char *R3FragmentShaderSource =
"#version 330 core\n"
"in vec3 V_Normal;\n"
"in vec4 V_Color;\n"
"in vec3 V_WorldPos;\n"
"uniform vec3 U_LightDir;\n"
"uniform vec3 U_SunColor;\n"
"uniform vec3 U_AmbientColor;\n"
"uniform vec3 U_CameraPos;\n"
"uniform vec3 U_FogColor;\n"
"uniform float U_FogStart;\n"
"uniform float U_FogEnd;\n"
"out vec4 O_Color;\n"
"void main()\n"
"{\n"
"  vec3 Color = V_Color.rgb;\n"
"  if(dot(V_Normal, V_Normal) > 0.25)\n"
"  {\n"
"    vec3 N = normalize(V_Normal);\n"
"    float Diffuse = max(dot(N, -U_LightDir), 0.0);\n"
"    float SkyLight = 0.5 + 0.5*N.y;\n"
"    Color = Color*(U_AmbientColor*(0.6 + 0.4*SkyLight) + U_SunColor*0.9*Diffuse);\n"
"  }\n"
"  float Distance = length(V_WorldPos - U_CameraPos);\n"
"  float Fog = clamp((Distance - U_FogStart)/(U_FogEnd - U_FogStart), 0.0, 1.0);\n"
"  if(dot(V_Normal, V_Normal) > 0.25)\n"
"  {\n"
"    Color = mix(Color, U_FogColor, 0.65*Fog);\n"
"  }\n"
"  O_Color = vec4(Color, V_Color.a);\n"
"}\n";

internal void
R3InitGL(r3_state *R3)
{
 gl_uint VertexShader = GL_CompileShaderFromString(S8FromCString(R3VertexShaderSource), GL_VERTEX_SHADER);
 gl_uint FragmentShader = GL_CompileShaderFromString(S8FromCString(R3FragmentShaderSource), GL_FRAGMENT_SHADER);
 R3->Program = GL_ProgramFromShaders(VertexShader, FragmentShader);

 glGenVertexArrays(1, &R3->VAO);
 glGenBuffers(1, &R3->VBO);

 glBindVertexArray(R3->VAO);
 glBindBuffer(GL_ARRAY_BUFFER, R3->VBO);
 glEnableVertexAttribArray(0);
 glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(r3_vertex), (void *)offsetof(r3_vertex, Pos));
 glEnableVertexAttribArray(1);
 glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(r3_vertex), (void *)offsetof(r3_vertex, Normal));
 glEnableVertexAttribArray(2);
 glVertexAttribPointer(2, 4, GL_FLOAT, false, sizeof(r3_vertex), (void *)offsetof(r3_vertex, Color));

 R3->GLReady = true;
}

//~ Day and night
// NOTE: t = 0 gives A, t = 1 gives B. (base Lerp, and so R3LerpColor, has them the other
// way round, which is easy to trip over.)
internal v4
R3MixColor(v4 A, v4 B, f32 t)
{
 v4 Result = V4(A.X + (B.X - A.X)*t, A.Y + (B.Y - A.Y)*t, A.Z + (B.Z - A.Z)*t, A.W + (B.W - A.W)*t);
 return Result;
}

internal f32
R3SmoothStep(f32 Edge0, f32 Edge1, f32 X)
{
 f32 t = Clamp(0.f, (X - Edge0)/(Edge1 - Edge0), 1.f);   // base Clamp is (Low, X, High)
 f32 Result = t*t*(3.f - 2.f*t);
 return Result;
}

// NOTE: Eases the shown time of day towards Target (0..1) the short way round the
// clock, so stepping or jumping on the timeline turns the sky smoothly.
internal void
R3UpdateDayPhase(r3_state *R3, f32 Target, f32 dt)
{
 if(!R3->DayPhaseSet)
 {
  R3->DayPhase = Target;
  R3->DayPhaseSet = true;
 }
 f32 Diff = Target - R3->DayPhase;
 Diff -= floorf(Diff + .5f);   // -.5 .. .5
 // Normal running moves a little each frame: follow exactly. Jumps turn smoothly.
 f32 Step = (Diff*Diff < R3_DaySnap*R3_DaySnap) ? Diff : Diff*Min(1.f, R3_DayEaseRate*dt);
 R3->DayPhase += Step;
 R3->DayPhase -= floorf(R3->DayPhase);
}

// NOTE: A round disc with soft glow rings around it, in clip space (round on screen,
// so the horizontal radius is divided by the aspect ratio).
internal void
R3PushSkyDisc(r3_mesh *Sky, v3 Center, f32 Radius, f32 Aspect, v4 Core, v4 Glow, f32 Alpha)
{
 f32 Radii[4] = {4.f, 2.4f, 1.5f, 1.f};
 f32 Alphas[4] = {.08f, .16f, .32f, 1.f};
 u32 Sides = 40;
 for(u32 Layer = 0; Layer < 4; Layer += 1)
 {
  v4 Color = (Layer == 3) ? Core : Glow;
  Color = R3WithAlpha(Color, Alphas[Layer]*Alpha);
  f32 RadiusY = Radius*Radii[Layer];
  f32 RadiusX = RadiusY/Aspect;
  for(u32 Side = 0; Side < Sides; Side += 1)
  {
   f32 Angle0 = 2.f*Pi32*(f32)Side/(f32)Sides;
   f32 Angle1 = 2.f*Pi32*(f32)(Side + 1)/(f32)Sides;
   R3PushTriUnlit(Sky, Center,
                  V3(Center.X + RadiusX*R3Cos(Angle0), Center.Y + RadiusY*R3Sin(Angle0), 0.f),
                  V3(Center.X + RadiusX*R3Cos(Angle1), Center.Y + RadiusY*R3Sin(Angle1), 0.f),
                  Color);
  }
 }
}

// NOTE: Builds the sky (gradient, sun, moon) for R3->DayPhase.
internal void
R3BuildSky(r3_state *R3, f32 Aspect)
{
 f32 Phase = R3->DayPhase;

 //- Sun and moon on opposite sides of the clock
 f32 SunAngle = 2.f*Pi32*(Phase - .25f);
 f32 SunHeight = R3Sin(SunAngle);    // -1 midnight .. 0 at 06:00/18:00 .. 1 noon
 f32 SunSide = -R3Cos(SunAngle);     // -1 = rising (left), 1 = setting (right)

 //- How much day and how much golden glow (both smooth in the time of day)
 f32 Daylight = .5f + .5f*SunHeight;
 f32 Gold = 1.f - Min(1.f, (SunHeight*SunHeight)/(R3_GoldWidth*R3_GoldWidth));
 Gold *= Gold;
#define R3_BlendKey(Field) R3MixColor(R3MixColor(R3Color(R3_ColorNight##Field), R3Color(R3_ColorDay##Field), Daylight), \
R3Color(R3_ColorGold##Field), Gold)

 //- Gradient: two bands, horizon -> middle (bottom 60% of the screen), middle -> top.
 // Drawn with an identity matrix (positions are in clip space).
 r3_mesh *Sky = &R3->Sky;
 v4 Colors[3];
 Colors[0] = R3_BlendKey(Horizon);
 Colors[1] = R3_BlendKey(Middle);
 Colors[2] = R3_BlendKey(Top);
 f32 Ys[3] = {-1.f, R3_SkyMiddleY, 1.f};
 {
  v3 Zero = V3(0.f, 0.f, 0.f);
  for(u32 Band = 0; Band < 2; Band += 1)
  {
   f32 Y0 = Ys[Band];
   f32 Y1 = Ys[Band + 1];
   R3PushVertex(Sky, V3(-1.f, Y0, 0.f), Zero, Colors[Band]);
   R3PushVertex(Sky, V3( 1.f, Y0, 0.f), Zero, Colors[Band]);
   R3PushVertex(Sky, V3( 1.f, Y1, 0.f), Zero, Colors[Band + 1]);
   R3PushVertex(Sky, V3(-1.f, Y0, 0.f), Zero, Colors[Band]);
   R3PushVertex(Sky, V3( 1.f, Y1, 0.f), Zero, Colors[Band + 1]);
   R3PushVertex(Sky, V3(-1.f, Y1, 0.f), Zero, Colors[Band + 1]);
  }
 }
#undef R3_BlendKey

 //- Discs: they rise at the left edge, set at the right edge, and fade in and out
 // slowly around the horizon (about three hours) so they never pop.
 for(u32 Body = 0; Body < 2; Body += 1)
 {
  b32 IsSun = (Body == 0);
  f32 Height = IsSun ? SunHeight : -SunHeight;
  f32 Side = IsSun ? SunSide : -SunSide;
  f32 Alpha = R3SmoothStep(R3_BodyFadeLow, R3_BodyFadeHigh, Height);
  if(Alpha > 0.f)
  {
   // Free area in clip space: x from -1 to 1 + 2*LensShift.X, y up to 1 + 2*LensShift.Y.
   f32 FreeCenterX = R3->LensShift.X;
   f32 FreeHalfWidth = 1.f + R3->LensShift.X;
   f32 ArcTop = 1.f + 2.f*R3->LensShift.Y - R3_SkyArcTopGap;
   v3 Center = V3(FreeCenterX + Side*R3_SkyArcHalfWidth*FreeHalfWidth,
                  ArcTop - R3_SkyArcDrop*(1.f - Height), 0.f);
   if(IsSun)
   {
    R3PushSkyDisc(Sky, Center, R3_SunRadius, Aspect, R3Color(R3_ColorSunCore), R3Color(R3_ColorSunGlow), Alpha);
   }
   else
   {
    v4 Glow = R3Color(R3_ColorMoonGlow);
    R3PushSkyDisc(Sky, Center, R3_MoonRadius, Aspect, R3Color(R3_ColorMoonCore), Glow, Alpha);

    // Crescent: cover part of the moon with a disc of exactly what is behind it there
    // (the sky at this height under the moon's glow rings), offset to the upper right.
    f32 t = (Center.Y < Ys[1]) ? (Center.Y - Ys[0])/(Ys[1] - Ys[0]) : (Center.Y - Ys[1])/(Ys[2] - Ys[1]);
    v4 Behind = (Center.Y < Ys[1]) ? R3MixColor(Colors[0], Colors[1], t) : R3MixColor(Colors[1], Colors[2], t);
    Behind = R3MixColor(Behind, Glow, .08f*Alpha);
    Behind = R3MixColor(Behind, Glow, .16f*Alpha);
    Behind = R3MixColor(Behind, Glow, .32f*Alpha);
    Behind.W = 1.f;
    f32 ShadowRadiusY = R3_MoonRadius*R3_MoonShadowSize;
    f32 ShadowRadiusX = ShadowRadiusY/Aspect;
    v3 ShadowCenter = V3(Center.X + R3_MoonShadowOffset*R3_MoonRadius/Aspect,
                         Center.Y + R3_MoonShadowOffset*R3_MoonRadius, 0.f);
    u32 Sides = 40;
    for(u32 Side2 = 0; Side2 < Sides; Side2 += 1)
    {
     f32 Angle0 = 2.f*Pi32*(f32)Side2/(f32)Sides;
     f32 Angle1 = 2.f*Pi32*(f32)(Side2 + 1)/(f32)Sides;
     R3PushTriUnlit(Sky, ShadowCenter,
                    V3(ShadowCenter.X + ShadowRadiusX*R3Cos(Angle0), ShadowCenter.Y + ShadowRadiusY*R3Sin(Angle0), 0.f),
                    V3(ShadowCenter.X + ShadowRadiusX*R3Cos(Angle1), ShadowCenter.Y + ShadowRadiusY*R3Sin(Angle1), 0.f),
                    Behind);
    }
   }
  }
 }
}

// NOTE: The cached buffer for the hand-built islands (same vertex layout).
internal void
R3InitHeroGL(r3_state *R3)
{
 glGenVertexArrays(1, &R3->HeroVAO);
 glGenBuffers(1, &R3->HeroVBO);
 glBindVertexArray(R3->HeroVAO);
 glBindBuffer(GL_ARRAY_BUFFER, R3->HeroVBO);
 glEnableVertexAttribArray(0);
 glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(r3_vertex), (void *)offsetof(r3_vertex, Pos));
 glEnableVertexAttribArray(1);
 glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(r3_vertex), (void *)offsetof(r3_vertex, Normal));
 glEnableVertexAttribArray(2);
 glVertexAttribPointer(2, 4, GL_FLOAT, false, sizeof(r3_vertex), (void *)offsetof(r3_vertex, Color));
}

// NOTE: Uploads the hand-built islands' still geometry once; drawn every frame by R3EndFrame.
internal void
R3UploadHeroCache(r3_state *R3, r3_mesh *Opaque, r3_mesh *Translucent)
{
 if(!R3->GLReady) R3InitGL(R3);
 if(!R3->HeroVAO) R3InitHeroGL(R3);
 glBindVertexArray(R3->HeroVAO);
 glBindBuffer(GL_ARRAY_BUFFER, R3->HeroVBO);
 u32 Total = Opaque->Count + Translucent->Count;
 glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(Total*sizeof(r3_vertex)), 0, GL_STATIC_DRAW);
 glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(Opaque->Count*sizeof(r3_vertex)), Opaque->Vertices);
 glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)(Opaque->Count*sizeof(r3_vertex)), (GLsizeiptr)(Translucent->Count*sizeof(r3_vertex)), Translucent->Vertices);
 R3->HeroOpaqueCount = Opaque->Count;
 R3->HeroTranslucentCount = Translucent->Count;
}

// NOTE: Start a 3D frame: camera matrices and empty meshes.
internal void
R3BeginFrame(r3_state *R3, arena *Arena, v2 ViewportDim)
{
 R3->ViewportDim = ViewportDim;
 R3->Sky = R3MeshAlloc(Arena, R3_MaxSkyVertices);
 R3->Opaque = R3MeshAlloc(Arena, R3_MaxOpaqueVertices);
 R3->Translucent = R3MeshAlloc(Arena, R3_MaxTranslucentVertices);

 f32 Aspect = ViewportDim.X/Max(ViewportDim.Y, 1.f);
 R3->Eye = R3CameraEye(&R3->Camera);
 m4 View = M4LookAt(R3->Eye, R3->Camera.Target, V3(0.f, 1.f, 0.f));
 m4 Projection = M4Perspective(50.f*Pi32/180.f, Aspect, .5f, 600.f);
 // Lens shift: moves the picture's centre by LensShift (in -1..1 screen units) without
 // turning the camera, so the map is centred in the part of the window the UI leaves free.
 Projection.e[2][0] = -R3->LensShift.X;
 Projection.e[2][1] = -R3->LensShift.Y;
 R3->ViewProj = M4Mul(Projection, View);

 R3BuildSky(R3, Aspect);
}

internal void
R3SetMatrix(r3_state *R3, m4 *Matrix)
{
 gl_int Location = glGetUniformLocation(R3->Program, "U_ViewProj");
 glUniformMatrix4fv(Location, 1, false, &Matrix->e[0][0]);
}

// NOTE: Draws the sky, the opaque and then the translucent triangles, then restores the
// state the rectangle renderer (drawn after us) depends on.
internal void
R3EndFrame(r3_state *R3, gl_render_state *RectRender)
{
 if(!R3->GLReady)
 {
  R3InitGL(R3);
 }

 u32 SkyCount = R3->Sky.Count;
 u32 OpaqueCount = R3->Opaque.Count;
 u32 TranslucentCount = R3->Translucent.Count;
 u32 TotalCount = SkyCount + OpaqueCount + TranslucentCount;

 glUseProgram(R3->Program);
 glBindVertexArray(R3->VAO);
 glBindBuffer(GL_ARRAY_BUFFER, R3->VBO);

 glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(TotalCount*sizeof(r3_vertex)), 0, GL_STREAM_DRAW);
 glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(SkyCount*sizeof(r3_vertex)), R3->Sky.Vertices);
 glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)(SkyCount*sizeof(r3_vertex)),
                 (GLsizeiptr)(OpaqueCount*sizeof(r3_vertex)), R3->Opaque.Vertices);
 glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)((SkyCount + OpaqueCount)*sizeof(r3_vertex)),
                 (GLsizeiptr)(TranslucentCount*sizeof(r3_vertex)), R3->Translucent.Vertices);

 // Uniforms
 {
  v3 LightDir = R3Scale(R3Normalize(V3(R3_SunDirX, R3_SunDirY, R3_SunDirZ)), -1.f);
  v4 Fog = R3Color(R3_ColorFog);
  glUniform3f(glGetUniformLocation(R3->Program, "U_LightDir"), LightDir.X, LightDir.Y, LightDir.Z);
  glUniform3f(glGetUniformLocation(R3->Program, "U_SunColor"), R3_SunColorR, R3_SunColorG, R3_SunColorB);
  glUniform3f(glGetUniformLocation(R3->Program, "U_AmbientColor"), R3_AmbientR, R3_AmbientG, R3_AmbientB);
  glUniform3f(glGetUniformLocation(R3->Program, "U_CameraPos"), R3->Eye.X, R3->Eye.Y, R3->Eye.Z);
  glUniform3f(glGetUniformLocation(R3->Program, "U_FogColor"), Fog.X, Fog.Y, Fog.Z);
  glUniform1f(glGetUniformLocation(R3->Program, "U_FogStart"), R3_FogStart);
  glUniform1f(glGetUniformLocation(R3->Program, "U_FogEnd"), R3_FogEnd);
 }

 glDisable(GL_CULL_FACE);
 glEnable(GL_BLEND);
 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

 //- Sky
 {
  m4 Identity = M4Identity();
  R3SetMatrix(R3, &Identity);
  glDisable(GL_DEPTH_TEST);
  glDrawArrays(GL_TRIANGLES, 0, (gl_int)SkyCount);
 }

 //- World
 R3SetMatrix(R3, &R3->ViewProj);
 glClear(GL_DEPTH_BUFFER_BIT);
 glEnable(GL_DEPTH_TEST);
 glDepthFunc(GL_LESS);
 glDepthMask(true);
 glDrawArrays(GL_TRIANGLES, (gl_int)SkyCount, (gl_int)OpaqueCount);
 b32 HeroCache = (R3->HeroVAO && R3->HeroCacheValid);
 if(HeroCache)
 {
  glBindVertexArray(R3->HeroVAO);
  glDrawArrays(GL_TRIANGLES, 0, (gl_int)R3->HeroOpaqueCount);
 }

 glDepthMask(false);
 if(HeroCache)
 {
  glDrawArrays(GL_TRIANGLES, (gl_int)R3->HeroOpaqueCount, (gl_int)R3->HeroTranslucentCount);
  glBindVertexArray(R3->VAO);
 }
 glDrawArrays(GL_TRIANGLES, (gl_int)(SkyCount + OpaqueCount), (gl_int)TranslucentCount);

 //- Restore what the rectangle renderer expects
 glDepthMask(true);
 glDisable(GL_DEPTH_TEST);
 glUseProgram(RectRender->RectShader);
 glBindVertexArray(RectRender->VAOs[0]);
 glBindBuffer(GL_ARRAY_BUFFER, RectRender->VBOs[0]);
}
