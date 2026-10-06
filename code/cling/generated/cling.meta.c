#define Cng_ConfigMatchers(Name, Value) \
if(0) {} \
  else if(ConfigMatch(Name, S8("Asan"), Value)) Asan = true; \
  else if(ConfigMatch(Name, S8("Release"), Value)) Release = true; \
  else if(ConfigMatch(Name, S8("Slow"), Value)) Slow = true; \
  else if(ConfigMatch(Name, S8("Clang"), Value)) Clang = true; \
  else if(ConfigMatch(Name, S8("GCC"), Value)) GCC = true; \
  else if(ConfigMatch(Name, S8("Wine"), Value)) Wine = true;
#define Cng_ConfigBools(CmdLine, Help) \
 \
  b32 Asan = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("Asan"), S8("Enable adress sanitizer")); \
  b32 Release = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("Release"), S8("Enable optimizations and remove debug tooling")); \
  b32 Slow = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("Slow"), S8("When building applications, rebuild all libraries every time")); \
  b32 Clang = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("Clang"), S8("Use Clang (Linux only)")); \
  b32 GCC = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("GCC"), S8("Use GCC (Linux only)")); \
  b32 Wine = CmdLineHasEntryAndAddHelp(CmdLine, Help, S8("Wine"), S8("Build with wine (Linux only)"));

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
 
