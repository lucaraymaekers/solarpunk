/* date = October 5th 2026 0:38 pm */

#ifndef MUZE_SERIALIZE_H
#define MUZE_SERIALIZE_H

//~ Serialization
typedef struct ms_serializer ms_serializer;
struct ms_serializer
{
 u64 At;
 str8 Buffer;
 
 b32 IsReading;
 u64 Version;
};

enum ms_version
{
 MSV_Initial,
 MSV_PieceNoteKindAddAndZerothNote,
 MSV_PieceNoteHidden,
 MSV_PieceMultiStaff,
 MSV_StaffClefKind,
 MSV_PieceNoteVelocity,
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

#endif //MUZE_SERIALIZE_H
