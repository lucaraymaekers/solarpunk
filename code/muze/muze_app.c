//-
// Muze
//-
#define BASE_EXTERNAL_LIBS 1
#define BASE_NO_ENTRYPOINT 1
#include "base/base.h"
#include "base/base.c"

#define TSF_IMPLEMENTATION
NO_WARNINGS_BEGIN
#include "lib/tsf.h"
NO_WARNINGS_END

#include "rl/generated/rl_tables.meta.c"
#include "rl/generated/rl_ui.meta.c"
#include "rl/generated/rl_render.meta.c"
#include "rl/generated/rl_font.meta.c"
#include "rl/rl_libs.h"
#include "rl/rl_font.h"
#include "rl/rl_platform.h"
#include "rl/rl_random.h"
#include "rl/rl_gl.h"
#include "rl/rl_renderer.h"
#include "rl/rl_ui.h"
#include "rl/rl_midi.h"

#include "muze/generated/muze.meta.c"
#include "muze/muze_serialize.h"
#include "muze/muze_app.h"

#include "muze/muze_serialize.c"
#include "rl/rl_renderer.c"
#include "rl/rl_ui.c"
#include "rl/rl_widgets.c"

#if OS_WINDOWS
# pragma comment(linker, "/export:GetAudioSamples")
# pragma comment(linker, "/export:UpdateAndRender")
#endif

//~ Macros
#define IsNilPiece(Piece) CheckNil(Piece, NilPiece)
#define IsNilPanel(Panel) CheckNil(Panel, NilPanel)
#define IsNilNote(Note) CheckNil(Note, NilNote)
#define IsNilVoice(Voice) CheckNil(Voice, NilVoice)
#define IsNilNote(Note) CheckNil(Note, NilNote)

#define Disabled() if(true) { Log("Temporarily disabled.\n"); } else

//~ Globals
global_variable tsf *GlobalTSF;
global_variable drag_data *DragData;
global_variable ui_size GlobalItemPadding;

global_variable tsf *NilTSF;
global_variable panel *NilPanel;
global_variable note *NilNote;
global_variable piece *NilPiece;

global_variable f32 *dtForFrame;

global_variable platform_midi_get_devices	*PlatformMIDIGetDevices;
global_variable platform_midi_send	*PlatformMIDISend;
global_variable platform_midi_listen	*PlatformMIDIListen;

//~ Helpers
internal inline b32
EqualsWithEpsilon(f32 A, f32 B, f32 Epsilon)
{
 b32 Result = (A < (B + Epsilon) && 
               A > (B - Epsilon));
 return Result;
}

internal void
InitReadOnlyGlobals(arena *Arena)
{
 ArenaSetPos(Arena, 0);
 UI_NilBox = PushArray(Arena, ui_box, 1);
 NilPanel = PushArray(Arena, panel, 1);
 NilNote = PushArray(Arena, note, 1);
 NilPiece = PushArray(Arena, piece, 1);
}

internal void
TSFPlayNote(app_state *App, s32 Channel, s32 Key, f32 Velocity)
{
 if(!App->OutputSynthDisabled)
 {
  tsf_channel_note_on(GlobalTSF, Channel, Key, Velocity);
 }
}

internal void
TSFPlayNoteU8(app_state *App, s32 Channel, s32 Key, u8 Velocity)
{
 TSFPlayNote(App, Channel, Key, ((f32)Velocity/256.f));
}

internal s32
TSFNextChannel(app_state *App)
{
 s32 Result = App->NextChannelIdx;
 App->NextChannelIdx += 1;
 return Result;
}

//~ Panels
global_variable arena *PanelArena = 0;
global_variable b32 PanelAppendToParent = false;
global_variable panel *PanelCurrent = 0;
global_variable app_state *PanelApp = 0;
global_variable app_input *PanelInput = 0;
global_variable panel *PanelDragging = 0;

global_variable s32 PanelDebugIndentation = 0;
global_variable axis2_stack_node *PanelAxisTop = 0; 

//~ Misc
typedef struct u64_array u64_array;
struct u64_array
{
 u64 Count;
 u64 *V;
};

internal u64_array
GetWrapPositions(arena *Arena, str8 Text, font_atlas *Atlas, f32 MaxWidth)
{
 u64_array Result = {0};
 
 // 1px per char
 u64 MaxChars = (u64)MaxWidth;
 
 Result.V = PushArray(Arena, u64, MaxChars);
 
 f32 X = 0.f;
 for EachIndex(Idx, Text.Size)
 {
  u8 Char = Text.Data[Idx];
  f32 CharWidth = Atlas->PackedChars[Char].xadvance;
  
  if(Char == '\n' || (X + CharWidth > MaxWidth))
  {
   Result.V[Result.Count] = Idx;
   Result.Count += 1;
   X = 0.f;
  }
  else
  {            
   X += CharWidth;
  }
 }
 
 return Result;
}

//~ Panels 
internal inline f32 
SizeOnAxis(v4 Rec, axis2 Axis)
{
 f32 Result = (Rec.Max.e[Axis] - Rec.Min.e[Axis]);
 return Result;
}

internal void
ZeroPanel(panel *Panel)
{
 MemoryZero(Panel);
 Panel->First = Panel->Last = Panel->Next = Panel->Prev = Panel->Parent = NilPanel;
 Panel->Piece = NilPiece;
}

global_variable u64 PanelsAllocated;

internal panel *
PanelAlloc(void)
{
 panel *Result = NilPanel;
 
 if(!IsNilPanel(PanelApp->FreePanel))
 {
  Result = PanelApp->FreePanel;
  PanelApp->FreePanel = PanelApp->FreePanel->Next;
 }
 else
 {
  Result = PushArray(PanelArena, panel, 1);
  PanelsAllocated += 1;
 }
 
 ZeroPanel(Result);
 
 return Result;
}

internal void PanelPushAxis(axis2 Axis) { StackPush(PanelArena, axis2_stack_node, Axis, PanelAxisTop); }
internal void PanelPopAxis(void) { PanelAxisTop = PanelAxisTop->Prev; }

internal void PanelPushChildren(void) 
{
 PanelAppendToParent = true;
 
 axis2 Axis = 1 - PanelAxisTop->Value;
 PanelPushAxis(Axis);
}
internal void PanelPopChildren(void)
{ 
 PanelCurrent = PanelCurrent->Parent; 
 PanelPopAxis();
}

internal panel *
PanelAdd(f32 ParentPct)
{
 panel *New = PanelAlloc();
 
 New->ParentPct = ParentPct;
 New->Axis = PanelAxisTop->Value;
 
 if(!IsNilPanel(PanelCurrent))
 {            
  if(PanelAppendToParent)
  {
   PanelCurrent->First = New;
   New->Parent = PanelCurrent;
  }
  else
  {
   PanelCurrent->Next = New;
   New->Prev = PanelCurrent;
   New->Parent = PanelCurrent->Parent;
  }
  
  New->Parent->Last = New;
 }
 
 PanelCurrent = New;
 PanelAppendToParent = false;
 
 return New;
}

#define PanelChildren() DeferLoop(PanelPushChildren(), PanelPopChildren())
#define PanelAxis(Axis) DeferLoop(PanelPushAxis(Axis), PanelPopAxis())
internal inline b32 
IsLeafPanel(panel *Panel)
{
 b32 Result = IsNilPanel(Panel->First);
 return Result;
}

// NOTE(luca): Returns new panel.
internal panel *
SplitPanel(app_state *App, panel *To, axis2 Axis, b32 Backwards)
{
 // NOTE(luca): If no new panel created return To as selected.
 panel *Result = To;
 
 if(!IsNilPanel(To))
 {
  // NOTE(luca): Must be a leaf node.
  Assert(IsLeafPanel(To));
  
  panel *Parent = To->Parent;
  
  if(IsNilPanel(Parent))
  {
   Parent = PanelAlloc();
   
   Parent->Axis = Axis;
   Parent->ParentPct = 1.f;
   
   Parent->First = To;
   Parent->Last = To;
   To->Parent = Parent;
   
   To->Axis = 1 - Axis;
   To->ParentPct = 1.f;
   
   if(To == App->FirstPanel)
   {
    App->FirstPanel = Parent;
   }
  }
  
  if(Axis != Parent->Axis)
  {
   // NOTE(luca): This branch is only taken when Parent has more than one child.  
   
   // Create new parent and move "To" to be the first child of this parent.
   Parent = PanelAlloc();
   Parent->Axis = Axis;
   Parent->ParentPct = To->ParentPct;
   
   // Inherit links from To
   Parent->Parent = To->Parent;
   Parent->Prev = To->Prev;
   Parent->Next = To->Next;
   
   // Update Links to To
   if(!IsNilPanel(To->Next)) To->Next->Prev = Parent;
   if(!IsNilPanel(To->Prev)) To->Prev->Next = Parent;
   if(To == To->Parent->Last) To->Parent->Last = Parent;
   if(To == To->Parent->First) To->Parent->First = Parent;
   
   // Make To new child for Parent
   To->Next = NilPanel;
   To->Prev = NilPanel;
   To->Parent = Parent;
   To->ParentPct = 1.f;
   Parent->First = To;
   Parent->Last = To;
  }
  
  f32 NewParentPct = 1.f;
  {
   //The new child's ParentPct becomes 1/n where n is the children count
   //Other children's ParentPct *= (1-1/n) 
   {        
    s32 ChildrenCount = 0;
    for EachChildPanel(Child, Parent)
    {
     ChildrenCount += 1;
    }
    Assert(ChildrenCount > 0);
    
    // Account for new child
    ChildrenCount += 1;
    
    NewParentPct = 1.f/(f32)ChildrenCount;;
   }
   
   // Update children
   for EachChildPanel(Child, Parent)
   {
    Child->ParentPct *= (1.f - NewParentPct);
   }
   
   // Append after To
   panel *New = PanelAlloc();
   {   
    New->ParentPct = NewParentPct;
    New->Parent = Parent;
    if(!IsNilPanel(To->Next)) To->Next->Prev = New;
    else          Parent->Last = New;
    New->Next = To->Next;
    New->Prev = To;
    To->Next = New;
   }
   
   Result = New;
  }
 }
 
 return Result;
}

internal void
PanelDebugPrint(panel *Panel)
{
 Log("%*s %.2f %u:\n"
     "%*s  (%.0f,%.0f)\n"
     "%*s  (%.0f,%.0f)\n",
     PanelDebugIndentation, "", Panel->ParentPct, Panel->Axis,
     PanelDebugIndentation, "", V2Arg(Panel->Region.Min),
     PanelDebugIndentation, "", V2Arg(Panel->Region.Max));
 
 if(!IsNilPanel(Panel->First))
 {
  PanelDebugIndentation += 1;
  PanelDebugPrint(Panel->First);
  PanelDebugIndentation -= 1;
 }
 
 if(!IsNilPanel(Panel->Next))
 {
  PanelDebugPrint(Panel->Next);
 }
}

internal panel *
PanelNextLeaf(panel *Start, b32 Backwards)
{
 panel *Result = Start;
 // NOTE(luca): If nothing was found we will return the starting point
 
 b32 IsLeaf = false;
 
 panel *Search = Start;
 
 while(!IsLeaf && !IsNilPanel(Search))
 {
  panel *Next = (Backwards ? Search->Prev : Search->Next);
  
  if(!IsNilPanel(Next) || IsNilPanel(Search->Parent))
  {
   if(!IsNilPanel(Next))
   {            
    Search = Next;
    
    IsLeaf = IsLeafPanel(Search);
   }
   
   while(!IsLeaf)
   {
    Search = (Backwards ? Search->Last : Search->First);
    
    IsLeaf = IsLeafPanel(Search);
   }
  }
  else
  {
   Search = Search->Parent;
  }
 }
 
 if(IsLeaf)
 {
  Result = (IsNilPanel(Search) ? Start : Search);
 }
 
 return Result;
}

// NOTE(luca): Returns the panel that absorbed the size of the closed panel
internal panel *
ClosePanel(app_state *App, panel *Panel)
{
 panel *Collapse = NilPanel;
 panel *Parent = Panel->Parent;
 
 if(!IsNilPanel(Panel->Next)) Collapse = Panel->Next;
 if(!IsNilPanel(Panel->Prev)) Collapse = Panel->Prev;
 
 if(IsNilPanel(Collapse))
 {
  // If there is no other panel, just make this one an empty one.
  {
   Panel->Kind = PanelKind_Empty;
   Collapse = Panel;
  }
 }
 else if(IsLeafPanel(Collapse))
 {
  
  // Remove the panel from the tree.
  {        
   if(!IsNilPanel(Panel->Next)) Panel->Next->Prev = Panel->Prev;
   if(!IsNilPanel(Panel->Prev)) Panel->Prev->Next = Panel->Next;
   if(!IsNilPanel(Parent))
   {
    if(Parent->First == Panel) Parent->First = Parent->First->Next;
    if(Parent->Last  == Panel) Parent->Last  = Parent->Last->Prev;
   }
  }
  
  // Absorb the size of the closed panel
  {        
   Collapse->ParentPct += Panel->ParentPct;
  }
  
  // If this is the only child of the parent, collapse the child into the parent.
  {
   if(Collapse == Parent->First && 
      Collapse == Parent->Last)
   {
    // Collapse becomes the parent.  We don't make the parent become the child, because
    // this way panel address is consistent and can be used for things like ui hashing.
    
    // Copy panel fields
    {
     Collapse->Parent = Parent->Parent;
     Collapse->First = NilPanel;
     Collapse->Last = NilPanel;
     Collapse->Next = Parent->Next;
     Collapse->Prev = Parent->Prev;
     
     Collapse->Axis = Parent->Axis;
     Collapse->ParentPct = Parent->ParentPct;
     Collapse->Region = Parent->Region;
    }
    
    if(!IsNilPanel(Collapse->Prev)) Collapse->Prev->Next = Collapse;
    if(!IsNilPanel(Collapse->Next)) Collapse->Next->Prev = Collapse;
    if(Parent->Parent->First == Parent) Parent->Parent->First = Collapse;
    if(Parent->Parent->Last  == Parent) Parent->Parent->Last  = Collapse;
    
    // Free the parent
    {
     Parent->Next = App->FreePanel;
     App->FreePanel = Parent;
    }
    
    if(App->FirstPanel == Parent)
    {
     App->FirstPanel = Collapse;
    }
    
   }
  }
  
  // Add removed panel to the free list
  {
   Panel->Next = App->FreePanel;
   App->FreePanel = Panel;
  }
 }
 else
 {
  // NOTE(luca): I don't know when this can even happen.
  InvalidPath();
 }
 
 return Collapse;
}

internal panel_rec
PanelRecDepthFirstPreOrder(panel *Panel)
{
 panel_rec Rec = {0};
 
 if(!IsNilPanel(Panel->First))
 {
  Rec.Next = Panel->First;
  Rec.PushCount = 1;
 }
 else for(panel *P = Panel; !IsNilPanel(P); P = P->Parent)
 {
  if(!IsNilPanel(P->Next))
  {
   Rec.Next = P->Next;
   break;
  }
  Rec.PopCount += 1;
 }
 
 return Rec;
}

//~ Muze - Commands
global_variable command NilCommand = {0};

internal command *
PushCommand(app_state *App, command_kind Kind)
{
 command *Result = &NilCommand;
 
 if(App->CommandCount < App->MaxCommandCount)
 {
  Result = App->Commands + App->CommandCount;
  MemoryZero(Result);
  
  App->CommandCount += 1;
  Result->Kind = Kind;
 }
 
 return Result;
}

//~ Muze - Piece
internal note *
PieceCodeNoteAdd(piece *Piece, 
                 note_kind Kind, u8 Pitch, f32 Length)
{
 note *Note = NilNote;
 
 if(Piece->NoteCount < ArrayCount(Piece->Notes))
 { 
  Note = Piece->Notes + Piece->NoteCount;
  
  // TODO(luca): Better way?
  // NOTE(luca): Sometimes we will add fractions which are not powers of 2, this is to
  // correct the inaccuracies
  f32 DecimalPart = ModF32(Piece->RecordLength, 1.f);
  if(EqualsWithEpsilon(DecimalPart, 1.f, .0001f))
  {
   Piece->RecordLength += 1.f - DecimalPart;
  }
  
  Note->Kind = Kind;
  Note->Pitch = Pitch + MiddleC;
  Note->Length = Length;
  Note->Start = Piece->RecordLength;
  Note->Velocity = 64;
  
  Piece->NoteCount += 1;
  Piece->RecordLength += Length;
 }
 else
 {
  ErrorLog("Note out of bounds.\n");
 }
 
 return Note;
}

internal b32
PiecePosPassedTimestamp(piece *Piece, f32 Pos, f32 Timestamp)
{
 b32 Result = false;
 
 f32 BPS = Piece->BPM/60.f;
 
 f32 Now = Pos;
 f32 Before = Pos - BPS*(*dtForFrame);
 
 Result = (Before < Timestamp && Timestamp <= Now);
 
 return Result;
}

internal b32
PieceWasTimestampPlayed(piece *Piece, f32 Timestamp)
{
 b32 Result = PiecePosPassedTimestamp(Piece, Piece->PlayPos, Timestamp);
 return Result;
}

