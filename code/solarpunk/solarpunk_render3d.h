/* date = November 21st 2026 3:30 pm */

#ifndef SOLARPUNK_RENDER3D_H
#define SOLARPUNK_RENDER3D_H

//~ Constants
#define R3_MaxIslands 64
#define R3_MaxUIRects 20
#define R3_SummaryProductBase 100
#define R3_SummaryClusters    99   // the "All clusters" list

// NOTE: How an island is doing, shown as the colour of its ring and label dot.
enum island_status
{
 IslandStatus_Ok,       // knows a supplier for everything it needs
 IslandStatus_Waiting,  // has not heard of a supplier for some need yet
 IslandStatus_Short,    // knows its suppliers, but needs a product that is short worldwide
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

 // 2D overlay. The help card blocks the rest of the UI and the 3D view while open;
 // it opens by itself on the first start.
 b32 HelpOpen;
 s32 TextSizeIdxPlusOne;  // 0 = default size (see UiTextSizes)
 b32 HelpShownOnce;
 v4 HelpButtonRect;   // the "?" button, which toggles the card
 v4 TopBarRect;       // this frame
 v4 ShortStripRect;   // products short worldwide, under the top bar (this frame)

 // Cluster whose island names are shown (0 = none, only the cluster names).
 s32 FocusedClusterPlusOne;
 
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
};

#endif //SOLARPUNK_RENDER3D_H
