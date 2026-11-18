global_variable voice *NilVoice;
global_variable rec_note *NilNote;

typedef struct muze_box_data muze_box_data;
struct muze_box_data
{
 ui_box *Box;
 app_state *App;
 voice *Voice;
};

typedef enum note_kind note_kind;
enum note_kind
{
 NoteKind_Note,
 NoteKind_Pedal
};

typedef struct rec_note rec_note;
struct rec_note
{
 rec_note *Next;
 rec_note *Prev;
 
 u8 Pitch;
 u8 Velocity;
 u8 Controller;
 f32 Timestamp;
 f32 Duration;
 
 note_kind Kind;
};
raddbg_type_view(note, rows(Note, (s32)Pitch, no_char((note_pitch)(Pitch%Note_Count)), no_char(Velocity), omit($, Pitch, Velocity)));

typedef struct rec_note_node rec_note_node;
struct rec_note_node
{
 rec_note *Value;
 rec_note_node *Next;
 rec_note_node *Prev;
};
#define EachRecNoteNode(Index, First) \
(rec_note_node *_node = (First); _node; _node = _node->Next) \
for (rec_note *Index = _node->Value, *_note = _node->Value; _note; _note = 0)
#define EachRecNote(Index, First) (rec_note *Index = First; !IsNilNote(Index); Note = Note->Next)
#define EachRecNoteBack(Index, Last) (rec_note *Index = Last; !IsNilNote(Index); Note = Note->Prev)

typedef struct voice voice;
struct voice
{
 rec_note *FirstNote;
 rec_note *LastNote;
 s64 NoteCount;
 
 b32 IsRecording;
 b32 IsPlaying;
 b32 AutoScroll;
 
 f32 RecordStart;
 f32 RecordLength;
 f32 PlayPos;
 
 u8 MaxPitch;
 u8 MinPitch;
 
 f32 NoteSetLength;
 
 rec_note_node *NoteSel;
 
 int Channel;
 int PresetIdx;
 f32 Volume;
 f32 MIDISoftness;
 
 f32 SheetScrollX;
 
 arena *Arena;
};

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
VoiceNoteAdd(voice *Voice)
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
VoicePlayNote(app_memory *Memory, app_state *App, voice *Voice, rec_note *Note)
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
VoiceIsNotePlaying(rec_note *Note, voice *Voice)
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
VoiceStopAllPlayingNotes(app_memory *Memory, app_state *App, voice *Voice)
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

internal void
VoiceUIs()
{
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
 
 else if(Kind == PanelKind_Sheet)
 {     
  voice *Voice = App->SelectedVoice;
  if(Command->S32 != Voice->PresetIdx)
  {
   tsf_channel_note_off_all(GlobalTSF, Voice->Channel);
  }
  
  Voice->PresetIdx = Command->S32;
  tsf_channel_set_presetindex(GlobalTSF, Voice->Channel, Voice->PresetIdx);
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
}
//~ Muze - Piece
internal note *
PieceCodeNoteAdd(piece *Piece, 
                 note_kind Kind, u8 Pitch, f32 Length)
{
 note *Note = PieceNoteAlloc;
 
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
