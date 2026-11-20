
internal void
PieceClear(piece *Piece)
{
 ArenaClear(Piece->NotesArena);
 ArenaClear(Piece->SelArena);
 
 if(Piece->IsPlaying)
 {
  Log("WARNING: Piece should not be playing when cleared.\n");
 }
 
 // Defaults
 Piece->FilePath = nil(str8);
 
 Piece->IsPlaying = false;
 Piece->IsRecording = false;
 Piece->RecordLength = 0.f;
 Piece->PlayPos = 0.f;
 Piece->PracticePos = 0.f;
 Piece->PracticeEnd = 0.f;
 Piece->PracticeStart = 0.f;
 
 Piece->BPM = 60;
 Piece->TimeSigNum = 2;
 Piece->TimeSigDen = 4;
 
 Piece->NoteCount = 0;
 Piece->SelCount = 0;
 Piece->FreeNote = NilNote;
 Piece->FirstNote = NilNote;
 Piece->LastNote = NilNote;
 Piece->FirstSel = 0;
 Piece->LastSel = 0;
 Piece->FirstPracticeNote = NilNote;
 Piece->LastPracticeNote = NilNote;
 
 Piece->StaffCount = 1;
 Piece->Staffs[0].Octave = 5;
 Piece->SetNoteLength = 1.f;
 
 Piece->BarPadding = 16.f;
 Piece->BeatWidth = 64.f;
 Piece->NoteSize = 11.f;
 Piece->StaffLineWidth = 1.f;
 Piece->TailHeight = 24.f;
 
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
  if(!IsNilPiece(App->FreePiece))
  {
   Result = App->FreePiece;
   App->FreePiece = Result->Next;
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
   
   Result->NotesArena = PushArena(App->PieceArena, KB(1)*sizeof(note), false);
   Result->SelArena = PushArena(App->PieceArena, KB(1)*sizeof(note_node), false);
   Result->UndoArena = PushArena(App->PieceArena, MB(1), false);
  }
  
  // Defaults
  if(!IsNilPiece(Result))
  {
   PieceClear(Result);
   
   Result->Prev = NilPiece;
   Result->Next = NilPiece;
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

internal note *
PieceNoteAlloc(piece *Piece)
{
 note *Result = NilNote;
 
 if(!IsNilNote(Piece->FreeNote))
 {
  Assert(Piece->FreeNote != 0);
  Result = Piece->FreeNote;
  Piece->FreeNote = Piece->FreeNote->Next;
 }
 else
 {
  Result = PushArray(Piece->NotesArena, note, 1);
  if(Result == 0)
  {
   Result = NilNote;
  }
 }
 
 // Zero
 if(!IsNilNote(Result))
 {
  *Result = (note){NilNote, NilNote};
 }
 
 return Result;
}

internal void
PieceNoteRelease(piece *Piece, note *Note)
{
 Note->Next = Piece->FreeNote;
 Piece->FreeNote = Note;
}

internal void
MIDIOutputMutePiecePlayingNotes(app_state *App, piece *Piece)
{
 if(App->OutputMIDIEnabled)
 {
  for EachNote(Note, Piece->FirstNote)
  {
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

#if 0
# include "../data/muze/muze_pieces.c"
#endif