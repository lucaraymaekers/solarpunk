//-
// Platform layer
//-

//~ Libraries 
#define BASE_EXTERNAL_LIBS 1

#define BASE_CONSOLE_APPLICATION RL_PLATFORM_INTERNAL
#include "base/base.h"
#include "base/base.c"

#include "rl/rl_libs.h"
#include "rl/rl_midi.h"

#include "rl/generated/rl_tables.meta.c"
#include "rl/generated/rl_render.meta.c"
#include "rl/generated/rl_font.meta.c"
#include "rl/generated/rl_ui.meta.c"
#include "rl/rl_font.h"
#include "rl/rl_platform.h"
#include "rl/rl_gl.h"
#include "rl/rl_renderer.h"
#include "rl/rl_ui.h"
#include "rl/rl_renderer.c"
#include "rl/rl_ui.c"
#include "rl/rl_widgets.c"

#if OS_LINUX
# include "rl/rl_platform_linux.c"
#elif OS_WINDOWS
# include "rl/rl_platform_windows.c"
#endif

//~ Recording 
typedef struct platform_replay platform_replay;
struct platform_replay
{
 void *Buffer;
 u64 PlayingPos;
 u64 RecordingSize;
 
 b32 IsRecording;
 
 b32 IsStepping;
 b32 IsLooping;
 b32 IsSkipping;
 
 u64 StepIdx;
 u64 StepCount;
 u64 StepTarget;
};

internal void
ReplayStopRecording(platform_replay *Replay, app_memory *Memory)
{
 if(Replay->IsRecording)
 {
  Replay->IsRecording = false;
 }
}

internal void
ReplayLoadMemory(platform_replay *Replay, app_memory *Memory)
{
 if(Replay->RecordingSize)
 {
  MemoryCopy(Memory->Memory, Replay->Buffer, Memory->MemorySize);
  Replay->PlayingPos = Memory->MemorySize;
  Replay->StepIdx = 0;
 }
}

internal void
ReplayRecordMemory(platform_replay *Replay, app_memory *Memory)
{
 f64 Start = OS_GetWallClock();
 MemoryCopy(Replay->Buffer, Memory->Memory, Memory->MemorySize);
 f64 End = OS_GetWallClock();
 Log("Memory copy: %.6f\n", End - Start);
 Replay->RecordingSize = Memory->MemorySize;
}

internal void
ReplayToggleRecording(platform_replay *Replay, app_memory *Memory, b32 KeepRecordingIfStarted)
{
 if(!Replay->IsRecording)
 {
  ReplayRecordMemory(Replay, Memory);
  
  Replay->StepIdx = 0;
  Replay->StepTarget = 0;
  Replay->PlayingPos = 0;
  
  Replay->IsLooping = false;
  Replay->IsStepping = false;
  
  Replay->IsRecording = KeepRecordingIfStarted;
 }
 else
 {
  ReplayStopRecording(Replay, Memory);
 }
}

internal void
ReplayToggleLooping(platform_replay *Replay, app_memory *Memory)
{
 if(Replay->RecordingSize > 0)
 {
  ReplayStopRecording(Replay, Memory);
  Replay->IsLooping = !Replay->IsLooping;
  
  if(Replay->IsLooping)
  {
   Replay->IsStepping = true;
  }
  
 }
}

internal void
ReplayStep(platform_replay *Replay, app_memory *Memory)
{
 if(Replay->PlayingPos == 0)
 {
  ReplayLoadMemory(Replay, Memory);
  
  Replay->IsStepping = (Replay->RecordingSize != Replay->PlayingPos);
 }
 else
 {
  Replay->IsStepping = true;
 }
 
 if(Replay->StepCount == 0)
 {
  Replay->IsStepping = false;
 }
 
 if(Replay->IsStepping)
 {
  ReplayStopRecording(Replay, Memory);
  
  Replay->IsLooping = false;
 }
}

internal void
ReplayStepNext(platform_replay *Replay)
{
 if(Replay->RecordingSize > 0)
 {
  if(Replay->PlayingPos == 0)
  {
   Replay->StepTarget = 0;
  }
  else
  {
   Replay->StepTarget = (Replay->StepIdx + 1)%Replay->StepCount;
  }
 }
}

//~ UI (Debug) 
internal void DebugSpacer(void) { UI_Spacer(UI_SizeEm(.2f, 1.f)); }

internal ui_box *
PUI_Label(s32 Flags, str8 String)
{
 ui_box *Result = UI_AddBox(String, (UI_BoxFlag_Clip|
                                     UI_BoxFlag_DrawDisplayString|
                                     UI_BoxFlag_DrawBackground|
                                     UI_BoxFlag_CenterTextVertically|
                                     Flags));
 return Result;
}

internal ui_box *
PUI_Labelf(s32 Flags, char *Format, ...)
{
 ui_box *Result = UI_NilBox;
 str8 String = {0};
 
 va_list Args;
 va_start(Args, Format);
 String = Str8VFmt(Format, Args);
 
 Result = PUI_Label(Flags, String);
 
 return Result;
}

