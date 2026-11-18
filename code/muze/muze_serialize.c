internal void
MS_Struct(ms_serializer *Serializer, void *Value, u64 Size)
{
 u8 *Dest = Serializer->Buffer.Data + Serializer->At;
 
 if(Serializer->At < Serializer->Buffer.Size)
 { 
  if(0) {}
  else if(Serializer->Mode & MS_Reading) MemoryCopy(Value, Dest, Size);
  else if(Serializer->Mode & MS_Writing) MemoryCopy(Dest, Value, Size);
  
  Assert(Serializer->At + Size <= Serializer->Buffer.Size);
  Serializer->At += Size;
 }
 else
 {
  ErrorLog("Buffer overflow while deserializing.\n");
 }
}

internal void
MS_Staff(ms_serializer *Serializer, staff *Staff)
{
 MS_Add(MSV_PieceMultiStaff, &Staff->Key);
 MS_Add(MSV_PieceMultiStaff, &Staff->Minor);
 MS_Add(MSV_PieceMultiStaff, &Staff->Octave);
 MS_Add(MSV_StaffClefKind, &Staff->ClefKind);
}

internal void
MS_Note(ms_serializer *Serializer, note *Note)
{
 MS_Add(MSV_Initial, &Note->Kind);
 MS_Add(MSV_Initial, &Note->Start);
 MS_Add(MSV_Initial, &Note->Length);
 
 if(MS_Before(MSV_PieceNoteVelocity))
 {
  s32 Pitch = 0;
  MS_Ser(Serializer, &Pitch);
  Note->Pitch = (u8)Pitch;
 }
 else
 {
  MS_Ser(Serializer, &Note->Pitch);
 }
 
 MS_Add(MSV_PieceNoteHidden, &Note->Hidden);
 MS_Add(MSV_PieceMultiStaff, &Note->StaffIdx);
 MS_Add(MSV_PieceNoteVelocity, &Note->Velocity);
 
 if(MS_Before(MSV_PieceNoteVelocity))
 {
  Note->Velocity = 64;
 }
}

internal void
MS_Piece(ms_serializer *Serializer, piece *Piece)
{
 MS_Add(MSV_Initial, &Piece->BPM);
 MS_Add(MSV_Initial, &Piece->TimeSigNum);
 MS_Add(MSV_Initial, &Piece->TimeSigDen);
 
 if(MS_Before(MSV_PieceMultiStaff))
 {
  Piece->StaffCount = 1;
  MS_Ser(Serializer, &Piece->Staffs[0].Key); 
  MS_Ser(Serializer, &Piece->Staffs[0].Minor); 
  Piece->Staffs[0].Minor ^= 1; // Used to be "Major"
  Piece->Staffs[0].Octave = 4;
 }
 else
 {
  MS_Ser(Serializer, &Piece->StaffCount);
  for EachIndex(Idx, Piece->StaffCount)
  {
   MS_Staff(Serializer, (Piece->Staffs + Idx));
  }
 }
 
 MS_Add(MSV_Initial, &Piece->RecordLength);
 MS_Add(MSV_Initial, &Piece->NoteCount);
 
 // Notes
 { 
  u64 Count = 0;
  for EachIndex(Idx, Piece->NoteCount)
  {
   note *Note = PieceNoteAlloc(Piece);
   MS_Note(Serializer, Note); 
   DLLAppendNil(Note, Piece->FirstNote, Piece->LastNote, NilNote);
   Count += 1;
  }
  
  Assert(Count == Piece->NoteCount);
 }
 
 // Increase for PieceNoteKind_None
 if(MS_Before(MSV_PieceNoteKindAddAndZerothNote))
 {  
  for EachNote(Note, Piece->FirstNote)
  {
   Note->Kind += 1;
  }
 }
 
 MS_Add(MSV_Initial, &Piece->BarWrapping);
 
 if(MS_Before(MSV_PieceMultiStaff))
 {
  Piece->BarWrapping ^= 1;
 }
}

internal void
MS_SerializePiece(arena *Arena, ms_mode Mode, str8 Path, piece *Piece)
{
 ms_serializer Serializer = {0};
 Serializer.Mode = Mode;
 
 char *FilePath = PathFromExe(Path);
 
 if(Serializer.Mode == MS_Reading)
 {
  Serializer.Buffer = OS_ReadEntireFileIntoMemory(FilePath);
  if(Serializer.Buffer.Size)
  {               
   MS_Ser(&Serializer, &Serializer.Version);
   if(Serializer.Version <= MSV_Latest)
   {
    MS_Piece(&Serializer, Piece);
   }
   else
   {
    DebugBreak();
    ErrorLog("Cannot read file from the future.\n");
   }
   
   OS_FreeFileMemory(Serializer.Buffer);
  }
 }
 else if(Serializer.Mode == MS_Writing)
 {
  S8OverPush(Arena, Serializer.Buffer, KB(64), Serializer.At)
  {   
   Serializer.Version = MSV_Latest;
   
   MS_Ser(&Serializer, &Serializer.Version);
   MS_Piece(&Serializer, Piece);
   
   str8 Out = {.Data = Serializer.Buffer.Data, .Size = Serializer.At};
   OS_WriteEntireFile(FilePath, Out);
  }
 }
}

internal void 
MS_Config(ms_serializer *Serializer, app_state *App)
{
 MS_Ser(Serializer, &App->In);
 MS_Ser(Serializer, &App->Out);
}

internal b32
MS_SerializeConfig(arena *Arena, b32 Read, str8 Path, app_state *App)
{
 b32 Result = !Read;
 ms_serializer Serializer = {0};
 
 //- Begin 
 
 char *FilePath = PathFromExe(Path);
 
 if(0) {}
 else if(Serializer.Mode == MS_Reading)
 {
  Serializer.Buffer = OS_ReadEntireFileIntoMemory(FilePath);
  if(Serializer.Buffer.Size) Result = true;
 }
 else if(Serializer.Mode == MS_Writing)
 {
  Serializer.Buffer = PushS8(Arena, KB(64));
 }
 
 Serializer.Version = MSV_Latest;
 MS_Ser(&Serializer, &Serializer.Version);
 
 if(Serializer.Version > MSV_Latest)
 {
  ErrorLog("Cannot read file from the future.\n");
  Serializer.At = Serializer.Buffer.Size;
 }
 
 //- Data 
 
 MS_Config(&Serializer, App);
 
 //- End 
 
 if(0) {}
 else if(Serializer.Mode == MS_Reading)
 {
  OS_FreeFileMemory(Serializer.Buffer);
 }
 else if(Serializer.Mode == MS_Writing)
 {
  str8 Out = {.Data = Serializer.Buffer.Data, .Size = Serializer.At};
  OS_WriteEntireFile(FilePath, Out);
 }
 
 //- 
 
 return Result;
}