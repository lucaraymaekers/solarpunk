/* date = October 5th 2026 0:38 pm */

#ifndef MUZE_SERIALIZE_H
#define MUZE_SERIALIZE_H

//~ Serialization
enum ms_mode
{
 MS_Read,
 MS_Write,
 MS_GenText,
};
typedef enum ms_mode ms_mode;

typedef struct ms_serializer ms_serializer;
struct ms_serializer
{
 ms_mode Mode;
 
 u64 Version;
 
 u64 At;
 str8 Buffer;
};

enum ms_version
{
 MSV_Initial,
 MSV_PieceNoteKindAddAndZerothNote,
 MSV_PieceNoteHidden,
 MSV_PieceMultiStaff,
 MSV_StaffClefKind,
 MSV_PieceNoteVelocity,
 MSV_PieceDLL,
 MSV_MagicNumber,
 MSV_Count,
};
typedef enum ms_serializer_version ms_serializer_version;
#define MSV_Latest (MSV_Count - 1)

internal void MS_Struct(ms_serializer *Serializer, void *Value, u64 Size);

#define MS_Ser(Serializer, Value) \
MS_Struct(Serializer, Value, (sizeof(*Value)))

#define MS_After(_Version) Serializer->Version >= _Version
#define MS_Before(_Version) Serializer->Version < _Version

#define MS_Add(VersionAdded, Value) \
if(MS_After(VersionAdded)) MS_Ser(Serializer, Value)

#define MS_Rem(VersionAdded, VersionRemoved, Value) \
do { \
TypeOf(*Value) Dummy = {0}; \
if(MS_After(VersionAdded) && \
MS_Before(VersionRemoved)) \
{ \
MS_Ser(Serializer, &Dummy); \
} \
} while(0)

#define MS_SerializeFromFile(Serializer, Arena, Mode, FilePath) \
for(ms_serializer Serializer = MS_SerializeFromFileBegin(Arena, Mode, FilePath), *_Counter = 0; \
_Counter == 0; \
MS_SerializeFromFileEnd(&Serializer, Arena, FilePath), _Counter += 1)

#define MS_Magic "rlMuze"

#endif //MUZE_SERIALIZE_H
