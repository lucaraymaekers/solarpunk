internal void
MS_Struct(ms_serializer *Serializer, void *Value, u64 Size)
{
 u8 *Dest = Serializer->Buffer.Data + Serializer->At;
 
 if(Serializer->IsReading) MemoryCopy(Value, Dest, Size);
 else                      MemoryCopy(Dest, Value, Size);
 
 Assert(Serializer->At + Size <= Serializer->Buffer.Size);
 Serializer->At += Size;
}

internal void
MS_Piece(ms_serializer *Serializer, piece *Piece)
{
 MS_Add(MSV_Initial, &Piece->BPM);
 MS_Add(MSV_Initial, &Piece->TimeSigNum);
 MS_Add(MSV_Initial, &Piece->TimeSigDen);
 MS_Add(MSV_Initial, &Piece->Key);
 MS_Add(MSV_Initial, &Piece->Major);
 MS_Add(MSV_Initial, &Piece->RecordLength);
 MS_Add(MSV_Initial, &Piece->NoteCount);
 
 if(Serializer->Version >= MSV_Initial)
 {
  MS_Struct(Serializer, Piece->Notes, sizeof(piece_note)*Piece->NoteCount);
 }
 
 if(Serializer->Version < MSV_Practice)
 {
  if(Piece->NoteCount == ArrayCount(Piece->Notes))
  {
   // Bummer, we have to crop.
   Log("Error: Since piece has max note amount, last note cropped.\n");
  }
  MemoryCopy(Piece->Notes + 1, Piece->Notes, sizeof(piece_note)*(Piece->NoteCount - 1));
  Piece->Notes[0] = Nil(piece_note);
  
  for(u64 Idx = 1; Idx < Piece->NoteCount; Idx += 1)
  {
   Piece->Notes[Idx].Kind += 1;
  }
 }
 
 MS_Add(MSV_Initial, &Piece->BarWrapping);
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
