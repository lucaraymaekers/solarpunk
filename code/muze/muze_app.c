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
#include "rl/rl_platform.h"
#include "rl/rl_libs.h"
#include "rl/rl_font.h"
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
#define IsNilPieceNote(Note) CheckNil(Note, NilPieceNote)

#define Disabled() Log("Temporarily disabled.\n");

//~ Globals
global_variable tsf *GlobalTSF;
global_variable drag_data *DragData;
global_variable ui_size GlobalItemPadding;

global_variable tsf *NilTSF;
global_variable panel *NilPanel;
global_variable voice *NilVoice;
global_variable rec_note *NilNote;
global_variable piece_note *NilPieceNote;
global_variable piece *NilPiece;

global_variable f32 *dtForFrame;

//~ Helpers
internal f32
GetWallTime(void)
{
 f32 Result = (f32)OS_GetWallClock();
 return Result;
}

internal void
InitReadOnlyGlobals(arena *Arena)
{
 ArenaSetPos(Arena, 0);
 UI_NilBox = PushArray(Arena, ui_box, 1);
 NilPanel = PushArray(Arena, panel, 1);
 NilVoice  = PushArray(Arena, voice, 1);
 NilNote = PushArray(Arena, rec_note, 1);
 NilPieceNote = PushArray(Arena, piece_note, 1);
 NilPiece = PushArray(Arena, piece, 1);
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
 Panel->Voice = NilVoice;
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
 else if(Collapse != Panel)
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
  
  // Collapse could be a non-leaf panel, use the next leaf from there.
  {
   if(!IsLeafPanel(Collapse)) Collapse = PanelNextLeaf(Collapse, false);
   Assert(!IsNilPanel(Collapse));
   Assert(IsLeafPanel(Collapse));
  }
  
  // If this is the only child of the parent, collapse the child into the parent.
  {
   if(Collapse == Parent->First && 
      Collapse == Parent->Last)
   {
    Assert(EqualsWithEpsilon(Collapse->ParentPct, 1.f, 0.001f));
    
    Parent->First = NilPanel;
    Parent->Last = NilPanel;
    
    Parent->Kind = Collapse->Kind;
    Parent->Voice = Collapse->Voice;
    Parent->Scroll = Collapse->Scroll;
    
    Collapse = Parent;
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

internal void
PanelSelect(app_state *App, panel *Panel)
{
 App->SelectedPanel	= Panel;
 if(Panel->Kind == PanelKind_Sheet ||
    Panel->Kind == PanelKind_Roll)
 {
  App->SelectedVoice = Panel->Voice;
 }
}

//~ Muze - Voices 
internal void
VoiceReset(voice *Voice)
{
 Voice->NoteCount = 0;
 Voice->MaxPitch = 0;
 Voice->MinPitch = (u8)-1;
 
 Voice->RecordStart = 0.f;
 Voice->RecordLength = 0.f;
 Voice->PlayPos = 0.f;
 
 Voice->FirstNote = NilNote;
 Voice->LastNote = NilNote;
 Voice->NoteSel = 0;
 
 Voice->NoteSetLength = 0.f;
 
 ArenaSetPos(Voice->Arena, 0);
}

internal voice *
VoiceAdd(app_state *App)
{
 voice *Voice = App->Voices + App->VoiceCount;
 
 if(App->VoiceCount < App->MaxVoiceCount)
 {        
  MemoryZero(Voice);
  Voice->Arena = PushArena(App->Arena, KB(64), false);
  
  VoiceReset(Voice);
  Voice->Channel = App->NextChannelIdx;
  App->NextChannelIdx += 1;
  tsf_channel_set_presetindex(GlobalTSF, Voice->Channel, Voice->PresetIdx);
  
  Voice->Volume = 1.f;
  Voice->MIDISoftness = 1.f;
  
  App->VoiceCount += 1;
 }
 else
 {
  Assert(App->VoiceCount > 0);
  Voice = App->Voices + App->VoiceCount - 1;
  // TODO(luca): Show error message that max amount of voices has been reached.
 }
 
 return Voice;
}

internal rec_note *
NoteAdd(voice *Voice)
{
 rec_note *Note = NilNote;
 
 // Push on to the back
 {                
  Note = PushArrayZero(Voice->Arena, rec_note, 1);
  
  if(IsNilNote(Voice->FirstNote))
  { 
   Voice->FirstNote = Note;
  }
  
  if(!IsNilNote(Voice->LastNote))
  {
   Voice->LastNote->Next = Note;
  }
  
  Note->Prev = Voice->LastNote;
  Voice->LastNote = Note;
  
  Voice->NoteCount += 1;
 }
 
 return Note;
}

//~ Muze - Record and replay

internal void
PlayNote(app_memory *Memory, app_state *App, voice *Voice, rec_note *Note)
{
 if(App->OutputSynthEnabled)
 {
  if(0) {}
  else if(Note->Kind == NoteKind_Note)
  {
   if(Note->Velocity > 0)
   {
    f32 Volume = (f32)Note->Velocity/127.f;
    tsf_channel_note_on(GlobalTSF, Voice->Channel, Note->Pitch, Volume);
   }
   else
   {
    tsf_channel_note_off(GlobalTSF, Voice->Channel, Note->Pitch);
   }
  }
  else if(Note->Kind == NoteKind_Pedal)
  {
   tsf_channel_midi_control(GlobalTSF, Voice->Channel, Note->Controller, Note->Velocity);
  }
 }
 
 if(App->OutputMIDIEnabled)
 {
  // TODO(luca): Channel ?
  // 1. Equip parameters to the Voice struct and use them to create messages here.
  
  midi_message Message = {0};
  switch(Note->Kind)
  {
   case NoteKind_Note:
   {
    Message.U8[0] = MIDIEventType_NoteOn;
    Message.U8[1] = Note->Pitch;
    f32 Volume = (f32)Note->Velocity*Voice->MIDISoftness;
    Message.U8[2] = (u8)Volume;
   } break;
   
   case NoteKind_Pedal:
   {
    Message.U8[0] = MIDIEventType_Control;
    Message.U8[1] = Note->Controller;
    Message.U8[2] = Note->Velocity;
   } break;
  }
  
  Memory->PlatformMIDISend(App->Out, Message.U32[0]);
 }
}

internal b32
IsNotePlaying(rec_note *Note, voice *Voice)
{
 b32 Result = false;
 
 if(Voice->IsPlaying)
 {
  f32 NoteStart = (Note->Timestamp);
  f32 NoteEnd = (NoteStart + Note->Duration);
  
  Result = (Voice->PlayPos >= NoteStart - *dtForFrame &&
            Voice->PlayPos < NoteEnd + *dtForFrame);
 }
 
 if(Voice->IsRecording)
 {
  Result = (Note->Duration == 0.f);
 }
 
 return Result;
}

internal void
StopAllPlayingNotes(app_memory *Memory, app_state *App, voice *Voice)
{
 for EachRecNote(Note, Voice->FirstNote)
 {
  b32 NoteIsPlaying = IsNotePlaying(Note, Voice);
  if(NoteIsPlaying)
  {
   if(Voice->IsRecording)
   {
    f32 NoteMaxDuration = (Voice->RecordLength - Note->Timestamp);
    f32 Now = (GetWallTime() - Voice->RecordStart);
    Note->Duration = ClampTop(Now - Note->Timestamp, NoteMaxDuration);
   }
   
   rec_note OffNote = *Note;
   OffNote.Velocity = 0;
   PlayNote(Memory, App, Voice, &OffNote);
  }
 }
}

//~ Muze - MIDI
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

internal void
ProcessMIDINotes(app_memory *Memory, app_state *App, voice *Voice, app_midi_event *Events, u64 Count)
{
 for EachIndex(Idx, Count)
 {
  app_midi_event *Event = Events + Idx;
  f32 Timestamp = Event->Timestamp;
  
#if MUZE_INTERNAL
  // NOTE(luca): This is a hack to get loop editing to work properly.
  Timestamp = (f32)GetWallTime();
#endif
  
  Timestamp -= Voice->RecordStart;
  
  midi_message Message = {Event->Message};
  
  u8 Status = Message.U8[0];
  u8 Data1  = Message.U8[1];
  u8 Data2 = Message.U8[2];
  u8 Type = Status & 0xF0;
  u8 Channel = Status & 0x0F;
  
  b32 IsOutputDeviceDifferent = !(!App->OutputSynthEnabled && 
                                  App->In.Id == App->Out.Id);
  if(Voice->IsRecording)
  {
   if(0) {} 
   else if(Type == MIDIEventType_NoteOn && Data2 > 0) 
   {
    rec_note *Note = NoteAdd(Voice);
    
    Note->Timestamp = Timestamp;
    Note->Pitch = Data1;
    Note->Velocity = Data2;
    Note->Kind = NoteKind_Note;
    
    Voice->MaxPitch = Max(Note->Pitch, Voice->MaxPitch);
    Voice->MinPitch = Min(Note->Pitch, Voice->MinPitch);
    
    
    if(IsOutputDeviceDifferent)
    {
     PlayNote(Memory, App, Voice, Note);
    }
   }
   else if(Type == MIDIEventType_NoteOff || 
           (Type == MIDIEventType_NoteOn && Data2 == 0))
   {
    u8 Pitch = Data1;
    u8 Velocity = Data2;
    
    // NOTE(luca): NoteCount can be 0 if we start recording when a note is still playing.
    if(Voice->NoteCount > 0)
    {
     b32 NoteFound = false;
     // Set the duration of the last note with same pitch
     for EachRecNoteBack(Note, Voice->LastNote)
     {
      if(Note->Kind == NoteKind_Note && 
         Note->Pitch == Pitch)
      {
       NoteFound = true;
       Note->Duration = (Timestamp - Note->Timestamp);
       break;
      }
     }
     
     if(NoteFound)
     {                
      rec_note OffNote = {0};
      OffNote.Pitch = Pitch;
      if(IsOutputDeviceDifferent)
      {
       PlayNote(Memory, App, Voice, &OffNote);
      }
     }
     else
     {
      // Discard.
     }
    }
   }
   else if(Type == MIDIEventType_Control) 
   {
    u8 Controller = Data1;
    if(Controller == 64)
    {
     u8 Velocity = Data2;
     
     b32 On = (Velocity != 0);
     
     if(On)
     {                    
      rec_note *Note = NoteAdd(Voice);
      Note->Kind = NoteKind_Pedal;
      Note->Controller = Controller;
      Note->Velocity = Velocity;
      Note->Timestamp = Timestamp;
      
      if(IsOutputDeviceDifferent)
      {
       PlayNote(Memory, App, Voice, Note);
      }
     }
     else
     {
      rec_note *NoteFound = NilNote;
      
      // Find matching pedal start
      {
       for EachRecNoteBack(Note, Voice->LastNote)
       {
        if(Note->Kind == NoteKind_Pedal)
        {
         NoteFound = Note;
         break;
        }
       }
       Assert(!IsNilNote(NoteFound));
      }
      
      NoteFound->Duration = (Timestamp - NoteFound->Timestamp);
      
      // Stop pedal
      {
       rec_note OffNote = *NoteFound;
       OffNote.Velocity = 0;
       
       if(IsOutputDeviceDifferent)
       {
        PlayNote(Memory, App, Voice, &OffNote);
       }
      }
     }
    }
   }
  }
 }
}

internal void 
TogglePlayingAllVoices(app_memory *Memory, app_state *App)
{
 for EachIndex(VoiceIdx, App->VoiceCount)
 {
  voice *Voice = App->Voices + VoiceIdx;
  
  if(Voice->IsPlaying)
  {
   Voice->IsPlaying = false;
  }
  else
  {
   StopAllPlayingNotes(Memory, App, Voice);
   Voice->IsPlaying = true;
  }
 }
}


internal void
StopRecording(voice *Voice)
{
 Voice->IsRecording = false;
}

//~ Muze - Piece
internal piece_note *
PieceNoteAdd(piece *Piece, 
             piece_note_kind Kind, s32 Pitch, f32 Length)
{
 piece_note *Note = NilPieceNote;
 
 if(Piece->NoteCount < ArrayCount(Piece->Notes))
 { 
  Note = Piece->Notes + Piece->NoteCount;
  
  // TODO(luca): Better way?
  // NOTE(luca): Sometimes we will add fractions which are not powers of 2, this is to
  // correct the inaccuracies
  f32 DecimalPart = ModF32(Piece->RecordLength, 1.f);
  if(EqualsWithEpsilon(DecimalPart, 1.f, .001f))
  {
   Piece->RecordLength += 1.f - DecimalPart;
  }
  
  Note->Kind = Kind;
  Note->Pitch = Pitch + Note_Count*Piece->Octave;
  Note->Length = Length;
  Note->Start = Piece->RecordLength;
  
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
PieceWasTimestampPlayed(piece *Piece, f32 Timestamp)
{
 b32 Result = false;
 
 f32 BPS = Piece->BPM/60.f;
 
 f32 Now = Piece->PlayPos;
 f32 Before = Piece->PlayPos - BPS*(*dtForFrame);
 
 Result = (Timestamp > Before && 
           Timestamp <= Now);
 
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
#if 1
    Result->Preset = 0;
#else
    Result->Preset = 73;
#endif
    Result->Channel = App->NextChannelIdx;
    App->NextChannelIdx += 1;
    tsf_channel_set_presetindex(GlobalTSF, Result->Channel, Result->Preset);
   }
  }
  
  // Defaults
  if(!IsNilPiece(Result))
  {
   Result->BPM = 60;
   Result->TimeSigNum = 2;
   Result->TimeSigDen = 4;
   Result->Key = Note_C;
   Result->Major = true;
   
   Result->NoteCount = 0;
   Result->Octave = 4;
   
   Result->IsLooping = true;
   Result->BarWrapping = true;
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
PieceStopPlaying(piece *Piece)
{
 if(Piece->IsPlaying)
 {
  tsf_channel_note_off_all(GlobalTSF, Piece->Channel);
  Piece->IsPlaying = false;
 }
}

#if 0
# include "../data/muze/muze_pieces.c"
#endif

//~ UI 
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
  
  str8 String = Str8Fmt("(mem) %S: %.3f%S/%.1f%S", 
                        Name, 
                        (f32)Arena->Pos/(f32)PosUnit.Value, 
                        PosUnit.Name, 
                        (f32)Arena->Size/(f32)SizeUnit.Value,
                        SizeUnit.Name);
  
  App->DebugStrings[App->DebugStringCount] = String;
  App->DebugStringCount += 1;
 }
}

//~ Custom draw UI elements 
internal rec_note_node *
FindSelForNote(rec_note_node *Selection, rec_note *Note)
{
 rec_note_node *Result = 0;
 
 for EachNode(Node, rec_note_node, Selection)
 {
  if(Node->Value == Note)
  {
   Result = Node;
   break;
  }
 }
 
 return Result;
}


typedef struct custom_draw_note_input_result custom_draw_note_input_result;
struct custom_draw_note_input_result
{
 b32 LeftDown;
 b32 RightDown;
 v2 MouseP;
 v2 MouseStartP;
 b32 Adding;
 v4 SelDest;
};

internal custom_draw_note_input_result
CustomDrawNoteInput(ui_box *Box)
{
 custom_draw_note_input_result Result = {0};
 
 app_input *Input = UI_State->Input;
 
 if(UI_State->InputConsumerBox == Box)
 {
  Result.LeftDown = Input->Mouse.Buttons[PlatformMouseButton_Left].EndedDown;
  Result.RightDown = Input->Mouse.Buttons[PlatformMouseButton_Right].EndedDown;
  UI_ConsumeInput(Input, Box);
 }
 
 Result.Adding = (Result.LeftDown && !Result.RightDown);
 Result.MouseP = MousePosFromInput(Input);
 Result.MouseStartP = V2V2S32(Input->Mouse.Start);
 
 v4 SelDest;
 // Get selection rectangle
 {        
  SelDest = (v4){.Min = Result.MouseP, .Max = Result.MouseStartP};
  if(SelDest.Min.X > SelDest.Max.X) Swap(SelDest.Min.X, SelDest.Max.X);
  if(SelDest.Min.Y > SelDest.Max.Y) Swap(SelDest.Min.Y, SelDest.Max.Y);
  SelDest = RectIntersect(Box->Rec, SelDest);
 }
 Result.SelDest = SelDest;
 
 if(Result.LeftDown || Result.RightDown)
 {
  DrawRect(SelDest, V4(V3Arg(Result.Adding ? Color_Blue : Color_Snow2), .3f), 1.f, 0.f, 0.f);
  DrawRect(SelDest, V4(V3Arg(Result.Adding ? Color_Orange : Color_Black), 1.f), 1.f, 1.f, 0.f);
 }
 
 return Result;
}

internal void
AddNoteToSelection(custom_draw_note_input_result *NoteInput, v4 Dest,
                   app_state *App, voice *Voice, rec_note_node *Sel, rec_note *Note)
{
 // Draw selection rectangle
 if(NoteInput->LeftDown || NoteInput->RightDown)
 {
  // NOTE(luca): @Command
  if(RectOverlap(Dest, NoteInput->SelDest))
  {
   if(NoteInput->Adding && !Sel)
   {
    rec_note_node *Node;
    
    // Allocate
    {
     if(!App->FirstFreeNode)
     {
      Node = PushArray(App->Arena, rec_note_node, 1);
     }
     else
     {
      Node = App->FirstFreeNode;
      App->FirstFreeNode = Node->Next;
     }
     MemoryZero(Node);
    }
    
    // Add to selection
    {                            
     Node->Value = Note;
     
     if(Voice->NoteSel) 
     {
      Voice->NoteSel->Prev = Node;
     }
     Node->Next = Voice->NoteSel;
     
     Voice->NoteSel = Node;
    }
   }
   else if(!NoteInput->Adding && Sel)
   {
    // Remove from selection
    {
     if(Sel->Prev)
     {
      Sel->Prev->Next = Sel->Next;
     }
     
     if(Sel->Next)
     {
      Sel->Next->Prev = Sel->Prev;
     }
     
     if(Sel == Voice->NoteSel)
     {
      Voice->NoteSel = (Sel->Next ? Sel->Next : Sel->Prev);
     }
    }
    
    // Add to free list
    {
     Sel->Next = App->FirstFreeNode;
     App->FirstFreeNode = Sel;
    }
   }
  }
 }
}

UI_CUSTOM_DRAW(CustomDrawSheetMusic)
{
 muze_box_data *Data = (muze_box_data *)CustomDrawData;
 ui_box *Box = Data->Box;
 voice *Voice = Data->Voice;
 app_input *Input = UI_State->Input;
 app_state *App = Data->App;
 
 v4 BackgroundColor = Box->BackgroundColor;
 v4 ForegroundColor = Color_Black;
 
 DrawRect(Box->Rec, BackgroundColor, 0.f, 0.f, 0.f);
 
 f32 Padding = 16.f;
 
 f32 NoteSize = 11.f;
 v2 NoteDim = V2(NoteSize, NoteSize);
 v2 TailDim = V2(2.f, 24.f);
 
 f32 SheetMusicWidth = (Box->FixedSize.X);
 
 v4 BarColor = ForegroundColor;
 
 v2 BoxPos = V2SubV2(Box->FixedPos, Box->Scroll);
 v2 BoxSize = Box->FixedSize;
 
 f32 BPS = (App->BPM/60.f);
 
 // Staff Lines
 v2 StaffPos = BoxPos;
 StaffPos.X += Padding;
 
 s32 StaffLineCount = 5;
 f32 StaffLineWidth = 1.f;
 f32 StaffHeight = (f32)(StaffLineCount-1)*NoteSize + StaffLineWidth;
 
 // Bars
 f32 WholeBarWidth = 100.f;
 f32 BarDuration = (1.f/BPS)*(f32)App->TimeSig;
 
 // NOTE(luca): When it is exactly the length it is wrong, so we add a little offset to unmatch it.
 f32 BarCount = 1.f + FloorF32(Voice->RecordLength/(BarDuration + 1e-6f));
 v2 BarDim = V2(2.f, StaffHeight);
 
 // Draw staff
 {    
  StaffPos.Y += .5f*(BoxSize.Y - StaffHeight);
  {   
   for EachIndex(Idx, StaffLineCount)
   {
    f32 X = StaffPos.X;
    f32 Y = StaffPos.Y + (f32)Idx*(NoteSize);
    
    f32 Width = (BarCount)*WholeBarWidth;
    
    v4 Dest = RectFromSize(V2(X, Y), V2(Width, StaffLineWidth));
    Dest = RectIntersect(Dest, Box->Rec);
    
    DrawRect(Dest, ForegroundColor, 0.f, 0.f, 0.f); 
   }
  }
 }
 
 // Draw bars
 {    
  for EachIndex(Idx, (s32)BarCount + 1)
  {
   v2 BarPos = V2(BoxPos.X + ((f32)(Idx) * WholeBarWidth),
                  StaffPos.Y);
   BarPos.X += Padding;
   
   v4 Dest = RectIntersect(RectFromSize(BarPos, BarDim), Box->Rec);
   DrawRect(Dest, BarColor, 0.f, 0.f, 0.f); 
  }
 }
 
 // Input
 custom_draw_note_input_result NoteInput = CustomDrawNoteInput(Box);
 
 for EachRecNote(Note, Voice->FirstNote)
 {        
  if(Note->Kind == NoteKind_Note)
  {
   rec_note_node *Sel = FindSelForNote(Voice->NoteSel, Note);
   b32 NoteIsPlaying = IsNotePlaying(Note, Voice);
   
   v4 NoteColor = (Sel ? Color_Blue : 
                   (NoteIsPlaying ? Color_Yellow :
                    ForegroundColor));
   
   Assert(!(Voice->IsPlaying && Note->Duration == 0.f));
   
   u8 PitchClass = (Note->Pitch%Note_Count);
   
   s32 Length = 1;
   
   f32 NoteBorderSize = 0.f;
   
   f32 Duration = Note->Duration;
   if(Duration == 0.f)
   {
    Assert(Voice->IsRecording);
    f32 Now = (GetWallTime() - Voice->RecordStart);
    Duration = (Now - Note->Timestamp);
   }
   
   f32 NoteLength = (Duration*BPS);
   
   // NOTE(luca): Get pos relative of time inside bar 
   f32 NoteStart = Note->Timestamp;
   f32 NoteX = (NoteStart*BPS/(f32)App->TimeSig)*WholeBarWidth;
   
   s32 FirstNoteOctave = 6;
   s32 FirstNoteSteps = NoteStepFromPitch[Note_F];
   
   s32 NoteSteps = NoteStepFromPitch[PitchClass];
   s32 NoteOctave = Note->Pitch/Note_Count;
   
   s32 Steps = ((FirstNoteOctave*BaseNote_Count + FirstNoteSteps) - (NoteOctave*BaseNote_Count + NoteSteps));
   f32 StepHeight = .5f*NoteSize;
   
   f32 NoteYOffset = ((f32)(Steps - 1)*StepHeight);
   v2 NotePos = V2(Padding + BoxPos.X + NoteX, StaffPos.Y + NoteYOffset + .5f*StaffLineWidth);
   
   if(NoteLength < 4.f)
   {
    f32 X = NotePos.X + NoteSize - NoteBorderSize - 2.f;
    f32 Y = NotePos.Y + .5f*NoteSize;
    
    v4 Dest = RectFromSize(V2(X, Y - TailDim.Y),
                           TailDim);
    Dest = RectIntersect(Dest, Box->Rec);
    
    rect_instance *Inst = DrawRect(Dest, NoteColor, 0.f, 0.f, 0.f);
    Inst->CornerRadii.e[2] = .5f;
    
    // NOTE(luca): Will be negative for notes longer than 1
    s32 NumOfSideTails = (s32)RoundF32(-Log2F32(NoteLength));
    for EachIndex(TailIdx, NumOfSideTails)
    {
     f32 SideTailWidth = (TailDim.X);
     f32 SideTailY = ((Y + (2.f*SideTailWidth)*(f32)TailIdx) - TailDim.Y);
     v4 TailDest = RectFromSize(V2(X, SideTailY), V2(8.f, SideTailWidth));
     TailDest = RectIntersect(TailDest, Box->Rec);
     
     DrawRect(TailDest, NoteColor, 0.f, 0.f, 0.f);
    }
    
   }
   
   if(RoundF32(NoteLength) >= 2.f)
   {
    NoteBorderSize = 2.f;
   }
   
   // Draw note and do input
   {
    v4 Dest = RectFromSize(NotePos, NoteDim);
    Dest = RectIntersect(Dest, Box->Rec);
    DrawRect(Dest, NoteColor, .5f*NoteSize, NoteBorderSize, .5);
    
    AddNoteToSelection(&NoteInput, Dest, App, Voice, Sel, Note);
   }
   
   b32 HasSharp = !NoteIsWhite[PitchClass];
   if(HasSharp)
   {
    // TODO(luca): Draw sharp
   }
   
  }
 }
 
 // Draw the roll and markers
 {
  v2 MarkerPos = BoxPos;
  MarkerPos.X += Padding;
  f32 Height = Box->FixedSize.Y;
  
  f32 Zoom = (BPS/(f32)App->TimeSig*100.f);
  
  // NOTE(luca): This is also the current time when recording.
  f32 RecordMarkerX;
  {
   f32 Timestamp = Max(Voice->RecordLength, 0.f);
   f32 ZoomedTimestamp = Timestamp*Zoom;
   RecordMarkerX = MarkerPos.X + RoundF32(ZoomedTimestamp);
  }
  
  f32 PlayMarkerX;
  {
   f32 Timestamp = Voice->PlayPos;
   f32 ZoomedTimestamp = Timestamp*Zoom;
   PlayMarkerX = MarkerPos.X + RoundF32(ZoomedTimestamp); 
  }
  
  v4 RecDest = RectFromSize(V2(PlayMarkerX, MarkerPos.Y), 
                            V2(2.f, Height));
  v4 RecordDest = RectFromSize(V2(RecordMarkerX, MarkerPos.Y), 
                               V2(2.f, Height));
  v4 PlayDest = RectFromSize(V2(PlayMarkerX, MarkerPos.Y), 
                             V2(2.f, Height));
  
  DrawRect(RectIntersect(RecordDest, Box->Rec), Color_Red, 0.f, 0.f, 0.f);
  DrawRect(RectIntersect(PlayDest, Box->Rec), Color_Green, 0.f, 0.f, 0.f);
 }
 
 if(Box->Flags & UI_BoxFlag_DrawBorders)
 {
  DrawRect(Box->Rec, Box->BorderColor, 0.f, Box->BorderThickness, Box->Softness);
 }
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
   v2 BarPos = StaffPos;
   
#if 0 && MUZE_INTERNAL
   // Visualize space taken up by padding etc. 
   {
    v2 P = BarPos;
    
    DrawFilledRect(P, V2(BarPadding, 4.f), Color_Orange);
    P.X += BarPadding;
    
    DrawFilledRect(P, V2(BeatWidth, 4.f), Color_Blue);
    P.X += BeatWidth;
    
    DrawFilledRect(P, V2(BeatWidth, 4.f), Color_Yellow);
    P.X += BeatWidth;
    
    DrawFilledRect(P, V2(BarPadding, 4.f), Color_Orange);
   }
#endif
   
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
     
     if(Piece->BarWrapping)
     {
      BarPos.X = StaffPos.X;
      BarPos.Y += 2.f*StaffHeight;
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
   
   f32 BarHeightIdx = 0;
   
   if(0) {}
   else if(Kind == PieceMarkerKind_Play)
   {
    Color = Color_Green;
    Start = Piece->PlayPos;
   }
   else if(Kind == PieceMarkerKind_PrevPlay)
   {
    Color = Color_Yellow;
    Start = Piece->PlayPos;
    FollowPos = &Piece->PrevPlayPos;
   }
   else if(Kind == PieceMarkerKind_Record)
   {
    Color = Color_Red;
    Start = Piece->RecordLength;
    AlignMarkerToBarEnd = true;
    BarHeightIdx = 1.f;
   }
   else if(Kind == PieceMarkerKind_PracticeStart)
   {
    Color = Color_Blue;
    Start = Piece->PracticeStart;
    BarHeightIdx = 2.f;
   }
   else if(Kind == PieceMarkerKind_PracticeEnd)
   {
    Color = Color_Orange;
    Start = Piece->PracticeEnd;
    BarHeightIdx = 3.f;
   }
   else if(Kind == PieceMarkerKind_PracticePos)
   {
    Color = Color_Green;
    Start = Piece->PracticePos;
    BarHeightIdx = 4.f;
   }
   else if(Kind == PieceMarkerKind_PrevPracticePos)
   {
    Color = Color_Green;
    Start = Piece->PracticePos;
    FollowPos = &Piece->PrevPracticePos;
    BarHeightIdx = 4.f;
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
   
   f32 BeatsPerBar = Piece->TimeSigNum;
   
   v2 MarkerPos = Pos;
   s32 BarIdx = 0;
   f32 Remainder = 0.f;
   
   // NOTE(luca): PracticePos must use same calculations as notes such that the marker is
   // placed before the note.
   // All other markers have the special case that they when they equal the bar length, they
   // are placed on the end of the bar and not on the next bar's start.
   if(AlignMarkerToBarEnd)
   {
    BarIdx = (Start == 0.f ? 0 : (s32)CeilF32(Start/BeatsPerBar) - 1);
    Remainder = Start - BeatsPerBar*(f32)BarIdx;
   }
   else
   {
    BarIdx = (s32)FloorF32(Start/Piece->TimeSigNum);
    Remainder = ModF32(Start, Piece->TimeSigNum);
   }
   
   if(Piece->BarWrapping)
   {
    s32 HeightIdx = BarIdx/BarsPerWidth;
    BarIdx = (BarIdx%BarsPerWidth);
    MarkerPos.Y += (f32)HeightIdx*2.f*StaffHeight;
   }
   MarkerPos.Y += BarHeightIdx*NoteDim.Y;
   
   f32 BarStart = BarPadding + (f32)BarIdx*WholeBarWidth;
   MarkerPos.X = Pos.X + BarStart + Remainder*BeatWidth;
   
   if(FollowPos)
   {
    // NOTE(luca): We draw first, then update the position.  This ensures that the first
    // trail will start from the end.
    
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
     DrawFilledRect(TrailPos, V2(Width, NoteDim.Y), Color);
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
 
 // Draw notes
 {
  v2 NotePos = Pos;
  NotePos.X += BarPadding;
  
  f32 BPS = Piece->BPM/60.f;
  
  // Used to see when a new bar begins and we should add padding.
  f32 LengthSum = 0.f;
  
  for EachIndex(Idx, Piece->NoteCount)
  {
   piece_note *Note = Piece->Notes + Idx;
   
   v4 NoteColor = StaffColor;
   
   b32 NoteIsPlaying = false;
   if(Piece->IsPlaying)
   {
    f32 Start = Note->Start;
    f32 End = Start + Note->Length;
    
    NoteIsPlaying = (Piece->PlayPos >= Start &&
                     Piece->PlayPos < End);
   }
   
   if(Note->Kind == PieceNoteKind_Pitch)
   {
    s32 RelPitch = Note->Pitch%Note_Count;
    b32 FillHead = (Note->Length < 2.f);;
    s32 FlagCount = 0;
    b32 HasStem = (Note->Length < 4.f);
    b32 IsDotted = false;
    b32 IsBarred = false;
    b32 IsSharp = !GetEl(NoteIsWhite, RelPitch);
    b32 IsHovered = false;
    b32 IsSelected = false;
    v2 TailPos = {0};
    b32 DebugDrawBoundingBox = false;
    
    // Calculate position
    {
     s32 RelOctave = 6;
     // Get Y From pitch
     // Sets the notes at the octave's C
     s32 MinStepCount = 5;
     f32 StepSize = NoteSize/2.f;
     
     s32 NoteOctave = Note->Pitch/Note_Count;
     s32 StepForPitch = GetEl(NoteStepFromPitch, RelPitch);
     s32 StepCount = BaseNote_Count*(RelOctave - NoteOctave);
     StepCount -= MinStepCount + StepForPitch;
     
     NotePos.Y = Pos.Y + .5f*StaffLineWidth + StepSize*(f32)StepCount;
     
     s32 BarIdx = (s32)FloorF32(Note->Start/Piece->TimeSigNum);
     s32 HeightIdx = BarIdx/BarsPerWidth;
     
     if(Piece->BarWrapping)
     {
      BarIdx = (BarIdx%BarsPerWidth);
      NotePos.Y += (f32)HeightIdx*2.f*StaffHeight;
     }
     
     f32 BarStart = BarPadding + (f32)BarIdx*WholeBarWidth;
     f32 Remainder = ModF32(Note->Start, Piece->TimeSigNum);
     NotePos.X = Pos.X + BarStart + Remainder*BeatWidth;
     
     TailPos = NotePos;
     {
      TailPos.X += NoteDim.X - 2.f;
      TailPos.Y -= TailDim.Y - .5f*NoteDim.Y;
     }
    }
    
    if(NoteIsPlaying) NoteColor = Color_Yellow;
    
    v4 HeadBounds = {0};
    v4 TailBounds = {0};
    // Is Hovered and compute bounds
    {
     v2 MousePos = V2V2S32(Input->Mouse.Pos);
     HeadBounds = RectFromSize(NotePos, NoteDim);
     IsHovered = IsInsideRectV2(MousePos, HeadBounds);
     
     if(HasStem)
     {
      f32 ExtraWidth = 3.f;
      TailBounds = RectFromSize(TailPos, TailDim);
      TailBounds.Min.X -= ExtraWidth;
      TailBounds.Max.X += ExtraWidth;
      IsHovered |= IsInsideRectV2(MousePos, TailBounds);
     }
    }
    
    // Selected
    {
     IsSelected = (Idx == Piece->SelectedNoteIdx);
    }
    
    if(IsSelected)
    {
     NoteColor = Color_Orange;
    }
    
    if(IsHovered)
    {
     NoteColor = Color_Blue;
     
     if(WasPressed(Input->Mouse.Buttons[PlatformMouseButton_Left]))
     {
      command *Command = PushCommand(App, Command_PieceSelectNote);
      Command->Piece = Piece;
      Command->U64 = Idx;
     }
    }
    
    if(Piece->IsPracticing) NoteColor.W *= 0.5f;
    
    // Find out if is dotted
    {
     f32 Exp = -Log2F32(Note->Length);
     f32 Remainder = ModF32(Exp, 1.f);
     IsDotted = !(EqualsWithEpsilon(Remainder, 0.f, 0.0001f));
    }
    
    // Find out if is barred
    {
     IsBarred = (Note->Pitch == Note_C);
    }
    
    if(HasStem)
    {
     // Draw Flags
     {
      FlagCount = (s32)(-Log2F32(Note->Length));
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
     SharpPos.Y -= 5.f;
     
     DrawRectChar(UI_State->Atlas, FontKind_Text, SharpPos, '#', NoteColor);
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
  }
 }
}

UI_CUSTOM_DRAW(CustomDrawPianoRoll)
{
 muze_box_data *Data = (muze_box_data *)CustomDrawData;
 ui_box *Box = Data->Box;
 voice *Voice = Data->Voice;
 app_state *App = Data->App;
 
 app_input *Input = UI_State->Input;
 
 f32 BPS = App->BPM/60.f;
 
 f32 Zoom = (BPS/(f32)App->TimeSig*100.f);
 
 v4 BackgroundColor = Box->BackgroundColor;
 v4 ForegroundColor = Color_Black;
 
 DrawRect(Box->Rec, BackgroundColor, 0.f, 0.f, 0.f);
 
 custom_draw_note_input_result NoteInput = CustomDrawNoteInput(Box);
 
 u8 MaxPitch = Max(Voice->MaxPitch, 75);
 u8 MinPitch = Min(Voice->MinPitch, 40);
 
 // Piano roll
 {
  // NOTE(luca): One extra note for pedal
  u8 Range = (MaxPitch - MinPitch) + 1;
  
  f32 NoteHeight = (Box->FixedSize.Y/(f32)Range);
  f32 NoteWidth = 1.5f*NoteHeight;
  f32 PianoKeyGap = 1.f;
  
  f32 RollX = (Box->FixedPos.X - Box->Scroll.X);
  f32 RollY = (Box->FixedPos.Y - Box->Scroll.Y);
  f32 RollHeight = Box->FixedSize.Y;
  f32 RollWidth = Box->FixedSize.X;
  
  // Draw piano overlay
  {
   f32 PianoWidth = 1.5f*NoteWidth;
   v2 PianoPos = Box->FixedPos;
   
   for EachIndex(Idx, Range)
   {
    f32 YOffset = NoteHeight*(f32)(Range - Idx - 1);
    
    f32 X = (PianoPos.X);
    f32 Y = (PianoPos.Y + YOffset);
    
    // NOTE(luca): Minus one for pedal
    u8 PitchClass = ((MinPitch + (u8)Idx)%Note_Count);
    b32 White = NoteIsWhite[PitchClass];
    
    // NOTE(luca): First note is pedal.
    v4 KeyColor = (Idx == 0 ?
                   Color_Orange :
                   (White ? Color_Snow0 : Color_Black));
    KeyColor.A = .2f; 
    
    v4 NoteRec = RectFromSize(V2(X, Y), V2(PianoWidth, NoteHeight - PianoKeyGap));
    v4 Dest = RectIntersect(NoteRec, Box->Rec);
    DrawRect(Dest, KeyColor, 0.f, 0.f, 0.f);
   }
  }
  
  // Draw the roll and markers
  {        
   // NOTE(luca): This is also the current time when recording.
   f32 RecordMarkerX;
   {
    f32 Timestamp = Max(Voice->RecordLength, 0.f);
    f32 ZoomedTimestamp = Timestamp*Zoom;
    RecordMarkerX = RollX + RoundF32(ZoomedTimestamp);
   }
   
   f32 PlayMarkerX;
   {
    f32 Timestamp = Voice->PlayPos;
    f32 ZoomedTimestamp = Timestamp*Zoom;
    PlayMarkerX = RollX + RoundF32(ZoomedTimestamp); 
   }
   
   for EachRecNote(Note, Voice->FirstNote)
   {
    b32 NoteIsRecording = (Note->Duration == 0.f);
    
    u8 Velocity = Note->Velocity;
    
    f32 StartX = RoundF32(Note->Timestamp*Zoom);
    
    // NOTE(luca): We have to flip the Y since we have TopDown coordinates for UI.
    // NOTE(luca): Minus one for pedal note
    f32 StartY = NoteHeight*(f32)((Range - 1) - (Note->Pitch - MinPitch));
    
    if(Note->Kind == NoteKind_Pedal)
    {
     StartY = NoteHeight*(f32)(Range - 1);
    }
    
    f32 Width;
    {                
     f32 EndX = StartX + NoteWidth;
     if(NoteIsRecording)
     {
      EndX = RecordMarkerX - RollX;
     }
     else
     {
      // NOTE(luca): We might want to shrink this
      EndX = RoundF32((Note->Timestamp + Note->Duration)*Zoom);
     }
     
     Width = (EndX - StartX);
    }
    
    v2 NotePos = V2((RollX + StartX),
                    (RollY + StartY));
    
    u8 PitchClass = (Note->Pitch%Note_Count);
    b32 White = NoteIsWhite[PitchClass];
    
    rec_note_node *Sel = FindSelForNote(Voice->NoteSel, Note);
    
    v4 NoteColor = (IsNotePlaying(Note, Voice) ? Color_Yellow :
                    (Sel ? Color_Blue : 
                     ((Note->Kind == NoteKind_Pedal) ? Color_Orange : 
                      (White ? Color_Snow0 : Color_Black))));
    
    v4 NoteDest = RectFromSize(NotePos, V2(Width, NoteHeight)); 
    v4 Dest = RectIntersect(NoteDest, Box->Rec);
    
    AddNoteToSelection(&NoteInput, Dest, App, Voice, Sel, Note);
    
    // Draw Note
    {                
     rect_instance *Instance = DrawRect(Dest, NoteColor, 5.f, 0.f, .5f);
     Instance->Color0.A = .2f;
     Instance->Color2.A = .2f;
     
     if(IsInsideRectV2(NoteInput.MouseP, Dest))
     {
      // Draw border around when hovered
      if(Sel) White = !White;
      v4 BorderColor = (!White ? Color_Snow0 : Color_Black);
      DrawRect(Dest, BorderColor, 5.f, 2.f, 0.5f);
     }
    }
   }
   
   // Time markers
   {    
    v4 RecordDest = RectFromSize(V2(RecordMarkerX, RollY), 
                                 V2(2.f, RollHeight));
    v4 PlayDest = RectFromSize(V2(PlayMarkerX, RollY), 
                               V2(2.f, RollHeight));
    
    DrawRect(RectIntersect(RecordDest, Box->Rec), Color_Red, 0.f, 0.f, 0.f);
    DrawRect(RectIntersect(PlayDest, Box->Rec), Color_Green, 0.f, 0.f, 0.f);
   }
  }
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
 
 f32 EllipsisWidth = Atlas->PackedChars[' ' - Atlas->TextFont->FirstCodepoint].xadvance;
 
 f32 CurMaxX = (Box->FixedPos.X + Box->FixedSize.X - 
                (HorizontalPadding + EllipsisWidth));
 font *Font = Atlas->TextFont;
 b32 Overflowed = false;
 for EachIndex(Idx, Text.Size)
 {
  rune Char = Text.Data[Idx]; 
  
  s32 PackedCharIdx = GetPackedCharIdx(Font, Char);
  f32 CharWidth = (Atlas->PackedChars[PackedCharIdx].xadvance);
  
  DrawRectChar(Atlas, FontKind_Text, Cur, Char, Box->TextColor);
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

//~ UI 
internal void
DebugStringAdd(app_state *App, char *Format, ...)
{
 if(App->DebugStringCount < App->MaxDebugStringCount)
 {
  va_list Args;
  va_start(Args, Format);
  
  str8 String = Str8VFmt(Format, Args);
  App->DebugStrings[App->DebugStringCount] = String;
  App->DebugStringCount += 1;
 }
}

#define M_Button(ButtonText, ...) \
Button(.Text = ButtonText, .CenterText = true, .Padding = GlobalItemPadding, ##__VA_ARGS__).Pressed

#define M_IconButton(Kind, Name, ...) \
M_Button(UI_Icons[Kind], .String = Name, ##__VA_ARGS__)

#define M_ControlButton(ButtonText, ...) \
Button(.Text = ButtonText, .Padding = GlobalItemPadding, ##__VA_ARGS__).Pressed

#define M_SimpleToggleButton(ButtonText, Toggled, Color, ...) \
Button(.Text = ButtonText, .Padding = GlobalItemPadding, .HasToggle = true, .ToggleToggled = Toggled, Color_Green, .ToggleToggledColor = Color, ##__VA_ARGS__).OneClicked

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
  
  Result.Clicked = Slider->Clicked;
 }
 
 return Result;
}

internal f32
M_Scrollbar(axis2 Axis, f32 TotalSize, f32 Scroll)
{
 f32 Result = 0.f;
 
 f32 ScrollbarSize = 16.f;
 f32 ThumbSize = 32.f;
 f32 HalfThumbSize = .5f*ThumbSize;
 f32 Padding = 2.f;
 
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
   
   if(UI_IsActive(Thumb) || UI_IsActive(Scrollbar))
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

internal void
M_ControlLabel(str8 Name)
{
 UI_BackgroundColor(Color_Frost2) M_Label(Name);
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
 
 //- Some constants 
 Input->PlatformCursor = PlatformCursorShape_Arrow;
 v2 BufferDim = V2V2S32(Buffer->Dim);
 dtForFrame = &Input->dtForFrame;
 
 //- Init state 
 OS_ProfileInit(" G");
 
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
   
   App->FontAtlasArena = PushArena(PermanentArena, MB(100), false);
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
 
 platform_midi_get_devices_result Devices = Memory->PlatformMIDIGetDevices();
 
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
   char *Path = PathFromExe(FrameArena, S8("sounds.sf2"));
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
   
   App->MetronomeChannel = App->NextChannelIdx;
   App->NextChannelIdx += 1;
   App->MetronomePreset = 115; // Woodblock
   tsf_channel_set_presetindex(GlobalTSF, App->MetronomeChannel, App->MetronomePreset);
  }
  
  OS_ProfileAndPrint("TSF init");
  
  //- Init Muze 
  {
   App->MaxTextSize = 256;
   App->Text = PushS8(App->Arena, App->MaxTextSize);
   App->Text.Size = 0;
   
   App->TimeSig = 3;
   App->BPM = 100.f;
   App->InputMIDIEnabled = true;
   App->OutputSynthEnabled = true;
   App->InputVirtualKeyboardEnabled = true;
   App->MaxVoiceCount = 99;
   App->Voices = PushArray(App->Arena, voice, (u64)App->MaxVoiceCount);
   
   App->MaxCommandCount = KB(4);
   App->Commands = PushArray(App->Arena, command, App->MaxCommandCount);
   
   // Select default input and output device
   {
    b32 InputFound = false;
    b32 OutputFound = false;
    for EachIndex(Idx, Devices.Count)
    {
     platform_midi_device *Device = Devices.Devices + Idx;
     if(Device->IsOutput)
     {
      App->Out = *Device;
      OutputFound = true;
     }
     if(Device->IsInput)
     {
      App->In = *Device;
      InputFound = true;
     }
     
     if(InputFound && OutputFound)
     {
      break;
     }
    }
   }
   
   App->MaxPieceCount = (App->PieceArena->Size/sizeof(piece));
   App->PieceCount = 0;
  }
  
  OS_ProfileAndPrint("Muze init");
  
  //- Init Fonts
  {
   {
    char *FontPath = PathFromExe(FrameArena, S8("font_regular.ttf"));
    InitFont(&App->Font, FontPath);
   }
   {
    char *FontPath = PathFromExe(FrameArena, S8("icons.ttf"));
    InitFont(&App->IconsFont, FontPath);
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
    *NilNote = (rec_note){NilNote, NilNote};
    *NilPiece = (piece){NilPiece};
    *NilVoice = (voice){NilNote, NilNote};
   }
   
   OS_MarkReadonly(Arena->Base, Arena->Size);
   
   
  }
  OS_ProfileAndPrint("RO Init");
  
  RenderInit(&App->Render);
  OS_ProfileAndPrint("Renderer Init");
  
  // Init UI 
  {
   UI_InitState(App->UIArena);
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
      PanelAdd(.5f);
      PanelAdd(.5f);
      PanelChildren()
      {
       PanelAdd(.5f);
       PanelAdd(.5f);
      }
     }
    }
    
    for(panel *Panel = App->FirstPanel;
        !IsNilPanel(Panel);
        Panel = PanelRecDepthFirstPreOrder(Panel).Next)
    {
     if(IsLeafPanel(Panel))
     {
      Panel->Kind = PanelKind_Sheet;
      Panel->Voice = VoiceAdd(App);
     }
    }
    
    PanelSelect(App, PanelNextLeaf(App->FirstPanel, false));
    
    App->SelectedPanel->Kind = PanelKind_Piece;
   }
  }
  
  App->MaxDebugStringCount = 128;
 }
 
 // Frame Initialization
 {
  // Init read-only variables
  {    
   InitReadOnlyGlobals(App->ReadOnlyArena);
  }
  
  // Init Muze
  {
   App->CommandCount = 0;
  }
  
  OS_ProfileAndPrint("Misc. Init");
 }
 
 if(App->ListerOpened)
 {            
  for EachTextButton(Key, Id, Input)
  {
   s32 ModifiersWithoutShift = (Key->Modifiers & ~PlatformKeyModifier_Shift);
   
   if(ModifiersWithoutShift == PlatformKeyModifier_None)
   {
    if(!Key->IsSymbol)
    {
     App->Text.Data[App->Text.Size] = (u8)Key->Codepoint;
     App->Text.Size += 1;
     App->CursorAnimTime = 0.f;
     
     RemoveTextButton(Input, Id);
    }
    else
    {
     switch(Key->Symbol)
     {
      default: break;
      
      case PlatformKey_BackSpace:
      {
       App->Text.Size -= !!(App->Text.Size > 0);
       App->CursorAnimTime = 0.f;
      } break;
     }
    }
   }
  }
 }
 
 for EachTextButton(Key, Id, Input)
 {
  b32 Control = Key->Modifiers & PlatformKeyModifier_Control;
  b32 Shift = Key->Modifiers & PlatformKeyModifier_Shift;
  b32 Alt = Key->Modifiers & PlatformKeyModifier_Alt;
  s32 ModifiersShiftIgnored = (Key->Modifiers & ~PlatformKeyModifier_Shift);
  b32 None = (Key->Modifiers == PlatformKeyModifier_None);
  
  b32 Handled = true;
  
  if(!Key->IsSymbol)
  {
   if(Control)
   {
    switch(Key->Codepoint)
    {
     case 'b':
     {
      if(!Shift) App->BPM += 1;
      else       App->BPM -= 1;
     } break;
     
     case 'p': 
     {
      if(!Shift)
      {
       PushCommand(App, Command_SplitSelectedPanelX);
      }
      else
      {
       PushCommand(App, Command_ClosePanel)->Panel = App->SelectedPanel;
      }
     } break;
     
     case 't': PushCommand(App, Command_OpenLister)->ListerKind = ListerKind_PanelTypes; break;
     
     case '-':
     {
      PushCommand(App, Command_SplitSelectedPanelY);
     } break;
     
     case ',':
     {
      PushCommand(App, Command_SelectNextPanel);
     } break;
     
     case 's': PushCommand(App, Command_OpenLister)->ListerKind = ListerKind_Instruments; break;
     
     default: 
     {
      Handled = false;
     } break;
    }
   }
   else if(Alt)
   {
    switch(Key->Codepoint)
    {
     case 'b': DebugBreak(); break;
     
     default:
     {
      Handled = false;
     } break;
    }
   }
   else
   {
    switch(Key->Codepoint)
    {
     case ' ': 
     {
      if(App->SelectedPanel->Kind == PanelKind_Sheet)
      {
       PushCommand(App, Command_ToggleRecording); 
      }
     } break;
     default: 
     {
      Handled = false;
     } break;
    }
   }
   
  }
  else
  {
   switch(Key->Codepoint)
   {
    case PlatformKey_Return:
    {
     if(App->ListerOpened)
     {
      App->ListerReturnPressed = true;
     }
    } break;
    
    case PlatformKey_Escape:
    {
     PushCommand(App, Command_CloseLister);
     
     default: 
     {
      Handled = false;
     } break;
    } break;
   }
  }
  
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
  
  RenderBuildAtlas(App->FontAtlasArena, &App->FontAtlas, &App->Font, &App->IconsFont, 
                   App->HeightPx);
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
   App->DebugStringCount = 0;
   { 
    ui_box *Hot = UI_BoxFromKey(UI_State->Hot);
    ui_box *Active = UI_BoxFromKey(UI_State->Active);
    str8 ActiveString = Active->DisplayString;
    str8 HotString = Hot->DisplayString;
    if(ActiveString.Size == 0) ActiveString = S8("null");
    if(HotString.Size == 0) HotString = S8("null");
    
    ui_box *Box;
    
#if MUZE_INTERNAL
    if(!Memory->Reloaded)
    {     
     // NOTE(luca): When DLL is reloaded strings may have their location changed.  So, to not 
     // dereference them we just skip the frame where reloaded.
     StringsScratchTemp(UI_State->FrameArenaBack)
     {
      DebugStringAdd(App, "(ui) Active: %S", ActiveString);
      DebugStringAdd(App, "(ui) Hot: %S", HotString);
      DebugStringAdd(App, "(ui) Active->tHot = %.2f", Active->tHot);
      DebugStringAdd(App, "(ui) Active->tActive = %.2f", Active->tActive);
      DebugStringAdd(App, "(ui) Hot->tHot = %.2f", Hot->tHot);
      DebugStringAdd(App, "(ui) Hot->tActive = %.2f", Hot->tActive);
      
      DebugStringAddArena(App, S8("Perm"), PermanentArena);
      DebugStringAddArena(App, S8("Frame"), FrameArena);
      DebugStringAddArena(App, S8("R/O"), App->ReadOnlyArena);
      DebugStringAddArena(App, S8("Font"), App->FontAtlasArena);
      DebugStringAddArena(App, S8("UI"), App->UIArena);
      DebugStringAddArena(App, S8("Panel"), App->PanelArena);
      DebugStringAddArena(App, S8("Muze"), App->Arena);
      DebugStringAddArena(App, S8("Voice"), App->SelectedVoice->Arena);
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
   u64 SelectedIdx = 0; 
   u64 NameListSize = 0;
   str8 *NameList = 0;
   u64 StartIdx = 0;
   
   // Output
   s32 FilteredItemCount = 0;
   lister_item *FilteredItems = 0;
   platform_midi_device *OutDevices = 0;
   
   switch(App->ListerKind)
   {
    default: InvalidPath(); break;
    case ListerKind_Instruments:
    {
     SelectedIdx = (u64)App->SelectedVoice->PresetIdx; 
     NameListSize = (u64)App->InstrumentCount;
     NameList = App->InstrumentNames;
    } break;
    
    case ListerKind_MIDIInput:
    case ListerKind_MIDIOutput:
    {
     b32 IsInput = (App->ListerKind == ListerKind_MIDIInput);
     
     NameList = PushArray(FrameArena, str8, Devices.Count);
     OutDevices = PushArray(FrameArena, platform_midi_device, Devices.Count);
     for EachIndex(Idx, Devices.Count)
     {
      platform_midi_device *Device = Devices.Devices + Idx;
      platform_midi_device *Out = OutDevices + NameListSize;
      str8 *Name = NameList + NameListSize;
      
      if((IsInput && Device->IsInput) ||
         (!IsInput && Device->IsOutput))
      {
       *Name = Device->Name;
       *Out = *Device;
       
       if(Device->Id == App->In.Id)
       {
        SelectedIdx = NameListSize;
       }
       
       NameListSize += 1;
      }
     }
    } break;
    
    case ListerKind_DebugInfo:
    {
     SelectedIdx = (u64)-1;
     NameList = App->DebugStrings;
     NameListSize = App->DebugStringCount;
    } break;
    
    case ListerKind_PanelTypes:
    {
     panel *Panel = App->SelectedPanel;
     SelectedIdx = Panel->Kind;
     NameList = PanelTypeStrings;
     NameListSize = ArrayCount(PanelTypeStrings);
     StartIdx = 1;
    } break;
   }
   
   FilteredItems = PushArray(FrameArena, lister_item, NameListSize);
   
   for(u64 Idx = StartIdx; Idx < NameListSize; Idx += 1)
   {
    lister_item *Item = FilteredItems + FilteredItemCount;
    str8 Name = NameList[Idx];
    
    b32 Matched = false;
    
    // Match if Name contains App->Text case-insensitive
    {                
     if(App->Text.Size == 0)
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
     Item->Name = Name;
     Item->Idx = Idx;
     FilteredItemCount += 1;
    }
   }
   
   v2 ListerDim = V2(600.f, (f32)ShowItemCount*ItemHeight);
   V2Math ListerDim.E += 2.f*ListerBorderSize;
   ListerDim.Y += ItemHeight + ListerBorderSize;
   
   ui_box *Root = App->ListerBox;
   {    
    Root->Key.U64[0] = U64HashFromSeedStr8((u64)Root, S8("Root"));
    Root->FixedPos = V2(0.f, 0.f);
    Root->FixedSize = BufferDim;
    Root->Rec = RectFromSize(Root->FixedPos, Root->FixedSize);
    Root->LastTouchedFrameIdx = UI_State->FrameIdx;
   }
   
   UI_BeginLayout(Root, App->HeightPx);
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
       UI_BackgroundColor(Color_Night1)
       UI_AddBox(S8("Lister"), 
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
          
          UI_FillAll()
           UI_Push()
          {
           MaxIdx = Min(FilteredItemCount, MinIdx + ShowItemCount + 1);
           UI_Spacer(UI_SizePx(ItemHeight*(f32)MinIdx, 1.f));
           for(s32 Idx = MinIdx; Idx < MaxIdx; Idx += 1)
           {
            lister_item *Item = FilteredItems + Idx;
            str8 ItemString = Str8Fmt("Item%d", Idx);
            
            b32 Selected = (SelectedIdx == Item->Idx);
            
            UI_SemanticHeight(UI_SizePx(ItemHeight, 1.f))
            {
             if(Button(.String = ItemString,
                       .Text = Item->Name, 
                       .Disabled = Selected, 
                       .DisabledBackgroundColor = Color_Yellow, 
                       .Padding = ItemPadding,
                       .ClipSize = false).Released || 
                App->ListerReturnPressed)
             {
              switch(App->ListerKind)
              {
               default: InvalidPath(); break;
               
               case ListerKind_Instruments:
               {
                command *Command = PushCommand(App, Command_SelectInstrument);
                Command->S32 = (s32)Item->Idx;
               } break;
               
               case ListerKind_MIDIInput:
               {
                command *Command = PushCommand(App, Command_SelectInputDevice);
                Command->Device = OutDevices + Item->Idx;
               } break;
               
               case ListerKind_MIDIOutput:
               {
                command *Command = PushCommand(App, Command_SelectOutputDevice);
                Command->Device = OutDevices + Item->Idx;
               } break;
               
               case ListerKind_PanelTypes:
               {
                command *Command = PushCommand(App, Command_SetPanelType);
                Command->Panel = App->SelectedPanel;
                Command->PanelKind = (panel_kind)Item->Idx;
               } break;
              }
              
              PushCommand(App, Command_CloseLister);
             }
            }
           }
          }
         }
         
         App->ListerReturnPressed = false;
         
         FilteredItemCount = Max(FilteredItemCount, 1);
         f32 TotalElementSize = (f32)(FilteredItemCount - 1)*ItemHeight;
         
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
    Root->Key.U64[0] = U64HashFromSeedStr8((u64)Root, S8("Root"));
    Root->FixedPos = V2(0.f, 0.f);
    Root->FixedSize = BufferDim;
    Root->Rec = RectFromSize(Root->FixedPos, Root->FixedSize);
    Root->LastTouchedFrameIdx = UI_State->FrameIdx;
   }
   
   UI_BeginLayout(Root, App->HeightPx);
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
              
              if(Type->WasClicked)
              {
               command *Command = PushCommand(App, Command_OpenLister);
               Command->Panel = Panel;
               Command->ListerKind = ListerKind_PanelTypes;
              }
             }
             
             
             UI_FillWidth()
              UI_AddBox(S8(""), UI_BoxFlag_DrawBackground|
                        UI_BoxFlag_DrawBorders);
             
             // Close button 
             {
              ui_box *Close;
              UI_SemanticWidth(UI_SizePx(30.f, 1.f))
               UI_FontKind(FontKind_Icon)
               //UI_Debug(true)
               Close = UI_AddBox(S8("CloseButton"), (UI_BoxFlag_DrawBackground|
                                                     UI_BoxFlag_DrawBorders|
                                                     UI_BoxFlag_MouseClickable|
                                                     UI_BoxFlag_DrawHotEffects|
                                                     UI_BoxFlag_DrawActiveEffects|
                                                     UI_BoxFlag_DrawDisplayString|
                                                     UI_BoxFlag_CenterTextVertically|
                                                     UI_BoxFlag_CenterTextHorizontally|
                                                     0));
              Close->DisplayString = UI_Icons[UI_IconKind_Close];
              
              if(Close->Clicked)
              {
               command *Command = PushCommand(App, Command_ClosePanel);
               Command->Panel = Panel;
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
             
#if MUZE_INTERNAL && 0
             Piece->RecordLength = 0.f;
             Piece->NoteCount = 0;
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f);
             
#if 0
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 7.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 8.f);
#elif 0
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f);
#elif 0
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/16.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/16.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/16.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/16.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/16.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/16.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/16.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/16.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/16.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/16.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/16.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/16.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/16.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/16.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/16.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/16.f);
#elif 1
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/1.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/1.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/1.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/1.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/1.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/1.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/1.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/1.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/1.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/1.f);
             PieceNoteAdd(Piece, PieceNoteKind_Pitch, Note_C, 1.f/1.f);
#endif
             
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
                UI_FontKind(FontKind_Icon)
                 UI_SemanticWidth(UI_SizePx(30.f, 1.f))
                {
                 str8 PieceFilePath = S8("song.muze_piece");
                 
                 if(M_IconButton(UI_IconKind_File, S8("Open")))
                 {
                  M_SerializePiece(FrameArena, true, PieceFilePath, Piece);
                  Piece->IsRecording = false;
                  PieceStopPlaying(Piece);
                 }
                 
                 if(M_IconButton(UI_IconKind_Save, S8("Save")))
                 {
#if 0
                  M_SerializePiece(FrameArena, false, PieceFilePath, Piece);
#else
                  Disabled();
#endif
                 }
                 
                 if(M_IconButton(UI_IconKind_Record, S8("Record"), .TextColor = (Piece->IsRecording ? Color_Red : Color_ButtonText)))
                 {
                  PieceStopPlaying(Piece);
                  
                  if(Piece->IsRecording)
                  {
                   Piece->IsRecording = false;
                  }
                  else
                  {
                   Piece->IsRecording = true;
                  }
                 }
                 
                 if(M_IconButton(UI_IconKind_Play, S8("Play"), .TextColor = (Piece->IsPlaying ? Color_Green : Color_ButtonText)))
                 {
                  if(!Piece->IsPlaying)
                  {
                   tsf_channel_note_off_all(GlobalTSF, Piece->Channel);
                   Piece->IsPlaying = true;
                   
                   Piece->IsRecording = false;
                  }
                 }
                 
                 if(M_IconButton(UI_IconKind_Pause, S8("Pause")))
                 {
                  PieceStopPlaying(Piece);
                 }
                 
                 if(M_IconButton(UI_IconKind_Stop, S8("Stop")))
                 {
                  PieceStopPlaying(Piece);
                  Piece->PlayPos = Piece->PracticeStart;
                  Piece->PracticePos = Piece->PracticeStart;
                 }
                 
                 if(M_IconButton(UI_IconKind_Gauge, S8("Metronome"), .TextColor = (Piece->Metronome ? Color_Magenta : Color_Black)))
                 {
                  PushCommand(App, Command_PieceToggleMetronome)->Piece = Piece;
                 }
                 
                 if(M_IconButton(UI_IconKind_Loop, S8("Loop"), .TextColor = (Piece->IsLooping ? Color_Yellow : Color_Black)))
                 {
                  Piece->IsLooping ^= 1;
                 }
                 
                 
                }
                
                UI_FillWidth()
                 UI_AddBox(S8("TopSpacer"), 
                           UI_BoxFlag_DrawBackground|
                           UI_BoxFlag_DrawBorders);
                UI_SemanticWidth(UI_SizePx(200.f, 1))
                 Piece->BPM = M_Slider(S8("PieceBPM"), S8("BPM"), 
                                       Str8Fmt("%3.0f", Piece->BPM),
                                       Piece->BPM, 0.f, 300.f, 1.f, false,
                                       .Align = false).Value;
                
                UI_SemanticWidth(UI_SizeText(6.f, 1.f))
                 if(M_Button(S8("Clear")))
                {
                 PieceStopPlaying(Piece);
                 Piece->IsRecording = false;
                 Piece->NoteCount = 0;
                 Piece->RecordLength = 0.f;
                 Piece->PlayPos = 0.f;
                }
                
               }
               
               UI_SemanticHeight(UI_SizePx(ItemHeight,  1.f))
                UI_Row()
                UI_SemanticWidth(UI_SizePx(150.f, 1))
               {
                if(M_SimpleToggleButton(S8("Practice"), Piece->IsPracticing, Color_Yellow))
                {
                 PushCommand(App, Command_PieceTogglePracticing)->Piece = Piece;
                }
                
                UI_SemanticWidth(UI_SizeText(6.f, 1.f))
                 if(M_Button(S8("Start")))
                {
                 f32 Start = Piece->Notes[Piece->SelectedNoteIdx].Start;
                 if(Start < Piece->PracticeEnd)
                 {
                  Piece->PracticeStart = Start;
                 }
                }
                
                UI_SemanticWidth(UI_SizeText(6.f, 1.f))
                 if(M_Button(S8("End")))
                {
                 f32 Start = Piece->Notes[Piece->SelectedNoteIdx].Start;
                 if(Start > Piece->PracticeStart)
                 {
                  Piece->PracticeEnd = Start;
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
                 if(Piece->BarWrapping)
                 {
                  f32 WholeBarWidth = Piece->BeatWidth*Piece->TimeSigNum + 2.f*Piece->BarPadding;
                  s32 BarsPerWidth = (s32)((PieceBox->FixedSize.X - 2.f*Piece->BarPadding)/WholeBarWidth);
                  BarsPerWidth = Max(1, BarsPerWidth);
                  s32 BarCount = (s32)Max(1.f, CeilF32(Piece->RecordLength/Piece->TimeSigNum));
                  
#if 0            
                  // TODO(luca): Maybe we want this API, waiting for 3rd case
                  s32 WholeBarWidth = PieceWholeBarWidth(Piece);
                  s32 BarCount = PieceBarCount(Piece);
                  s32 BarsPerWidth = PieceBarsPerWidth(Piece, WholeBarWidth);
                  f32 StaffHeight = PieceStaffHeight(Piece);
                  
                  s32 Rows = ((BarCount/BarsPerWidth + 1)*2);
                  f32 Height = StaffHeight*Rows; 
                  
                  // Or something like this
                  f32 Height = PieceHeight();
                  // Or compute it in Piece
                  f32 Height = PieceHeight();
#endif
                  
                  s32 StaffLineCount = 5;
                  f32 StaffLineWidth = Piece->StaffLineWidth;
                  f32 StaffHeight = (f32)(StaffLineCount-1)*Piece->NoteSize + StaffLineWidth;
                  
                  f32 Height = PieceBox->FixedSize.Y;
                  s32 Rows = BarCount/BarsPerWidth + 2;
                  Rows *= 2;
                  f32 HeightForRows = (f32)Rows*StaffHeight; 
                  
                  VerticalOverflow = HeightForRows - Height;
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
             if(!Piece->BarWrapping)
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
                  if(UI_ButtonWithToggle(S8("Toggle keyboard input"), App->InputVirtualKeyboardEnabled,
                                         ItemPadding, Color_Yellow).OneClicked)
                  {
                   PushCommand(App, Command_ToggleVirtualKeyboardInput);
                  }
                  
                  str8 Name = Str8Fmt("Set MIDI Input device  - \"%S\"", Devices.Devices[App->In.Id].Name);
                  
                  ui_button_result Click = UI_ButtonWithToggle(Name, App->InputMIDIEnabled,
                                                               ItemPadding, Color_Yellow,
                                                               .String = S8("MIDIInput"));
                  if(Click.Toggled)
                  {
                   PushCommand(App, Command_ToggleMIDIInput);
                  }
                  if(Click.Pressed)
                  {
                   PushCommand(App, Command_OpenLister)->ListerKind = ListerKind_MIDIInput;
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
                   PushCommand(App, Command_OpenLister)->ListerKind = ListerKind_MIDIOutput;
                  }
                  
                  if(Click.Toggled)
                  {
                   PushCommand(App, Command_ToggleMIDIOutput);
                  }
                 }
                 
                 // Synth toggle and open instrument lister
                 {      
                  str8 Text = (SoundFontLoaded ? 
                               App->InstrumentNames[App->SelectedVoice->PresetIdx] : 
                               S8("No soundfont loaded."));
                  
                  Text = Str8Fmt("Set synth instrument   - \"%S\"", Text);
                  
                  ui_button_result Click = UI_ButtonWithToggle(Text, App->OutputSynthEnabled, ItemPadding, Color_Yellow, .String = S8("Synth"));
                  
                  App->OutputSynthEnabled ^= Click.Toggled;
                  if(Click.Pressed)
                  {
                   PushCommand(App, Command_OpenLister)->ListerKind = ListerKind_Instruments;
                  }
                 }
                }
                
                //- Song 
                {
                 App->BPM = M_Slider(S8("BPM"), S8("Set BPM"), 
                                     Str8Fmt("%3.0f", App->BPM),  
                                     App->BPM, 30.f, 300.f, 1.f, false, 
                                     .SliderSize = SliderSize).Value;
                 
                 App->TimeSig = (s32)M_Slider(S8("TimeSig"), S8("Set time signature"), 
                                              Str8Fmt("%d/4",  App->TimeSig),
                                              (f32)App->TimeSig, 1.f, 4.f, 1.f, false, 
                                              .SliderSize = SliderSize).Value;
                 
                 //- Song 
                 {
                  M_ControlLabel(S8("Song"));
                  
                  if(UI_ButtonWithToggle(S8("Record"), App->SelectedVoice->IsRecording, ItemPadding, Color_Red).OneClicked)
                  {
                   PushCommand(App, Command_ToggleRecording);
                  }
                  
                  if(UI_ButtonWithToggle(S8("Play"), App->SelectedVoice->IsPlaying, ItemPadding, Color_Green).OneClicked)
                  {
                   PushCommand(App, Command_TogglePlaying);
                  }
                  
                  if(UI_ButtonWithToggle(S8("Auto Scroll"), App->SelectedVoice->AutoScroll, ItemPadding, Color_Green).OneClicked)
                  {
                   PushCommand(App, Command_ToggleAutoScroll);
                  }
                  
                  if(M_ControlButton(S8("Stop recording/playing"))) PushCommand(App, Command_Stop);
                  if(M_ControlButton(S8("Trim recording"))) PushCommand(App, Command_Trim);
                  if(M_ControlButton(S8("Reset recording"))) PushCommand(App, Command_Reset);
                  if(M_ControlButton(S8("Play from beginning"))) PushCommand(App, Command_PlayFromStart);
                  if(M_ControlButton(S8("Play from note"))) PushCommand(App, Command_PlayFromNote);
                 }
                }
                
                //- Notes 
                {
                 M_ControlLabel(S8("Notes"));
                 
                 if(M_ControlButton(S8("Round notes' length"))) PushCommand(App, Command_Round);
                 if(M_ControlButton(S8("Sync notes to the beat"))) PushCommand(App, Command_Sync);
                 
                 if(M_ControlButton(S8("Guess BPM from notes"))) PushCommand(App, Command_GuessBPM);
                 if(M_ControlButton(S8("Delete selected notes"))) PushCommand(App, Command_DeleteSelection);
                 
                 {
                  f32 Length = App->SelectedVoice->NoteSetLength;
                  char *Format = (Length >= 0.f ? "%3.0f" : "1/%1.0f");
                  f32 Value = PowF32(2.f, AbsF32(Length));
                  str8 DisplayString = Str8Fmt(Format, Value);
                  
                  m_slider_result SliderResult = M_Slider(S8("SetLength"), S8("Set note Length"), 
                                                          Str8Fmt(Format, Value), 
                                                          App->SelectedVoice->NoteSetLength, -3.f, 2.f, 1.f, true, 
                                                          .SliderSize = SliderSize);
                  App->SelectedVoice->NoteSetLength = SliderResult.Value;
                  
                  if(SliderResult.Clicked)
                  {
                   PushCommand(App, Command_SetLength)->F32 = SliderResult.Value;
                  }
                 }
                 
                 if(M_ControlButton(S8("Clear selection"))) PushCommand(App, Command_ClearSelection);
                }
                
                //- Voice
                {
                 M_ControlLabel(S8("Voice"));
                 
                 if(M_ControlButton(S8("Delete voice"))) PushCommand(App, Command_DeleteVoice);
                 if(M_ControlButton(S8("New voice"))) PushCommand(App, Command_AddVoice);
                 if(M_ControlButton(S8("Enqueue playing"))) PushCommand(App, Command_Enqueue);
                 if(M_ControlButton(S8("Dequeue playing"))) PushCommand(App, Command_Dequeue);
                 if(M_ControlButton(S8("Play all voices"))) PushCommand(App, Command_PlayAllVoices);
                 if(M_ControlButton(S8("Stop all voices"))) PushCommand(App, Command_StopAllVoices);
                 
                 // Volume control
                 {                                    
                  f32 Volume = App->SelectedVoice->Volume;
                  Volume = M_Slider(S8("Volume"), S8("Set volume"),
                                    Str8Fmt("%3.0f%%", Volume*100.f), 
                                    Volume, 0.f, 1.f, .01f,
                                    false, 
                                    .SliderSize = SliderSize).Value;
                  tsf_channel_set_volume(GlobalTSF, App->SelectedVoice->Channel, Volume);
                  App->SelectedVoice->Volume = Volume;
                 }
                 
                 // 
                 {
                  f32 Softness = App->SelectedVoice->MIDISoftness;
                  Softness = M_Slider(S8("MIDISoftness"), S8("Set MIDI output softness"),
                                      Str8Fmt("%3.0f%%", Softness*100.f),
                                      Softness, 0.f, 1.f, .01f, false, 
                                      .SliderSize = SliderSize).Value;
                  App->SelectedVoice->MIDISoftness = Softness;
                 }
                 
                }
                
                
                
               }
               
               // Scrollbar
               {
                f32 ItemCount = (f32)32;
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
                
                if(M_Button(S8("Debug Info"), .CenterText = false))
                {
                 PushCommand(App, Command_OpenLister)->ListerKind = ListerKind_DebugInfo;
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
                  
                  
                  
                 }
                 
                 
                 
                }
                
                
               }
               
               // Scrollbar
               {                                                            
                f32 TotalElementSize = 2.f*ItemHeight;
                TotalElementSize -= ItemHeight;
                
                f32 Scroll = M_Scrollbar(Axis2_Y, TotalElementSize, Panel->Scroll.Y);
                ConfigList->Scroll.Y = Scroll;
                Panel->Scroll.Y = Scroll;
               }
              }
             }
             
            }
            else if(Panel->Kind == PanelKind_Sheet)
            {
             if(IsNilVoice(Panel->Voice)) Panel->Voice = VoiceAdd(App);
             voice *Voice = Panel->Voice;
             
             UI_FillAll()
              UI_Push()
              UI_Column()
             {
              //- Top controls 
              {
               UI_SemanticHeight(UI_SizePx(ItemHeight, 1.f))
                UI_Row()
                UI_SemanticWidth(UI_SizePx(150.f, 1.f))
               {
                UI_SemanticWidth(UI_SizePx(30.f, 1.f))
                 UI_FontKind(FontKind_Icon)
                {                 
                 if(M_IconButton(UI_IconKind_Record, S8("Record"), .TextColor = (Voice->IsRecording ? Color_Red : Color_ButtonText)))
                 {
                  PushCommand(App, Command_ToggleRecording);
                 }
                 
                 if(M_IconButton(UI_IconKind_Play, S8("Play"), .TextColor = (Voice->IsPlaying ? Color_Green : Color_ButtonText)))
                 {
                  PushCommand(App, Command_TogglePlaying);
                 }
                }
                
                // Voice select
                {                
                 u64 Idx = (u64)(Voice - App->Voices);
                 f32 NewIdx = 0.f;
                 
                 UI_SemanticWidth(UI_SizePx(200.f, 1.f))
                  NewIdx = M_Slider(S8("VoiceSelect"), S8("Voice"),
                                    Str8Fmt("%2llu", Idx + 1),
                                    (f32)Idx + 1.f, 1.f, (f32)App->VoiceCount, 1.f, false,
                                    .Align = false).Value;
                 NewIdx -= 1.f;
                 
                 if((u64)NewIdx != Idx)
                 {
                  voice *NewVoice = App->Voices + (u64)NewIdx;
                  PushCommand(App, Command_SetPanelVoice)->Voice = NewVoice;
                 }
                }
                
                UI_FillWidth()
                 UI_BackgroundColor(Color_ButtonBackground)
                 UI_AddBox(S8("TopSpacer"), 
                           UI_BoxFlag_DrawBackground|
                           UI_BoxFlag_DrawBorders);
               }
              }
              
              ui_box *SheetBox;
              //- Sheet contents 
              {             
               SheetBox = UI_AddBox(S8("Sheet"), UI_BoxFlag_MouseClickable);
               
               muze_box_data *Data = PushArray(FrameArena, muze_box_data, 1);
               Data->Box = SheetBox;
               Data->Voice = Panel->Voice;
               Data->App = App;
               
               SheetBox->CustomDraw = CustomDrawSheetMusic;
               SheetBox->CustomDrawData = Data;
              }
              
              //- Scrollbar 
              {
               f32 WholeBarWidth = 100.f;
               f32 BPS = (App->BPM/60.f);
               f32 Zoom = (BPS/(f32)App->TimeSig*WholeBarWidth);
               f32 WholeSheetWidth = Panel->Voice->RecordLength*Zoom;
               
#if 0               
               // NOTE(luca): Make it so that we can scroll until there is only one bar of space left empty.
               WholeSheetWidth -= WholeBarWidth;
#endif
               
               f32 Scroll = M_Scrollbar(Axis2_X, WholeSheetWidth, Panel->Scroll.X);
               SheetBox->Scroll.X = Scroll;
               Panel->Scroll.X = Scroll;
              }
             }
            }
            else if(Panel->Kind == PanelKind_Roll)
            {
             muze_box_data *Data = PushArray(FrameArena, muze_box_data, 1);
             Data->Box = Contents;
             Data->Voice = Panel->Voice;
             Data->App = App;
             
             Contents->CustomDraw = CustomDrawPianoRoll;
             Contents->CustomDrawData = Data;
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
           if(UI_IsActive(Child) && !UI_KeyMatch(UI_KeyNull(), Child->Key))
           {
            ChildIsActive = true;
            break;
           }
          }
          
          if(PanelBox->WasClicked || ChildIsActive)
          {
           PanelSelect(App, Panel);
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
          
          // Set cursor shape to resizing one
          {                                    
           if(UI_IsHot(GapBox))
           {
            Input->PlatformCursor = (PlatformCursorShape_ResizeHorizontal +
                                     !!(LayoutAxis == Axis2_Y));
           }
          }
          
          // Gap box (resize border)
          {                                    
           if(UI_IsActive(GapBox))
           {
            // NOTE(luca): Since this will change the position of the elements, they will 
            // want to animate from their FixedPos to their AnimatedPos.  To reset their 
            // AnimatedPos we disable animations while dragging.
            UI_State->IsAnimating = false;
            
            s32 PosOnAxis = Input->Mouse.Pos.e[LayoutAxis];
            
            if(GapBox->WasClicked)
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
   UI_EndLayout(Root->First);
   
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
     // NOTE(luca): Stop all notes on all voices. 
     StopAllPlayingNotes(Memory, App, App->SelectedVoice);
     
     App->In = *Command->Device;
     Memory->PlatformMIDIListen(App->In);
    } break;
    
    case Command_SelectOutputDevice:
    {
     App->Out = *Command->Device;
    } break;
    
    case Command_SelectInstrument:
    {
     if(Command->S32 != App->SelectedVoice->PresetIdx)
     {
      tsf_channel_note_off_all(GlobalTSF, App->SelectedVoice->Channel);
     }
     
     App->SelectedVoice->PresetIdx = Command->S32;
     tsf_channel_set_presetindex(GlobalTSF, App->SelectedVoice->Channel, App->SelectedVoice->PresetIdx);
    } break;
    
    case Command_ToggleMIDIInput:
    {
     App->InputMIDIEnabled ^= 1;
    } break;
    case Command_ToggleMIDIOutput:
    {
     App->OutputMIDIEnabled ^= 1;
    } break;
    case Command_ToggleVirtualKeyboardInput:
    {
     App->InputVirtualKeyboardEnabled ^= 1;
    } break;
    case Command_ToggleSynthOutput:
    {
     App->OutputSynthEnabled ^= 1;
    } break;
   }
   
   //- Voice commands 
   {   
    case Command_AddVoice: 
    { 
     App->SelectedPanel->Voice = VoiceAdd(App);
    } break;
    case Command_DeleteVoice:
    {
     NotImplemented();
     // 1. Find all references to this voice
     // 2. Set them to the previous voice ?
     // 3. Delete the voice
    } break;
    
    case Command_SetPanelVoice:
    {
     App->SelectedPanel->Voice = Command->Voice;
    } break;
    
    case Command_PlayAllVoices:
    {
     for EachIndex(VoiceIdx, App->VoiceCount)
     {
      voice *Voice = App->Voices + (u64)VoiceIdx;
      StopAllPlayingNotes(Memory, App, Voice);
      StopRecording(Voice);
      
      Voice->PlayPos = 0.f;
      Voice->IsPlaying = true;
     }
    } break;
    
    case Command_StopAllVoices:
    {
     for EachIndex(VoiceIdx, App->VoiceCount)
     {
      voice *Voice = App->Voices + (u64)VoiceIdx;
      StopAllPlayingNotes(Memory, App, Voice);
      StopRecording(Voice);
      
      Voice->IsPlaying = false;
     }
    } break;
    
    case Command_Enqueue:
    {
     NotImplemented();
    } break;
    
    case Command_Dequeue:
    {
     NotImplemented();
    } break;
    
   }
   
   //- Note commands 
   {   
    case Command_ToggleRecording:
    {
     StopAllPlayingNotes(Memory, App, App->SelectedVoice);
     
     if(App->SelectedVoice->IsRecording)
     {
      StopRecording(App->SelectedVoice);
     }
     else
     {
      VoiceReset(App->SelectedVoice);
      App->SelectedVoice->RecordStart = GetWallTime();
      App->SelectedVoice->PlayPos = 0.f;
      App->SelectedVoice->IsRecording = true;
     }
     
     App->SelectedVoice->IsPlaying = false;
    } break;
    
    case Command_TogglePlaying:
    {
     StopAllPlayingNotes(Memory, App, App->SelectedVoice);
     StopRecording(App->SelectedVoice);
     
     App->SelectedVoice->IsPlaying ^= 1;
    } break;
    
    case Command_ToggleAutoScroll:
    {
     App->SelectedVoice->AutoScroll ^= 1;
    } break;
    
    case Command_Reset:
    {
     VoiceReset(App->SelectedVoice);
     App->SelectedVoice->IsRecording = false;
     App->SelectedVoice->IsPlaying = false;
     StopAllPlayingNotes(Memory, App, App->SelectedVoice);
    } break;
    
    case Command_Stop:
    {
     StopRecording(App->SelectedVoice);
     StopAllPlayingNotes(Memory, App, App->SelectedVoice);
     App->SelectedVoice->IsPlaying = false;
     App->SelectedVoice->PlayPos = 0.f;
    } break;
    
    case Command_PlayFromNote:
    case Command_PlayFromStart:
    {
     StopRecording(App->SelectedVoice);
     if(App->OutputSynthEnabled)
     {
      tsf_channel_note_off_all(GlobalTSF, App->SelectedVoice->Channel);
     }
     f32 Pos = 0.f;
     
     if(Command->Kind == Command_PlayFromNote)
     {
      rec_note_node *Note = App->SelectedVoice->NoteSel;
      if(Note)
      {
       Pos = Note->Value->Timestamp;
      }
     }
     
     App->SelectedVoice->PlayPos = Pos;
     App->SelectedVoice->IsPlaying = true;
    } break;
    
    case Command_Trim:
    {
     if(!IsNilNote(App->SelectedVoice->FirstNote))
     {
      StopAllPlayingNotes(Memory, App, App->SelectedVoice);
      
      rec_note *LastNote = App->SelectedVoice->LastNote;
      
      f32 LastNoteEnd = (LastNote->Timestamp + LastNote->Duration); 
      //- Find the last note's end.
      {
       for EachRecNote(Note, App->SelectedVoice->FirstNote)
       {
        if(Note->Kind == NoteKind_Note)
        {
         LastNoteEnd = Max(LastNoteEnd, (Note->Timestamp + Note->Duration));
        }
       }
      }
      
      App->SelectedVoice->RecordLength = LastNoteEnd;
      
      rec_note *FirstNote = App->SelectedVoice->FirstNote;
      //- Find the first note played.
      {
       for EachRecNote(Note, FirstNote)
       {
        if(Note->Kind == NoteKind_Note)
        {
         FirstNote = Note;
         break;
        }
       }
      }
      
      f32 StartSilence = FirstNote->Timestamp;
      
      //- Get new voice length 
      {                                    
       App->SelectedVoice->RecordLength -= StartSilence;
       
       f32 BeatsPerSecond = App->BPM/60.f;
       f32 SecondsPerBeat = 1.f/BeatsPerSecond;
       
       f32 BarTime = SecondsPerBeat*(f32)App->TimeSig;
       f32 Pad = CeilF32(App->SelectedVoice->RecordLength/BarTime)*BarTime;
       
       App->SelectedVoice->RecordLength = Pad;
      }
      
      //- Update notes .
      {
       for EachRecNote(Note, App->SelectedVoice->FirstNote)
       {
        
        if(Note->Kind == NoteKind_Pedal)
        {
         // If the pedal note is before the first note, then we forward it up to the firstnote.
         f32 Diff = (FirstNote->Timestamp - Note->Timestamp);
         if(Diff > 0.f)
         {
          Note->Duration -= Diff;
          Note->Timestamp += Diff;
         }
         
         // Clamp the end to the record's length.
         {
          f32 NewTimestamp = Note->Timestamp - StartSilence;
          
          f32 NoteEnd = NewTimestamp + Note->Duration;
          f32 NewEnd = Min(NoteEnd, App->SelectedVoice->RecordLength);
          Note->Duration = (NewEnd - NewTimestamp);
         }
        }
        
        Note->Timestamp -= StartSilence;
        
       }
      }
     }
    } break;
    
    case Command_Round:
    {
     f32 BPS = App->BPM/60.f;
     
     for EachRecNoteNode(Note, App->SelectedVoice->NoteSel)
     {
      f32 NoteLength = Note->Duration*BPS;
      f32 DenomOfFraction = RoundF32(-Log2F32(NoteLength));
      f32 NoteValue = (DenomOfFraction > 0.f ? 
                       (1.f/2.f*DenomOfFraction) : 
                       RoundF32(NoteLength));
      f32 NewDuration = NoteValue/BPS;
      Note->Duration = NewDuration;
     }
    } break;
    
    case Command_Sync:
    {
     f32 BPS = App->BPM/60.f;
     for EachRecNoteNode(Note, App->SelectedVoice->NoteSel)
     {
      f32 NoteLength = Note->Duration*BPS;
      f32 DenomOfFraction = RoundF32(-Log2F32(NoteLength));
      f32 NoteValue = (DenomOfFraction > 0.f ? 
                       (1.f/2.f*DenomOfFraction) : 
                       RoundF32(NoteLength));
      
      f32 Timestamp = BPS*Note->Timestamp;
      
      f32 NewTimestamp = Note->Timestamp;
      NewTimestamp = (NoteValue >= 1.f ? 
                      RoundF32(Timestamp) : 
                      (RoundF32(Timestamp/NoteValue)*NoteValue));
      
      f32 RealNewTimestamp = NewTimestamp/BPS;
      Note->Timestamp = RealNewTimestamp;
      
     }
    } break;
    
    case Command_GuessBPM:
    {
     rec_note_node *Node = App->SelectedVoice->NoteSel;
     
     f32 AveragedDiff = 0.f;
     f32 Diff = 0.f;
     
     if(Node && Node->Next)
     {
      rec_note *Start = Node->Value;
      rec_note *End = Node->Next->Value;
      
      if(Start->Timestamp > End->Timestamp) Swap(Start, End);
      
      Diff = (End->Timestamp - Start->Timestamp);
      
      Node = Node->Next;
      
      // Time that goes in a bar
      f32 SecondsPerBeat = Diff/(f32)App->TimeSig;
      App->BPM = (1.f/SecondsPerBeat)*60.f;
     }
     
    } break;
    
    case Command_SetLength:
    {
     
     f32 NoteSetLength = App->SelectedVoice->NoteSetLength;
     f32 Length = PowF32(2.f, App->SelectedVoice->NoteSetLength);
     
     f32 SecondsPerBeat = 1.f/(App->BPM/60.f);
     f32 Duration = SecondsPerBeat * Length;
     
     for EachRecNoteNode(Note, App->SelectedVoice->NoteSel)
     {
      Note->Duration = Duration;
     }
     
    } break;
    
    case Command_ClearSelection:
    {
     App->SelectedVoice->NoteSel = 0;
    } break;
    
    case Command_DeleteSelection: 
    {
     for(rec_note_node *Node = App->SelectedVoice->NoteSel;
         Node;)
     {
      App->SelectedVoice->NoteCount -= 1;
      
      rec_note *Note = Node->Value;
      
      // Remove the note
      {                                
       if(!IsNilNote(Note->Next))
       {
        Note->Next->Prev = Note->Prev;
       }
       
       if(!IsNilNote(Note->Prev))
       {
        Note->Prev->Next = Note->Next;
       }
       
       if(Note == App->SelectedVoice->FirstNote)
       {
        App->SelectedVoice->FirstNote = Note->Next;
       }
       
       if(Note == App->SelectedVoice->LastNote)
       {
        App->SelectedVoice->LastNote = Note->Prev;
       }
      }
      
      rec_note_node *NextNode = Node->Next;
      
      // Remove from selection
      {                                
       // Remove links
       {
        if(Node->Prev)
        {
         Node->Prev->Next = Node->Next;
        }
        
        if(Node->Next)
        {
         Node->Next->Prev = Node->Prev;
        }
        
        if(Node == App->SelectedVoice->NoteSel)
        {
         App->SelectedVoice->NoteSel = (Node->Next ? Node->Next : Node->Prev);
        }
       }
       
       // Put on free list
       {
        Node->Next = App->FirstFreeNode;
        App->FirstFreeNode = Node;
       }
      }
      
      Node = NextNode;
     }
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
     App->ListerKind = Command->ListerKind;
     App->ListerScroll = 0.f;
     App->Text.Size = 0;
    } break;
   }
   
   //- Panel commands 
   case Command_ClosePanel:
   {
    App->SelectedPanel = ClosePanel(App, Command->Panel);
   } break;
   
   case Command_SplitSelectedPanelY:
   {
    Command->Axis = Axis2_Y;
   }
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
   
   case Command_SetPanelType:
   {
    Command->Panel->Kind = Command->PanelKind;
    MemoryZero(&Command->Panel->Scroll);
   } break;
   
   //- Piece commands
   case Command_PieceToggleMetronome:
   {
    Command->Piece->Metronome ^= 1;
   } break;
   
   case Command_PieceSetInstrument:
   {
    piece *Piece = Command->Piece;
    if(Piece->Preset != Command->S32)
    {
     tsf_channel_note_off_all(GlobalTSF, Piece->Channel);
    }
    
    Piece->Preset = Command->S32;
    tsf_channel_set_presetindex(GlobalTSF, Piece->Channel, Piece->Preset);
   } break;
   
   case Command_PieceTogglePracticing:
   {
    piece *Piece = Command->Piece;
    if(Piece->IsPracticing)
    {
     Piece->IsPracticing = false;
    }
    else
    {
     Piece->IsPracticing = true;
     
     // Conflicts
     PieceStopPlaying(Piece);
     Piece->IsRecording = false;
     
     if(Piece->PracticeEnd == Piece->PracticeStart)
     {
      Piece->PracticeEnd = Min(Piece->RecordLength, 
                               Piece->PracticeStart + Piece->TimeSigNum);
     }
     
     Piece->PracticePos = Piece->PracticeStart;
    }
   } break;
   
   case Command_PieceSelectNote:
   {
    Command->Piece->SelectedNoteIdx = Command->U64;
   } break;
   
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
   
   if(Piece->IsPlaying)
   {
    // Play notes
    {
     for EachIndex(Idx, Piece->NoteCount)
     {
      piece_note *Note = Piece->Notes + Idx;
      
      if(Note->Kind == PieceNoteKind_Pitch)
      {
       if(Piece->Metronome)
       {
        f32 Duration = 0.2f; 
        f32 Start = FloorF32(Piece->PlayPos);
        f32 End = Start + Duration*BPS;
        b32 HasPlayed = PieceWasTimestampPlayed(Piece, Start);
        b32 HasStopped = PieceWasTimestampPlayed(Piece, End);
        
        s32 Pitch = Note_A + 6*12;
        
        if(HasPlayed)
        {
         tsf_channel_note_on(GlobalTSF, App->MetronomeChannel, Pitch, .05f);
        }
        
        if(HasStopped)
        {
         tsf_channel_note_off(GlobalTSF, App->MetronomeChannel, Pitch);
        }
       }
       
       // Piece notes
       {                 
        f32 Start = Note->Start;
        f32 End = Start + Note->Length;
        
        b32 NoteIsPlaying = PieceWasTimestampPlayed(Piece, Start);
        b32 NoteHasEnded = PieceWasTimestampPlayed(Piece, End);
        
        s32 Pitch = Note->Pitch + 1*12;
        
        if(NoteIsPlaying)
        {
         tsf_channel_note_on(GlobalTSF, Piece->Channel, Pitch, .5f);
        }
        
        if(NoteHasEnded)
        {
         tsf_channel_note_off(GlobalTSF, Piece->Channel, Pitch);
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
      if(Piece->IsPracticing) Piece->PlayPos = Piece->PracticeStart;
      
      if(!Piece->IsLooping) Piece->IsPlaying = false;
     }
    }
   }
   
   if(Piece->IsRecording)
   {
    Piece->RecordLength += BPS*Input->dtForFrame;
   }
   
   if(Piece->IsPracticing)
   {
    piece_note *NoteToPlay = NilPieceNote;
    for EachIndex(Idx, Piece->NoteCount)
    {
     piece_note *Note = Piece->Notes + Idx;
     if(Piece->PracticePos <= Note->Start)
     {
      NoteToPlay = Note;
      break;
     }
    }
    
    b32 NoteWasPlayed = false;
    
    for EachIndex(Idx, Input->MIDI.EventCount)
    {
     app_midi_event *Event = Input->MIDI.Events + Idx;
     
     midi_message Message = {Event->Message};
     u8 Status = Message.U8[0];
     u8 Data1  = Message.U8[1];
     u8 Data2 = Message.U8[2];
     u8 Type = Status & 0xF0;
     u8 Channel = Status & 0x0F;
     
     if(0) {}
     else if(Type == MIDIEventType_NoteOn && Data2 > 0)
     {
      u8 Pitch = Data1;
      u8 Velocity = Data2;
      
      if(Pitch == NoteToPlay->Pitch)
      {
       NoteWasPlayed = true;
      }
      else
      {
       Log("Pitch diff: %d\n", Pitch - NoteToPlay->Pitch);
      }
     }
     else if(Type == MIDIEventType_NoteOff ||
             Type == MIDIEventType_NoteOn && Data2 == 0)
     {
      u8 Pitch = Data1;
      u8 Velocity = Data2;
     }
    }
    
    if(!NoteWasPlayed && NoteToPlay->Kind == PieceNoteKind_Silence) 
    {
     NoteWasPlayed = true;
    }
    
    if(NoteWasPlayed)
    {
     // NOTE(luca): Instead of adding up Length as we go which would cumulate precision error
     // we start from the note start which makes it way less.
     Piece->PracticePos = NoteToPlay->Start + NoteToPlay->Length;
     if(Piece->PracticePos >= Piece->PracticeEnd) 
     {
      Piece->PracticePos = Piece->PracticeStart;
     }
    }
   }
   
  }
 }
 
 //- Update voices 
 { 
  f32 MaxRecordingLength = 3600.f;
  
  for EachIndex(Idx, App->VoiceCount)
  {    
   voice *Voice = App->Voices + Idx;
   
   if(Voice->IsPlaying)
   {
    // NOTE(luca): N^M ? But I won't ever have many voices, yes?  If I do, I can just cache the last played note and loop form there as an optimization.
    for EachRecNote(Note, Voice->FirstNote)
    {
     f32 NoteStart = Note->Timestamp;
     f32 NoteEnd = NoteStart + Note->Duration;
     
     // TODO(luca): If a note is shorter than dtForFrame it could skip it.
     
     if(NoteStart <= Voice->PlayPos &&
        NoteStart > (Voice->PlayPos - Input->dtForFrame))
     {
      PlayNote(Memory, App, Voice, Note);
     }
     
     if(NoteEnd <= Voice->PlayPos &&
        NoteEnd > (Voice->PlayPos - Input->dtForFrame))
     {
      rec_note OffNote = *Note;
      OffNote.Velocity = 0;
      PlayNote(Memory, App, Voice, &OffNote);
     }
    }
    
    Voice->PlayPos += Input->dtForFrame;
    if(Voice->PlayPos >= Voice->RecordLength)
    {
     StopAllPlayingNotes(Memory, App, Voice);
     
     Voice->PlayPos = 0.f;
    }
   }
   
   if(Voice->IsRecording)
   {
    Voice->RecordLength = (GetWallTime() - Voice->RecordStart);
    
    if(Voice->RecordLength >= MaxRecordingLength)
    {
     StopRecording(Voice);
    }
   }
  }
 }
 
 // MIDI Processing
 {    
  //- MIDI notes from virtual keyboard 
  if(App->InputVirtualKeyboardEnabled && !App->ListerOpened)
  {
   u64 MaxNotesPerFrame = 128; 
   app_midi_event *Events = PushArray(FrameArena, app_midi_event, MaxNotesPerFrame);
   u64 EventCount = 0;
   
   u8 StartPitch = 60;
   for EachElement(Idx, Input->MIDI.Buttons)
   {
    app_button_state *Key = Input->MIDI.Buttons + Idx;
    
    b32 IsOn = (Key->EndedDown && Key->HalfTransitionCount == 1);
    b32 IsOff = (!Key->EndedDown && Key->HalfTransitionCount & 1);
    Assert(!(IsOn && IsOff));
    
    if(IsOn || IsOff)
    {                            
     Assert(Idx < 127);
     u8 Pitch = StartPitch + (u8)Idx;
     u8 Velocity = 64;
     
     app_midi_event *Event = Events + EventCount;
     EventCount += 1;
     Event->Timestamp = (f32)OS_GetWallClock();
     
     midi_message Message = {0};
     
     if(IsOn)
     {
      Message.U8[0] = MIDIEventType_NoteOn;
      Message.U8[1] = Pitch;
      Message.U8[2] = Velocity;
     }
     else if(IsOff)
     {
      Message.U8[0] = MIDIEventType_NoteOff;
      Message.U8[1] = Pitch;
     }
     
     Event->Message = Message.U32[0];
    }
   }
   
   ProcessMIDINotes(Memory, App, App->SelectedVoice, Events, EventCount);
  }
  
  //- MIDI notes from Input 
  if(App->InputMIDIEnabled)
  {
   ProcessMIDINotes(Memory, App, App->SelectedVoice, Input->MIDI.Events, Input->MIDI.EventCount);
  }
 }
 
 //- Rendering 
 
 // Render rectangles
 if(!Input->SkipRendering)
 {
  RenderClear();
  RenderDrawAllRectangles(&App->Render, BufferDim, &App->FontAtlas);
 }
 
 OS_ProfileAndPrint("R rects");
 
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