internal piece *
PieceAlloc(app_state *App)
{
 piece *Result = NilPiece;
 
 if(App->PieceCount < App->MaxPieceCount)
 { 
  if(!IsNilPiece(App->NextFreePiece))
  {
   Result = App->NextFreePiece;
   App->NextFreePiece = Result->Next;
  }
  else
  {
   Result = PushArray(App->PieceArena, piece, 1);
   
   // Allocate channel
   {   
    Result->Channel = TSFNextChannel(App);
    tsf_channel_set_presetindex(GlobalTSF, Result->Channel, Result->Preset);
    tsf_channel_set_volume(GlobalTSF, Result->Channel, .3f);
   }
  }
  
  // Defaults
  if(!IsNilPiece(Result))
  {
   Result->BPM = 60;
   Result->TimeSigNum = 2;
   Result->TimeSigDen = 4;
   Result->StaffCount = 1;
   Result->Staffs[0].Octave = 5;
   
   // First note is nil
   Result->NoteCount = 1;
   Result->PracticeNoteCount = 1;
   Result->SetNoteLength = 1.f;
   
   Result->BarPadding = 16.f;
   Result->BeatWidth = 64.f;
   Result->NoteSize = 11.f;
   Result->StaffLineWidth = 1.f;
   Result->TailHeight = 24.f;
  }
  
  DLLAppend(Result, App->FirstPiece, App->LastPiece);
  App->PieceCount += 1;
 }
 else
 {
  // TODO(luca): Popup?
  ErrorLog("Max piece count reached.\n");
 }
 
 return Result;
}

internal void
MIDIOutputMutePiecePlayingNotes(app_state *App, piece *Piece)
{
 if(App->OutputMIDIEnabled)
 {
  for EachIndex(Idx, Piece->NoteCount)
  {
   note *Note = Piece->Notes + Idx;
   
   f32 Start = Note->Start;
   f32 End = Note->Start + Note->Length;
   
   b32 NoteIsPlaying = (Start <= Piece->PlayPos && Piece->PlayPos < End);
   if(NoteIsPlaying)
   {   
    midi_message Message = {0};
    Message.U8[0] = MIDIEventType_NoteOff;
    Message.U8[1] = Note->Pitch;
    
    PlatformMIDISend(App->Out, Message.U32[0]);
   }
  }
 }
 
}

internal void
PieceStopPlaying(app_state *App, piece *Piece)
{
 if(Piece->IsPlaying)
 {
  Piece->IsPlaying = false;
  
  if(!App->OutputSynthDisabled)
  {   
   tsf_channel_note_off_all(GlobalTSF, Piece->Channel);
  }
  
  MIDIOutputMutePiecePlayingNotes(App, Piece);
 }
 
}

internal void
PieceDeselectNote(piece *Piece, note_node *Node)
{
 DLLRemove(Node, Piece->SelectionFirst, Piece->SelectionLast);
 Node->Next = Piece->SelectionFree;
 Piece->SelectionFree = Node;
 Piece->SelectionCount -= 1;
}

#if 0
# include "../data/muze/muze_pieces.c"
#endif

//~ UI 
internal void
DebugStringAdd(app_state *App, str8 Tag, char *Format, ...)
{
 if(App->DebugStringCount < App->MaxDebugStringCount)
 {
  va_list Args;
  va_start(Args, Format);
  str8 String = Str8VFmt(Format, Args);
  
  App->DebugStringTags[App->DebugStringCount] = Tag;
  App->DebugStrings[App->DebugStringCount] = String;
  App->DebugStringCount += 1;
 }
}

