//-
// SolarPunk
//-

//~ Includes
#define BASE_EXTERNAL_LIBS 1
#define BASE_NO_ENTRYPOINT 1
#include "base/base.h"
#include "base/base.c"

//- H Includes
#include "rl/generated/rl_tables.meta.c"
#include "rl/generated/rl_ui.meta.c"
#include "rl/generated/rl_render.meta.c"
#include "rl/generated/rl_font.meta.c"
#include "solarpunk/generated/solarpunk.meta.c"
#include "rl/rl_libs.h"
#include "rl/rl_font.h"
#include "rl/rl_platform.h"
#include "rl/rl_random.h"
#include "rl/rl_gl.h"
#include "rl/rl_renderer.h"
#include "rl/rl_ui.h"
#include "rl/rl_midi.h"
#include "rl/rl_profile.h"   // Prof_TimeScope blocks; compiled out unless RL_PROFILE

//- C Includes
#include "rl/rl_renderer.c"
#include "rl/rl_ui.c"
#include "rl/rl_widgets.c"

#include "solarpunk/solarpunk_broadcast.h"
#include "solarpunk/solarpunk_food.h"
#include "solarpunk/solarpunk_routing.h"
#include "solarpunk/solarpunk_render3d.h"
#include "solarpunk/solarpunk_panel.h"
#include "solarpunk/solarpunk.h"
#include "solarpunk/solarpunk_food.c"
#include "solarpunk/solarpunk_world.c"
#include "solarpunk/solarpunk_supply.c"
#include "solarpunk/solarpunk_world_gen.c"
#include "solarpunk/solarpunk_adapt.c"
#include "solarpunk/solarpunk_food_app.c"
#include "solarpunk/solarpunk_routing.c"
#include "solarpunk/solarpunk_distribution.c"
#include "solarpunk/solarpunk_broadcast.c"
#include "solarpunk/solarpunk_graph.c"
#include "solarpunk/solarpunk_ui.c"
#include "solarpunk/solarpunk_broadcast_view.c"
#include "solarpunk/solarpunk_render3d.c"
#include "solarpunk/solarpunk_world3d.c"
#include "solarpunk/solarpunk_hero_island.c"
#include "solarpunk/solarpunk_forest_island.c"
#include "solarpunk/solarpunk_fishing_island.c"
#include "solarpunk/solarpunk_beach_island.c"
#include "solarpunk/solarpunk_ruins_island.c"
#include "solarpunk/solarpunk_ice_island.c"
#include "solarpunk/solarpunk_crystal_island.c"
#include "solarpunk/solarpunk_view3d.c"
#include "solarpunk/solarpunk_panel.c"

//~ Macros
#if OS_WINDOWS
# pragma comment(linker, "/export:GetAudioSamples")
# pragma comment(linker, "/export:UpdateAndRender")
#endif

//~ Functions
UPDATE_AND_RENDER(UpdateAndRender)
{
 b32 ShouldQuit = false;

 v2 BufferDim = V2V2S32(Buffer->Dim);

 // Globals
 {
  GlobalDebuggerIsAttached = Memory->IsDebuggerAttached;
  ExeDirPath = Memory->ExeDirPath;
  ThreadContext = Memory->ThreadCtx;
 }

 //- Initialization
 arena *PermanentArena;
 {
  arena Arena = {0};
  Arena.Base = Memory->Memory;
  Arena.Size = (u64)-1;
  PermanentArena = PushArena(&Arena, 0, false);
  PermanentArena->Size = (Memory->MemorySize - Arena.Pos);
  PermanentArena->Pos = 0;
 }

 app_state *App = PushArray(PermanentArena, app_state, 1);

 // Arenas init
 {
  App->FontAtlas.Arena = PushArena(PermanentArena, MB(100), false);
  FrameArena = PushArena(PermanentArena, MB(380), true);   // up to ~320 MB of 3D meshes on a frame that rebuilds the island cache
  SetStringsScratch(FrameArena);

  App->LinksArena = PushArena(PermanentArena, MB(16), false);
  App->IslandsArena = PushArena(PermanentArena, MB(16), false);
  App->ProductsArena = PushArena(PermanentArena, MB(16), false);
 }

 if(Memory->Reloaded)
 {
  gladLoaderLoadGL();
 }


 if(!Memory->Initialized)
 {
  // Font init
  {
   InitFontFromKind(&App->Font, FontKind_Text);
   // Icons: solarpunk_icons.ttf next to the exe (copied from data/, like the other fonts).
   InitFont(&App->IconFont, PathFromExe(S8("solarpunk_icons.ttf")));
   App->IconFont.FirstCodepoint = Icon_First;
   App->IconFont.CodepointCount = Icon_Count;
   font *Fonts[] = {&App->Font, &App->IconFont};
   RenderBuildAtlas(&App->FontAtlas, ArrayCount(Fonts), Fonts, 20.f);
  }
  RenderInit(&App->Render);

  // Start paused: round 1 is set up, Play (or Up) lets time run at 1x.
  App->Broadcast.SpeedOffset = -Broadcast_DefaultSpeedIdx;
  App->Broadcast.SpeedOffsetBeforePause = 0;
  // Start with the islands panel closed (the top bar button opens it).
  App->Panel.Hidden = true;

  Memory->Initialized = true;
 }

 RenderBeginFrame(FrameArena, 0, 0, V2Arg(Buffer->Dim));

 //- Update
 {
  // TODO: Replace with real island generation/loading.
  // NOTE: BuildHardcodedWorld(App) builds the small 5-island world instead (2D view only:
  // it sets no MapPos, so the 3D view would stack its islands).
  Prof_TimeScope("BuildGeneratedWorld") { BuildGeneratedWorld(App, WorldGen_IslandCount); }
  Prof_TimeScope("BroadcastUpdate") { BroadcastUpdate(App, Input); }
 }

 //- Rendering
 {
  RenderClear();
  // NOTE: DrawIslandGraph(App, BufferDim) is the old 2D view.
  Prof_TimeScope("DrawWorld3D") { DrawWorld3D(App, Input, BufferDim); }
  Prof_TimeScope("DrawIslandPanel") { DrawIslandPanel(App, Input, BufferDim); }
  Prof_TimeScope("Popups") 
  {
   DrawSummaryPopup(App, Input, BufferDim);
   DrawIslandTooltip(App, Input);
   DrawHelpCard(App, Input, BufferDim);
   UiDrawHint(App, BufferDim);
  }
  Prof_TimeScope("RenderDrawAllRectangles") { RenderDrawAllRectangles(&App->Render, BufferDim, &App->FontAtlas); }
 }

 return ShouldQuit;
}

GET_AUDIO_SAMPLES(GetAudioSamples)
{

}