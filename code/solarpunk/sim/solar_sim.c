//-
// SolarSim: headless console tool for the food module (solarpunk_food.c).
//   SolarSim test                       runs the food tests, exits with the failure count
//   SolarSim sim [--seed 7] [--days 10] prints waste and shortage with and without sharing
// Sharing here is a test-only stand-in for the routing engine (FoodTake -> FoodGive to a
// neighbour). It shows what the router gains by moving soonest-expiring food first.
//-

//~ Includes
#define BASE_CONSOLE_APPLICATION 1
#define BASE_FORCE_THREADS_COUNT 1
#include "base/base.h"
#include "base/base.c"

#include <stdlib.h>

#include "rl/rl_random.h"
#include "solarpunk/generated/solarpunk.meta.c"
#include "solarpunk/solarpunk_food.h"
#include "solarpunk/solarpunk_food.c"

//~ Test harness
global_variable s32 GlobalCheckCount;
global_variable s32 GlobalFailCount;

#define Check(Cond, Message) CheckAt((Cond) ? 1 : 0, (Message), __FILE__, __LINE__)

internal void
CheckAt(b32 Passed, char *Message, char *File, int Line)
{
 GlobalCheckCount += 1;
 if(!Passed)
 {
  GlobalFailCount += 1;
  Log("  FAIL %s(%d): %s\n", File, Line, Message);
 }
}

internal food_state *
SimAllocFood(void)
{
 // NOTE: OS_Allocate returns zeroed pages, the same as a fresh app_state.
 food_state *Result = (food_state *)OS_Allocate(sizeof(food_state));
 Assert(Result);
 return Result;
}

internal b32
SimAllConserved(food_state *Food, u32 IslandCount)
{
 b32 Result = true;
 for(u32 I = 0; I < IslandCount; I += 1)
 {
  for(u32 K = 0; K < ProductKind_Count; K += 1)
  {
   Result = Result && FoodIsConserved(Food, I, (product_kind)K);
  }
 }
 return Result;
}

internal b32
SimLotsSorted(food_state *Food, u32 Island, product_kind Kind)
{
 b32 Result = true;
 food_stock *Stock = &Food->Stock[Island][Kind];
 for(u32 L = 1; L < Stock->LotCount; L += 1)
 {
  Result = Result && Stock->Lots[L - 1].ExpiresRound <= Stock->Lots[L].ExpiresRound;
 }
 for(u32 L = 0; L < Stock->LotCount; L += 1)
 {
  Result = Result && Stock->Lots[L].Qty > 0;
 }
 return Result;
}

//~ Tests
internal void
TestCatalog(void)
{
 Check(FoodIsPerishable(ProductKind_Fish) && FoodIsPerishable(ProductKind_Algae) &&
       FoodIsPerishable(ProductKind_FreshWater) && FoodIsPerishable(ProductKind_Seeds), "food + seeds spoil");
 Check(!FoodIsPerishable(ProductKind_Copper) && !FoodIsPerishable(ProductKind_Timber) &&
       !FoodIsPerishable(ProductKind_SolarPanels) && !FoodIsPerishable(ProductKind_Batteries) &&
       !FoodIsPerishable(ProductKind_None), "non-food never spoils");
 Check(FoodShelfLifeRounds(ProductKind_Fish) == (u64)(FoodShelfLifeDays[ProductKind_Fish]*Food_RoundsPerDay),
       "shelf life in rounds = days * rounds per day");

 food_state *Food = SimAllocFood();
 FoodIslandRound(Food, 0, ProductKind_Copper, 100.f, 50.f, 1);
 Check(FoodStockQty(Food, 0, ProductKind_Copper) == 0 && Food->Counts[0][ProductKind_Copper].Produced == 0,
       "non-food is not tracked by the food module");
}

// Fish produced and never eaten goes bad exactly after its shelf life.
internal void
TestSpoil(void)
{
 food_state *Food = SimAllocFood();
 u64 Shelf = FoodShelfLifeRounds(ProductKind_Fish);
 f32 PerDay = 10.f*(f32)Food_RoundsPerDay;      // 10 units every round

 FoodIslandRound(Food, 0, ProductKind_Fish, PerDay, 0.f, 1);
 Check(FoodStockQty(Food, 0, ProductKind_Fish) == 10, "a round of production adds a lot");
 Check(Food->Stock[0][ProductKind_Fish].Lots[0].ExpiresRound == 1 + Shelf, "lot expires after the shelf life");

 b32 Sorted = true;
 for(u64 Round = 2; Round <= Shelf; Round += 1)
 {
  FoodIslandRound(Food, 0, ProductKind_Fish, PerDay, 0.f, Round);
  Sorted = Sorted && SimLotsSorted(Food, 0, ProductKind_Fish);
 }
 Check(Sorted, "lots stay sorted by expiry");
 Check(Food->Counts[0][ProductKind_Fish].Wasted == 0, "nothing spoils before the shelf life");
 FoodIslandRound(Food, 0, ProductKind_Fish, PerDay, 0.f, Shelf + 1);
 Check(Food->Counts[0][ProductKind_Fish].Wasted >= 10, "the first lot spoils on its expiry round");
 Check(Food->Stock[0][ProductKind_Fish].Lots[0].ExpiresRound > Shelf + 1, "no bad food left in stock");
 Check(SimAllConserved(Food, 1), "conserved");
}