global_variable v4 Color_Disabled = {U32ToV4Arg(0xFF616E88)};

internal v4
GetDisabledColorCondition(b32 Condition)
{
 v4 BackgroundColor = UI_State->BackgroundColorTop->Value;
 if(Condition)
 {
  BackgroundColor = V4(U32ToV4Arg(0xFF616E88));
 }
 
 return BackgroundColor;
}


internal void
ResetButtons(app_button_state *NewButtons, app_button_state *OldButtons, u64 Count)
{
 for EachIndex(Idx, Count)
 {
  app_button_state *NewButton = NewButtons + Idx;
  app_button_state *OldButton = OldButtons + Idx;
  NewButton->EndedDown = OldButton->EndedDown;
  NewButton->HalfTransitionCount = 0;
 }
}

global_variable ui_size GlobalPadding; 

internal b32
DisabledButton(str8 Text, b32 Disabled)
{
 b32 Result = UI_Button(.Text = Text, 
                        .Disabled = Disabled, 
                        .Padding = GlobalPadding,
                        .CenterText = true,
                        .ClipSize = true).Pressed;
 Result = (Result && !Disabled);
 return Result;
}

#define ToggleButton(ButtonText, Toggled, ButtonColor, ...) \
UI_ButtonWithToggle(S8(ButtonText), Toggled, GlobalPadding, ButtonColor, .ClipSize = true, ##__VA_ARGS__).OneClicked

//~ Entrypoint
BASE_ENTRY_POINT()
{
 if(LaneIndex() == 0)
 {
  OS_ProfileInit("S");
  
  u64 PlatformMemorySize = GB(4);
  u64 AppMemorySize = GB(1);
  
  // NOTE(luca): Total memory for platform and app.
  u64 PermanentMemoryOffset = TB(2);
  u64 ReadOnlyMemoryOffset = TB(3);
#if !RL_PLATFORM_INTERNAL || ASAN_ENABLED
  PermanentMemoryOffset = 0;
  ReadOnlyMemoryOffset = 0;
#endif
  
  arena *PermanentArena = ArenaAlloc(.Size = PlatformMemorySize, .Offset = PermanentMemoryOffset);
  FrameArena = PushArena(PermanentArena, ArenaAllocDefaultSize, false);
  
  SetStringsScratch(FrameArena);
  
#if ASAN_ENABLED
  Log("Asan is enabled.\n");
#endif
  
  OS_ProfileAndPrint("Memory");
  
  b32 *Running = PushArray(PermanentArena, b32, 1);
  *Running = true;
  
  s32 WindowBorderSize = 2;
  app_offscreen_buffer WindowBuffer = {0};
  WindowBuffer.Dim.X = 1600;
  WindowBuffer.Dim.Y = 900;
  WindowBuffer.BytesPerPixel = 4;
  WindowBuffer.Pitch = WindowBuffer.BytesPerPixel*WindowBuffer.Dim.X;
  WindowBuffer.Pixels = PushArray(PermanentArena, u8, (u64)(WindowBuffer.Pitch*WindowBuffer.Dim.Y));
  
  app_sound_buffer SoundBuffer = {0};
  
  // TODO(luca): Detect refresh rate
  f32 GameUpdateHz = RL_PLATFORM_FORCE_UPDATE_HZ;
  f32 TargetSecondsPerFrame = 1.0f/GameUpdateHz; 
  
  P_context PlatformContext = P_Init(PermanentArena, &WindowBuffer, &SoundBuffer, Running, Stringify(RL_PLATFORM_WINDOW_NAME), GameUpdateHz);
  
  OS_ProfileAndPrint("Context");
  
  if(!PlatformContext)
  {
   ErrorLog("Could not initialize graphical context, running in headless mode.\n");
  }
  
  // Get ExeDirPath
  {
   str8 FileName = S8FromCString(Params->Args[0]);
   u64 OnePastLastSlash = S8FindChar(FileName, OS_SlashChar, false, true);
   ExeDirPath = Str8Fmt("%S", S8To(FileName, OnePastLastSlash));
  }
  
  app_memory AppMemory = {0};
  AppMemory.MemorySize = AppMemorySize;
  AppMemory.Memory = PushArray(PermanentArena, u8, AppMemory.MemorySize);
  AppMemory.ThreadCtx = ThreadContext;
  AppMemory.ExeDirPath = ExeDirPath;
  AppMemory.IsDebuggerAttached = GlobalDebuggerIsAttached;
#if OS_WINDOWS
  AppMemory.PerfCountFrequency = GlobalPerfCountFrequency;
#endif
  
  AppMemory.PlatformMIDIGetDevices = P_MIDIGetDevices;
  AppMemory.PlatformMIDISend = P_MIDISend;
  AppMemory.PlatformMIDIListen = P_MIDIListen;
  
  app_input _Input[3] = {0};
  app_input *NewInput = &_Input[0];
  app_input *OldInput = &_Input[1];
  app_input *ReplayInput = &_Input[2];
  
  f64 LastCounter = OS_GetWallClock();
  
  //- Sound 
  
  umm BytesPerSample = sizeof(sample_format)*SoundBuffer.ChannelCount;
  
  //- Code
  app_code Code = {0};
  
  b32 CurrentOSLinux = false;
#if OS_LINUX
  CurrentOSLinux = true;
#endif
  
  str8 LibraryPath = Str8Fmt(Stringify(RL_PLATFORM_APP_NAME) ".%s", (CurrentOSLinux ? "so" : "dll"));
  StringsScratchTemp(PermanentArena)
   Code.LibraryPath = PathFromExe(LibraryPath);
  
  //- Debug UI 
  b32 ShowDebugUI = false;
  
#if RL_PLATFORM_DEBUG_UI
  v2 DebugUIOffset = {0};
  v2 DebugUIDragOffset = {0};
  f32 DebugHeightPx = 20.f;
  gl_render_state DebugRender = {0};
  font_atlas DebugRenderAtlas = {0};
  font TextFont = {0};
  font IconsFont = {0};
  ui_box *DebugRootBox = 0;
  {
   InitFontFromKind(&TextFont, FontKind_Text);
   InitFontFromKind(&IconsFont, FontKind_Icons);
  }
  
  gladLoaderLoadGL();
  
#if 1   
  GLint Major, Minor;
  glGetIntegerv(GL_MAJOR_VERSION, &Major);
  glGetIntegerv(GL_MINOR_VERSION, &Minor);
  Log("Loaded OpenGL %d.%d\n", Major, Minor);
#endif
  
  RenderInit(&DebugRender);
  DebugRenderAtlas.Arena = PushArena(PermanentArena, MB(150), false);
  // TODO(luca): Building this atlas is slow and tanks our startup time.  So we should speed it up by using a glyph cache.
  
  
  font *Fonts[] = {&TextFont, &IconsFont};
  RenderBuildAtlas(&DebugRenderAtlas, ArrayCount(Fonts), Fonts, DebugHeightPx);
  
  arena *ROArena = ArenaAlloc(.Offset = ReadOnlyMemoryOffset);
  
  // Init UI state
  {
   arena *UIArena = PushArena(PermanentArena, MB(64), false);
   UI_State = PushArray(UIArena, ui_state, 1);
   UI_InitState(UIArena);
   
   UI_State->Fonts[FontKind_Text] = &TextFont;
   UI_State->Fonts[FontKind_Icons] = &IconsFont;
   
   ui_box *Box = PushArray(ROArena, ui_box, 1);
   *Box = (ui_box){Box, Box, Box, Box, Box, Box, Box};
   UI_NilBox = Box;
   
   DebugRootBox = UI_BoxAlloc(UI_State->Arena);
  }
  
  OS_MarkReadonly(ROArena->Base, ROArena->Size);
  
  OS_ProfileAndPrint("Debug UI setup");
#endif
  
  platform_replay Replay = {0};
  
  b32 Paused = false;
  b32 Logging = false;
  
  // NOTE(luca): 20 minutes of gameplay, at 60Hz this is ~120MB
  u64 ReplayMaxRecordingFramesCount = (u64)GameUpdateHz * 60 * 10;
  u64 ReplayMaxBufferSize = AppMemory.MemorySize + (ReplayMaxRecordingFramesCount*sizeof(app_input));
  Replay.Buffer = PushArray(PermanentArena, u8, ReplayMaxBufferSize);
  u64 MaxReplaySlots = 4;
  u64 ReplaySlot = 0;
  
  OS_ProfileAndPrint("Misc");
  
  OS_ProfileInit("P");
  
  u64 FrameIdx = 0;
  f64 EndCounter = 0.f;
  f64 LastWorkMSPerFrame = 0.f;
  b32 CodeLoaded = false;
  while(*Running)
  {
   Scratch(FrameArena)
   {
#if RL_PLATFORM_DEBUG_UI
    RenderBeginFrame(FrameArena, 0, 0, WindowBuffer.Dim.X, WindowBuffer.Dim.Y);
    OS_ProfileAndPrint("Render Setup");
#endif
    
    if(!CodeLoaded)
    {
     P_LoadAppCode(PlatformContext, &Code, &AppMemory);
     CodeLoaded = true;
    }
    
#if RL_PLATFORM_HOT_RELOAD_CODE
    CodeLoaded = false;
#endif
    
    OS_ProfileAndPrint("Code");
    
    NewInput->PlatformWindowIsFocused = OldInput->PlatformWindowIsFocused;
    b32 PlatformWindowIsFocused;
    
    // Input
    {
     for EachElement(Idx, NewInput->Consumed)
     {
      NewInput->Consumed[Idx] = false;
     }
     NewInput->SkipRendering = false;
     NewInput->MIDI.EventCount = 0;
     MemoryZero(&NewInput->Text);
     
     ResetButtons(NewInput->Mouse.Buttons, OldInput->Mouse.Buttons, ArrayCount(NewInput->Mouse.Buttons));
     ResetButtons(NewInput->GameButtons, OldInput->GameButtons, ArrayCount(NewInput->GameButtons));
     ResetButtons(NewInput->MIDI.Buttons, OldInput->MIDI.Buttons, ArrayCount(NewInput->MIDI.Buttons));
     
     NewInput->dtForFrame = TargetSecondsPerFrame;
     NewInput->PlatformCursor = OldInput->PlatformCursor;
     NewInput->PlatformSetClipboard = OldInput->PlatformSetClipboard;
     
     NewInput->PlatformClipboard = OldInput->PlatformClipboard;
     NewInput->PlatformSetClipboard = OldInput->PlatformSetClipboard;
     NewInput->Mouse.Pos = OldInput->Mouse.Pos;
     NewInput->Mouse.Start = OldInput->Mouse.Start;
     
     P_ProcessMessages(PlatformContext, NewInput, &WindowBuffer, Running);
     
     PlatformWindowIsFocused = NewInput->PlatformWindowIsFocused;
     
     app_button_state MouseLeft = NewInput->Mouse.Buttons[PlatformMouseButton_Left];
     app_button_state MouseRight = NewInput->Mouse.Buttons[PlatformMouseButton_Right];
     
     b32 OnlyOnePressed = (!!MouseLeft.EndedDown ^ !!MouseRight.EndedDown); 
     
     if(OnlyOnePressed &&
        (WasPressed(MouseLeft) || WasPressed(MouseRight)))
     {                        
      NewInput->Mouse.Start = NewInput->Mouse.Pos;
     }
    }
    
    OS_ProfileAndPrint("Messages");
    
    for EachTextButton(Key, Id, NewInput)
    {
     b32 Alt = (Key->Modifiers == PlatformKeyModifier_Alt);
     b32 AltShift = (Key->Modifiers == (PlatformKeyModifier_Alt|
                                        PlatformKeyModifier_Shift));
     b32 AltControl = (Key->Modifiers == (PlatformKeyModifier_Alt|
                                          PlatformKeyModifier_Control));
     b32 AltControlShift = (Key->Modifiers == (PlatformKeyModifier_Alt|
                                               PlatformKeyModifier_Control|
                                               PlatformKeyModifier_Shift));
     
     b32 Handled = true;
     
     switch(ToLowercase((u8)Key->Codepoint))
     {
      default:
      {
       Handled = false;
      } break;
      
      case 'b':
      {
       if(0) {}
       else if(AltShift)
       {
        DebugBreak();
       }
       else Handled = false;
      } break;
      
      case 'p':
      {
       if(AltShift)
       {
        GlobalIsProfiling = !GlobalIsProfiling;
       }
       else Handled = false;
      } break;
      
      case 'd':
      {
       if(0) {}
       else if(Alt)
       {
        ShowDebugUI = !ShowDebugUI;
       }
       else Handled = false;
      } break;
      
      case 'g': 
      {
       if(0) {}
       else if(Alt)
       {
        Logging ^= 1;
       }
       else Handled = false;
      } break;
      
      case 'r':
      {
       if(0) {}
       if(Alt)
       { 
        ReplayToggleRecording(&Replay, &AppMemory, true);
       }
       else if(AltShift)
       {
        ReplayToggleRecording(&Replay, &AppMemory, false);
       }
       else Handled = false;
      } break;
      
      case 'l':
      {
       if(0) {}
       if(Alt)
       {
        ReplayToggleLooping(&Replay, &AppMemory);
       }
       else if(AltShift)
       {
        ReplayStep(&Replay, &AppMemory);
        Replay.IsSkipping = true;
        Replay.StepTarget = 0;
       }
       else Handled = false;
      } break;
      
      case 's':
      {
       if(0) {}
       else if(Alt)
       {
        ReplayStep(&Replay, &AppMemory);
        if(Replay.StepCount)
        {
         Replay.StepTarget = (Replay.StepIdx + 1)%Replay.StepCount;
        }
       }
       else if(AltControl)
       {
        Replay.IsStepping = !Replay.IsStepping;
       }
       else Handled = false;
      } break;
     }
     
     if(Handled)
     {
      RemoveTextButton(NewInput, Id);
     }
    }
    
#if RL_PLATFORM_DEBUG_UI
    if(Replay.IsStepping)
    {
     // NOTE(luca): Draw a Shadow cursor
     app_input *AppInput = (app_input *)((u8 *)Replay.Buffer + Replay.PlayingPos);
     
     v4 Dest = RectFromCenterDim(V2V2S32(AppInput->Mouse.Pos), V2(5.f, 5.f)); 
     DrawRect(Dest, V4F32(1.f), 0.f, 0.f, 0.f);
     DrawRect(Dest, Color_Black, 0.f, 1.f, 0.f);
    }
    
    if(ShowDebugUI)
    {
     UI_BeginFrame(&DebugRenderAtlas, FrameIdx, NewInput);
     UI_BeginLayout(DebugRootBox, WindowBuffer.Dim, DebugHeightPx);
     
     // UI
     platform_mouse_button_type ButtonType = PlatformMouseButton_Left;
     
     f32 ItemHeight = DebugHeightPx + 2.f*8.f;
     f32 ListWidth = 300.f;
     ui_size ItemPadding = UI_SizePx(5.f, 1.f);
     
     GlobalPadding = ItemPadding;
     
     b32 RecordingIsEmpty = (Replay.RecordingSize == 0);
     b32 RecordingHasNoSteps = (Replay.StepCount == 0);
     
     ui_box *List;
     
     UI_Column()
     {
      // For dragging Debug UI box
      {     
       f32 Size = DebugUIOffset.Y + DebugUIDragOffset.Y;
       UI_Spacer(UI_SizePx(Size, 1.f));
      }
      
      UI_Row()
      {
       // For dragging Debug UI box
       {     
        f32 Size = DebugUIOffset.X + DebugUIDragOffset.X;
        UI_Spacer(UI_SizePx(Size, 1.f));
       }
       
       UI_SemanticWidth(UI_SizePx(ListWidth, 1.f))
        UI_SemanticHeight(UI_SizeChildren(1.f))
        UI_BorderThickness(2.f)
        UI_BorderColor(Color_Snow2)
        UI_BackgroundColor(Color_Night0)
        List = UI_AddBox(S8("List"), 
                         UI_BoxFlag_Clip|
                         UI_BoxFlag_DrawBackground|
                         UI_BoxFlag_DrawBorders|
                         UI_BoxFlag_MouseClickable);
       
       UI_Push()
        UI_FillWidth()
        UI_SemanticHeight(UI_SizeChildren(1.f))
        UI_Row() UI_Padding(UI_SizePx(2.f, 1.f))
        UI_Column() UI_Padding(UI_SizePx(2.f, 1.f))
        UI_SemanticWidth(UI_SizePx(ListWidth, 1.f))
        UI_SemanticHeight(UI_SizePx(ItemHeight, 1.f))
       {
        
        {      
         ui_box *TopBar;
         UI_BackgroundColor(Color_Blue)
          UI_SemanticHeight(UI_SizePx(ItemHeight/2.f, 1.f))
          TopBar = UI_AddBox(S8("TopBar"), 
                             UI_BoxFlag_Clip|
                             UI_BoxFlag_MouseClickable|
                             UI_BoxFlag_DrawBorders|
                             UI_BoxFlag_DrawBackground|
                             UI_BoxFlag_DrawHotEffects|
                             UI_BoxFlag_DrawActiveEffects);
         
         for EachIndex(Axis, Axis2_Count)
         {
          if(UI_IsActive(TopBar, ButtonType))
          {
           DebugUIDragOffset.e[Axis] = (f32)(NewInput->Mouse.Pos.e[Axis] - NewInput->Mouse.Start.e[Axis]);
           UI_State->IsAnimating = false;
          }
          else if(TopBar->Released[PlatformMouseButton_Left])
          {
           DebugUIOffset.e[Axis] += DebugUIDragOffset.e[Axis];
           DebugUIOffset.e[Axis] = Clamp(0.f, DebugUIOffset.e[Axis], (f32)WindowBuffer.Dim.e[Axis] - List->FixedSize.e[Axis]);
           DebugUIDragOffset.e[Axis] = 0.f;
          }
         }
         
         UI_BackgroundColor(Color_Snow0)
          UI_SemanticHeight(UI_SizePx(2.f, 1.f))
          UI_AddBox(S8(""), 
                    UI_BoxFlag_Clip|
                    UI_BoxFlag_DrawBackground);
        }
        
        if(ToggleButton("Record", Replay.IsRecording, Color_Red))
        {
         ReplayToggleRecording(&Replay, &AppMemory, true);
        }
        
        if(ToggleButton("Looping", Replay.IsLooping, Color_Yellow, 
                        .Disabled = RecordingHasNoSteps))
        {
         if(!RecordingHasNoSteps)
         {
          ReplayToggleLooping(&Replay, &AppMemory);
         }
        }
        
        if(ToggleButton("Stepping", Replay.IsStepping, Color_Yellow,
                        .Disabled = RecordingHasNoSteps))
        {
         if(!RecordingHasNoSteps && !RecordingIsEmpty)
         {
          Replay.IsStepping = !Replay.IsStepping;
          if(!Replay.IsStepping) Replay.IsLooping = false;
         }
        }
        
        DebugSpacer();
        
        // Stepping sliders
        {                        
         s32 MaxDigits = (s32)(Replay.StepCount > 0 ? 
                               log10f((f32)Replay.StepCount) :
                               0);
         MaxDigits = Max(3, MaxDigits + 1);
         u64 MaxStepCount = (Replay.StepCount ? Replay.StepCount - 1 : 0);
         
         // StepTarget
         {                            
          UI_BackgroundColor(Color_Disabled)
           UI_AddBox(S8("StepTarget"), 
                     UI_BoxFlag_Clip|
                     UI_BoxFlag_DrawBorders|
                     UI_BoxFlag_DrawBackground);
          UI_PaddingAround(ItemPadding)
          {
           ui_box *Box;
           UI_SemanticWidth(UI_SizeText(1.f, 1.f))
            Box = UI_AddBox(S8("StepTarget"), 
                            UI_BoxFlag_DrawDisplayString|
                            UI_BoxFlag_CenterTextVertically|
                            UI_BoxFlag_CenterTextHorizontally);
           
           str8 FormatString = Str8Fmt("Target: %%%dllu/%%-%dllu", MaxDigits, MaxDigits);
           
           Box->DisplayString = Str8Fmt((char *)FormatString.Data, Replay.StepTarget, MaxStepCount);
           
           UI_BackgroundColor(Color_Orange)
            UI_BorderColor(Color_Black)
            Replay.StepTarget = (u64)UI_Slider((f32)Replay.StepTarget, 0.f, (f32)MaxStepCount, 1.f, 0, true);
          }
         }
         
         // StepIdx
         {
          UI_BackgroundColor(Color_Disabled)
           UI_AddBox(S8("StepIdx"), 
                     UI_BoxFlag_Clip|
                     UI_BoxFlag_DrawBorders|
                     UI_BoxFlag_DrawBackground);
          UI_PaddingAround(ItemPadding)
          {
           {                                
            ui_box *Box;
            UI_SemanticWidth(UI_SizeText(1.f, 1.f))
             Box = UI_AddBox(S8("StepIdx"), 
                             UI_BoxFlag_DrawDisplayString|
                             UI_BoxFlag_CenterTextVertically|
                             UI_BoxFlag_CenterTextHorizontally);
            
            str8 FormatString = Str8Fmt("StepAt: %%%dllu/%%-%dllu", MaxDigits, MaxDigits);
            Box->DisplayString = Str8Fmt((char *)FormatString.Data, Replay.StepIdx, MaxStepCount);
           }
           
           UI_BackgroundColor(Color_Yellow)
            UI_BorderColor(Color_Black)
            UI_Slider((f32)Replay.StepIdx, 0.f, (f32)MaxStepCount, 1.f, 0, false);
          }
         }
        }
        
        DebugSpacer();
        
        if(DisabledButton(S8("Reload"), RecordingHasNoSteps))
        {
         Replay.StepIdx = 0;
         Replay.IsSkipping = true;
        }
        
        UI_Row()
         UI_SemanticWidth(UI_SizeParent(.5f, 1.f))
        {                            
         if(DisabledButton(S8("Step"), RecordingHasNoSteps))
         {
          ReplayStep(&Replay, &AppMemory);
          if(Replay.StepCount)
          {
           Replay.StepTarget = (Replay.StepIdx + 1)%Replay.StepCount;
          }
         }
         
         if(DisabledButton(S8("Skip"), RecordingHasNoSteps))
         {
          if(Replay.RecordingSize)
          {
           ReplayStep(&Replay, &AppMemory);
           Replay.IsSkipping = true;
          }
         }
        }
        
        UI_Row()
         UI_SemanticWidth(UI_SizeParent(.5f, 1.f))
        {                            
         if(DisabledButton(S8("Load record"), RecordingIsEmpty))
         {
          if(Replay.RecordingSize)
          {
           ReplayLoadMemory(&Replay, &AppMemory);
          }
         }
         
         if(DisabledButton(S8("Save record"), false))
         {
          ReplayToggleRecording(&Replay, &AppMemory, false);
         }
        }
        
        DebugSpacer();
        
        if(DisabledButton(S8("Load from disk"), false))
        {
         char *FileName = PathFromExe(Str8Fmt("replay_%lu.edr", ReplaySlot));
         
         str8 ReplayBuffer = OS_ReadEntireFileIntoMemory(FileName);
         
         Assert(ReplayBuffer.Size < ReplayMaxBufferSize);
         
         if(ReplayBuffer.Size)
         {                                            
          MemoryCopy(Replay.Buffer, ReplayBuffer.Data, ReplayBuffer.Size);
          Replay.RecordingSize = ReplayBuffer.Size;
          Replay.StepIdx = 0;
          Replay.StepTarget = 0;
          ReplayLoadMemory(&Replay, &AppMemory);
         }
         
         OS_FreeFileMemory(ReplayBuffer);
        }
        
        if(DisabledButton(S8("Save to disk"), RecordingIsEmpty))
        {
         if(Replay.RecordingSize)
         {
          char *FileName = PathFromExe(Str8Fmt("replay_%lu.edr", ReplaySlot));
          str8 ReplayBuffer = {0};
          ReplayBuffer.Data = Replay.Buffer;
          ReplayBuffer.Size = Replay.RecordingSize;
          OS_WriteEntireFile(FileName, ReplayBuffer);
         }
         Log("Replay saved to disk.\n");
        }
        
        // ReplaySlot
        {                            
         UI_BackgroundColor(Color_Disabled)
          UI_AddBox(S8("ReplaySlot"), 
                    UI_BoxFlag_Clip|
                    UI_BoxFlag_DrawBorders|
                    UI_BoxFlag_DrawBackground);
         UI_PaddingAround(ItemPadding)
         {
          ui_box *Box;
          UI_SemanticWidth(UI_SizeText(1.f, 1.f))
           Box = UI_AddBox(S8("Slot"), 
                           UI_BoxFlag_DrawDisplayString|
                           UI_BoxFlag_CenterTextVertically|
                           UI_BoxFlag_CenterTextHorizontally);
          
          Box->DisplayString = Str8Fmt("ReplaySlot: %2d/%-2d", ReplaySlot, MaxReplaySlots);
          
          UI_BackgroundColor(Color_Orange)
           UI_BorderColor(Color_Black)
           ReplaySlot = (u64)UI_Slider((f32)ReplaySlot, 0.f, (f32)MaxReplaySlots, 1.f, 0, true);
         }
        }
        
        DebugSpacer();
        
        if(GlobalDebuggerIsAttached)
        {
         if(DisabledButton(S8("DebugBreak"), false))
         {
          DebugBreak();
         }
         
         DebugSpacer();
        }
        
        
        Logging ^= ToggleButton("Logging", Logging, Color_Red);
        Paused  ^= ToggleButton("Pause",   Paused,  Color_Red);
        GlobalIsProfiling ^= ToggleButton("Profiling", GlobalIsProfiling, Color_Red);
        
        DebugSpacer();
        UI_State->RectDebugMode ^= ToggleButton("UI Rects", UI_State->RectDebugMode, Color_Red);
        DebugSpacer();
        
        UI_BackgroundColor(Color_Disabled)
         UI_AddBox(S8("CPU"), UI_BoxFlag_Clip|
                   UI_BoxFlag_DrawBorders|
                   UI_BoxFlag_DrawBackground);
        UI_PaddingAround(ItemPadding)
        {
         ui_box *Box = UI_AddBox(S8("CPU"), UI_BoxFlag_DrawDisplayString|UI_BoxFlag_CenterTextVertically);
         Box->DisplayString = Str8Fmt("cpu: %.2fms/f", LastWorkMSPerFrame);
        }
       }
      }
     }
     
     if(!UI_IsActive(UI_NilBox, ButtonType) || !UI_IsHot(UI_NilBox, ButtonType))
     {
      NewInput->Consumed[ButtonType] = true;
     }
     
     UI_EndLayout(DebugRootBox);
     
     // Prune unused boxes
     for EachIndex(Idx, UI_State->BoxTableSize)
     {
      ui_box *First = UI_State->BoxTable + Idx;
      
      for UI_EachHashBox(Node, First)
      {
       if(FrameIdx > Node->LastTouchedFrameIdx)
       {
        if(!UI_KeyMatch(Node->Key, UI_KeyNull()))
        {
         Node->Key = UI_KeyNull();
        }
       }
      }
     }
    }
    
    OS_ProfileAndPrint("Debug UI");
#endif
    
    if(!Paused)
    {
     // Playback
     {
      if(Replay.RecordingSize)
      {                        
       Replay.StepCount = (Replay.RecordingSize - AppMemory.MemorySize)/sizeof(app_input);
      }
      
      if(Replay.IsRecording)
      {
       MemoryCopy((u8 *)Replay.Buffer + Replay.RecordingSize, NewInput, sizeof(*ReplayInput));
       Replay.RecordingSize += sizeof(*ReplayInput);
       Assert(Replay.RecordingSize < ReplayMaxBufferSize);
      }
      
      if(Replay.IsLooping)
      {
       Replay.StepTarget = (Replay.StepIdx + 1)%Replay.StepCount;
       Assert(Replay.IsStepping);
      }
      
      if(Replay.IsStepping)
      {
       b32 HasReachedStepTarget = false;
       
       if(Replay.StepTarget < Replay.StepIdx)
       {
        ReplayLoadMemory(&Replay, &AppMemory);
        Replay.StepIdx = 0;
       }
       
       if(Replay.PlayingPos == 0)
       {
        ReplayLoadMemory(&Replay, &AppMemory);
       }
       else
       {                                
        HasReachedStepTarget = (Replay.StepIdx == Replay.StepTarget);
        if(!HasReachedStepTarget)
        {
         Replay.StepIdx = (Replay.StepIdx + 1)%Replay.StepCount;
        }
       }
       
       // NOTE(luca): We are "pausing" the application by not passing zeroed inputs while the step target is reached. 
       if(HasReachedStepTarget)
       {
        MemoryZero(ReplayInput);
        for EachElement(Idx, ReplayInput->Consumed)
        {
         ReplayInput->Consumed[Idx] = true;
        }
        Replay.IsSkipping = false;
        ReplayInput->dtForFrame = 0.f;
       }
       else
       {
        // NOTE(luca): Have we thought about the fact that we don't need to do this copy at all.
        *ReplayInput = *(app_input *)((u8 *)Replay.Buffer + Replay.PlayingPos);
        Replay.PlayingPos += sizeof(*ReplayInput);
        
        if(Replay.PlayingPos >= Replay.RecordingSize)
        {
         Assert(Replay.PlayingPos == Replay.RecordingSize);
         ReplayLoadMemory(&Replay, &AppMemory);
        }
       }
      }
     }
     OS_ProfileAndPrint("Playback");
     
     AppMemory.IsProfiling = GlobalIsProfiling;
     
     app_input *AppInput = (Replay.IsStepping ? ReplayInput : NewInput);
     {
      // NOTE(luca): Need to be overwritten. Since they reflect the platform state but the the window border is drawn by the app based on the input.
      AppInput->PlatformIsRecording = Replay.IsRecording;
      AppInput->PlatformIsStepping = Replay.IsStepping;
      AppInput->PlatformWindowIsFocused = PlatformWindowIsFocused;
      AppInput->SkipRendering = Replay.IsSkipping;
     }
     
     if(Code.UpdateAndRender)
     {
      b32 ShouldQuit = Code.UpdateAndRender(&AppMemory, &WindowBuffer, AppInput);
      // NOTE(luca): Since UpdateAndRender can take some time, there could have been a signal sent to INT the app.
      ReadWriteBarrier();
      *Running = *Running && !ShouldQuit;
     }
     
     OS_ProfileAndPrint("UpdateAndRender");
    }
    
    b32 SkipRendering = Replay.IsSkipping;
    if(!SkipRendering)
    {
#if RL_PLATFORM_DEBUG_UI
     // TODO(luca): Would be nice if we could be non blocking in the case where the debug UI wants to be shown but the rendering of the app happens offscreen.
     // Window border
     if(0)
     {
      v4 WindowBorderColor;
      if(0) {}
      else if(NewInput->PlatformIsRecording) WindowBorderColor = Color_Red;
      else if(NewInput->PlatformIsStepping) WindowBorderColor = Color_Yellow;
      else WindowBorderColor = Color_Black;
      
      v4 Dest = {.Max = V2V2S32(WindowBuffer.Dim)};
      rect_instance *Inst = DrawRect(Dest, WindowBorderColor, 0.f, (f32)WindowBorderSize, 0.f);
      Inst->Color0 = WindowBorderColor;
      Inst->Color1 = (NewInput->PlatformWindowIsFocused ? Color_Snow2 : V4(V3Arg(Color_Snow2), 0.2f));
      Inst->Color2 = (NewInput->PlatformWindowIsFocused ? Color_Snow2 : V4(V3Arg(Color_Snow2), 0.2f));
      Inst->Color3 = WindowBorderColor;
     }
     
     // TODO(luca): bug here...
     RenderDrawAllRectangles(&DebugRender, V2V2S32(WindowBuffer.Dim), &DebugRenderAtlas);
#endif
     P_UpdateImage(PlatformContext, &WindowBuffer);
     OS_ProfileAndPrint("UpdateImage");
    }
    
    P_PlaySound(PlatformContext, &SoundBuffer, &Code);
    OS_ProfileAndPrint("Sound output");
    
    f64 MSElapsedForFrame = 0.f;
    f64 WorkCounter = OS_GetWallClock();
    f64 WorkMSPerFrame = OS_MSElapsed(LastCounter, WorkCounter);
    LastWorkMSPerFrame = WorkMSPerFrame;
    
    // Sleep
    {
     // TODO(luca): Think about framerate.
     if(!SkipRendering)
     {            
      f64 SecondsElapsedForFrame = OS_SecondsElapsed(LastCounter, WorkCounter);
      
      if(SecondsElapsedForFrame < TargetSecondsPerFrame)
      {
       f64 SleepUS = ((TargetSecondsPerFrame - 0.001f - SecondsElapsedForFrame)*1e6);
       
       if(SleepUS > 0)
       {
        OS_Sleep((u32)SleepUS);
       }
       else
       {
        // TODO(luca): Logging
       }
       
       // NOTE(luca): This is to help against sleep granularity.
       f64 TestSecondsElapsedForFrame;
       do
       {
        TestSecondsElapsedForFrame = OS_SecondsElapsed(LastCounter, OS_GetWallClock());
       } while(TestSecondsElapsedForFrame < TargetSecondsPerFrame);
      }
      else
      {
       // TODO(luca): Log missed frame rate!
      }
     }
     
     MSElapsedForFrame = OS_MSElapsed(LastCounter, OS_GetWallClock());
     
     LastCounter = OS_GetWallClock();
     
     OS_ProfileAndPrint("Sleep");
    }
    
    Swap(OldInput, NewInput);
    
    u8 Codepoint = NewInput->Text.First ? (u8)NewInput->Text.Buffer[NewInput->Text.First].Codepoint : 0;
    
    if(Logging)
    {
     Log("'%c' (%d %d -> %d, %d) 1:%c 2:%c 3:%c", 
         ((Codepoint == 0) ?
          '\a' : Codepoint),
         V2Arg(NewInput->Mouse.Start),
         V2Arg(NewInput->Mouse.Pos),
         (NewInput->Mouse.Buttons[PlatformMouseButton_Left  ].EndedDown ? 'x' : 'o'),
         (NewInput->Mouse.Buttons[PlatformMouseButton_Middle].EndedDown ? 'x' : 'o'),
         (NewInput->Mouse.Buttons[PlatformMouseButton_Right ].EndedDown ? 'x' : 'o')); 
     
     Log(" %.2fms/f %.2fms/f", (f64)WorkMSPerFrame, MSElapsedForFrame);
     Log("\n");
    }
    
    FrameIdx += 1;
   }
  }
  
  P_Cleanup(PlatformContext);
 }
 
 return 0;
}
