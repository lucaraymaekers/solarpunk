/* date = November 21st 2026 3:30 pm */

#ifndef SOLARPUNK_RENDER3D_H
#define SOLARPUNK_RENDER3D_H

//~ Constants
#define R3_MaxIslands 64
#define R3_MaxCargoSigns 256
#define R3_MaxUIRects 20
#define R3_MaxRails 256
#define R3_RailSegments 20   // pieces a rail is built from
#define R3_SummaryProductBase 100

// NOTE: How an island is doing, shown as the colour of its ring and label dot.
enum island_status
{
 IslandStatus_Ok,       // knows a supplier for everything it needs
 IslandStatus_Waiting,  // has not heard of a supplier for some need yet
 IslandStatus_Short,    // went without some food in the last Food_RecentRounds rounds
 IslandStatus_Count
};
typedef enum island_status island_status;

//~ Types
// NOTE: Column-major 4x4 matrix, e[Column][Row], the layout OpenGL expects.
typedef struct m4 m4;
struct m4
{
 f32 e[4][4];
};

typedef struct r3_vertex r3_vertex;
struct r3_vertex
{
 v3 Pos;
 v3 Normal;   // zero = unlit (sky, glowing things)
 v4 Color;
};

// NOTE: A growable-in-frame list of triangles. Lives in the frame arena, rebuilt every frame.
typedef struct r3_mesh r3_mesh;
struct r3_mesh
{
 r3_vertex *Vertices;
 u32 Count;
 u32 Capacity;
};

typedef struct r3_camera r3_camera;
struct r3_camera
{
 b32 Initialized;
 f32 Yaw;        // radians around the up axis
 f32 Pitch;      // radians above the horizon
 f32 Distance;   // from the target
 v3 Target;
 b32 Rotating;   // middle mouse button held
 b32 Panning;    // right mouse button held
 b32 Flying;     // moving smoothly to FlyTarget / FlyDistance
 v3 FlyTarget;
 f32 FlyDistance;
 v2 LastMousePos;
};

// NOTE: A moving part of a hand-built island, replayed every frame while the rest of
// the island stays cached on the GPU (see solarpunk_hero_island.c).
#define R3_MaxHeroCommands 2048
enum hero_command_kind
{
 HeroCmd_Rotor,       // windmill rotor: P = hub, A = phase
 HeroCmd_Sails,       // tower mill sails: P = hub, A = phase
 HeroCmd_WaterWheel,  // P = centre, A = radius
 HeroCmd_Boat,        // P = hull centre, A = facing, B = phase, C = rower
 HeroCmd_Flames,      // P = campfire base
 HeroCmd_Crystal,     // knowledge crystal: P = centre, A = size
 HeroCmd_Ring,        // status/hover ring: P = island top, A = radius (0 = square slab)
 HeroCmd_Trawler,     // fishing boat with a cabin: P = hull centre, A = facing, B = phase
};

typedef struct hero_command hero_command;
struct hero_command
{
 u32 Kind;
 u32 Island;
 v3 P;
 f32 A;
 f32 B;
 f32 C;
};

typedef struct r3_state r3_state;
struct r3_state
{
 // GL objects, created once (they survive hot reloads, the GL context does not change).
 b32 GLReady;
 gl_uint Program;
 gl_uint VAO;
 gl_uint VBO;

 r3_camera Camera;
 v2 LensShift;   // see R3BeginFrame; eased towards the free part of the window

 // Time of day shown by the sky, 0..1 (0 = midnight, .5 = noon). Eased towards the
 // current round's time of day, see R3UpdateDayPhase. Visual only.
 f32 DayPhase;
 b32 DayPhaseSet;

 // Per frame
 m4 ViewProj;
 v3 Eye;
 v2 ViewportDim;
 r3_mesh Sky;
 r3_mesh Opaque;
 r3_mesh Translucent;

 // Islands this frame, in the same order as the island list (and the broadcast tables).
 u32 IslandCount;
 v3 IslandTop[R3_MaxIslands];      // centre of the grassy top
 f32 IslandRadius[R3_MaxIslands];
 v2 IslandScreen[R3_MaxIslands];   // label anchor in window pixels
 b32 IslandVisible[R3_MaxIslands]; // false when behind the camera

 // Island under the mouse, found by its projected position. -1 = none.
 s32 HoveredIdx;

 // The hand-built hero island (the one nearest the map centre). -1 = none.
 s32 HeroIdx;
 // How each island is drawn: W3Style_Normal, W3Style_Hero or W3Style_Themed (the
 // hero island's neighbours, built the same way with their own contents).
 u8 IslandStyle[R3_MaxIslands];
 // Seconds, for windmills, water wheels, boats and the campfire.
 f32 AnimTime;