// Eating uses the soonest-to-expire food first; missing food is a shortage.
internal void
TestEat(void)
{
 food_state *Food = SimAllocFood();
 food_lot Lots[2];
 Lots[0].Qty = 5; Lots[0].ExpiresRound = 50;
 Lots[1].Qty = 5; Lots[1].ExpiresRound = 20;
 FoodGive(Food, 0, ProductKind_Algae, Lots, 2, 1);
 Check(Food->Stock[0][ProductKind_Algae].Lots[0].ExpiresRound == 20, "received lots are inserted by expiry");

 FoodIslandRound(Food, 0, ProductKind_Algae, 0.f, 3.f*(f32)Food_RoundsPerDay, 2);   // eat 3
 Check(Food->Stock[0][ProductKind_Algae].Lots[0].ExpiresRound == 20 && Food->Stock[0][ProductKind_Algae].Lots[0].Qty == 2,
       "eats the soonest-expiring lot first");
 FoodIslandRound(Food, 0, ProductKind_Algae, 0.f, 9.f*(f32)Food_RoundsPerDay, 3);   // wants 9, has 7
 Check(FoodStockQty(Food, 0, ProductKind_Algae) == 0, "eats everything there is");
 Check(Food->Counts[0][ProductKind_Algae].Shortage == 2, "the rest is a shortage");

 // Fractions carry: 1.5 units/round -> 3 units every 2 rounds.
 food_state *Frac = SimAllocFood();
 for(u64 Round = 1; Round <= 10; Round += 1)
 {
  FoodIslandRound(Frac, 0, ProductKind_Seeds, 1.5f*(f32)Food_RoundsPerDay, 0.f, Round);
 }
 Check(Frac->Counts[0][ProductKind_Seeds].Produced == 15, "fractional production carries over");
 Check(SimAllConserved(Food, 1) && SimAllConserved(Frac, 1), "conserved");
}

// The routing engine API keeps expiry dates on the move.
internal void
TestTakeGive(void)
{
 food_state *Food = SimAllocFood();
 for(u64 Round = 1; Round <= 8; Round += 1)
 {
  FoodIslandRound(Food, 0, ProductKind_Fish, 4.f*(f32)Food_RoundsPerDay, 0.f, Round);
 }
 u64 Soonest = Food->Stock[0][ProductKind_Fish].Lots[0].ExpiresRound;

 food_lot Out[8];
 u32 OutCount = 0;
 u32 Took = FoodTake(Food, 0, ProductKind_Fish, 6, Out, ArrayCount(Out), &OutCount);
 Check(Took == 6 && OutCount >= 1 && Out[0].ExpiresRound == Soonest, "FoodTake gives the soonest-expiring food first");
 Check(Food->Counts[0][ProductKind_Fish].Sent == 6, "Sent counted");

 FoodGive(Food, 1, ProductKind_Fish, Out, OutCount, 9);
 Check(FoodStockQty(Food, 1, ProductKind_Fish) == 6 && Food->Stock[1][ProductKind_Fish].Lots[0].ExpiresRound == Soonest,
       "FoodGive keeps the expiry date");

 food_lot Old;
 Old.Qty = 3; Old.ExpiresRound = 9;
 FoodGive(Food, 1, ProductKind_Fish, &Old, 1, 9);
 Check(Food->Counts[1][ProductKind_Fish].Wasted == 3 && FoodStockQty(Food, 1, ProductKind_Fish) == 6,
       "food that arrives already bad is waste at the receiver");

 u32 Soon = FoodExpiringSoon(Food, 0, ProductKind_Fish, 9);
 Check(Soon <= FoodStockQty(Food, 0, ProductKind_Fish), "expiring-soon is part of the stock");
 Check(SimAllConserved(Food, 2), "conserved across take/give");

 // Lot cap: seeds keep for 20 days, one lot a round; merging must hold the count and never extend dates.
 food_state *Many = SimAllocFood();
 u64 Rounds = 3*FoodShelfLifeRounds(ProductKind_Seeds);
 b32 Ok = true;
 for(u64 Round = 1; Round <= Rounds; Round += 1)
 {
  FoodIslandRound(Many, 0, ProductKind_Seeds, 3.f*(f32)Food_RoundsPerDay, 0.f, Round);
  food_stock *Stock = &Many->Stock[0][ProductKind_Seeds];
  Ok = Ok && Stock->LotCount <= Food_MaxLots && SimLotsSorted(Many, 0, ProductKind_Seeds);
  Ok = Ok && (Stock->LotCount == 0 || Stock->Lots[Stock->LotCount - 1].ExpiresRound <= Round + FoodShelfLifeRounds(ProductKind_Seeds));
 }
 Check(Ok, "lot cap holds, lots sorted, merging never extends an expiry");
 Check(SimAllConserved(Many, 1), "conserved with merging");
}