#define M_Button(ButtonText, ...) \
UI_Button(.Text = ButtonText, .CenterText = true, .Padding = GlobalItemPadding, ##__VA_ARGS__).Pressed

#define M_IconButton(Kind, Name, ...) \
M_Button(UI_Icons[Kind], .String = Name, ##__VA_ARGS__)

#define M_ControlButton(ButtonText, ...) \
UI_Button(.Text = ButtonText, .Padding = GlobalItemPadding, ##__VA_ARGS__).Pressed

#define M_SimpleToggleButton(ButtonText, Toggled, Color, ...) \
UI_Button(.Text = ButtonText, .Padding = GlobalItemPadding, .HasToggle = true, .ToggleToggled = Toggled, Color_Green, .ToggleToggledColor = Color, ##__VA_ARGS__).OneClicked

typedef struct m_slider_result m_slider_result;
struct m_slider_result
{
 f32 Value;
 b32 Clicked;
};

typedef struct m_slider_params m_slider_params;
struct m_slider_params
{
 str8 String; 
 str8 DisplayString; 
 str8 ValueString;
 ui_size SliderSize;
 b32 Align;
 
 f32 StepValue;
 f32 StepMin; 
 f32 StepMax; 
 f32 StepSize;
 
 b32 IsClickable;
};

#define M_Slider(Str, DisplayStr, ValueStr, Value, Min, Max, Step, Clickable, ...) \
M_Slider_((m_slider_params){ \
.String = Str, \
.DisplayString = DisplayStr, \
.ValueString = ValueStr, \
.SliderSize = UI_SizeFull(), \
.Align = true, \
.StepValue = Value, \
.StepMin = Min, \
.StepMax = Max, \
.StepSize = Step, \
.IsClickable = Clickable, \
##__VA_ARGS__})

internal m_slider_result
M_Slider_(m_slider_params Params)
{
 m_slider_result Result = {0};
 
 str8 String = Params.String; 
 str8 DisplayString = Params.DisplayString; 
 str8 ValueString = Params.ValueString;
 ui_size SliderSize = Params.SliderSize;
 b32 NoAlign = !Params.Align;
 
 f32 Value = Params.StepValue;
 f32 Min = Params.StepMin; 
 f32 Max = Params.StepMax; 
 f32 Step = Params.StepSize;
 
 b32 Clickable = Params.IsClickable;
 
 s32 Flags = (UI_BoxFlag_Clip|
              UI_BoxFlag_DrawBorders|
              UI_BoxFlag_DrawBackground);
 
 if(Clickable)
 {
  Flags |= (UI_BoxFlag_MouseClickable|
            UI_BoxFlag_DrawHotEffects|
            UI_BoxFlag_DrawActiveEffects);
 }
 
 {
  ui_box *Slider;
  UI_BackgroundColor(Color_ButtonBackground)
   Slider = UI_AddBox(String, Flags);
  
  UI_PaddingAround(GlobalItemPadding)
  {
   if(DisplayString.Size > 0)
   {
    ui_box *Box;
    UI_SemanticWidth(UI_SizeText(1.f, 1.f))
     Box = UI_AddBox(S8("SliderText"),
                     UI_BoxFlag_DrawDisplayString|
                     UI_BoxFlag_CenterTextVertically);
    Box->DisplayString = DisplayString;
   }
   
   if(!NoAlign)
   {   
    UI_Spacer(UI_SizeFull());
   }
   
   if(ValueString.Size > 0)
   {
    ui_box *Box;
    UI_SemanticWidth(UI_SizeText(1.f, 1.f))
     Box = UI_AddBox(S8("ValueText"),
                     UI_BoxFlag_DrawDisplayString|
                     UI_BoxFlag_CenterTextVertically);
    Box->DisplayString = ValueString;
    
    UI_Spacer(GlobalItemPadding);
   }
   
   UI_SemanticWidth(SliderSize)
    UI_BackgroundColor(Color_Orange)
    UI_BorderColor(Color_Black)
    Result.Value = UI_Slider(Value, Min, Max, Step, 0, true);
  }
  
  Result.Clicked = Slider->Clicked[PlatformMouseButton_Left];
 }
 
 return Result;
}

internal f32
M_Scrollbar(axis2 Axis, f32 TotalSize, f32 Scroll)
{
 f32 Result = 0.f;
 
 f32 ScrollbarSize = 16.f;
 f32 Padding = 2.f;
 
 f32 ThumbSize = 48.f;
 f32 HalfThumbSize = .5f*ThumbSize;
 
 ui_box *Scrollbar;
 ui_box *Thumb;
 ui_box *Spacer;
 
 app_input *Input = UI_State->Input;
 
 UI_BackgroundColor(Color_Night3)
  UI_LayoutAxis(Axis)
  UI_SemanticSizeOnAxis(Axis, UI_SizeParent(1.f, 0.f))
  UI_SemanticSizeOnAxis(1 - Axis, UI_SizePx(ScrollbarSize, 1.f))
  UI_AddBox(S8("ScrollbarParent"), 
            UI_BoxFlag_Clip|
            UI_BoxFlag_DrawBackground);
 
 UI_PaddingAround(UI_SizePx(Padding, 1.f))
 {
  UI_LayoutAxis(Axis)
   UI_FillAll()
   Scrollbar = UI_AddBox(S8("Scrollbar"), UI_BoxFlag_MouseClickable);
  
#if 0  
  // TODO(luca): Grow scroll bar proportional to amount to scroll.
  ThumbSize = Scrollbar->FixedSize.e[Axis]/TotalSize*200.f;
  HalfThumbSize = ThumbSize*.5f;
#endif
  
  UI_Push()
  {                                                    
   f32 ScrollableSize = Scrollbar->FixedSize.e[Axis] - ThumbSize;
   f32 Factor = TotalSize/ScrollableSize;
   
   Spacer = UI_AddBox(S8("Spacer"), 0);
   
   UI_FillOnAxis(1 - Axis)
    UI_SemanticSizeOnAxis(Axis, UI_SizePx(ThumbSize, 1.f))
    UI_BorderColor(Color_Black)
    UI_BackgroundColor(Color_Orange)
    Thumb = UI_AddBox(S8("Thumb"), 
                      UI_BoxFlag_Clip|
                      UI_BoxFlag_DrawBackground|
                      UI_BoxFlag_DrawBorders|
                      UI_BoxFlag_MouseClickable|
                      UI_BoxFlag_DrawHotEffects|
                      UI_BoxFlag_DrawActiveEffects);
   
   // Clamp in container shrunk.
   f32 ScrollP = Min(Scroll, TotalSize);
   
   platform_mouse_button_type ButtonType = PlatformMouseButton_Left;
   
   if(UI_IsActive(Thumb, ButtonType) || UI_IsActive(Scrollbar, ButtonType))
   {                                                                            
    f32 MouseP = (f32)Input->Mouse.Pos.e[Axis];
    f32 RelP = MouseP - Scrollbar->FixedPos.e[Axis];
    RelP -= HalfThumbSize;
    RelP = Clamp(0, RelP, ScrollableSize);
    
    ScrollP = RelP*Factor;
   }
   
   Result = ScrollP;
   
   f32 SpaceBeforeThumb = (Factor > 0.f ? ScrollP/Factor : 0.f);
   
   Spacer->SemanticSize[Axis] = UI_SizePx(SpaceBeforeThumb, 1.f);
   Spacer->SemanticSize[1 - Axis] = UI_SizeParent(1.f, 0.f);
  }
  
 }
 
 return Result;
}

internal ui_box *
M_Label(str8 String)
{
 ui_box *Result = UI_AddBox(String, 
                            UI_BoxFlag_DrawDisplayString|
                            UI_BoxFlag_DrawBackground|
                            UI_BoxFlag_DrawBorders|
                            UI_BoxFlag_CenterTextHorizontally|
                            UI_BoxFlag_CenterTextVertically);
 return Result;
}

internal ui_box *
M_Labelf(char *Format, ...)
{
 ui_box *Result = UI_NilBox;
 str8 String = {0};
 
 va_list Args;
 va_start(Args, Format);
 String = Str8VFmt(Format, Args);
 
 Result = M_Label(String);
 
 return Result;
}

internal void
M_ControlLabel(str8 Name)
{
 UI_BackgroundColor(Color_Frost2) M_Label(Name);
}

typedef struct display_unit display_unit;
struct display_unit
{
 u64 Value;
 str8 Name;
}; 

internal display_unit
DisplayUnitFromSize(f32 Size)
{
 display_unit Result = {0};
 
 f32 X = Size;
 if(X > 0.f)
 {
  X = Log10F32(X);
  X = X/3.f; // Thousands
  X = FloorF32(X);
 }
 
 display_unit UnitTable[] = 
 {
  {   1,  S8("B")},
  {KB(1), S8("KB")},
  {MB(1), S8("MB")},
  {GB(1), S8("GB")},
  {TB(1), S8("TB")},
 };
 
 Result = UnitTable[(s32)X];
 
 return Result;
}

internal void
DebugStringAddArena(app_state *App, str8 Name, arena *Arena)
{
 if(Arena)
 {  
  display_unit PosUnit = DisplayUnitFromSize((f32)Arena->Pos);
  display_unit SizeUnit = DisplayUnitFromSize((f32)Arena->Size);
  
  DebugStringAdd(App, S8("mem"), "%S: %.3f%S/%.1f%S", 
                 Name, 
                 (f32)Arena->Pos/(f32)PosUnit.Value, 
                 PosUnit.Name, 
                 (f32)Arena->Size/(f32)SizeUnit.Value,
                 SizeUnit.Name);
 }
}

UI_CUSTOM_DRAW(CustomDrawPiano)
{
 piece_box_data *Data = (piece_box_data *)CustomDrawData;
 ui_box *Box = Data->Box;
 app_input *Input = UI_State->Input;
 
 // Draw horizontal piano
 // Highlight the keys that are pressed.
 
 s32 KeyCount = (7*7 + 3);
 
 f32 GapSize = 2.f;
 f32 KeyWidth = Box->FixedSize.X/(f32)KeyCount;
 KeyWidth -= GapSize;
 v2 Pos = Box->FixedPos;
 f32 Height = Box->FixedSize.Y;
 
 platform_mouse_button_type ButtonType = PlatformMouseButton_Left;
 
 b32 InputIsOurs = (UI_IsHot(Box, ButtonType) || UI_IsActive(Box, ButtonType));
 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 
 for EachIndex(Idx, KeyCount)
 {
  f32 X = Pos.X + (f32)Idx*(KeyWidth + GapSize);
  
  v4 Color = V4F32(.93f);
  v4 Dest = RectFromSize(V2(X, Pos.Y), V2(KeyWidth, Height));
  b32 IsHovered = (InputIsOurs && IsInsideRectV2(MousePos, Dest));
  if(IsHovered) V3Math Color.E *= 0.9f;
  DrawRect(Dest, Color, 0.f, 0.f, 0.f);
 }
}

//~ Custom draw UI elements 
internal v2 
PiecePosFromTime(piece *Piece, f32 Start, v2 StartPos, b32 AlignToEnd, 
                 s32 BarsPerWidth, f32 StaffHeight)
{
 v2 Pos = StartPos;
 
 f32 BarPadding = Piece->BarPadding;
 f32 BeatWidth = Piece->BeatWidth;
 // 1 for beginning, 1 for end and 1 for last note pad
 f32 WholeBarWidth = BarPadding + BeatWidth*Piece->TimeSigNum + BarPadding;
 
 s32 BarIdx = 0;
 f32 Remainder = 0.f;
 s32 BeatsPerBar = (s32)Piece->TimeSigNum;
 
 // NOTE(luca): PracticePos must use same calculations as notes such that the marker is
 // placed before the note.
 // All other markers have the special case that they when they equal the bar length, they
 // are placed on the end of the bar and not on the next bar's start.
 if(AlignToEnd)
 {
  BarIdx = (Start == 0.f ? 0 : (s32)CeilF32(Start/(f32)BeatsPerBar) - 1);
  Remainder = Start - (f32)BeatsPerBar*(f32)BarIdx;
 }
 else
 {
  BarIdx = (s32)FloorF32(Start/Piece->TimeSigNum);
  Remainder = ModF32(Start, Piece->TimeSigNum);
 }
 
 if(!Piece->BarWrapping)
 {
  s32 HeightIdx = BarIdx/BarsPerWidth;
  BarIdx = (BarIdx%BarsPerWidth);
  //Pos.Y += (f32)HeightIdx*2.f*StaffHeight;
  
  f32 SizeToNextBar = ((f32)Piece->StaffCount*2.f*StaffHeight + StaffHeight);
  
  Pos.Y += (f32)HeightIdx*SizeToNextBar;
 }
 
 f32 BarStart = 2.f*BarPadding + (f32)BarIdx*WholeBarWidth;
 Pos.X = BarStart + Remainder*BeatWidth;
 
 return Pos;
}

UI_CUSTOM_DRAW(CustomDrawPiece)
{
 piece_box_data *Data = (piece_box_data *)CustomDrawData;
 ui_box *Box = Data->Box;
 piece *Piece = Data->Piece;
 app_state *App = Data->App;
 app_input *Input = UI_State->Input;
 
 f32 BarPadding = Piece->BarPadding;
 f32 BeatWidth = Piece->BeatWidth;
 // 1 for beginning, 1 for end and 1 for last note pad
 f32 WholeBarWidth = BarPadding + BeatWidth*Piece->TimeSigNum + BarPadding;
 
 f32 NoteSize = Piece->NoteSize;
 v2 NoteDim = V2(NoteSize, NoteSize);
 
 s32 StaffLineCount = 5;
 f32 StaffLineWidth = Piece->StaffLineWidth;
 f32 StaffHeight = (f32)(StaffLineCount-1)*NoteSize + StaffLineWidth;
 
 v2 TailDim = V2(StaffLineWidth, Piece->TailHeight);
 
 v2 Pos = V2SubV2(Box->FixedPos, Box->Scroll);
 Pos.Y += StaffHeight;
 Pos.X += BarPadding;
 
 f32 BarCount = CeilF32(Piece->RecordLength/Piece->TimeSigNum);
 BarCount = Max(BarCount, 1.f);
 
 s32 BarsPerWidth = (s32)((Box->FixedSize.X - 2.f*BarPadding)/WholeBarWidth);
 BarsPerWidth = Max(1, BarsPerWidth);
 
 v4 StaffColor = Box->TextColor;
 
 // Draw staff
 {    
  v2 StaffPos = Pos;
  
  // Draw bars
  { 
   for EachIndex(StaffIdx, Piece->StaffCount)
   {    
    v2 BarPos = StaffPos;
    
    BarPos.Y += (f32)StaffIdx*2.f*StaffHeight;
    
    for EachIndex(Idx, (s32)BarCount)
    {
     v4 Dest = RectFromSize(BarPos, V2(2.f, StaffHeight));
     DrawRect(Dest, StaffColor, 0.f, 0.f, 0.f); 
     
     for EachIndex(LineIdx, StaffLineCount)
     {
      v2 LinePos = BarPos;
      
      LinePos.Y += (f32)LineIdx*NoteSize;
      v4 LineDest = RectFromSize(LinePos, V2(WholeBarWidth, StaffLineWidth));
      DrawRect(LineDest, StaffColor, 0.f, 0.f, 0.f);
     }
     
     BarPos.X += WholeBarWidth;
     
     // Last bar gets double line
     if(Idx == (s32)BarCount - 1)
     {
      v2 LastBarPos = BarPos;
      
      LastBarPos.X -= 4.f;
      Dest = RectFromSize(LastBarPos, V2(2.f, StaffHeight));
      DrawRect(Dest, StaffColor, 0.f, 0.f, 0.f); 
     }
     
     // TODO(luca): Reduce world's biggest condition.
     b32 LastBar = (((Idx + 1)%BarsPerWidth == 0) ||
                    (BarsPerWidth == 1) || 
                    (Idx == (s32)BarCount - 1));
     if(LastBar)
     {
      Dest = RectFromSize(BarPos, V2(2.f, StaffHeight));
      DrawRect(Dest, StaffColor, 0.f, 0.f, 0.f); 
      
      if(!Piece->BarWrapping)
      {
       BarPos.X = StaffPos.X;
       BarPos.Y += (f32)Piece->StaffCount*2.f*StaffHeight + StaffHeight;
      }
     }
     
    }
   }
  }
 }
 
 enum piece_marker_kind
 {
  PieceMarkerKind_Play,
  PieceMarkerKind_PrevPlay,
  PieceMarkerKind_Record,
  PieceMarkerKind_PracticeStart,
  PieceMarkerKind_PracticeEnd,
  PieceMarkerKind_PracticePos,
  PieceMarkerKind_PrevPracticePos,
  PieceMarkerKind_Count
 };
 
 // Draw playing and recording marker 
 {
  for EachIndex(Kind, PieceMarkerKind_Count)
  {
   v4 Color = {0};
   f32 Start = 0.f;
   v2 *FollowPos = 0;
   b32 AlignMarkerToBarEnd = false;
   f32 MarkerHeightIdx = 0;
   
   if(0) {}
   else if(Kind == PieceMarkerKind_Play)
   {
    Color = Color_Green;
    Start = Piece->PlayPos;
    MarkerHeightIdx = 0.f;
   }
   else if(Kind == PieceMarkerKind_PrevPlay)
   {
    Color = Color_Yellow;
    Start = Piece->PlayPos;
    FollowPos = &Piece->PrevPlayPos;
    MarkerHeightIdx = 0.f;
   }
   else if(Kind == PieceMarkerKind_Record)
   {
    Color = Color_Red;
    Start = Piece->RecordLength;
    AlignMarkerToBarEnd = true;
    MarkerHeightIdx = 1.f;
   }
   else if(Kind == PieceMarkerKind_PracticeStart)
   {
    Color = Color_Blue;
    Start = Piece->PracticeStart;
    MarkerHeightIdx = 2.f;
   }
   else if(Kind == PieceMarkerKind_PracticeEnd)
   {
    Color = Color_Orange;
    Start = Piece->PracticeEnd;
    MarkerHeightIdx = 3.f;
   }
   else if(Kind == PieceMarkerKind_PracticePos)
   {
    Color = Color_Green;
    Start = Piece->PracticePos;
    MarkerHeightIdx = 4.f;
   }
   else if(Kind == PieceMarkerKind_PrevPracticePos)
   {
    Color = Color_Green;
    Start = Piece->PracticePos;
    FollowPos = &Piece->PrevPracticePos;
    MarkerHeightIdx = 4.f;
   }
   else
   {
    InvalidPath();
   }
   
   // NOTE(luca): Grey out markers when practicing
   if(Piece->IsPracticing && Kind < PieceMarkerKind_PracticeStart) 
   {
    Color.W *= 0.5f;
   }
   else if(!Piece->IsPracticing && Kind >= PieceMarkerKind_PracticeStart)
   {
    Color.W *= 0.5f;
   }
   
   v2 MarkerPos = PiecePosFromTime(Piece, Start, Pos, AlignMarkerToBarEnd, BarsPerWidth, StaffHeight);
   MarkerPos.Y += MarkerHeightIdx*NoteDim.Y;
   
   if(FollowPos)
   {
    // NOTE(luca): We draw first, then update the position.  This ensures that the first
    // trail will start from the end.
    
    if(!UI_State->IsAnimating)
    {
     *FollowPos = MarkerPos;
    }
    
    f32 Width = AbsF32(FollowPos->X - MarkerPos.X); 
    {
     // The whole width is a bit much
     Width = Min(Width, 50.f);
     // This gets rid of the wobbly cursor when the width is near 0
     if(Kind == PieceMarkerKind_PrevPlay && Width <= 20.f) Width = 0.f;
    }
    
    // Draw trail
    {
     v2 TrailPos = *FollowPos;
     if(FollowPos->X > MarkerPos.X) TrailPos.X -= Width;
     
     if(UI_State->IsAnimating)
     {
      DrawFilledRect(TrailPos, V2(Width, NoteDim.Y), Color);
     }
    }
    
    // Update trail
    if(*dtForFrame > 0.f)
    {
     f32 Speed = .5f*UI_State->AnimSpeed;
     FollowPos->X += (MarkerPos.X - FollowPos->X)*Speed;
     FollowPos->Y += (MarkerPos.Y - FollowPos->Y)*Speed;
    }
   }
   
   DrawFilledRect(MarkerPos, V2(StaffLineWidth, NoteDim.Y), Color);
  }
 }
 
 
 b32 LeftInputIsOurs = (UI_IsHot(Box, PlatformMouseButton_Left) ||
                        UI_IsActive(Box, PlatformMouseButton_Left));
 b32 RightInputIsOurs = (UI_IsHot(Box, PlatformMouseButton_Right) ||
                         UI_IsActive(Box, PlatformMouseButton_Right));
 v2 MousePos = V2V2S32(Input->Mouse.Pos);
 v2 MouseStart = V2V2S32(Input->Mouse.Start);
 b32 LeftPressed = (LeftInputIsOurs && Input->Mouse.Buttons[PlatformMouseButton_Left].EndedDown);
 b32 RightPressed = (RightInputIsOurs && Input->Mouse.Buttons[PlatformMouseButton_Right].EndedDown);
 b32 Adding = !RightPressed;
 
 v4 SelRec = {0};
 SelRec.Min = MousePos;
 SelRec.Max = ((LeftPressed || RightPressed) ? 
               MouseStart : 
               MousePos);
 
 V2Math if(SelRec.Min.E > SelRec.Max.E) Swap(SelRec.Min.E, SelRec.Max.E);
 SelRec = RectIntersect(SelRec, Box->Rec);
 
 // Draw notes
 {
  note *NoteArrays[] = {Piece->Notes, Piece->PracticeNotes};
  u64 NoteCounts[] = {Piece->NoteCount, Piece->PracticeNoteCount};
  
  for EachElement(ArrayIdx, NoteArrays)
  {
   note *Notes = NoteArrays[ArrayIdx];
   u64 NoteCount = NoteCounts[ArrayIdx];
   
   s32 LowestPitch = S32Max;
   s32 HighestPitch = S32Min;
   for(u64 Idx = 1; Idx < NoteCount; Idx += 1)
   {
    LowestPitch = Min(LowestPitch, Notes[Idx].Pitch);
    HighestPitch = Max(HighestPitch, Notes[Idx].Pitch);
   }
   
   for(u64 NoteIdx = 1; NoteIdx < NoteCount; NoteIdx += 1)
   {
    note *Note = Notes + NoteIdx;
    
    b32 IsHovered = false;
    b32 IsSelected = false;
    
    v2 NotePos = Pos;
    
    b32 NoteIsRecording = false;
    f32 NoteLength = Note->Length;
    if(NoteLength == 0.f)
    {
     NoteIsRecording = true;
     NoteLength = (Piece->IsRecording ? 
                   Piece->RecordLength - Note->Start : 
                   Piece->PracticePos - Note->Start);
    }
    
    v4 NoteColor = StaffColor;
    
    b32 NoteIsPlaying = false;
    if(Piece->IsPlaying)
    {
     f32 Start = Note->Start;
     f32 End = Start + NoteLength;
     
     NoteIsPlaying = (Piece->PlayPos >= Start &&
                      Piece->PlayPos < End);
    }
    
    NotePos = PiecePosFromTime(Piece, Note->Start, Pos, false, BarsPerWidth, StaffHeight);
    NotePos.Y += .5f*StaffLineWidth;
    NotePos.Y += ((f32)Note->StaffIdx*2.f*StaffHeight);
    
    // Selected
    {
     for EachNode(Node, note_node, Piece->SelectionFirst)
     {
      if(Node->V == Note)
      {
       IsSelected = true;
       break;
      }
     }
    }
    
    if(0) {}
    else if(Note->Kind == NoteKind_Rest)
    {
     // Each increment is halving the value.
     s32 WholeRest = 0xE4E3;
     f32 WholeRestLength = 4.f;
     
     font *Font = UI_State->Fonts[FontKind_Music];
     
     f32 Size = -Log2F32(NoteLength/WholeRestLength);
     s32 Increment = (s32)RoundF32(Size);
     s32 Codepoint = WholeRest + Increment;
     
     rect_instance *Instance = DrawRectChar(UI_State->Atlas, Font, NotePos, Codepoint, NoteColor);
     
     if(LeftInputIsOurs || RightInputIsOurs)
     {
      IsHovered = RectOverlap(SelRec, Instance->Dest);
     }
     
     if(IsHovered)
     {
      RenderRectSetColor(Instance, Color_Blue);
     }
     
     
     if(IsSelected)
     {
      RenderRectSetColor(Instance, Color_Orange);
     }
     
    }
    else if(Note->Kind == NoteKind_Note && !Note->Hidden)
    {
     s32 RelPitch = Note->Pitch%Note_Count;
     b32 FillHead = (NoteLength < 2.f);;
     s32 FlagCount = 0;
     b32 HasStem = (NoteLength < 4.f);
     b32 IsDotted = false;
     b32 IsBarred = false;
     b32 IsSharp = !GetEl(NoteIsWhite, RelPitch);
     v2 TailPos = {0};
     b32 DebugDrawBoundingBox = false;
     
     // Calculate position
     {
      staff *Staff = Piece->Staffs + Note->StaffIdx;
      s32 RelOctave = Staff->Octave;
      // Get Y From pitch
      // Sets the notes at the octave's C
      s32 MinStepCount = -(BaseNote_Count + 2);
      if(Staff->ClefKind == ClefKind_Bass)
      {
       MinStepCount = -4;
      }
      
      f32 StepSize = NoteSize/2.f;
      
      s32 NoteOctave = Note->Pitch/Note_Count;
      s32 StepForPitch = GetEl(NoteStepFromPitch, RelPitch);
      s32 StepCount = BaseNote_Count*(RelOctave - NoteOctave);
      StepCount -= MinStepCount + StepForPitch;
      
      NotePos.Y += StepSize*(f32)StepCount;
      
      TailPos = NotePos;
      {
       TailPos.X += NoteDim.X - 2.f;
       TailPos.Y -= TailDim.Y - .5f*NoteDim.Y;
      }
     }
     
     v4 HeadBounds = {0};
     v4 TailBounds = {0};
     // Is Hovered and compute bounds
     {
      HeadBounds = RectFromSize(NotePos, NoteDim);
      
      if(LeftInputIsOurs || RightInputIsOurs)
      {     
       IsHovered = RectOverlap(SelRec, HeadBounds);
       
       if(HasStem && !IsHovered)
       {
        f32 ExtraWidth = 3.f;
        TailBounds = RectFromSize(TailPos, TailDim);
        TailBounds.Min.X -= ExtraWidth;
        TailBounds.Max.X += ExtraWidth;
        IsHovered = RectOverlap(SelRec, TailBounds);
       }
      }
     }
     
     if(NoteIsPlaying || NoteIsRecording) NoteColor = Color_Yellow;
     
     if(IsHovered)
     {
      NoteColor = Color_Blue;
     }
     
     if(IsSelected)
     {
      NoteColor = Color_Orange;
     }
     
     if(ArrayIdx == 1) NoteColor = Color_Yellow;
     
     if(Piece->IsPracticing && ArrayIdx != 1) NoteColor.W *= 0.5f;
     
     // Find out if is dotted
     {
      f32 Exp = -Log2F32(NoteLength);
      f32 Remainder = ModF32(Exp, 1.f);
      IsDotted = !(EqualsWithEpsilon(Remainder, 0.f, 0.0001f));
      IsDotted = IsDotted && (ArrayIdx != 1);
     }
     
     // Find out if is barred
     {
      IsBarred = (Note->Pitch == Note_C);
     }
     
     if(HasStem)
     {
      // Draw Flags
      {
       FlagCount = (s32)(-Log2F32(NoteLength));
       v2 FlagPos = TailPos;
       
       for EachCount(FlagCount)
       {
        v4 Dest = RectFromSize(FlagPos, V2(8.f, TailDim.X*2.f));
        DrawRect(Dest, NoteColor, 0.f, 0.f, 0.f);
        
        FlagPos.Y += 5.f;
       }
      }
      
      // Draw Stem
      {  
       v4 Dest = RectFromSize(TailPos, TailDim);
       DrawRect(Dest, NoteColor, 0.f, 2.f, .5f);
      }
     }
     
     // Draw Head
     {
      v4 Dest = RectFromSize(NotePos, NoteDim);
      f32 BorderThickness = (FillHead ? 0.f : 2.f);
      DrawRect(Dest, NoteColor, NoteDim.X/2.f, BorderThickness, .5f);
     }
     
     // Draw dot
     if(IsDotted)
     {
      f32 dSize = 5.f;
      
      v2 DotDim = V2SubF32(NoteDim, dSize);
      v2 DotPos = NotePos;
      DotPos.X += NoteDim.X + 2.f;
      DotPos.Y = NotePos.Y + dSize/2.f;
      v4 Dest = RectFromSize(DotPos, DotDim);
      f32 BorderThickness = (FillHead ? 0.f : 2.f);
      DrawRect(Dest, NoteColor, DotDim.X/2.f, BorderThickness, .5f);
     }
     
     // Draw barred
     if(IsBarred)
     {
      // TODO(luca): Should draw bars above / below until reaching back to the staff. 
      f32 Portrusion = 1.f;
      v2 HeadBarDim = V2(NoteDim.X + Portrusion*2.f, 2.f);
      v2 HeadBarPos = NotePos;
      HeadBarPos.X -= Portrusion;
      HeadBarPos.Y += .5f*(NoteDim.Y - HeadBarDim.Y);
      v4 Dest = RectFromSize(HeadBarPos, HeadBarDim);
      DrawRect(Dest, StaffColor, 0.f, 0.f, 0.f);
     }
     
     // Draw sharp
     if(IsSharp)
     {
      v2 SharpPos = NotePos;
      SharpPos.X -= NoteSize;
      SharpPos.Y -= 4.f;
      
      font *Font = UI_State->Fonts[FontKind_Text];
      font_atlas *Atlas = UI_State->Atlas;
      s32 Codepoint = '#';
      
#if 0
      Font = UI_State->Fonts[FontKind_Music];
      SharpPos.Y -= Font->HeightPx/4.f;
      SharpPos.X += 3.f;
      Codepoint = 0xE262;
#endif
      
      DrawRectChar(Atlas, Font, SharpPos, Codepoint, NoteColor);
     }
     
     if(DebugDrawBoundingBox)
     {
      {
       if(((Note->Pitch - !!IsSharp) & 1))
        NotePos.Y += .5f;
      }
      DrawRect(HeadBounds, Color_Blue, 0.f, 1.f, 0.f);
      DrawRect(TailBounds, Color_Blue, 0.f, 1.f, 0.f);
     }
     
    }
    
    if(IsHovered && (LeftPressed || RightPressed))
    {
     PushCommand(App, (Adding ? 
                       Command_PieceSelectNote :
                       Command_PieceDeselectNote))->U64 = NoteIdx;
    }
    
   }
  }
 }
 
 if(LeftPressed || RightPressed)
 {
  v4 BorderColor = (!RightPressed ?
                    V4(V3Arg(Color_Black), 1.f) :
                    V4(V3Arg(Color_Snow2), 1.f));
  v4 Color = (!RightPressed ? 
              V4(V3Arg(Color_Blue), .3f) :
              V4(V3Arg(Color_Orange), .3f));
  
  DrawRect(SelRec, Color,       3.f, 0.f, 0.f);
  DrawRect(SelRec, BorderColor, 3.f, 1.f, 0.f);
 }
 
}


UI_CUSTOM_DRAW(CustomDrawSingleLineTextInput)
{
 custom_draw__single_line_text_input_params *Params = (custom_draw__single_line_text_input_params *)CustomDrawData;
 ui_box *Box = Params->Box;
 str8 Text = Params->Text;
 font_atlas *Atlas = UI_State->Atlas;
 f32 CursorAnimTime = Params->CursorAnimTime;
 
 DrawRect(Box->Rec, Color_Cyan, 0.f, 0.f, 0.f);
 
 f32 HorizontalPadding = 5.f;
 
 v2 Cur = Box->FixedPos;
 Cur.X += HorizontalPadding;
 Cur.Y += RoundF32(.5f*(Box->FixedSize.Y - Box->HeightPx));
 
 font *Font = UI_State->Fonts[FontKind_Text];
 s32 PackedCharIdx = GetPackedCharIdx(Font, ' ');
 f32 EllipsisWidth = Atlas->PackedChars[PackedCharIdx].xadvance;
 
 f32 CurMaxX = (Box->FixedPos.X + Box->FixedSize.X - 
                (HorizontalPadding + EllipsisWidth));
 b32 Overflowed = false;
 for EachIndex(Idx, Text.Size)
 {
  rune Char = Text.Data[Idx]; 
  
  PackedCharIdx = GetPackedCharIdx(Font, Char);
  f32 CharWidth = (Atlas->PackedChars[PackedCharIdx].xadvance);
  
  DrawRectChar(Atlas, Font, Cur, Char, Box->TextColor);
  Cur.X += CharWidth;
  
  if(Cur.X > CurMaxX)
  {
   break;
  }
 }
 
 v2 CursorPos = Cur;
 v4 CurRec = RectFromSize(CursorPos, V2(1.f, Box->HeightPx));
 if(Box->Flags & UI_BoxFlag_Clip)
 {
  CurRec = RectIntersect(Box->Rec, CurRec);
 }
 
 if(RectValid(CurRec))
 {
  f32 Speed = 2.f;
  v4 Color = Box->TextColor;
  if(cos(Pi32*CursorAnimTime*Speed) < 0.f)
  {
   Color.A = 0.f;
  }
  DrawRect(CurRec, Color, 0.f, 0.f, 0.f);
 }
}

//~ EntryPoint
C_LINKAGE
UPDATE_AND_RENDER(UpdateAndRender)
{
 b32 ShouldQuit = false;
 
 //- Inherit globals from platform layer
#if MUZE_INTERNAL
 GlobalDebuggerIsAttached = Memory->IsDebuggerAttached;
#endif
#if OS_WINDOWS
 GlobalPerfCountFrequency = Memory->PerfCountFrequency;
#endif
 GlobalIsProfiling = Memory->IsProfiling;
 ThreadContext = Memory->ThreadCtx;
 ExeDirPath = Memory->ExeDirPath;
 PlatformMIDIGetDevices = Memory->PlatformMIDIGetDevices;
 PlatformMIDIListen = Memory->PlatformMIDIListen;
 PlatformMIDISend = Memory->PlatformMIDISend;
 
 //- Some constants 
 Input->PlatformCursor = PlatformCursorShape_Arrow;
 v2 BufferDim = V2V2S32(Buffer->Dim);
 dtForFrame = &Input->dtForFrame;
 
 //- Init state 
 OS_ProfileInit(" G");
 
 str8 ConfigPath = S8("config.muze_settings");
 
 arena *PermanentArena;
 app_state *App;
 {
  {
   umm MemoryPos = (umm)Memory->Memory;
   umm AlignedPos = AlignPow2(MemoryPos, AlignOf(arena));
   PermanentArena = (arena *)AlignedPos;
   
   PermanentArena->Size = Memory->MemorySize - sizeof(arena);
   PermanentArena->Base = (u8 *)AlignedPos + sizeof(arena);
   PermanentArena->Pos = 0;
   AsanPoisonMemoryRegion(PermanentArena->Base, PermanentArena->Size);
  }
  
  // App
  {        
   App = PushArray(PermanentArena, app_state, 1);
   
   App->FontAtlas.Arena = PushArena(PermanentArena, MB(100), false);
   App->UIArena = PushArena(PermanentArena, MB(64), false);
   App->Arena = PushArena(PermanentArena, MB(64), false);
   App->TSFArena = PushArena(PermanentArena, MB(500), false);
   App->ReadOnlyArena = PushArray(PermanentArena, arena, 1);
   App->PanelArena = PushArena(PermanentArena, MB(64), false);
   App->PieceArena = PushArena(PermanentArena, MB(16), false);
  }
  
  // Globals
  {
   FrameArena = PushArena(PermanentArena, MB(64), true);
   SetStringsScratch(FrameArena);
   
   UI_State = PushArray(PermanentArena, ui_state, 1);
   NilTSF = PushArray(PermanentArena, tsf, 1);
   NilTSF->arena = App->TSFArena;
   
   // Panels
   {
    PanelInput = Input;
    PanelApp = App;
    PanelArena = App->PanelArena;
   }
   
   GlobalTSF = App->TrackerForTSF;
   
   DragData = PushArray(PermanentArena, drag_data, 1);
  }
 }
 
 platform_midi_get_devices_result Devices = PlatformMIDIGetDevices();
 
 local_persist s32 GLADVersion = 0;
 if(Memory->Reloaded)
 {
  GLADVersion = gladLoaderLoadGL();
  OS_ProfileAndPrint("glad Init");
 }
 
 if(!Memory->Initialized)
 {
  //- Init TSF 
  {        
   char *Path = PathFromExe(S8("sounds.sf2"));
   str8 File = OS_ReadEntireFileIntoMemory(Path);
   GlobalTSF = tsf_load_memory(App->TSFArena, File.Data, (int)File.Size);
   if(GlobalTSF == 0)
   {
    GlobalTSF = NilTSF;
   }
   tsf_set_output(GlobalTSF, TSF_STEREO_INTERLEAVED, 48000, -10);
   
   s32 InstrumentCount = tsf_get_presetcount(GlobalTSF);
   App->InstrumentCount = InstrumentCount;
   App->InstrumentNames = PushArray(App->TSFArena, str8, (umm)InstrumentCount);
   
   for EachIndex(Idx, InstrumentCount)
   {
    App->InstrumentNames[Idx] = S8DupCString(App->TSFArena, (char *)tsf_get_presetname(GlobalTSF, Idx));
   }
   
   App->TrackerForTSF = GlobalTSF;
   
   s32 NilChannel = TSFNextChannel(App); // Reserve nil channel
   tsf_channel_set_volume(GlobalTSF, NilChannel, 0.f);
   
   App->MetronomeChannel = TSFNextChannel(App);
   App->MetronomePreset = 115; // Woodblock
   tsf_channel_set_presetindex(GlobalTSF, App->MetronomeChannel, App->MetronomePreset);
  }
  
  OS_ProfileAndPrint("TSF init");
  
  //- Init Muze 
  {
   App->MaxTextSize = 256;
   App->Text = PushS8(App->Arena, App->MaxTextSize);
   App->Text.Size = 0;
   
   App->MaxCommandCount = KB(4);
   App->Commands = PushArray(App->Arena, command, App->MaxCommandCount);
   
   App->MaxPieceCount = (App->PieceArena->Size/sizeof(piece));
  }
  
  OS_ProfileAndPrint("Muze init");
  
  //- Init Fonts
  {
   {
    InitFontFromKind(&App->TextFont, FontKind_Text);
    InitFontFromKind(&App->IconsFont, FontKind_Icons);
    InitFontFromKind(&App->MusicFont, FontKind_Music);
   }
   
   App->HeightPx = DefaultHeightPx;
   App->PreviousHeightPx = App->HeightPx + 1.f;
   App->FrameIdx = 0;
  }
  
  OS_ProfileAndPrint("Font init");
  
  //- Read only structs 
  {        
   arena *Arena = App->ReadOnlyArena;
   
   // Initialize arena
   {            
    MemoryZero(Arena);
    Arena->Size = MB(1);
    
    umm Offset = TB(2) + GB(64);
#if !MUZE_INTERNAL || ASAN_ENABLED
    Offset = 0;
#endif
    
    Arena->Base = OS_AllocateAtOffset(Arena->Size, Offset);
   }
   
   // Initialize read only structs
   {
    InitReadOnlyGlobals(Arena);
    
    *UI_NilBox = (ui_box){UI_NilBox, UI_NilBox, UI_NilBox, UI_NilBox, UI_NilBox, UI_NilBox, UI_NilBox};
    *NilPanel = (panel){NilPanel, NilPanel, NilPanel, NilPanel, NilPanel};
    *NilPiece = (piece){NilPiece};
   }
   
   OS_MarkReadonly(Arena->Base, Arena->Size);
  }
  
  OS_ProfileAndPrint("RO Init");
  
  RenderInit(&App->Render);
  OS_ProfileAndPrint("Renderer Init");
  
  // Init UI 
  {
   UI_InitState(App->UIArena);
   UI_State->Fonts[FontKind_Text] = &App->TextFont;
   UI_State->Fonts[FontKind_Icons] = &App->IconsFont;
   UI_State->Fonts[FontKind_Music] = &App->MusicFont;
   App->ListerBox = UI_BoxAlloc(UI_State->Arena);
   App->TopBox = UI_BoxAlloc(UI_State->Arena);
  }
  
  // Init panels
  {
   ArenaSetPos(App->PanelArena, 0);
   App->FreePanel = NilPanel;
   App->SelectedPanel = NilPanel;
   App->DebugPanel = NilPanel;
   PanelCurrent = NilPanel;
   PanelAppendToParent = false;
   
   // Default panel setup
   {
    PanelAxis(Axis2_X)
    {            
     App->FirstPanel = PanelAdd(1.f);
     PanelChildren()
     {
      PanelAdd(.6f);
      PanelAdd(.4f);
      PanelChildren()
      {
       PanelAdd(.5f)->Kind = PanelKind_Controls;
       PanelAdd(.5f)->Kind = PanelKind_Debug;
      }
     }
    }
    
    App->SelectedPanel = PanelNextLeaf(App->FirstPanel, false);
    App->SelectedPanel->Kind = PanelKind_Piece;
    
   }
  }
  
  MS_SerializeConfig(FrameArena, true, ConfigPath, App);
  
  // MIDI: Select default input and output device, select's last device by default
  {
   b32 ConfigHasInputDevice = App->In.IsInput;
   b32 ConfigHasOutputDevice = App->Out.IsOutput;
   b32 ConfigInputFound = false;
   b32 ConfigOutputFound = false;
   s32 FoundInputIdx = 0;
   s32 FoundOutputIdx = 0;
   
   for EachIndex(Idx, Devices.Count)
   {
    platform_midi_device *Device = Devices.Devices + Idx;
    
    b32 IsDefaultDev = false;
    
    if(Device->IsInput)
    {
     if(ConfigHasInputDevice)
     {
      if(!ConfigInputFound)
      {
       ConfigInputFound = (App->In.Id == Device->Id);
       FoundInputIdx = Idx;
      }
     }
     
     if(!ConfigInputFound)
     {
      FoundInputIdx = Idx;
     }
    }
    
    if(Device->IsOutput)
    {
     if(ConfigHasOutputDevice)
     {
      if(!ConfigOutputFound)
      {
       ConfigOutputFound = (App->Out.Id == Device->Id);
       FoundOutputIdx = Idx;
      }
     }
     
     if(!ConfigOutputFound)
     {
      FoundOutputIdx = Idx;
     }
    }
   }
   
   PushCommand(App, Command_SelectInputDevice)->S32 = FoundInputIdx;
   PushCommand(App, Command_SelectOutputDevice)->S32 = FoundOutputIdx;
  }
  
  App->MaxListerEntryCount = KB(64);
  App->MaxDebugStringCount = 128;
 }
 
 // Frame Initialization
 {
  // Init read-only variables
  {    
   InitReadOnlyGlobals(App->ReadOnlyArena);
  }
  
  OS_ProfileAndPrint("Misc. Init");
 }
 
 if(App->ListerOpened)
 {            
  for EachTextButton(Key, Id, Input)
  {
   b32 Handled = false;
   
   s32 ModifiersWithoutShift = (Key->Modifiers & ~PlatformKeyModifier_Shift);
   
   if(Key->Modifiers == PlatformKeyModifier_Control &&
      !Key->IsSymbol &&
      Key->Codepoint == 'u')
   {
    App->Text.Size = 0;
    
    Handled = true;
   }
   
   if(ModifiersWithoutShift == PlatformKeyModifier_None)
   {
    if(!Key->IsSymbol)
    {
     App->Text.Data[App->Text.Size] = (u8)Key->Codepoint;
     App->Text.Size += 1;
     App->CursorAnimTime = 0.f;
     
     Handled = true;
    }
    else
    {
     Handled = true;
     
     switch(Key->Symbol)
     {
      default: 
      {
       Handled = false; 
      } break;
      
      case PlatformKey_Escape:
      {
       PushCommand(App, Command_CloseLister);
      } break;
      
      case PlatformKey_Return:
      {
       App->ListerReturnPressed = true;
      } break;
      
      case PlatformKey_BackSpace:
      {
       App->Text.Size -= !!(App->Text.Size > 0);
       App->CursorAnimTime = 0.f;
      } break;
     }
    }
   }
   
   if(Handled)
   {
    RemoveTextButton(Input, Id);
   }
  }
 }
 
 for EachTextButton(Key, Id, Input)
 {
  b32 Handled = false;
  
#if MUZE_INTERNAL  
  if(!Key->IsSymbol && Key->Modifiers == PlatformKeyModifier_Alt && Key->Codepoint == 'b')
  {
   DebugBreak();
   
   Handled = true;
   goto KeyHandled;
  }
#endif
  
  // NOTE(luca): Check if key bind was pressed and push command accordingly
  {  
   for EachElement(Idx, CommandsMetaInfo)
   {
    command_meta_info *Info = CommandsMetaInfo + Idx;
    
    s32 Mask = 0;
    if(Info->Shift)   Mask |= PlatformKeyModifier_Shift;
    if(Info->Control) Mask |= PlatformKeyModifier_Control;
    if(Info->Alt)     Mask |= PlatformKeyModifier_Alt;
    
    b32 IsMatch = false;
    
    b32 KeyIsAlpha = (PlatformKey_A <= Info->Key && Info->Key <= PlatformKey_9);  
    
    if(!Key->IsSymbol && KeyIsAlpha)
    {
     if(PlatformKey_A <= Info->Key && Info->Key <= PlatformKey_Z)
     {
      IsMatch = (Key->Codepoint == (rune)('a' + (Info->Key - PlatformKey_A)));
     }
     else if(PlatformKey_0 <= Info->Key && Info->Key <= PlatformKey_9)
     {
      IsMatch = (Key->Codepoint == (rune)('0' + (Info->Key - PlatformKey_0)));
     }
     else if(Info->Key == PlatformKey_Minus)
     {
      IsMatch = (Key->Codepoint == '-');
     }
     else if(Info->Key == PlatformKey_Comma)
     {
      IsMatch = (Key->Codepoint == ',');
     }
     else if(Info->Key == PlatformKey_Space)
     {
      IsMatch = (Key->Codepoint == ' ');
     }
    }
    else
    {
     IsMatch = (Key->Codepoint == (rune)Info->Key);
    }
    
    if(IsMatch && Key->Modifiers == Mask)
    {
     PushCommand(App, Info->Command);
     // TODO(luca): We could still run to detect conflicting key binds.
     Handled = true;
     break;
    }
    
   }
  }
  
  if(Handled) goto KeyHandled;
  Handled = true;
  
  KeyHandled:;
  if(Handled)
  {
   RemoveTextButton(Input, Id);
  }
 }
 
 OS_ProfileAndPrint("Input");
 
 if(App->PreviousHeightPx != App->HeightPx)
 {
#if MUZE_INTERNAL && 0
  gl_render_state *Render = &App->Render;
  glDeleteTextures(ArrayCount(Render->Textures), &Render->Textures[0]);
  glGenTextures(ArrayCount(Render->Textures), &Render->Textures[0]);
#endif
  
  App->PreviousHeightPx = App->HeightPx;
  
  font *Fonts[] = {&App->TextFont, &App->IconsFont, &App->MusicFont};
  
  RenderBuildAtlas(&App->FontAtlas, ArrayCount(Fonts), Fonts, App->HeightPx);
 }
 
 RenderBeginFrame(FrameArena, 0, 0, Buffer->Dim.X, Buffer->Dim.Y);
 
 OS_ProfileAndPrint("Atlas");
 
 // UI
 {
  f32 ItemHeight = App->HeightPx + 2.f*8.f;
  ui_size ItemPadding = UI_SizePx(5.f, 1.f);
  GlobalItemPadding = ItemPadding;
  f32 ListWidth = 1600.f/6.f;
  b32 SoundFontLoaded = (GlobalTSF != NilTSF);
  
  // Setup
  {        
   UI_BeginFrame(&App->FontAtlas, App->FrameIdx, Input);
   OS_ProfileAndPrint("Render setup");
   
   App->DebugStrings = PushArray(FrameArena, str8, App->MaxDebugStringCount);
   App->DebugStringTags = PushArray(FrameArena, str8, App->MaxDebugStringCount);
   App->DebugStringCount = 0;
   { 
#if MUZE_INTERNAL
    if(!Memory->Reloaded)
    {     
     // NOTE(luca): When DLL is reloaded strings may have their location changed.  So, to not 
     // dereference them we just skip the frame where reloaded.
     StringsScratchTemp(UI_State->FrameArenaBack)
     {
      
      {
       str8 ButtonTypeStrings[3] = {S8("L"), S8("R"), S8("M")};
       for EachElement(ButtonType, UI_State->Hot)
       {        
        ui_box *Hot = UI_BoxFromKey(UI_State->Hot[ButtonType]);
        ui_box *Active = UI_BoxFromKey(UI_State->Active[ButtonType]);
        str8 ActiveString = Active->DisplayString;
        str8 HotString = Hot->DisplayString;
        if(ActiveString.Size == 0) ActiveString = S8("null");
        if(HotString.Size == 0) HotString = S8("null");
        
        str8 ButtonTypeString = ButtonTypeStrings[ButtonType];
        
        DebugStringAdd(App, S8("ui"), "%SActive: %S", ButtonTypeString, ActiveString);
        DebugStringAdd(App, S8("ui"), "%SHot: %S", ButtonTypeString, HotString);
        DebugStringAdd(App, S8("ui"), "%SActive->tHot = %.2f", ButtonTypeString, Active->tHot);
        DebugStringAdd(App, S8("ui"), "%SActive->tActive = %.2f", ButtonTypeString, Active->tActive);
        DebugStringAdd(App, S8("ui"), "%SHot->tHot = %.2f", ButtonTypeString, Hot->tHot);
        DebugStringAdd(App, S8("ui"), "%SHot->tActive = %.2f", ButtonTypeString, Hot->tActive);
       }
      }
      
      DebugStringAddArena(App, S8("Perm"), PermanentArena);
      DebugStringAddArena(App, S8("Frame"), FrameArena);
      DebugStringAddArena(App, S8("R/O"), App->ReadOnlyArena);
      DebugStringAddArena(App, S8("Font"), App->FontAtlas.Arena);
      DebugStringAddArena(App, S8("UI"), App->UIArena);
      DebugStringAddArena(App, S8("Panel"), App->PanelArena);
      DebugStringAddArena(App, S8("Muze"), App->Arena);
      DebugStringAddArena(App, S8("TSF"), App->TSFArena);
     }
    }
#endif
   }
   
   OS_ProfileAndPrint("UI setup");
  }
  
  //- Lister UI 
  if(App->ListerOpened)
  {
   s32 ShowItemCount = 10;
   f32 ListerBorderSize = 2.f;
   
   // Input
   s32 SelectedIdx = 0; 
   s32 StartIdx = 0;
   str8 Title = {0};
   
   s32 EntryCount = 0;
   lister_entry *Entries = PushArrayZero(FrameArena, lister_entry, (u64)App->MaxListerEntryCount);
   
   // Output
   platform_midi_device *OutDevices = 0;
   
   s32 FilteredEntryCount = 0;
   lister_entry *FilteredEntries = PushArray(FrameArena, lister_entry, (u64)App->MaxListerEntryCount);
   
   switch(App->ListerKind)
   {
    default: InvalidPath(); break;
    
    case ListerKind_DebugInfo:
    {
     Title = S8("Search info");
     SelectedIdx = -1;
     
     EntryCount = (s32)App->DebugStringCount;
     
     for EachIndex(Idx, EntryCount)
     {
      lister_entry *Entry = Entries + Idx;
      Entry->Text = App->DebugStrings[Idx];
      Entry->Tag = App->DebugStringTags[Idx];
     }
    } break;
    
    case ListerKind_Commands:
    {
     StartIdx = 1;
     
     for EachIndex(Idx, Command_Count)
     {
      lister_entry *Entry = Entries + EntryCount;
      piece *Piece = App->SelectedPanel->Piece;
      
      if(!CommandsMetaInfo[Idx].NeedsParam)
      {
       Entry->Tag = CommandsMetaInfo[Idx].Tag;
       Entry->Text = CommandsMetaInfo[Idx].Description;
       Entry->Command = (command_kind)Idx;
       
       EntryCount += 1;
      }
      
      b32 IsToggle = true;
      switch(Idx)
      {
       default: 
       {
        IsToggle = false;
       } break;
       
#define CaseEntryFromB32(CommandName, Value) \
case Command_##CommandName: Entry->F32 = (f32)Value; break;
       
       case Command_PieceSelectInstrument:
       {
        Entry->Command = Command_ToggleOutputSynth;
        Entry->ButtonCommand = Command_OpenLister;
        Entry->F32 = (f32)!App->OutputSynthDisabled;
        Entry->S32 = ListerKind_Instruments;
       } break;
       
       case Command_PieceSetBPM: 
       {
        IsToggle = false;
        Entry->F32 = Piece->BPM; break;
       }
       
       CaseEntryFromB32(ToggleMIDIInput, !App->InputMIDIDisabled);
       CaseEntryFromB32(ToggleMIDIOutput, App->OutputMIDIEnabled);
       CaseEntryFromB32(ToggleOutputSynth, !App->OutputSynthDisabled);
       CaseEntryFromB32(ToggleVirtualKeyboardInput, !App->InputVirtualKeyboardDisabled);
       CaseEntryFromB32(PieceToggleLooping, !Piece->LoopingDisabled);
       CaseEntryFromB32(PieceToggleStrictTiming, Piece->PracticeStrictTiming);
       CaseEntryFromB32(PieceToggleBarWrapping, Piece->BarWrapping);
       CaseEntryFromB32(PieceToggleRecording, Piece->IsRecording);
       CaseEntryFromB32(PieceTogglePlaying, Piece->IsPlaying);
       CaseEntryFromB32(PieceTogglePracticing, Piece->IsPracticing);
       CaseEntryFromB32(PieceToggleMetronome, Piece->Metronome);
      }
      
      command_meta_info *Info = CommandsMetaInfo + Idx;
      
#if MUZE_INTERNAL     
      // Check to add toggle values to commands if forgotten.
      {      
       if(!IsToggle && 
          Info->Min == 0.f && 
          Info->Max == 1.f && 
          Info->Step == 1.f)
       {
        NotImplemented();
       }
      }
#endif
      
      Entry->Min = Info->Min;
      Entry->Max = Info->Max;
      Entry->Step = Info->Step;
     }
     
    } break;
    
    case ListerKind_Instruments:
    {
     Title = S8("Select instrument");
     
     EntryCount = App->InstrumentCount;
     
     if(0) {}
     else if(App->SelectedPanel->Kind == PanelKind_Piece) 
     {
      SelectedIdx = App->SelectedPanel->Piece->Preset; 
     }
     
     for EachIndex(Idx, EntryCount)
     {
      lister_entry *Entry = Entries + Idx;
      Entry->Text = App->InstrumentNames[Idx];
      Entry->Command = Command_SelectInstrument;
      Entry->S32 = Idx;
     }
     
    } break;
    
    case ListerKind_PanelTypes:
    {
     EntryCount = PanelKind_Count;
     
     Title = S8("Select panel type");
     panel *Panel = App->SelectedPanel;
     SelectedIdx = (s32)Panel->Kind;
     
     for EachIndex(Idx, EntryCount)
     {
      lister_entry *Entry = Entries + Idx;
      Entry->Text = PanelTypeStrings[Idx];
      Entry->Command = Command_SetPanelType;
      Entry->S32 = Idx;
     }
     
     StartIdx = 1;
#if MUZE_INTERNAL
     StartIdx = 2;
#endif
    } break;
    
    
    case ListerKind_MIDIInput:
    case ListerKind_MIDIOutput:
    {
     Title = S8("Select device");
     
     b32 IsInput = (App->ListerKind == ListerKind_MIDIInput);
     
     for EachIndex(Idx, Devices.Count)
     {
      platform_midi_device *Device = Devices.Devices + Idx;
      
      b32 DeviceIsGood = ((IsInput && Device->IsInput) ||
                          (!IsInput && Device->IsOutput));
      u64 SelectedDeviceId = (IsInput ? App->In.Id : App->Out.Id);
      
      if(DeviceIsGood)
      {
       lister_entry *Entry = Entries + EntryCount;
       
       Entry->Command = (IsInput ? Command_SelectInputDevice : Command_SelectOutputDevice);
       Entry->Text = Device->Name;
       Entry->S32 = (s32)Idx;
       
       if(Device->Id == SelectedDeviceId)
       {
        SelectedIdx = EntryCount;
       }
       EntryCount += 1;
      }
     }
    } break;
    
    
   }
   
   for(s32 Idx = StartIdx; Idx < EntryCount; Idx += 1)
   {
    lister_entry *Entry = Entries + Idx;
    str8 Name = Str8Fmt("%S %S", Entry->Tag, Entry->Text);
    
    b32 Matched = false;
    
    // Match if Name contains App->Text case-insensitive
    {
     piece *Piece = App->SelectedPanel->Piece;
     
     if(IsNilPiece(Piece) && S8Match(Entry->Tag, S8("Piece"), 0))
     {
      Matched = false;
     }
     else if(App->Text.Size == 0)
     {
      Matched = true;
     }
     else
     {                    
      u64 TextIdx = 0;
      for EachIndex(NameIdx, Name.Size)
      {
       if(ToLowercase(Name.Data[NameIdx]) == 
          ToLowercase(App->Text.Data[TextIdx]))
       {
        Matched = true;
       }
       else if(Matched)
       {
        // NOTE(luca): Start over
        Matched = false;
        TextIdx = 0;
       }
       
       if(Matched)
       {
        TextIdx += 1;
        if(TextIdx == App->Text.Size)
        {
         break;
        }
       }
      }
      
      // NOTE(luca): Whole text must have matched
      if(TextIdx < App->Text.Size)
      {
       Matched = false;
      }
     }
    }
    
    if(Matched)
    {
     Entry->Idx = Idx;
     FilteredEntries[FilteredEntryCount] = *Entry;
     FilteredEntryCount += 1;
    }
   }
   
   v2 ListerDim = V2(600.f, (f32)ShowItemCount*ItemHeight);
   V2Math ListerDim.E += 2.f*ListerBorderSize;
   ListerDim.Y += ItemHeight + ListerBorderSize;
   
   UI_BeginLayout(App->ListerBox, Buffer->Dim, App->HeightPx);
   {        
    UI_FillAll()
     UI_Column() UI_Center()
    {           
     
     UI_SemanticHeight(UI_SizeChildren(1.f))
      UI_Row() UI_Center()
     {
      UI_SemanticWidth(UI_SizePx(ListerDim.X, 1.f))
       UI_SemanticHeight(UI_SizePx(ListerDim.Y, 1.f))
       UI_BorderThickness(ListerBorderSize)
       UI_BorderColor(Color_Snow2)
       UI_BackgroundColor(Color_Night0)
       UI_AddBox(S8("Lister"), 
                 UI_BoxFlag_MouseClickable|
                 UI_BoxFlag_Clip|
                 UI_BoxFlag_DrawBorders|
                 UI_BoxFlag_DrawBackground|
                 UI_BoxFlag_DrawShadow);
      
      UI_PaddingAround(UI_SizePx(ListerBorderSize, 1.f)) 
      {
       UI_Column()
       {
        ui_box *SelectedBox;
        
        // Top input box
        UI_FillWidth()
         UI_SemanticHeight(UI_SizePx(ItemHeight, 1.f))
         UI_Row()
        {
         ui_box *InputBox;
         
         if(Title.Size)
         {          
          UI_BackgroundColor(Color_Blue)
           UI_SemanticWidth(UI_SizeText(GlobalItemPadding.Value, 1.f))
           M_Label(Title);
         }
         
         UI_BackgroundColor(Color_Blue)
          UI_FillAll()
          InputBox = UI_AddBox(S8("InputBox"), UI_BoxFlag_DrawBackground);
         
         custom_draw__single_line_text_input_params *Params = PushArray(FrameArena, custom_draw__single_line_text_input_params, 1);
         Params->Box = InputBox;
         Params->Text = App->Text;
         Params->CursorAnimTime = App->CursorAnimTime;
         InputBox->CustomDraw = CustomDrawSingleLineTextInput;
         InputBox->CustomDrawData = Params;
        }
        
        UI_FillWidth()
         UI_SemanticHeight(UI_SizePx(ListerBorderSize, 1.f))
         UI_BackgroundColor(Color_Snow2)
         UI_AddBox(S8("Separator"),
                   UI_BoxFlag_Clip|
                   UI_BoxFlag_DrawBackground);
        
        UI_Row()
        {
         s32 MinIdx;
         s32 MaxIdx;
         
         // Items
         {                            
          UI_BackgroundColor(Color_Red)
           UI_FillAll()
           UI_LayoutAxis(Axis2_Y)
          {
           ui_box *Box = UI_AddBox(S8("Items"), UI_BoxFlag_Clip|UI_BoxFlag_Scroll);
           Box->Scroll.Y = App->ListerScroll;
           MinIdx = (s32)(Box->Scroll.Y/ItemHeight);
           Assert(MinIdx >= 0 && "Negative MinIdx will create Item -1");
          }
          
          f32 MaxTagLength = 0.f;
          {
           for EachIndex(Idx, EntryCount)
           {
            MaxTagLength = Max(MaxTagLength, 
                               UI_MeasureTextWidth(Entries[Idx].Tag, &App->TextFont));
           }
           MaxTagLength = CeilF32(MaxTagLength);
          }
          
          UI_FillAll()
           UI_Push()
          {
           MaxIdx = Min(FilteredEntryCount, MinIdx + ShowItemCount + 1);
           
           UI_Spacer(UI_SizePx(ItemHeight*(f32)MinIdx, 1.f));
           
           for(s32 Idx = MinIdx; Idx < MaxIdx; Idx += 1)
           {
            lister_entry *Entry = FilteredEntries + Idx;
            str8 ItemString = Str8Fmt("Item%d", Entry->Idx);
            
            b32 Selected = (SelectedIdx == Entry->Idx);
            
            UI_SemanticHeight(UI_SizePx(ItemHeight, 1.f))
            {
             b32 IsFirstItem = false;
             b32 ItemSelected = false;
             
             IsFirstItem = (Idx == MinIdx);
             
             v4 BackgroundColor = (Selected ? Color_Yellow :
                                   (IsFirstItem ? 
                                    Color_Night1 :
                                    Color_ButtonBackground));
             
#define MUZE_WIP_LISTER 1
#if MUZE_WIP_LISTER
             
             ui_box *ItemBox;
             UI_BackgroundColor(BackgroundColor)
              UI_KeyFromParent()
              UI_BorderColor(IsFirstItem ? Color_Black : Color_ButtonBorder)
              ItemBox = UI_AddBox(ItemString, 
                                  UI_BoxFlag_MouseClickable|
                                  UI_BoxFlag_DrawBorders|
                                  UI_BoxFlag_DrawBackground|
                                  UI_BoxFlag_DrawHotEffects|
                                  UI_BoxFlag_DrawActiveEffects);
             
             
             UI_PaddingAround(UI_SizePx(1.f, 1.f))
              UI_Row()
             {
              ui_size PaddingMinusOne = GlobalItemPadding;
              
              if(Entry->Tag.Size)
              {
               UI_SemanticWidth(UI_SizePx(MaxTagLength + 4.f*5.f, 1.f))
                UI_AddBox(Str8Fmt("%d", Entry->Idx), 0);
               UI_PaddingAround(PaddingMinusOne)
                UI_SemanticWidth(UI_SizePx(MaxTagLength + 2.f*5.f, 1.f))
                UI_Softness(.5f)
                UI_CornerRadii(V4F32(10.f))
                UI_BackgroundColor(Color_Yellow)
                UI_Center()
               {
                UI_AddBox(Entry->Tag, 
                          UI_BoxFlag_DrawBackground |
                          UI_BoxFlag_DrawDisplayString | 
                          UI_BoxFlag_CenterTextVertically |
                          UI_BoxFlag_CenterTextHorizontally);
               }
              }
#endif
              
              UI_Row() UI_Padding(GlobalItemPadding)
               UI_FillWidth()
               UI_AddBox(S8("Text"), 
                         UI_BoxFlag_DrawDisplayString|
                         UI_BoxFlag_CenterTextVertically)->DisplayString = Entry->Text;
              
              ItemSelected = (ItemBox->WasClicked[PlatformMouseButton_Left] || 
                              (App->ListerReturnPressed && IsFirstItem));
              
              if(Entry->Max > 0.f)
              {         
               b32 IsToggle = (Entry->Min == 0.f &&
                               Entry->Max == 1.f && 
                               Entry->Step == 1.f);
               
               PaddingMinusOne.Value -= 1.f;
               
               if(IsToggle)
               {
                b32 Toggled = false;
                
                DebugBrokeIf(0, Entry->Command == Command_PieceTogglePracticing);
                
                UI_SemanticWidth(UI_SizePx(50.f + 2.f*PaddingMinusOne.Value, 1.f))
                 UI_AddBox(Str8Fmt("CheckBoxParent##%d", Entry->Idx), 0);
                
                UI_PaddingAround(PaddingMinusOne)
                 UI_SemanticWidth(UI_SizePx(50.f, 1.f))
                 Toggled = UI_Checkbox((b32)Entry->F32, Color_Yellow);
                if(Toggled)
                {
                 PushCommand(App, Entry->Command);
                }
               }
               else
               {
                command_meta_info *Info = CommandsMetaInfo + Entry->Command;
                
                f32 SliderValue = 0.f;
                ui_box *Slider;
                UI_KeyFromParent()
                 Slider = UI_AddBox(S8("Slider"), 0);
                UI_PaddingAround(PaddingMinusOne)
                 UI_BorderColor(Color_Black)
                 UI_BackgroundColor(Color_Orange)
                 SliderValue = UI_Slider(Entry->F32, Entry->Min, Entry->Max, Entry->Step,
                                         CSS8(Info->SliderFmt), true);
                UI_Spacer(GlobalItemPadding);
                
                if(SliderValue != Entry->F32)
                {
                 PushCommand(App, Entry->Command)->F32 = SliderValue;
                }
               }
               
              }
              
#if MUZE_WIP_LISTER     
             }
#endif
             
             if(ItemSelected)
             {
              PushCommand(App, Command_CloseLister);
              
              if(Entry->ButtonCommand != Command_None)
              {
               Entry->Command = Entry->ButtonCommand;
              }
              
              if(Entry->Command != Command_None)
              {
               command *Command = PushCommand(App, Entry->Command);
               
               Command->S32 = Entry->S32;
               Command->F32 = Entry->F32;
              }
             }
             
            }
           }
          }
         }
         
         App->ListerReturnPressed = false;
         
         FilteredEntryCount = Max(FilteredEntryCount, 1);
         f32 TotalElementSize = (f32)(FilteredEntryCount - 1)*ItemHeight;
         
         App->ListerScroll = M_Scrollbar(Axis2_Y, TotalElementSize, App->ListerScroll);  
         Assert(App->ListerScroll >= 0.f);
        }
       }
      }
     }
    }
    
   }
   
   App->CursorAnimTime += Input->dtForFrame;
  }
  
  //- UI Top "Controls" 
  {
   ui_box *Root = App->TopBox;
   {    
    Root->Key.U64[0] = U64HashFromSeedStr8((u64)Root, S8("PanelsRoot"));
    Root->FixedPos = V2(0.f, 0.f);
    Root->FixedSize = BufferDim;
    Root->Rec = RectFromSize(Root->FixedPos, Root->FixedSize);
    Root->LastTouchedFrameIdx = UI_State->FrameIdx;
   }
   
   UI_BeginLayout(Root, Buffer->Dim, App->HeightPx);
   {            
    UI_LayoutAxis(Axis2_Y)
     UI_AddBox(S8("RootFirst"), UI_BoxFlag_Clip);
    
    UI_Push()
     //- All
    {
     ui_box *PanelsBox;
     UI_FillAll()
      PanelsBox = UI_AddBox(S8("Panels"), UI_BoxFlag_Clip);
     
     v4 Region = PanelsBox->Rec;
     
     UI_FillAll() UI_Push()
     {      
      panel *ResizePanel = NilPanel;
      
      
      panel *Panel = App->FirstPanel;
      for(;!IsNilPanel(Panel);)
      {
       ui_box *PanelBox;
       axis2 LayoutAxis;
       
       // Create UI box for panel
       {                            
        LayoutAxis = (axis2)Panel->Parent->Axis;
        
        UI_BorderThickness(2.f)
         UI_SemanticSizeOnAxis(LayoutAxis, UI_SizeParent(Panel->ParentPct, 0.f))
         UI_TextColor(Color_Snow2)
         UI_BorderColor((Panel == App->SelectedPanel ? Color_Orange : Color_Night0))
         UI_LayoutAxis((axis2)Panel->Axis)
         PanelBox = UI_AddBox(Str8Fmt("%p", Panel), 0);
        
        if(IsLeafPanel(Panel))
        {
         PanelBox->Flags |= UI_BoxFlag_DrawBorders;
         PanelBox->Flags |= UI_BoxFlag_MouseClickable;
         
         UI_PaddingAround(UI_SizePx(2.f, 1.f))
         {
          UI_FillAll()
           UI_LayoutAxis(Axis2_Y)
           UI_AddBox(S8("Panel"), 0);
          
          UI_Push()
          {
           //- Top contents 
           {
            UI_BackgroundColor(Color_ButtonBackground)
             UI_FillWidth() 
             UI_SemanticHeight(UI_SizeText(4.f, 1.f))
             UI_AddBox(S8("Top"), 
                       UI_BoxFlag_DrawBackground|
                       UI_BoxFlag_DrawBorders);
            
            UI_Push()
             UI_BackgroundColor(Color_Frost3)
            {
             // Type button
             {                                                    
              ui_box *Type;
              UI_SemanticWidth(UI_SizeText(5.f, 1.f))
               Type = UI_AddBox(S8("PanelType"), (UI_BoxFlag_DrawBackground|
                                                  UI_BoxFlag_DrawBorders|
                                                  UI_BoxFlag_MouseClickable|
                                                  UI_BoxFlag_DrawHotEffects|
                                                  UI_BoxFlag_DrawActiveEffects|
                                                  UI_BoxFlag_DrawDisplayString|
                                                  UI_BoxFlag_CenterTextVertically|
                                                  UI_BoxFlag_CenterTextHorizontally));
              
              str8 TypeName = PanelTypeStrings[Panel->Kind];
              Type->DisplayString = TypeName;
              
              if(Type->WasClicked[PlatformMouseButton_Left])
              {
               PushCommand(App, Command_OpenLister)->S32 = ListerKind_PanelTypes;
              }
             }
             
             
             UI_FillWidth()
              UI_AddBox(S8(""), UI_BoxFlag_DrawBackground|
                        UI_BoxFlag_DrawBorders);
             
             // Settings button 
             {
              UI_SemanticWidth(UI_SizePx(30.f, 1.f))
               UI_FontKind(FontKind_Icons)
               if(M_IconButton(UI_IconKind_Cog, S8("Settings"), .BackgroundColor = Color_Frost3))
              {
               PushCommand(App, Command_OpenLister)->S32 = ListerKind_Commands;
              }
             }
             
             // Close button 
             {
              ui_box *Close;
              UI_SemanticWidth(UI_SizePx(30.f, 1.f))
               UI_FontKind(FontKind_Icons)
               Close = UI_AddBox(S8("Close"), (UI_BoxFlag_DrawBackground|
                                               UI_BoxFlag_DrawBorders|
                                               UI_BoxFlag_MouseClickable|
                                               UI_BoxFlag_DrawHotEffects|
                                               UI_BoxFlag_DrawActiveEffects|
                                               UI_BoxFlag_DrawDisplayString|
                                               UI_BoxFlag_CenterTextVertically|
                                               UI_BoxFlag_CenterTextHorizontally|
                                               0));
              Close->DisplayString = UI_Icons[UI_IconKind_Close];
              
              if(Close->Clicked[PlatformMouseButton_Left])
              {
               PushCommand(App, Command_ClosePanel);
              }
             }
             
            }
           }
           
           ui_box *Contents;
           UI_BackgroundColor(Color_Night2)
            UI_FillAll()
            Contents = UI_AddBox(S8("Contents"), 
                                 UI_BoxFlag_Scroll|
                                 UI_BoxFlag_MouseClickable|
                                 UI_BoxFlag_DrawBackground);
           
           //- Panel contents 
           UI_BackgroundColor(Color_Night2)
           {     
            ui_size SliderSize = UI_SizePx(250.f, 1.f);
            
            if(0) {}
            else if(Panel->Kind == PanelKind_Piece)
            {
             if(IsNilPiece(Panel->Piece)) Panel->Piece = PieceAlloc(App);
             piece *Piece = Panel->Piece;
             
             str8 PieceFilePath = S8("song.muze_piece");
             
#if 0 && MUZE_INTERNAL     
             if(!Memory->Initialized)
             {             
              M_SerializePiece(FrameArena, true, PieceFilePath, Piece); 
             }
#endif
             
             ui_box *PieceBox;
             // UI for piece panel
             {             
              UI_FillAll()
               UI_Push()
               UI_Column()
               UI_BackgroundColor(Color_ButtonBackground)
              {         
               UI_SemanticHeight(UI_SizePx(ItemHeight,  1.f))
                UI_Row()
                UI_SemanticWidth(UI_SizePx(150.f, 1))
               {               
                UI_FontKind(FontKind_Icons)
                 UI_SemanticWidth(UI_SizePx(30.f, 1.f))
                {
                 if(M_IconButton(UI_IconKind_File, S8("Open")))
                 {
                  M_SerializePiece(FrameArena, true, PieceFilePath, Piece);
                  Piece->IsRecording = false;
                  PieceStopPlaying(App, Piece);
                 }
                 
                 if(M_IconButton(UI_IconKind_Save, S8("Save")))
                 {
                  M_SerializePiece(FrameArena, false, PieceFilePath, Piece);
                 }
                 
                 if(M_IconButton(UI_IconKind_Record, S8("Record"), .TextColor = (Piece->IsRecording ? Color_Red : Color_ButtonText)))
                 {
                  PushCommand(App, Command_PieceToggleRecording);
                 }
                 
                 if(M_IconButton(UI_IconKind_Play, S8("Play"), .TextColor = (Piece->IsPlaying ? Color_Green : Color_ButtonText)))
                 {
                  PushCommand(App, Command_PieceStartPlaying);
                 }
                 
                 if(M_IconButton(UI_IconKind_Pause, S8("Pause")))
                 {
                  PushCommand(App, Command_PieceStopPlaying);
                 }
                 
                 if(M_IconButton(UI_IconKind_Stop, S8("Stop")))
                 {
                  PushCommand(App, Command_PieceStop);
                 }
                 
                 if(M_IconButton(UI_IconKind_Gauge, S8("Metronome"), .TextColor = (Piece->Metronome ? Color_Magenta : Color_Black)))
                 {
                  PushCommand(App, Command_PieceToggleMetronome);
                 }
                 
                 if(M_IconButton(UI_IconKind_Loop, S8("Loop"), .TextColor = (!Piece->LoopingDisabled ? Color_Yellow : Color_Black)))
                 {
                  PushCommand(App, Command_PieceToggleLooping);
                 }
                }
                
                // Synth
                {                
                 UI_SemanticWidth(UI_SizePx(120.f, 1.f))
                 {
                  ui_button_result Click = UI_Button(.Text = S8("Synth"), 
                                                     .Padding = GlobalItemPadding, 
                                                     .HasToggle = true,
                                                     .ToggleToggled = !App->OutputSynthDisabled, 
                                                     .ToggleToggledColor = Color_Yellow);
                  if(Click.Pressed)
                  {
                   PushCommand(App, Command_PieceSelectInstrument);
                  }
                  if(Click.Toggled)
                  {
                   PushCommand(App, Command_ToggleOutputSynth);
                  }
                 }
                }
                
                UI_SemanticWidth(UI_SizePx(200.f, 1))
                 
                {
                 f32 SliderValue = M_Slider(S8("PieceBPM"), S8("BPM"), 
                                            Str8Fmt("%3.0f", Piece->BPM),
                                            Piece->BPM, 0.f, 300.f, 1.f, false,
                                            .Align = false).Value;
                 
                 if(SliderValue != Piece->BPM)
                 {
                  PushCommand(App, Command_PieceSetBPM)->F32 = SliderValue;
                 }
                }
                
                UI_SemanticWidth(UI_SizeText(6.f, 1.f))
                 if(M_Button(S8("Clear")))
                {
                 PushCommand(App, Command_PieceClear);
                }
                
                UI_FillWidth()
                 UI_AddBox(S8("TopSpacer"), 
                           UI_BoxFlag_DrawBackground|
                           UI_BoxFlag_DrawBorders);
                
               }
               
               UI_SemanticHeight(UI_SizePx(ItemHeight,  1.f))
                UI_Row()
                UI_SemanticWidth(UI_SizePx(150.f, 1))
               {
                if(M_SimpleToggleButton(S8("Practice"), Piece->IsPracticing, Color_Yellow))
                {
                 PushCommand(App, Command_PieceTogglePracticing);
                }
                
                UI_SemanticWidth(UI_SizeText(6.f, 1.f))
                 if(M_Button(S8("Range")))
                {
#if 1
                 PushCommand(App, Command_PieceSetPracticeRangeFromSelection);
#else
                 f32 Start = Piece->Notes[Piece->SelectedNoteIdx].Start;
                 if(Start < Piece->PracticeEnd)
                 {
                  Piece->PracticeStart = Start;
                  
                  if(Start > Piece->PracticePos)
                  {
                   Piece->PracticePos = Piece->PracticeStart;
                  }
                  
                  if(Start > Piece->PlayPos || Piece->PlayPos > Piece->PracticeEnd)
                  {
                   Piece->PlayPos = Piece->PracticeStart;
                  }
                 }
#endif
                }
                
                UI_FontKind(FontKind_Icons)
                 UI_SemanticWidth(UI_SizePx(30.f, 1.f))
                 if(M_IconButton(UI_IconKind_Clock, S8("StrictTiming"), .TextColor = (Piece->PracticeStrictTiming ? Color_Yellow : Color_Black)))
                {
                 PushCommand(App, Command_PieceToggleStrictTiming);
                }
                
                UI_SemanticWidth(UI_SizePx(300.f, 1.f))
                 
                {                 
                 m_slider_result Slider = M_Slider(S8("SetNoteLength"), S8(""), 
                                                   Str8Fmt("Length 1/%-2.0f", Piece->SetNoteLength), 
                                                   Piece->SetNoteLength, 
                                                   1.f, 16.f, 1.f, true,
                                                   .Align = false); 
                 Piece->SetNoteLength = Slider.Value;
                 if(Slider.Clicked)
                 {
                  PushCommand(App, Command_PieceSetNoteLength);
                 }
                }
                
                UI_FillWidth()
                 UI_AddBox(S8("TopSpacer"), 
                           UI_BoxFlag_DrawBackground|
                           UI_BoxFlag_DrawBorders);
               }
               
               UI_Row()
               {
                UI_BackgroundColor(Color_Night2)
                 UI_TextColor(Color_Black)
                 UI_FillAll()
                 PieceBox = UI_AddBox(S8("Piece"), 
                                      UI_BoxFlag_Scroll|
                                      UI_BoxFlag_MouseClickable|
                                      UI_BoxFlag_DrawBackground);
                
                f32 VerticalOverflow = 0.f;
                // Find out if vertically overflowing and if it is, add a scrollbar
                {
                 if(!Piece->BarWrapping)
                 {
                  f32 WholeBarWidth = Piece->BeatWidth*Piece->TimeSigNum + 2.f*Piece->BarPadding;
                  s32 BarsPerWidth = (s32)((PieceBox->FixedSize.X - 2.f*Piece->BarPadding)/WholeBarWidth);
                  BarsPerWidth = Max(1, BarsPerWidth);
                  s32 StaffLineCount = 5;
                  f32 StaffLineWidth = Piece->StaffLineWidth;
                  f32 StaffHeight = (f32)(StaffLineCount-1)*Piece->NoteSize + StaffLineWidth;
                  
                  v2 Pos = PiecePosFromTime(Piece, Piece->RecordLength, PieceBox->FixedPos, true,
                                            BarsPerWidth, StaffHeight);
                  f32 PieceHeight = ((Pos.Y - PieceBox->FixedPos.Y) +
                                     (f32)Piece->StaffCount*StaffHeight*2.f + 
                                     StaffHeight);
                  f32 Height = PieceBox->FixedSize.Y;
                  
                  VerticalOverflow = PieceHeight - Height;
                 }
                 
                 if(VerticalOverflow > 0.f)
                 {               
                  f32 Scroll = Panel->Scroll.Y; 
                  Scroll = M_Scrollbar(Axis2_Y, VerticalOverflow, Scroll);
                  PieceBox->Scroll.Y = Scroll;
                  Panel->Scroll.Y = Scroll;
                 }
                }
               }
               
               piece_box_data *Data = PushArray(FrameArena, piece_box_data, 1);
               Data->Piece = Piece;
               Data->Box = PieceBox;
               Data->App = App;
               
               PieceBox->CustomDrawData = Data;
               PieceBox->CustomDraw = CustomDrawPiece;
              }
             }
             
             // Scrollbar
             if(Piece->BarWrapping)
             {
              f32 BarPadding = Piece->BarPadding;
              f32 BarCount = Max(1.f, CeilF32(Piece->RecordLength/Piece->TimeSigNum));
              f32 WholeBarWidth = Piece->BeatWidth*Piece->TimeSigNum + 2.f*BarPadding;
              f32 Width = BarCount*WholeBarWidth + 2.f*BarPadding;
              Width -= WholeBarWidth + 2.f*BarPadding;
              
              f32 Scroll = M_Scrollbar(Axis2_X, Width, Panel->Scroll.X);
              PieceBox->Scroll.X = Scroll;
              Panel->Scroll.X = Scroll;
             }
             
            }
            else if(Panel->Kind == PanelKind_Controls)
            {
             ui_box *ConfigList;
             
             UI_Push()
             {                                                    
              UI_FillAll()
               UI_Row()
              {
               UI_FillAll()
                UI_LayoutAxis(Axis2_Y)
                ConfigList = UI_AddBox(S8("Settings"), UI_BoxFlag_Clip);
               UI_Clip(ConfigList->Rec)
                UI_FillAll()
                UI_Push()
                UI_SemanticHeight(UI_SizePx(ItemHeight, 1.f))
               {
                M_ControlLabel(S8("Devices"));
                
                //- Input
                {
                 // Select MIDI Input device
                 {
                  if(UI_ButtonWithToggle(S8("Toggle keyboard input"), !App->InputVirtualKeyboardDisabled,
                                         ItemPadding, Color_Yellow).OneClicked)
                  {
                   PushCommand(App, Command_ToggleVirtualKeyboardInput);
                  }
                  
                  str8 Name = Str8Fmt("Set MIDI Input device  - \"%S\"", Devices.Devices[App->In.Id].Name);
                  
                  ui_button_result Click = UI_ButtonWithToggle(Name, !App->InputMIDIDisabled,
                                                               ItemPadding, Color_Yellow,
                                                               .String = S8("MIDIInput"));
                  if(Click.Toggled)
                  {
                   PushCommand(App, Command_ToggleMIDIInput);
                  }
                  if(Click.Pressed)
                  {
                   PushCommand(App, Command_OpenLister)->S32 = ListerKind_MIDIInput;
                  }
                  
                 }
                }
                
                //- Output 
                {
                 // Select MIDI output device
                 {
                  str8 Name = Str8Fmt("Set MIDI Output device - \"%S\"", Devices.Devices[App->Out.Id].Name);
                  
                  ui_button_result Click = UI_ButtonWithToggle(Name, App->OutputMIDIEnabled, 
                                                               ItemPadding, Color_Yellow,
                                                               .String = S8("MIDIOutput"));
                  if(Click.Pressed)
                  {
                   PushCommand(App, Command_OpenLister)->S32 = ListerKind_MIDIOutput;
                  }
                  
                  if(Click.Toggled)
                  {
                   PushCommand(App, Command_ToggleMIDIOutput);
                  }
                 }
                 
                }
                
               }
               
               // Scrollbar
               {
                f32 ItemCount = (f32)1;
                f32 TotalElementSize = ItemCount*ItemHeight;
                TotalElementSize -= ItemHeight;
                
                f32 Scroll = M_Scrollbar(Axis2_Y, TotalElementSize, Panel->Scroll.Y);
                ConfigList->Scroll.Y = Scroll;
                Panel->Scroll.Y = Scroll;
               }
               
              }
             }
             
             
            }
            else if(Panel->Kind == PanelKind_Debug)
            {
             ui_box *ConfigList;
             
             UI_Push()
             {                                                    
              UI_FillAll()
               UI_Row()
              {
               UI_FillAll()
                UI_LayoutAxis(Axis2_Y)
                ConfigList = UI_AddBox(S8("Debug"), UI_BoxFlag_Clip|UI_BoxFlag_Scroll);
               UI_FillAll()
                UI_Push()
                UI_SemanticHeight(UI_SizePx(ItemHeight, 1.f))
               {
                UI_State->RectDebugMode ^= UI_ButtonWithToggle(S8("UI Rects"), 
                                                               UI_State->RectDebugMode, 
                                                               ItemPadding, V4(1.f, 0.f, 1.f, 1.f)).OneClicked;
                
                if(M_Button(S8("Search"), .CenterText = false))
                {
                 PushCommand(App, Command_OpenLister)->S32 = ListerKind_DebugInfo;
                }
                
                {                
                 f32 Speed = UI_State->AnimSpeedCoeff;
                 Speed = M_Slider(S8("SetAnimSpeed"), S8("Set animation speed"), 
                                  Str8Fmt("%3.0f", Speed),
                                  Speed, 1.f, 120.f, 1.f, false).Value;
                 UI_State->AnimSpeedCoeff = Speed;
                }
                
                App->HeightPx = M_Slider(S8("SetFontSize"), S8("Set font size"), Str8Fmt("%.0f", App->HeightPx), App->HeightPx,
                                         11.f, 64.f, 1.f, false,
                                         .SliderSize = SliderSize).Value;
                
                UI_BackgroundColor(Color_Night0)
                {
                 local_persist u32 Scale = 0;
                 local_persist b32 IsMajor = false;
                 local_persist random_series Series = {0};
                 
                 {
                  DoOnce()
                  {
                   RandomSeed(&Series, (u64)&Series);
                   Scale = RandomNext(&Series)%Note_Count;
                   IsMajor = (RandomNext(&Series)%1);
                  }
                  str8 Text = Str8Fmt("Random scale is: %S %S\n", NotePitchStrings[Scale], (IsMajor ? S8("Major") : S8("Minor")));
                  if(M_Button(Text, .String = S8("RandomScale")))
                  {
                   Scale = RandomNext(&Series)%Note_Count;
                   IsMajor = (RandomNext(&Series)%1);
                  }
                  
                 }
                 
                 // Piece Debug Info
                 {                 
                  piece *Piece = App->SelectedPanel->Piece;
                  M_Labelf("Pos: %.2f "
                           "PrevPos: (%.2f,%.2f)"
                           "###PlayPos", 
                           Piece->PlayPos, 
                           V2Arg(Piece->PrevPlayPos));
                  M_Labelf("PracticePos: %.2f "
                           "PrevPracticePos: (%.2f,%.2f)"
                           "###PracticePos", 
                           Piece->PracticePos, 
                           V2Arg(Piece->PrevPracticePos));
                  
                  M_Labelf("Practice Start: %.2f "
                           "Practice End: %.2f)"
                           "###PracticePos", 
                           Piece->PracticeStart, 
                           Piece->PracticeEnd);
                  M_Labelf("Selected Panel: %S (%p)"
                           "###SelectedPanel",
                           PanelTypeStrings[App->SelectedPanel->Kind],
                           App->SelectedPanel);
                  
                  M_Labelf("Selection note count: %llu###SelNoteCount",
                           Piece->SelectionCount);
                  
                  
                  for EachIndex(Idx, App->DebugStringCount)
                  {
                   M_Labelf("%S###DebugString-%llu", App->DebugStrings[Idx], Idx);
                  }
                  
                 }
                 
                 
                 
                }
                
                
               }
               
               // Scrollbar
               {                                                            
                f32 TotalElementSize = ItemHeight*(f32)(8 + App->DebugStringCount);
                TotalElementSize -= ItemHeight;
                
                f32 Scroll = M_Scrollbar(Axis2_Y, TotalElementSize, Panel->Scroll.Y);
                ConfigList->Scroll.Y = Scroll;
                Panel->Scroll.Y = Scroll;
               }
              }
             }
             
            }
            else if(Panel->Kind == PanelKind_Piano)
            {
             piece_box_data *Data = PushArray(FrameArena, piece_box_data, 1);
             Data->Box = Contents;
             
             Contents->CustomDrawData = Data;
             Contents->CustomDraw = CustomDrawPiano;
            }
            
           }
          }
         }
         
         // Set selected panel
         {         
          b32 ChildIsActive = false;
          
          for(ui_box *Child = PanelBox->First;
              (!UI_IsNilBox(Child) && Child != PanelBox);
              Child = UI_BoxDepthFirstPreOrder(Child).Next)
          {
           platform_mouse_button_type ButtonType = PlatformMouseButton_Left;
           if(UI_IsActive(Child, ButtonType) && !UI_KeyMatch(UI_KeyNull(), Child->Key))
           {
            ChildIsActive = true;
            break;
           }
          }
          
          if(PanelBox->WasClicked[PlatformMouseButton_Left] || ChildIsActive)
          {
           App->SelectedPanel = Panel;
          }
         }
         
        }
        else
        {
         PanelBox->Flags |= UI_BoxFlag_Clip;
        }
        
        Panel->Region = PanelBox->Rec;
       }
       
       panel *NextPanel = NilPanel;
       
       //- Find the next panel in depth first pre order, if this panel is a leaf panel add a resize UI box 
       {                            
        if(!IsNilPanel(Panel->First))
        {
         NextPanel = Panel->First;
         
         UI_PushBox();
        }
        else for(panel *P = Panel; !IsNilPanel(P); P = P->Parent)
        {
         if(!IsNilPanel(P->Next))
         {
          LayoutAxis = P->Parent->Axis;
          
          ui_box *GapBox;
          
          // Create UI box for gap
          {                                    
           f32 GapSize = 12.f;
           
           s32 GapFlags = (UI_BoxFlag_MouseClickable|
                           UI_BoxFlag_DrawHotEffects|
                           UI_BoxFlag_DrawActiveEffects|
                           UI_BoxFlag_DrawBackground);
           if(LayoutAxis == Axis2_X) GapFlags |= UI_BoxFlag_MirrorEffects;
           
           UI_BackgroundColor(Color_Night2)
            UI_SemanticSizeOnAxis(LayoutAxis, UI_SizePx(GapSize, 1.f))
            UI_SemanticSizeOnAxis(1 - LayoutAxis, UI_SizeParent(1.f, 0.f))
            GapBox = UI_AddBox(S8("Gap"), GapFlags);
          }
          
          platform_mouse_button_type ButtonType = PlatformMouseButton_Left;
          
          // Set cursor shape to resizing one
          {                                    
           if(UI_IsHot(GapBox, ButtonType))
           {
            Input->PlatformCursor = (PlatformCursorShape_ResizeHorizontal +
                                     !!(LayoutAxis == Axis2_Y));
           }
          }
          
          // Gap box (resize border)
          {                                    
           if(UI_IsActive(GapBox, ButtonType))
           {
            // NOTE(luca): Since this will change the position of the elements, they will 
            // want to animate from their FixedPos to their AnimatedPos.  To reset their 
            // AnimatedPos we disable animations while dragging.
            UI_State->IsAnimating = false;
            
            s32 PosOnAxis = Input->Mouse.Pos.e[LayoutAxis];
            
            if(GapBox->WasClicked[PlatformMouseButton_Left])
            {
             DragData->StartPos = PosOnAxis;
             DragData->StartPct = P->ParentPct;
             DragData->NextStartPct = P->Next->ParentPct;
            }
            
            Assert(IsNilPanel(ResizePanel));
            ResizePanel = P; 
           }
           
          }
          
          NextPanel = P->Next;
          break;
         }
         
         UI_PopBox();
        }
       }
       
       Panel = NextPanel;
      }
      
      if(!IsNilPanel(ResizePanel))
      {                        
       panel *P = ResizePanel;
       
       axis2 LayoutAxis = P->Parent->Axis;
       s32 PosOnAxis = Input->Mouse.Pos.e[LayoutAxis];
       f32 Size = SizeOnAxis(P->Parent->Region, LayoutAxis);
       
       s32 dPos = PosOnAxis - DragData->StartPos;
       f32 dPct = (f32)dPos/Size;
       
       if(DragData->NextStartPct - dPct > .05f &&
          DragData->StartPct     + dPct > .05f)
       {                                                
        P->ParentPct = DragData->StartPct + dPct;
        P->Next->ParentPct = DragData->NextStartPct - dPct; 
       }
      }
      
      
     }
     
     NoOp();
    }
   }
   UI_EndLayout(Root);
   
   // Prune unused boxes
   for EachIndex(Idx, UI_State->BoxTableSize)
   {
    ui_box *First = UI_State->BoxTable + Idx;
    
    for UI_EachHashBox(Node, First)
    {
     if(App->FrameIdx > Node->LastTouchedFrameIdx)
     {
      if(!UI_KeyMatch(Node->Key, UI_KeyNull()))
      {
       Node->Key = UI_KeyNull();
      }
     }
    }
   }
   
   OS_ProfileAndPrint("UI");
  }
  
  if(App->ListerOpened)
  {
   UI_EndLayout(App->ListerBox);
  }
  
 }
 
 for EachIndex(Idx, App->CommandCount)
 {
  command *Command = App->Commands + Idx;
  // NOTE(luca): Default to selected objects. 
  if(IsNilPanel(Command->Panel)) Command->Panel =  App->SelectedPanel;
  if(IsNilPiece(Command->Piece)) Command->Piece =  App->SelectedPanel->Piece;
  
  piece *Piece = Command->Piece;
  if(IsNilPiece(Piece)) Piece = NilP(piece);
  
  panel *Panel = Command->Panel;
  
  switch(Command->Kind)
  {
   default: InvalidPath(); break;
   
   //- App commands 
   {
    case Command_SetFontSize:
    {
     App->HeightPx = Command->F32;
    } break;
   } 
   
   //- Device commands 
   {   
    case Command_SelectInputDevice:
    {
     App->In = Devices.Devices[Command->S32];;
     PlatformMIDIListen(App->In);
    } break;
    
    case Command_SelectOutputDevice:
    {
     App->Out = Devices.Devices[Command->S32];
    } break;
    
    case Command_SelectInstrument:
    {
     panel_kind Kind = App->SelectedPanel->Kind;
     if(0) {}
     else if(Kind == PanelKind_Piece)
     {
      if(Piece->Preset != Command->S32)
      {
       tsf_channel_note_off_all(GlobalTSF, Piece->Channel);
      }
      
      Piece->Preset = Command->S32;
      tsf_channel_set_presetindex(GlobalTSF, Piece->Channel, Piece->Preset);
     }
     
    } break;
    
    case Command_ToggleMIDIInput:
    {
     App->InputMIDIDisabled ^= 1;
    } break;
    case Command_ToggleMIDIOutput:
    {
     if(Piece->IsPlaying)
     {
      MIDIOutputMutePiecePlayingNotes(App, Piece);
     }
     App->OutputMIDIEnabled ^= 1;
    } break;
    case Command_ToggleVirtualKeyboardInput:
    {
     App->InputVirtualKeyboardDisabled ^= 1;
    } break;
    case Command_ToggleOutputSynth:
    {
     if(!App->OutputSynthDisabled)
     {
      tsf_note_off_all(GlobalTSF);
     }
     App->OutputSynthDisabled ^= 1;
    } break;
   }
   
   //- Lister commands 
   {   
    case Command_CloseLister:
    {
     App->ListerOpened = false;
    } break;
    case Command_OpenLister:
    {
     App->ListerOpened = true;
     App->ListerKind = (lister_kind)Command->S32;
     App->ListerScroll = 0.f;
     App->Text.Size = 0;
    } break;
    
    case Command_SelectCommand:
    {
     PushCommand(App, Command_OpenLister)->S32 = ListerKind_Commands;
    } break;
   }
   
   //- Panel commands 
   {
    case Command_ClosePanel:
    {
     App->SelectedPanel = ClosePanel(App, Command->Panel);
    } break;
    
    case Command_SplitSelectedPanelY:
    {
     Command->Axis = Axis2_Y;
    } FallThrough();
    case Command_SplitSelectedPanelX:
    {
     panel *To = App->SelectedPanel;
     
     App->SelectedPanel = SplitPanel(App, To, Command->Axis, false);
     App->SelectedPanel->Kind = To->Kind;
    } break;
    
    case Command_SelectNextPanel:
    {
     App->SelectedPanel = PanelNextLeaf(App->SelectedPanel, false);
    } break;
    
    case Command_SelectPanelType:
    {
     PushCommand(App, Command_OpenLister)->S32 = ListerKind_PanelTypes; 
    } break;
    
    case Command_SetPanelType:
    {
     Command->Panel->Kind = (panel_kind)Command->S32;
     MemoryZero(&Command->Panel->Scroll);
    } break;
   }
   
   //- Piece commands
   {   
    case Command_PieceToggleRecording:
    {
     if(Piece->IsRecording)
     {
      Piece->IsRecording = false;
      
      // NOTE(luca): Now that recording has stopped MIDI events with NoteOff will be ignored
      // So we manually set their length.
      
      for(u64 NoteIdx = 1; NoteIdx < Piece->NoteCount; NoteIdx += 1)
      {
       note *Note = Piece->Notes + NoteIdx;
       if(Note->Length == 0.f) Note->Length = (Piece->RecordLength - Note->Start);
      }
      
     }
     else
     {
      tsf_channel_note_off_all(GlobalTSF, Piece->Channel);
      PieceStopPlaying(App, Piece);
      Piece->IsPracticing = false;
      Piece->IsRecording = true;
     }
    } break;
    
    case Command_PieceTogglePlaying:
    {
     if(!Piece->IsPlaying)
     {
      PushCommand(App, Command_PieceStartPlaying);
     }
     else
     {
      PushCommand(App, Command_PieceStopPlaying);
     }
    } break;
    
    case Command_PieceStartPlaying:
    {
     if(!Piece->IsPlaying)
     {
      tsf_channel_note_off_all(GlobalTSF, Piece->Channel);
      Piece->IsPlaying = true;
      
      Piece->IsRecording = false;
      Piece->MetronomeTime = Piece->PlayPos;
     }
    } break;
    
    case Command_PieceStopPlaying:
    {
     PieceStopPlaying(App, Piece);
    } break;
    
    case Command_PieceStop:
    {
     PieceStopPlaying(App, Piece);
     if(Piece->IsPracticing)
     {
      Piece->PlayPos = Piece->PracticeStart;
     }
     else
     {
      Piece->PlayPos = 0.f;
     }
     
     Piece->PracticeNoteCount = 1;
     Piece->PracticePos = Piece->PracticeStart;
    } break;
    
    case Command_PieceClear:
    {
     PieceStopPlaying(App, Piece);
     Piece->IsRecording = false;
     Piece->NoteCount = 1;
     Piece->PracticeNoteCount = 0;
     Piece->RecordLength = 0.f;
     Piece->PlayPos = 0.f;
     Piece->PracticePos = 0.f;
     Piece->PracticeEnd = 0.f;
     Piece->PracticeStart = 0.f;
     Piece->SelectionFirst = 0;
     Piece->SelectionLast = 0;
    } break;
    
    case Command_PieceToggleMetronome:
    {
     if(Piece->Metronome)
     {
      Piece->Metronome = false;
     }
     else
     {
      Piece->MetronomeTime = (Piece->IsPlaying ? Piece->PlayPos : 0.f);
      Piece->Metronome = true;
     }
    } break;
    
    case Command_PieceToggleStrictTiming:
    {
     Command->Piece->PracticeStrictTiming ^= 1;
    } break;
    
    case Command_PieceTogglePracticing:
    {
     Piece->PracticeNoteCount = 1;
     if(Piece->IsPracticing)
     {
      Piece->IsPracticing = false;
     }
     else
     {
      Piece->IsPracticing = true;
      
      tsf_channel_note_off_all(GlobalTSF, Piece->Channel);
      
      // Conflicts
      PieceStopPlaying(App, Piece);
      Piece->IsRecording = false;
      
      // If Start = End, which should only be possible when unitiliazed set End to next bar.
      if(Piece->PracticeEnd == Piece->PracticeStart)
      {
       Piece->PracticeEnd = Min(Piece->RecordLength, 
                                Piece->PracticeStart + Piece->TimeSigNum);
      }
      
      Piece->PracticePos = Piece->PracticeStart;
     }
    } break;
    
    case Command_PieceToggleLooping:
    {
     Piece->LoopingDisabled ^= 1;
    } break;
    
    case Command_PieceToggleBarWrapping:
    {
     Piece->BarWrapping ^= 1;
    } break;
    
    case Command_PieceSetBPM:
    {
     Piece->BPM = Command->F32;
    } break;
    
    case Command_PieceDeselectAllNotes:
    {
     for EachNodeSafe(Node, note_node, Piece->SelectionFirst)
     {
      PieceDeselectNote(Piece, Node);
     }
    } break;
    
    case Command_PieceDeselectNote:
    {
     u64 NoteIdx = Command->U64;
     note *Note = Piece->Notes + NoteIdx;
     
     note_node *Selected = 0;
     // Selected
     {
      for EachNode(Node, note_node, Piece->SelectionFirst)
      {
       if(Node->V == Note)
       {
        Selected = Node;
        break;
       }
      }
     }
     
     if(Selected) 
     {
      PieceDeselectNote(Piece, Selected);
     }
     
    } break;
    
    case Command_PieceSelectNote:
    {
     u64 NoteIdx = Command->U64;
     note *Note = Piece->Notes + NoteIdx;
     
     note_node *Selected = 0;
     // Selected
     {
      for EachNode(Node, note_node, Piece->SelectionFirst)
      {
       if(Node->V == Note)
       {
        Selected = Node;
        break;
       }
      }
     }
     
     if(!Selected)
     {
      note_node *Node = 0;
      if(Piece->SelectionFree)
      {
       // Take from free list
       Node = Piece->SelectionFree;
       Piece->SelectionFree = Piece->SelectionFree->Next;
      }
      else
      {
       Node = Piece->Selection + Piece->SelectionNextIdx;
       Piece->SelectionNextIdx += 1;
      }
      MemoryZero(Node);
      
      Node->V = Note;
      DLLAppend(Node, Piece->SelectionFirst, Piece->SelectionLast);
      Piece->SelectionCount += 1;
     }
     
    } break;
    
    case Command_PieceDeleteNote:
    {
     
     // Sort notes by idx in array.
     
     for EachNodeBack(Node, note_node, Piece->SelectionLast)
     {
      for(note_node *Compare = Piece->SelectionFirst; 
          Compare != Node; 
          Compare = Compare->Next)
      {
       if(Compare->V > Compare->Next->V)
       {
        Swap(Compare->V, Compare->Next->V);
       }
      }
     }
     
     // NOTE(luca): Iterate backwards so that MemoryCopy works
     for EachNodeBackSafe(Node, note_node, Piece->SelectionLast)
     {
      note *Note = Node->V;
      
      if(Note->Kind == NoteKind_Note)
      {
       Note->Kind = NoteKind_Rest;
      }
      else if(Note->Kind == NoteKind_Rest)
      {     
#if 0
       if(Piece->NoteCount)
       {
        // PieceDeselectNote(Piece, Node);
        
        u64 NoteIdx = (Note - Piece->Notes); 
        MemoryCopy(Note, 
                   Note + 1,
                   (Piece->NoteCount - (NoteIdx + 1))*sizeof(note));
        Piece->NoteCount -= 1;
       }
#endif
       NotImplemented();
      }
      
     }
     
    } break;
    
    case Command_PieceSetNoteLength:
    {
     for EachNode(Node, note_node, Piece->SelectionFirst)
     {
      Node->V->Length = (Piece->TimeSigDen/Piece->SetNoteLength);
     }
    } break;
    
    case Command_PieceSelectInstrument:
    {
     PushCommand(App, Command_OpenLister)->S32 = ListerKind_Instruments;
    } break;
    
    case Command_PieceAddStaff:
    {
     if(Piece->StaffCount < 2)
     {
      staff *PreviousStaff = Piece->Staffs + (Piece->StaffCount - 1);
      staff *Staff = Piece->Staffs + Piece->StaffCount;
      Piece->StaffCount += 1;
      
      *Staff = *PreviousStaff;
      
      Staff->Octave = 4;
      Staff->ClefKind = ClefKind_Bass;
     }
    } break;
    
    case Command_PieceTrimAtFirstNote:
    {
     
    } break;
    
    case Command_PieceSetPracticeRangeFromSelection:
    {
     f32 MinStart = Piece->RecordLength;
     f32 MaxEnd = 0.f;
     
     for EachNode(Node, note_node, Piece->SelectionFirst)
     {
      note *Note = Node->V;
      
      f32 Start = Note->Start;
      f32 End = Note->Start + Note->Length;
      MinStart = Min(MinStart, Start);
      MaxEnd = Max(MaxEnd, End);
     }
     
     Piece->PracticeStart = MinStart;
     Piece->PracticeEnd = MaxEnd;
     PieceStopPlaying(App, Piece);
     Piece->PlayPos = Piece->PracticeStart;
     
    } break;
   }
  }
 }
 
 //- Virtual MIDI keyboard 
 {
  if(!App->InputVirtualKeyboardDisabled && !App->ListerOpened)
  {
   for EachElement(Idx, Input->MIDI.Buttons)
   {
    app_button_state Key = Input->MIDI.Buttons[Idx];
    b32 Pressed = (Key.HalfTransitionCount != 0 && Key.EndedDown);
    b32 Released = (Key.HalfTransitionCount != 0 && !Key.EndedDown);
    u8 Pitch = (u8)Idx + MiddleC;
    
    if(Pressed)
    {
     if(Input->MIDI.EventCount < ArrayCount(Input->MIDI.Events))
     {
      app_midi_event *Event = Input->MIDI.Events + Input->MIDI.EventCount;
      Input->MIDI.EventCount += 1;
      
      midi_message Message = {0};
      Message.Type = MIDIEventType_NoteOn;
      Message.Pitch = Pitch; 
      Message.Velocity = 64;
      
      Event->Message = Message.U32[0];
     }
    }
    
    if(Released)
    {
     if(Input->MIDI.EventCount < ArrayCount(Input->MIDI.Events))
     {
      app_midi_event *Event = Input->MIDI.Events + Input->MIDI.EventCount;
      Input->MIDI.EventCount += 1;
      
      midi_message Message = {0};
      Message.Type = MIDIEventType_NoteOff;
      Message.Pitch = Pitch;
      
      Event->Message = Message.U32[0];
     }
    }
    
   }
  }
 }
 
 //- Update pieces 
 {
  for(panel *Panel = App->FirstPanel;
      !IsNilPanel(Panel);
      Panel = PanelRecDepthFirstPreOrder(Panel).Next)
  {
   piece *Piece = Panel->Piece;
   
   f32 BPS = Piece->BPM/60.f;
   
   if(Piece->Metronome)
   {
    f32 Duration = 0.2f; 
    f32 Start = FloorF32(Piece->MetronomeTime);
    f32 End = Start + Duration*BPS;
    b32 HasPlayed = PiecePosPassedTimestamp(Piece, Piece->MetronomeTime, Start);
    b32 HasStopped = PiecePosPassedTimestamp(Piece, Piece->MetronomeTime, End);
    
    s32 Pitch = Note_A + 6*12;
    
    if(HasPlayed)
    {
     tsf_channel_note_on(GlobalTSF, App->MetronomeChannel, Pitch, 1.f);
    }
    
    if(HasStopped)
    {
     tsf_channel_note_off(GlobalTSF, App->MetronomeChannel, Pitch);
    }
    
    Piece->MetronomeTime += BPS*Input->dtForFrame;
   }
   
   if(Piece->IsPlaying)
   {
    // Play notes
    {
     for(u64 Idx = 1; Idx < Piece->NoteCount; Idx += 1)
     {
      note *Note = Piece->Notes + Idx;
      
      if(Note->Kind == NoteKind_Note)
      {
       // Piece notes
       {                 
        f32 Start = Note->Start;
        f32 End = Start + Note->Length;
        
        b32 NoteWasPlayed = PieceWasTimestampPlayed(Piece, Start);
        b32 NoteHasEnded = PieceWasTimestampPlayed(Piece, End);
        
        if(!App->OutputSynthDisabled)
        {        
         if(NoteWasPlayed)
         {
          TSFPlayNoteU8(App, Piece->Channel, Note->Pitch, Note->Velocity); 
         }
         
         if(NoteHasEnded)
         {
          tsf_channel_note_off(GlobalTSF, Piece->Channel, Note->Pitch);
         }
        }
        
        if(App->OutputMIDIEnabled)
        {
         if(NoteWasPlayed)
         {
          midi_message Message = {0};
          Message.Type = MIDIEventType_NoteOn;
          Message.Pitch = (u8)Note->Pitch;
          Message.Velocity = Note->Velocity;
          
          PlatformMIDISend(App->Out, Message.U32[0]);
         }
         
         if(NoteHasEnded)
         {
          midi_message Message = {0};
          Message.Type = MIDIEventType_NoteOff;
          Message.Pitch = Note->Pitch;
          
          PlatformMIDISend(App->Out, Message.U32[0]);
         }
        }
        
       }
      }
     }
    }
    
    // Increment playpos
    {               
     Piece->PlayPos += BPS*Input->dtForFrame;
     
     f32 PieceEnd = (Piece->IsPracticing ? Piece->PracticeEnd : Piece->RecordLength);
     b32 PieceEnded = (Piece->PlayPos > PieceEnd);
     if(PieceEnded)
     {
      tsf_channel_note_off_all(GlobalTSF, Piece->Channel);
      Piece->PlayPos = 0.f;
      if(Piece->IsPracticing) 
      {
       Piece->PlayPos = Piece->PracticeStart;
      }
      
      Piece->MetronomeTime = 0.f;
      
      PieceStopPlaying(App, Piece);
      if(!Piece->LoopingDisabled) 
      {
       Piece->IsPlaying = true;
      }
      
     }
    }
   }
   
   if(Piece->IsRecording)
   {
    Assert(!Piece->IsPracticing);
    Piece->IsPracticing = false; // Bug if needed
    
    for EachIndex(Idx, Input->MIDI.EventCount)
    {
     app_midi_event *Event = Input->MIDI.Events + Idx;
     midi_message Message = {Event->Message};
     u8 Status = Message.U8[0];
     u8 Type = Status & 0xF0;
     u8 Channel = Status & 0x0F;
     
     if(0) {}
     else if(Type == MIDIEventType_NoteOn && Message.Velocity > 0)
     {
      if(Piece->NoteCount < ArrayCount(Piece->Notes))
      { 
       note *Note = Piece->Notes + Piece->NoteCount;
       MemoryZero(Note);
       
       u8 Pitch = Message.Pitch;
       u8 Velocity = Message.Velocity;
       
       Note->Kind = NoteKind_Note;
       Note->Pitch = Pitch;
       Note->Start = Piece->RecordLength;
       Note->Velocity = Message.Velocity;
       
       s32 MinPitch = Piece->Staffs[0].Octave*Note_Count;
       if(Note->Pitch < MinPitch)
       {
        Note->StaffIdx = 1;
       }
       
       TSFPlayNoteU8(App, Piece->Channel, Note->Pitch, Velocity);
       
       Piece->NoteCount += 1;
      }
     }
     else if(Type == MIDIEventType_NoteOff ||
             (Type == MIDIEventType_NoteOn && Message.Velocity == 0))
     {
      // Find first note with same pitch and set its length.
      for(u64 NoteIdx = 1; NoteIdx < Piece->NoteCount; NoteIdx += 1)
      {
       note *Note = Piece->Notes + NoteIdx;
       u8 Pitch = Message.Pitch;
       
       tsf_channel_note_off(GlobalTSF, Piece->Channel, Pitch);
       
       // NOTE(luca): Find matching note, and set its length we now know.
       if(Note->Length == 0.f &&
          Note->Pitch == Pitch)
       {
        Note->Length = Piece->RecordLength - Note->Start;
       }
      }
     }
    }
    
    Piece->RecordLength += BPS*Input->dtForFrame;
   }
   
   if(Piece->IsPracticing)
   {    
    u64 NotesToPlayCount = 0;
    note **NotesToPlay = PushArray(FrameArena, note *, Piece->NoteCount);
    note *FirstNoteToPlay = NilNote;
    
    // Find all notes that have to be played at PracticePos
    {
     for(u64 Idx = 1; Idx < Piece->NoteCount; Idx += 1)
     {
      note *Note = Piece->Notes + Idx;
      
      if(Note->Kind == NoteKind_Note &&
         Piece->PracticePos <= Note->Start)
      {
       FirstNoteToPlay = Note;
       break;
      }
     }
     
     if(!IsNilNote(FirstNoteToPlay))
     {
      for(u64 Idx = 1; Idx < Piece->NoteCount; Idx += 1)
      {
       note *Note = Piece->Notes + Idx;
       if(FirstNoteToPlay->Start == Note->Start)
       {
        NotesToPlay[NotesToPlayCount] = Note;
        NotesToPlayCount += 1;
       }
      }
     }
    }
    
    for EachIndex(Idx, Input->MIDI.EventCount)
    {
     app_midi_event *Event = Input->MIDI.Events + Idx;
     midi_message Message = {Event->Message};
     u8 Status = Message.U8[0];
     u8 Data1  = Message.U8[1];
     u8 Data2 = Message.U8[2];
     u8 Type = Status & 0xF0;
     u8 Channel = Status & 0x0F;
     
#if 0
     if(Type == 128)
     {
      DebugBreak();
     }
#endif
     
     if(0) {}
     else if(Type == MIDIEventType_NoteOn && Data2 > 0)
     {
      if(Piece->PracticeNoteCount < ArrayCount(Piece->PracticeNotes))
      { 
       note *Note = Piece->PracticeNotes + Piece->PracticeNoteCount;
       u8 Pitch = Data1;
       u8 Velocity = Data2;
       
       TSFPlayNoteU8(App, Piece->Channel, Pitch,  Velocity);
       
       Note->Kind = NoteKind_Note;
       Note->Pitch = Pitch;
       Note->Length = 0.f;
#if 0
       Note->Start = Piece->PracticePos;
#else
       // NOTE(luca): This should make it so that even if note is played a bit early it is
       // still ok if held
       Note->Start = FirstNoteToPlay->Start;
#endif
       Note->Hidden = false;
       
       Piece->PracticeNoteCount += 1;
      }
     }
     else if(Type == MIDIEventType_NoteOff ||
             (Type == MIDIEventType_NoteOn && Data2 == 0))
     {
      u8 Pitch = Data1;
      
      tsf_channel_note_off(GlobalTSF, Piece->Channel, Pitch);
      
      // Find first note with same pitch and set its length.
      for(u64 NoteIdx = 1; NoteIdx < Piece->PracticeNoteCount; NoteIdx += 1)
      {
       note *Note = Piece->PracticeNotes + NoteIdx;
       
       if(Note->Pitch == Pitch)
       {
        Note->Hidden = true;
        Note->Length = (Piece->PracticePos - Note->Start);
       }
      }
     }
    }
    
    u64 CorrectNotesCount = 0;
    
    // Update practice notes
    {    
     for(u64 NoteToPlayIdx = 0; 
         NoteToPlayIdx < NotesToPlayCount; 
         NoteToPlayIdx += 1) 
     {
      note *NoteToPlay = NotesToPlay[NoteToPlayIdx];
      
      if(NoteToPlay->Kind == NoteKind_Note)
      {   
       for(u64 Idx = 1; Idx < Piece->PracticeNoteCount; Idx += 1)
       {
        note *Note = Piece->PracticeNotes + Idx;
        
        if(!Note->Hidden && 
           Note->Start >= Piece->PracticePos && 
           Note->Pitch == NoteToPlay->Pitch)
        {
         CorrectNotesCount += 1;
        }
        
        if(!Note->Hidden)
        {
         Note->Length += BPS*Input->dtForFrame;
        }
       }
      }
     }
    }
    
    if(CorrectNotesCount == NotesToPlayCount)
    {
     // NOTE(luca): Instead of adding up Length as we go which would cumulate precision error
     // we start from the note start which makes reduces error.
     
     if(!Piece->PracticeStrictTiming)
     {
      Piece->PracticePos = FirstNoteToPlay->Start + FirstNoteToPlay->Length;
     }
     else
     {
      Piece->PracticePos += BPS*Input->dtForFrame;
     }
     
    }
    else if(FirstNoteToPlay->Start > Piece->PracticePos)
    {
     if(Piece->PracticeStrictTiming)
     {
      Piece->PracticePos += BPS*Input->dtForFrame;
      Piece->PracticePos = Min(Piece->PracticePos, FirstNoteToPlay->Start); 
     }
     else
     {
      Piece->PracticePos = FirstNoteToPlay->Start;
     }
    }
    
    // Loop 
    if(Piece->PracticePos >= Piece->PracticeEnd) 
    {
     Piece->PracticePos = Piece->PracticeStart;
     Piece->PracticeNoteCount = 1;
    }
   }
   
   if(!IsNilPiece(Piece))
   {
    Piece->Notes[0] = Nil(note);
   }
  }
 }
 
 // Save config
 {
  MS_SerializeConfig(FrameArena, false, ConfigPath, App);
 }
 
 //- Rendering 
 
 // Render rectangles
 if(!Input->SkipRendering)
 {
  RenderClear();
  RenderDrawAllRectangles(&App->Render, BufferDim, &App->FontAtlas);
 }
 
 OS_ProfileAndPrint("R rects");
 
 // Frame end cleanup
 {
  App->CommandCount = 0;
 }
 
 App->FrameIdx += 1;
 
 // NOTE(luca): This is so that we can split our initialization code if we want.
 Memory->Initialized = true;
 Memory->Reloaded = false;
 
#if MUZE_STARTUP_PROFILE
 ShouldQuit = true;
#endif
 
 return ShouldQuit;
}

GET_AUDIO_SAMPLES(GetAudioSamples)
{
#if 0
#elif SAMPLE_FORMAT == f32
 tsf_render_float(GlobalTSF, Sound->Samples, (int)SampleCount, 0);
#elif SAMPLE_FORMAT == s16
 tsf_render_short(GlobalTSF, Sound->Samples, (int)SampleCount, 0);
#endif
}