 // Hand-built islands: their still geometry is cached on the GPU and only rebuilt when
 // the islands change (HeroCacheKey); the moving parts are replayed from HeroCommands
 // every frame.
 gl_uint HeroVAO;
 gl_uint HeroVBO;
 b32 HeroCacheValid;
 u64 HeroCacheKey;
 u32 HeroOpaqueCount;
 u32 HeroTranslucentCount;
 u32 HeroCommandCount;
 hero_command HeroCommands[R3_MaxHeroCommands];
 f32 IslandKnown[R3_MaxIslands];   // knowledge fraction per island, this frame
 // Set only while the cache is being built: where the islands put their geometry.
 r3_mesh *HeroOpaque;
 r3_mesh *HeroTranslucent;
 b32 HeroRecording;

 // Island status (island_status) and world totals, computed every frame.
 u32 IslandStatus[R3_MaxIslands];
 u64 TotalProduced[ProductKind_Count];
 u64 TotalNeeded[ProductKind_Count];
 u64 TotalSpoiled[ProductKind_Count];  // per day, made but gone bad (solarpunk_food_app.c)
 u64 RecentShort[ProductKind_Count];   // units nobody had to eat, last Food_RecentRounds rounds (all islands)

 // 2D overlay. The help card blocks the rest of the UI and the 3D view while open;
 // it starts closed (the "?" button opens it).
 b32 HelpOpen;
 s32 TextSizeIdxPlusOne;  // 0 = default size (see UiTextSizes)
 b32 HelpShownOnce;
 v4 HelpButtonRect;   // the "?" button, which toggles the card
 v4 TopBarRect;       // this frame
 v4 ShortStripRect;   // products islands went without, under the top bar (this frame)

 // Cluster whose island names are shown (0 = none, only the cluster names).
 s32 FocusedClusterPlusOne;
 // Right click without dragging unfocuses it (DrawWorld3D).
 v2 RightClickStart;
 b32 RightClickArmed;
 
 // Summary bar dropdown: 0 = closed, 1 + island_status = islands with that status,
 // R3_SummaryProductBase + product kind = islands that need/produce that product.
 s32 SummaryOpen;
 v4 ClustersButtonRect; // "All clusters" in the top bar (this frame)
 v4 SummaryBarRect;   // the status chips (this frame); clicks there keep the dropdown open
 v4 PopupRect;        // where the dropdown was drawn (zero when closed)
 s32 PopupHoverPlusOne; // island row under the mouse in the dropdown
 // Screen areas covered by UI this frame (labels avoid them; clicks and hovers
 // there do not reach the 3D view).
 v4 UIRects[R3_MaxUIRects];
 u32 UIRectCount;

 // Rails, one per link (From < To, island indices). Lift raises a whole rail so it
 // clears the rails it crosses or runs beside and the islands it passes over; solved
 // in W3SolveRails and only redone when the islands or links change (RailKey).
 u32 RailCount;
 u8 RailFrom[R3_MaxRails];
 u8 RailTo[R3_MaxRails];
 f32 RailLift[R3_MaxRails];
 u64 RailKey;
 // The still parts of all rails (beams, frames, pads, stations) cached on the GPU and
 // rebuilt only when RailKey changes; the glowing lines are pushed every frame.
 gl_uint RailVAO;
 gl_uint RailVBO;
 b32 RailCacheValid;
 u64 RailCacheKey;
 u32 RailOpaqueCount;
 u32 RailTranslucentCount;
 // Kept with the cache, for the glowing lines pushed every frame (W3PushRailGlow):
 // the middle line of every rail and its frame (Side, Up) per piece, and the frame at
 // both stations (P, Dir, Side, Up).
 v3 RailGlowPoint[R3_MaxRails][R3_RailSegments + 1];
 v3 RailGlowSide[R3_MaxRails][R3_RailSegments];
 v3 RailGlowUp[R3_MaxRails][R3_RailSegments];
 v3 RailStation[R3_MaxRails][2][4];

 // With an island selected: bit i set = island i does not trade with it and is
 // drawn greyed out (W3DimMask). 0 = nothing greyed.
 u64 IslandDimMask;

 // Goods trains this frame, for the floating cargo signs (V3DrawCargoSigns):
 // where the sign hangs and which foods (bit per product kind) the train carries.
 u32 CargoSignCount;
 v3 CargoSignPos[R3_MaxCargoSigns];
 u32 CargoSignKinds[R3_MaxCargoSigns];
 u16 CargoSignFrom[R3_MaxCargoSigns];
 u16 CargoSignTo[R3_MaxCargoSigns];
};

#endif //SOLARPUNK_RENDER3D_H