//~ Small world for tests and the sim command
// NOTE: Islands on a ring; even islands produce, odd islands eat. Rates are per day.
typedef struct sim_world sim_world;
struct sim_world
{
 u32 IslandCount;
 f32 Produce[Food_MaxIslands][ProductKind_Count];
 f32 Need[Food_MaxIslands][ProductKind_Count];
};

internal void
SimMakeWorld(sim_world *World, u64 Seed, u32 IslandCount)
{
 MemorySet(World, 0, sizeof(*World));
 random_series Rng;
 RandomSeed(&Rng, Seed);
 World->IslandCount = IslandCount;
 for(u32 I = 0; I < IslandCount; I += 1)
 {
  for(u32 K = 0; K < ProductKind_Count; K += 1)
  {
   if(FoodIsPerishable((product_kind)K))
   {
    f32 Rate = 10.f*(f32)(2 + RandomNext(&Rng) % 19);   // 20..200 per day, like the world generator
    if((I % 2) == 0) World->Produce[I][K] = Rate;
    else             World->Need[I][K] = 0.4f*Rate;
   }
  }
 }
}

// One round. When Share is on, each island sends what goes bad within a day to the ring
// neighbour that is short (a stand-in for the routing engine, which is not ours to build).
internal void
SimRound(food_state *Food, sim_world *World, u64 Round, b32 Share)
{
 for(u32 I = 0; I < World->IslandCount; I += 1)
 {
  for(u32 K = 0; K < ProductKind_Count; K += 1)
  {
   FoodIslandRound(Food, I, (product_kind)K, World->Produce[I][K], World->Need[I][K], Round);
  }
 }
 if(Share)
 {
  for(u32 I = 0; I < World->IslandCount; I += 1)
  {
   for(u32 K = 0; K < ProductKind_Count; K += 1)
   {
    u32 Soon = FoodExpiringSoon(Food, I, (product_kind)K, Round);
    u32 To = (I + 1) % World->IslandCount;
    if(Soon > 0 && World->Need[To][K] > 0.f)
    {
     food_lot Lots[Food_MaxLots];
     u32 LotCount = 0;
     FoodTake(Food, I, (product_kind)K, Soon, Lots, ArrayCount(Lots), &LotCount);
     FoodGive(Food, To, (product_kind)K, Lots, LotCount, Round);
    }
   }
  }
 }
}

internal food_counts
SimRun(food_state *Food, sim_world *World, s32 Days, b32 Share, b32 *Conserved)
{
 FoodReset(Food);
 u64 Rounds = (u64)Days*Food_RoundsPerDay;
 for(u64 Round = 1; Round <= Rounds; Round += 1)
 {
  SimRound(Food, World, Round, Share);
  if(Conserved)
  {
   *Conserved = *Conserved && SimAllConserved(Food, World->IslandCount);
  }
 }
 food_counts Result = FoodTotals(Food, World->IslandCount);
 return Result;
}

