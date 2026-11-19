//-
// Serializing code
//-

//~ API
internal ms_serializer 
MS_SerializeBegin(arena *Arena, ms_mode Mode, str8 Buffer)
{
 ms_serializer Serializer = {0};
 
 // Init
 { 
  Serializer.Mode = Mode;
  Serializer.Version = MSV_Latest;
  Serializer.Buffer = Buffer;
  
  MS_Ser(&Serializer, &Serializer.Version);
 }
 
 if(Serializer.Version > MSV_Latest)
 {
  ErrorLog("Cannot read file from the future.\n");
  Serializer.At = Serializer.Buffer.Size;
 }
 else if(Serializer.Version >= MSV_MagicNumber)
 {
  str8 Magic = S8DupCString(Arena, MS_Magic);
  
  MS_Struct(&Serializer, Magic.Data, Magic.Size);
  
  if(!S8Match(Magic, S8(MS_Magic), 0))
  {
   ErrorLog("Magic number mismatch.\n");
   Serializer.At = Serializer.Buffer.Size;
  }
 }
 
 return Serializer;
}

internal ms_serializer
MS_SerializeFromFileBegin(arena *Arena, ms_mode Mode, str8 FilePath)
{
 ms_serializer Serializer = {0};
 
 str8 FileBuffer = {0};
 if(0) {}
 else if(Mode == MS_Read)
 {
  char *Path = PathFromExe(FilePath);
  FileBuffer = OS_ReadEntireFileIntoMemory(Path);
 }
 else if(Mode == MS_Write)
 {
  FileBuffer = PushS8(Arena, KB(64));
 }
 
 Serializer = MS_SerializeBegin(Arena, Mode, FileBuffer);
 
 return Serializer;
}

internal void
MS_SerializeFromFileEnd(ms_serializer *Serializer, arena *Arena, str8 FilePath)
{
 if(0) {}
 else if(Serializer->Mode == MS_Read)
 {
  OS_FreeFileMemory(Serializer->Buffer);
 }
 else if(Serializer->Mode == MS_Write)
 {
  Serializer->Buffer.Size = Serializer->At;
  
  str8 Out = {0};
  Out.Data = Serializer->Buffer.Data;
  Out.Size = Serializer->At;
  
  char *Path = PathFromExe(FilePath);
  OS_WriteEntireFile(Path, Out); 
  
  ArenaFree(Arena, Serializer->Buffer.Size);
 }
}

internal void
MS_Struct(ms_serializer *Serializer, void *Value, u64 Size)
{
 u8 *Dest = Serializer->Buffer.Data + Serializer->At;
 
 if(Serializer->At + Size <= Serializer->Buffer.Size)
 { 
  if(0) {}
  else if(Serializer->Mode == MS_Read) MemoryCopy(Value, Dest, Size);
  else if(Serializer->Mode == MS_Write) MemoryCopy(Dest, Value, Size);
  Serializer->At += Size;
 }
 else
 {
  ErrorLog("Buffer overflow while deserializing.\n");
 }
}

//~ Types
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
 if(MS_Before(MSV_PieceNoteKindAddAndZerothNote))
 {  
  Note->Kind += 1;
 }
 
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
  if(0) {}
  else if(Serializer->Mode == MS_Read)
  {
   for EachIndex(Idx, Piece->NoteCount)
   {
    note *Note = PieceNoteAlloc(Piece);
    if(!IsNilNote(Note))
    {
     MS_Note(Serializer, Note); 
     AppendNote(Note, Piece->FirstNote, Piece->LastNote);
     Count += 1;
    }
   }
   
  }
  else if(Serializer->Mode == MS_Write)
  {
   for EachNote(Note, Piece->FirstNote)
   {
    MS_Note(Serializer, Note); 
    Count += 1;
   }
  }
  
  Assert(Count == Piece->NoteCount);
 }
 
 MS_Add(MSV_Initial, &Piece->BarWrapping);
 
 if(MS_Before(MSV_PieceMultiStaff))
 {
  Piece->BarWrapping ^= 1;
 }
}

internal void 
MS_Config(ms_serializer *Serializer, app_state *App)
{
 MS_Ser(Serializer, &App->In);
 MS_Ser(Serializer, &App->Out);
}