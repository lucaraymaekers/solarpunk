/* date = November 21st 2026 3:30 pm */

#ifndef SOLARPUNK_RENDER3D_H
#define SOLARPUNK_RENDER3D_H

//~ Constants
#define R3_MaxIslands 64

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
 v2 LastMousePos;
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
};

#endif //SOLARPUNK_RENDER3D_H
