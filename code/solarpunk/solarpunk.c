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

//- C Includes 
#include "rl/rl_renderer.c"
#include "rl/rl_ui.c"
#include "rl/rl_widgets.c"

#include "solarpunk/solarpunk_broadcast.h"
#include "solarpunk/solarpunk_food.h"
#include "solarpunk/solarpunk.h"
#include "solarpunk/solarpunk_food.c"
#include "solarpunk/solarpunk_world.c"
#include "solarpunk/solarpunk_world_gen.c"
#include "solarpunk/solarpunk_food_app.c"
#include "solarpunk/solarpunk_broadcast.c"
#include "solarpunk/solarpunk_graph.c"
#include "solarpunk/solarpunk_broadcast_view.c"

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
  FrameArena = PushArena(PermanentArena, MB(64), true);
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
   font *Fonts[] = {&App->Font};
   InitFontFromKind(&App->Font, FontKind_Text);
   RenderBuildAtlas(&App->FontAtlas, ArrayCount(Fonts), Fonts, 20.f);
  }
  RenderInit(&App->Render);
  
  Memory->Initialized = true;
 }
 
 RenderBeginFrame(FrameArena, 0, 0, V2Arg(Buffer->Dim));
 
 //- Update 
 {
  // TODO: Replace with real island generation/loading.
  // NOTE: BuildHardcodedWorld(App) builds the small 5-island world instead.
  BuildGeneratedWorld(App, WorldGen_IslandCount);
  BroadcastUpdate(App, Input);
 }
 
 //- Rendering 
 { 
  RenderClear();
  DrawIslandGraph(App, BufferDim);
  DrawBroadcastControls(App, Input);
  RenderDrawAllRectangles(&App->Render, BufferDim, &App->FontAtlas);
 }
 
 return ShouldQuit;
}

GET_AUDIO_SAMPLES(GetAudioSamples)
{
 
}