internal void
TestDeterminismAndSharing(void)
{
 sim_world *World = (sim_world *)OS_Allocate(sizeof(sim_world));
 SimMakeWorld(World, 7, 16);
 food_state *A = SimAllocFood();
 food_state *B = SimAllocFood();
 b32 Conserved = true;
 SimRun(A, World, 10, true, &Conserved);
 SimRun(B, World, 10, true, &Conserved);
 Check(memcmp(A, B, sizeof(food_state)) == 0, "same world + same rounds = byte-identical food state (slider replay)");
 Check(Conserved, "conserved at every round");

 // Replay from zero after a reset gives the same state again (what the timeline slider does).
 SimRun(A, World, 4, true, 0);
 SimRun(A, World, 10, true, 0);
 Check(memcmp(A, B, sizeof(food_state)) == 0, "reset + replay gives the same history");

 food_counts Alone = SimRun(A, World, 10, false, 0);
 food_counts Shared = SimRun(B, World, 10, true, 0);
 Log("  10 days, 16 islands: no sharing wasted %llu short %llu | sharing soonest-expiring wasted %llu short %llu\n",
     Alone.Wasted, Alone.Shortage, Shared.Wasted, Shared.Shortage);
 Check(Shared.Wasted < Alone.Wasted && Shared.Shortage < Alone.Shortage, "sharing food before it spoils cuts waste and shortage");
}

internal s32
RunTests(void)
{
 struct { char *Name; s32 Kind; } Tests[] =
 {
  {"catalog: food + seeds spoil, the rest does not", 1},
  {"spoil after the shelf life", 2},
  {"eat soonest-expiring first, shortages, fractions", 3},
  {"routing API: FoodTake / FoodGive keep expiry, lot cap", 4},
  {"determinism, replay, sharing cuts waste", 5},
 };
 for EachIndex(T, ArrayCount(Tests))
 {
  s32 FailsBefore = GlobalFailCount;
  switch(Tests[T].Kind)
  {
   case 1: TestCatalog(); break;
   case 2: TestSpoil(); break;
   case 3: TestEat(); break;
   case 4: TestTakeGive(); break;
   case 5: TestDeterminismAndSharing(); break;
  }
  Log("[%s] %s\n", (GlobalFailCount == FailsBefore) ? "PASS" : "FAIL", Tests[T].Name);
 }
 Log("\n%d checks, %d failed.\n", GlobalCheckCount, GlobalFailCount);
 return GlobalFailCount;
}

//~ Sim (tuning report)
internal void
RunSim(u64 Seed, s32 Days)
{
 sim_world *World = (sim_world *)OS_Allocate(sizeof(sim_world));
 food_state *Food = SimAllocFood();
 SimMakeWorld(World, Seed, 16);
 Log("SolarSim sim: seed %llu, %d days, %u islands, %d rounds/day\n", Seed, Days, World->IslandCount, Food_RoundsPerDay);
 for(s32 Pass = 0; Pass < 2; Pass += 1)
 {
  b32 Share = (Pass == 1);
  Log("\n%s\n", Share ? "with sharing (soonest-expiring food to a short neighbour)" : "no sharing");
  Log("   kind          shelf(d)  produced     eaten    wasted  shortage\n");
  SimRun(Food, World, Days, Share, 0);
  for(u32 K = 0; K < ProductKind_Count; K += 1)
  {
   if(FoodIsPerishable((product_kind)K))
   {
    food_counts Sum = {0};
    for(u32 I = 0; I < World->IslandCount; I += 1)
    {
     Sum.Produced += Food->Counts[I][K].Produced;
     Sum.Eaten += Food->Counts[I][K].Eaten;
     Sum.Wasted += Food->Counts[I][K].Wasted;
     Sum.Shortage += Food->Counts[I][K].Shortage;
    }
    Log("   %-12.*s  %8.1f  %8llu  %8llu  %8llu  %8llu\n", (s32)ProductNames[K].Size, ProductNames[K].Data,
        FoodShelfLifeDays[K], Sum.Produced, Sum.Eaten, Sum.Wasted, Sum.Shortage);
   }
  }
 }
}

//~ Entry
internal char *
SimArgValue(int ArgCount, char **Args, char *Flag, char *Default)
{
 char *Result = Default;
 for(int A = 1; A + 1 < ArgCount; A += 1)
 {
  if(strcmp(Args[A], Flag) == 0)
  {
   Result = Args[A + 1];
  }
 }
 return Result;
}

ENTRY_POINT(EntryPoint)
{
 int ExitCode = 0;
 char *Command = (Params->ArgCount >= 2) ? Params->Args[1] : "";
 if(strcmp(Command, "test") == 0)
 {
  ExitCode = RunTests();
 }
 else if(strcmp(Command, "sim") == 0)
 {
  u64 Seed = (u64)atoll(SimArgValue(Params->ArgCount, Params->Args, "--seed", "7"));
  s32 Days = atoi(SimArgValue(Params->ArgCount, Params->Args, "--days", "10"));
  RunSim(Seed, Days);
 }
 else
 {
  Log("usage: SolarSim test\n"
      "       SolarSim sim [--seed 7] [--days 10]\n");
 }
 fflush(stdout);
 exit(ExitCode);
}
