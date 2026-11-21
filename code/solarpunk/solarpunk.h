/* date = November 21st 2026 10:22 am */

#ifndef SOLARPUNK_H
#define SOLARPUNK_H

//~ Types
typedef struct product product;
struct product
{
 product *Next;
 product *Prev;

 product_kind Kind;
 u64 Amount;
};

typedef struct link link;
typedef struct island island;
struct island
{
 island *Next;
 island *Prev;

 link *FirstLink;
 link *LastLink;

 product *FirstProduct;
 product *LastProduct;

 product *FirstRequirement;
 product *LastRequiremement;

 str8 Name;

 // Graph view
 v2 Pos;
 v2 MapPos; // 0..1 position on the map, set by the world generator
};

struct link
{
 link *Next;
 link *Prev;

 island *Start;
 island *Dest;
 s32 Cost;
};

typedef struct app_state app_state;
struct app_state
{
 // Rendering
 font Font;
 font_atlas FontAtlas;
 gl_render_state Render;

 // App state
 island *FirstIsland;
 island *LastIsland;

 arena *LinksArena;
 arena *IslandsArena;
 arena *ProductsArena;
 b32 HasMapLayout; // islands carry MapPos; the graph uses it instead of a circle

 // Announcements of production and needs between islands
 broadcast_state Broadcast;

 // 3D view: camera and GL objects
 r3_state R3;
 
 // Islands panel on the right
 island_panel_state Panel;
};

global_variable product *AllProducts;

#endif //SOLARPUNK_H
