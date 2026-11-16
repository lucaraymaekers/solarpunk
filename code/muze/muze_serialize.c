internal void
MS_Struct(ms_serializer *Serializer, void *Value, u64 Size)
{
 u8 *Dest = Serializer->Buffer.Data + Serializer->At;
 
 if(Serializer->At < Serializer->Buffer.Size)
 { 
  if(Serializer->IsReading) MemoryCopy(Value, Dest, Size);
  else                      MemoryCopy(Dest, Value, Size);
  
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
MS_PieceNote(ms_serializer *Serializer, piece_note *Note)
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
 
 if(MS_After(MSV_Initial))
 {
  for EachIndex(Idx, Piece->NoteCount)
  {
   MS_PieceNote(Serializer, (Piece->Notes + Idx)); 
  }
 }
 
 if(MS_Before(MSV_PieceNoteKindAddAndZerothNote))
 {
  if(Piece->NoteCount == ArrayCount(Piece->Notes))
  {
   // Bummer, we have to crop.
   Log("Error: Since piece has max note amount, last note cropped.\n");
  }
  
  // Move notes and make nil note
  {
   if(Piece->NoteCount)
   {   
    MemoryCopy(Piece->Notes + 1, Piece->Notes, sizeof(piece_note)*(Piece->NoteCount - 1));
   }
   Piece->Notes[0] = Nil(piece_note);
  }
  
  // Increase for PieceNoteKind_None
  {  
   for(u64 Idx = 1; Idx < Piece->NoteCount; Idx += 1)
   {
    Piece->Notes[Idx].Kind += 1;
   }
  }
 }
 
 MS_Add(MSV_Initial, &Piece->BarWrapping);
 
 if(MS_Before(MSV_PieceMultiStaff))
 {
  Piece->BarWrapping ^= 1;
 }
}

internal void
M_SerializePiece(arena *Arena, b32 Read, str8 Path, piece *Piece)
{
 ms_serializer Serializer = {0};
 Serializer.IsReading = Read;
 
 char *FilePath = PathFromExe(Path);
 
 if(Serializer.IsReading)
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
 else
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
 
 Serializer.IsReading = Read;
 
 char *FilePath = PathFromExe(Path);
 
 if(Serializer.IsReading)
 {
  Serializer.Buffer = OS_ReadEntireFileIntoMemory(FilePath);
  if(Serializer.Buffer.Size) Result = true;
 }
 else
 {
  Serializer.Buffer = PushS8(Arena, KB(64));
 }
 
 Serializer.Version = MSV_Latest;
 MS_Ser(&Serializer, &Serializer.Version);
 
 if(Serializer.Version <= MSV_Latest)
 {
  MS_Config(&Serializer, App);
 }
 else
 {
  DebugBreak();
  ErrorLog("Cannot read file from the future.\n");
 }
 
 if(Serializer.IsReading)
 {
  OS_FreeFileMemory(Serializer.Buffer);
 }
 else
 {
  str8 Out = {.Data = Serializer.Buffer.Data, .Size = Serializer.At};
  OS_WriteEntireFile(FilePath, Out);
 }
 
 return Result;
}