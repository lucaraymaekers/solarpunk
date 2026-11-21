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
};

global_variable product *AllProducts;

#endif //SOLARPUNK_H
