#define Cng_ConfigMatchers \
if(0) {} \
  else if(ConfigMatch(Name, S8("Asan"), Value)) Asan = true; \
  else if(ConfigMatch(Name, S8("Debug"), Value)) Debug = true; \
  else if(ConfigMatch(Name, S8("Clean"), Value)) Clean = true; \
  else if(ConfigMatch(Name, S8("Clang"), Value)) Clang = true; \
  else if(ConfigMatch(Name, S8("GCC"), Value)) GCC = true; \
  else if(ConfigMatch(Name, S8("Slow"), Value)) Slow = true; \
  else if(ConfigMatch(Name, S8("Wine"), Value)) Wine = true; \
  else if(ConfigMatch(Name, S8("Meta"), Value)) Meta = true; \
  else if(ConfigMatch(Name, S8("HaversineProcessor"), Value)) HaversineProcessor = true; \
  else if(ConfigMatch(Name, S8("HaversineGenerator"), Value)) HaversineGenerator = true; \
  else if(ConfigMatch(Name, S8("Sim86"), Value)) Sim86 = true; \
  else if(ConfigMatch(Name, S8("Editor"), Value)) Editor = true; \
  else if(ConfigMatch(Name, S8("Muze"), Value)) Muze = true; \
  else if(ConfigMatch(Name, S8("School"), Value)) School = true;
#define Cng_ConfigBools(CmdLine) \
 \
  b32 Asan = CmdLineHasEntry(CmdLine, S8("Asan")); \
  b32 Debug = CmdLineHasEntry(CmdLine, S8("Debug")); \
  b32 Clean = CmdLineHasEntry(CmdLine, S8("Clean")); \
  b32 Clang = CmdLineHasEntry(CmdLine, S8("Clang")); \
  b32 GCC = CmdLineHasEntry(CmdLine, S8("GCC")); \
  b32 Slow = CmdLineHasEntry(CmdLine, S8("Slow")); \
  b32 Wine = CmdLineHasEntry(CmdLine, S8("Wine")); \
  b32 Meta = CmdLineHasEntry(CmdLine, S8("Meta")); \
  b32 HaversineProcessor = CmdLineHasEntry(CmdLine, S8("HaversineProcessor")); \
  b32 HaversineGenerator = CmdLineHasEntry(CmdLine, S8("HaversineGenerator")); \
  b32 Sim86 = CmdLineHasEntry(CmdLine, S8("Sim86")); \
  b32 Editor = CmdLineHasEntry(CmdLine, S8("Editor")); \
  b32 Muze = CmdLineHasEntry(CmdLine, S8("Muze")); \
  b32 School = CmdLineHasEntry(CmdLine, S8("School"));

  internal field_hash_node *
  GetFieldHashNode(str8 String, u64 Seed, u64 ArraySize, field_hash_node *Array)
 {
  field_hash_node *Result = 0;
  
  u64 Key = U64HashFromSeedStr8(Seed, String);
  u64 Slot = Key%ArraySize;
  Result = Array + Slot;
  
  for(;Result && Result->Key != Key;)
  {
   Result = Result->Next;
  }
  
  return Result;
 }
 
 
  internal table_hash_node *
  GetTableHashNode(str8 String, u64 Seed, u64 ArraySize, table_hash_node *Array)
 {
  table_hash_node *Result = 0;
  
  u64 Key = U64HashFromSeedStr8(Seed, String);
  u64 Slot = Key%ArraySize;
  Result = Array + Slot;
  
  for(;Result && Result->Key != Key;)
  {
   Result = Result->Next;
  }
  
  return Result;
 }
 
 
  internal field_hash_node *
  AddFieldHashNode(arena *Arena, str8 String, u64 Seed, 
                     u64 ArraySize, field_hash_node *Array)
 {
  field_hash_node *Hash = 0;
  
  u64 Key = U64HashFromSeedStr8(Seed, String);
  u64 Slot = (Key%ArraySize);
  
  Hash = Array + Slot;
  
  for(;Hash->Key != 0;)
  {
   if(Hash->Key == Key)
   {
    // NOTE(luca): Collision.
    InvalidPath();
   }
   else
   {
    while(Hash->Next && Hash->Key != 0) Hash = Hash->Next;
    Hash->Next = PushArrayZero(Arena, field_hash_node, 1);
    Hash = Hash->Next;
   }
  }
  
  Hash->Key = Key;
  
  return Hash;
 }
 
  internal table_hash_node *
  AddTableHashNode(arena *Arena, str8 String, u64 Seed, 
                     u64 ArraySize, table_hash_node *Array)
 {
  table_hash_node *Hash = 0;
  
  u64 Key = U64HashFromSeedStr8(Seed, String);
  u64 Slot = (Key%ArraySize);
  
  Hash = Array + Slot;
  
  for(;Hash->Key != 0;)
  {
   if(Hash->Key == Key)
   {
    // NOTE(luca): Collision.
    InvalidPath();
   }
   else
   {
    while(Hash->Next && Hash->Key != 0) Hash = Hash->Next;
    Hash->Next = PushArrayZero(Arena, table_hash_node, 1);
    Hash = Hash->Next;
   }
  }
  
  Hash->Key = Key;
  
  return Hash;
 }
 
