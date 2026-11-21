//-
// SolarSim: headless console tool for the food module (solarpunk_food.c).
//   SolarSim test                       runs the food tests, exits with the failure count
//   SolarSim sim [--seed 7] [--days 10] prints waste and shortage with and without sharing
//   SolarSim supply                     prints the world's supply plan and harvest waves
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
#include "solarpunk/solarpunk_supply.c"

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
 b32 AllFood = true;
 for(u32 Kind = ProductKind_None + 1; Kind < ProductKind_Count; Kind += 1)
 {
  AllFood = AllFood && FoodIsPerishable((product_kind)Kind);
 }
 Check(AllFood, "every product in the catalog is food and spoils");
 Check(!FoodIsPerishable(ProductKind_None), "ProductKind_None is not tracked");
 Check(FoodShelfLifeRounds(ProductKind_Fish) < FoodShelfLifeRounds(ProductKind_Wheat), "fish goes bad before wheat");
 b32 AllInDemo = true;
 for(u32 Kind = ProductKind_None + 1; Kind < ProductKind_Count; Kind += 1)
 {
  AllInDemo = AllInDemo && FoodShelfLifeRounds((product_kind)Kind) <= 30;
 }
 Check(AllInDemo, "every food can go bad inside a 30-round demo");
 
 food_state *Food = SimAllocFood();
 FoodIslandRound(Food, 0, ProductKind_None, 100.f, 50.f, 1);
 Check(FoodStockQty(Food, 0, ProductKind_None) == 0 && Food->Counts[0][ProductKind_None].Produced == 0,
       "ProductKind_None is ignored");
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
 FoodGive(Food, 0, ProductKind_Carrots, Lots, 2, 1);
 Check(Food->Stock[0][ProductKind_Carrots].Lots[0].ExpiresRound == 20, "received lots are inserted by expiry");

 FoodIslandRound(Food, 0, ProductKind_Carrots, 0.f, 3.f*(f32)Food_RoundsPerDay, 2);   // eat 3
 Check(Food->Stock[0][ProductKind_Carrots].Lots[0].ExpiresRound == 20 && Food->Stock[0][ProductKind_Carrots].Lots[0].Qty == 2,
       "eats the soonest-expiring lot first");
 FoodIslandRound(Food, 0, ProductKind_Carrots, 0.f, 9.f*(f32)Food_RoundsPerDay, 3);   // wants 9, has 7
 Check(FoodStockQty(Food, 0, ProductKind_Carrots) == 0, "eats everything there is");
 Check(Food->Counts[0][ProductKind_Carrots].Shortage == 2, "the rest is a shortage");

 // Fractions carry: 1.5 units/round -> 3 units every 2 rounds.
 food_state *Frac = SimAllocFood();
 for(u64 Round = 1; Round <= 10; Round += 1)
 {
  FoodIslandRound(Frac, 0, ProductKind_Wheat, 1.5f*(f32)Food_RoundsPerDay, 0.f, Round);
 }
 Check(Frac->Counts[0][ProductKind_Wheat].Produced == 15, "fractional production carries over");
 Check(SimAllConserved(Food, 1) && SimAllConserved(Frac, 1), "conserved");
}

// The routing engine API keeps expiry dates on the move.
internal void
TestTakeGive(void)
{
 food_state *Food = SimAllocFood();
 // Fewer rounds than the fish shelf life, so nothing has gone bad before the hand-over.
 for(u64 Round = 1; Round <= 4; Round += 1)
 {
  FoodIslandRound(Food, 0, ProductKind_Fish, 4.f*(f32)Food_RoundsPerDay, 0.f, Round);
 }
 u64 Soonest = Food->Stock[0][ProductKind_Fish].Lots[0].ExpiresRound;

 food_lot Out[8];
 u32 OutCount = 0;
 u32 Took = FoodTake(Food, 0, ProductKind_Fish, 6, Out, ArrayCount(Out), &OutCount);
 Check(Took == 6 && OutCount >= 1 && Out[0].ExpiresRound == Soonest, "FoodTake gives the soonest-expiring food first");
 Check(Food->Counts[0][ProductKind_Fish].Sent == 6, "Sent counted");

 FoodGive(Food, 1, ProductKind_Fish, Out, OutCount, 5);
 Check(FoodStockQty(Food, 1, ProductKind_Fish) == 6 && Food->Stock[1][ProductKind_Fish].Lots[0].ExpiresRound == Soonest,
       "FoodGive keeps the expiry date");

 food_lot Old;
 Old.Qty = 3; Old.ExpiresRound = 5;
 FoodGive(Food, 1, ProductKind_Fish, &Old, 1, 5);
 Check(Food->Counts[1][ProductKind_Fish].Wasted == 3 && FoodStockQty(Food, 1, ProductKind_Fish) == 6,
       "food that arrives already bad is waste at the receiver");

 u32 Soon = FoodExpiringSoon(Food, 0, ProductKind_Fish, 5);
 Check(Soon <= FoodStockQty(Food, 0, ProductKind_Fish), "expiring-soon is part of the stock");
 Check(SimAllConserved(Food, 2), "conserved across take/give");

 // Lot cap: wheat keeps for 30 days, one lot a round; merging must hold the count and never extend dates.
 food_state *Many = SimAllocFood();
 u64 Rounds = 3*FoodShelfLifeRounds(ProductKind_Wheat);
 b32 Ok = true;
 for(u64 Round = 1; Round <= Rounds; Round += 1)
 {
  FoodIslandRound(Many, 0, ProductKind_Wheat, 3.f*(f32)Food_RoundsPerDay, 0.f, Round);
  food_stock *Stock = &Many->Stock[0][ProductKind_Wheat];
  Ok = Ok && Stock->LotCount <= Food_MaxLots && SimLotsSorted(Many, 0, ProductKind_Wheat);
  Ok = Ok && (Stock->LotCount == 0 || Stock->Lots[Stock->LotCount - 1].ExpiresRound <= Round + FoodShelfLifeRounds(ProductKind_Wheat));
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

//~ Supply plan (solarpunk_supply.c)
#define Sim_SupplyIslands 48

internal void
SimSupplyPlan(supply_plan *Plan)
{
 u32 ClusterOf[Supply_MaxIslands] = {0};
 for(u32 I = 0; I < Sim_SupplyIslands; I += 1)
 {
  ClusterOf[I] = (I*Supply_ClusterCount)/Sim_SupplyIslands;
 }
 SupplyMakePlan(Plan, Sim_SupplyIslands, ClusterOf);
}

internal void
TestSupplyPlan(void)
{
 supply_plan *A = (supply_plan *)OS_Allocate(sizeof(supply_plan));
 supply_plan *B = (supply_plan *)OS_Allocate(sizeof(supply_plan));
 SimSupplyPlan(A);
 SimSupplyPlan(B);
 Check(memcmp(A, B, sizeof(supply_plan)) == 0, "same inputs give the same supply plan");

 //- Every climate together covers every food
 b32 Covered = true;
 for(u32 K = 1; K < ProductKind_Count; K += 1)
 {
  b32 Any = false;
  for(u32 C = 0; C < Supply_ClusterCount; C += 1) Any = Any || SupplyIsClimateFood(C, (product_kind)K);
  Covered = Covered && Any;
 }
 Check(Covered, "every food is some cluster's climate food");

 b32 AllMade = true, AllNeeded = true, Balanced = true, ClimateMore = true, ClimateEverywhere = true, OthersToo = true;
 for(u32 K = 1; K < ProductKind_Count; K += 1)
 {
  u64 Made = 0, Needed = 0, MinClimate = ~0ull, MaxOther = 0;
  u32 Makers = 0;
  for(u32 I = 0; I < A->IslandCount; I += 1)
  {
   Made += A->Make[I][K];
   Needed += A->Need[I][K];
   Makers += (A->Make[I][K] > 0);
   if(SupplyIsClimateFood(A->Cluster[I], (product_kind)K))
   {
    MinClimate = Min(MinClimate, A->Make[I][K]);
    ClimateEverywhere = ClimateEverywhere && (A->Make[I][K] > 0);
   }
   else
   {
    MaxOther = Max(MaxOther, A->Make[I][K]);
   }
  }
  AllMade = AllMade && (Makers >= 1);
  AllNeeded = AllNeeded && (Needed > 0);
  Balanced = Balanced && (Made == (Needed*(100 + Supply_BufferPercent) + 99)/100);
  if(MinClimate != ~0ull) ClimateMore = ClimateMore && (MinClimate >= MaxOther);
 }
 for(u32 I = 0; I < A->IslandCount; I += 1)
 {
  u32 Others = 0;
  for(u32 K = 1; K < ProductKind_Count; K += 1) Others += (A->Make[I][K] > 0 && !SupplyIsClimateFood(A->Cluster[I], (product_kind)K));
  OthersToo = OthersToo && (Others >= Supply_MinOtherFoods);
 }
 Check(AllMade, "every food is made by at least one island");
 Check(AllNeeded, "every food is needed by at least one island");
 Check(Balanced, "per food, world makes = world needs + buffer, exactly");
 Check(ClimateEverywhere, "every island makes all of its climate's foods");
 Check(ClimateMore, "per food, every climate maker makes at least as much as any other maker");
 Check(OthersToo, "every island also makes at least one non-climate food");
}

internal void
TestHarvestWaves(void)
{
 b32 Averages = true, InRange = true, Moves = true;
 for(u32 I = 0; I < Sim_SupplyIslands; I += 1)
 {
  u32 Cluster = (I*Supply_ClusterCount)/Sim_SupplyIslands;
  for(u64 Start = 0; Start < 3; Start += 1)
  {
   f64 Sum = 0;
   f32 Lo = 10.f, Hi = 0.f;
   for(u64 R = 1; R <= Supply_WavePeriodRounds; R += 1)
   {
    f32 F = SupplyHarvestFactor(Cluster, I, Start*7 + R);
    Sum += F;
    Lo = Min(Lo, F);
    Hi = Max(Hi, F);
   }
   f64 Mean = Sum/(f64)Supply_WavePeriodRounds;
   Averages = Averages && (Mean > .9999 && Mean < 1.0001);
   InRange = InRange && (Lo >= 1.f - (f32)Supply_WaveAmplitude - .001f) && (Hi <= 1.f + (f32)Supply_WaveAmplitude + .001f);
   Moves = Moves && (Hi - Lo > (f32)Supply_WaveAmplitude);
  }
 }
 Check(Averages, "a harvest wave averages to 1 over any full period");
 Check(InRange, "harvest stays within 1 +- amplitude");
 Check(Moves, "harvest really goes up and down");

 //- Run the food rounds over whole waves: the food round makes what the plan says
 supply_plan *Plan = (supply_plan *)OS_Allocate(sizeof(supply_plan));
 SimSupplyPlan(Plan);
 food_state *Food = SimAllocFood();
 u64 Rounds = 4*Supply_WavePeriodRounds;
 for(u64 Round = 1; Round <= Rounds; Round += 1)
 {
  for(u32 I = 0; I < Plan->IslandCount; I += 1)
  {
   f32 Harvest = SupplyHarvestFactor(Plan->Cluster[I], I, Round);
   for(u32 K = 1; K < ProductKind_Count; K += 1)
   {
    FoodIslandRound(Food, I, (product_kind)K, Harvest*(f32)Plan->Make[I][K], 0.f, Round);
   }
  }
 }
 b32 MatchesPlan = true;
 for(u32 K = 1; K < ProductKind_Count; K += 1)
 {
  u64 Made = 0, Planned = 0;
  for(u32 I = 0; I < Plan->IslandCount; I += 1)
  {
   Made += Food->Counts[I][K].Produced;
   Planned += Plan->Make[I][K]*Rounds/Food_RoundsPerDay;
  }
  f64 Ratio = (f64)Made/(f64)Max(Planned, 1);
  MatchesPlan = MatchesPlan && Ratio > .98 && Ratio < 1.02;
 }
 Check(MatchesPlan, "over whole waves, the food round makes what the plan says (within 2%)");
 Check(SimAllConserved(Food, Plan->IslandCount), "conserved with harvest waves");
}

internal s32
RunTests(void)
{
 struct { char *Name; s32 Kind; } Tests[] =
 {
  {"catalog: every product is food and spoils", 1},
  {"spoil after the shelf life", 2},
  {"eat soonest-expiring first, shortages, fractions", 3},
  {"routing API: FoodTake / FoodGive keep expiry, lot cap", 4},
  {"determinism, replay, sharing cuts waste", 5},
  {"supply plan: balanced, every food made, climate makes more", 6},
  {"harvest waves: average 1, go up and down", 7},
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
   case 6: TestSupplyPlan(); break;
   case 7: TestHarvestWaves(); break;
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
  Log("   kind       shelf(rnd)  produced     eaten    wasted  shortage\n");
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
        (f32)FoodShelfLife[K], Sum.Produced, Sum.Eaten, Sum.Wasted, Sum.Shortage);
   }
  }
 }
}

//~ Supply report
internal void
RunSupply(void)
{
 supply_plan *Plan = (supply_plan *)OS_Allocate(sizeof(supply_plan));
 SimSupplyPlan(Plan);
 Log("SolarSim supply: %u islands, %d clusters, buffer %d%%, climate weight %dx\n\n",
     Plan->IslandCount, Supply_ClusterCount, Supply_BufferPercent, Supply_ClimateWeight);
 Log("   kind        needed/day  made/day  ratio  makers (climate)\n");
 for(u32 K = 1; K < ProductKind_Count; K += 1)
 {
  u64 Made = 0, Needed = 0;
  u32 Makers = 0, Climate = 0;
  for(u32 I = 0; I < Plan->IslandCount; I += 1)
  {
   Made += Plan->Make[I][K];
   Needed += Plan->Need[I][K];
   Makers += (Plan->Make[I][K] > 0);
   Climate += (Plan->Make[I][K] > 0 && SupplyIsClimateFood(Plan->Cluster[I], (product_kind)K));
  }
  Log("   %-10.*s  %10llu  %8llu  %5.2f  %3u (%u)\n", (s32)ProductNames[K].Size, ProductNames[K].Data,
      Needed, Made, (f64)Made/(f64)Max(Needed, 1), Makers, Climate);
 }

 Log("\nHarvest per cluster over one wave (%d rounds), %% of average:\n   round", Supply_WavePeriodRounds);
 for(u32 C = 0; C < Supply_ClusterCount; C += 1) Log("  cl%u", C);
 Log("\n");
 for(u64 R = 1; R <= Supply_WavePeriodRounds; R += 2)
 {
  Log("   %5llu", R);
  for(u32 C = 0; C < Supply_ClusterCount; C += 1) Log("  %3.0f", 100.0*SupplyHarvestFactor(C, 0, R));
  Log("\n");
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
 else if(strcmp(Command, "supply") == 0)
 {
  RunSupply();
 }
 else
 {
  Log("usage: SolarSim test\n"
      "       SolarSim sim [--seed 7] [--days 10]\n"
      "       SolarSim supply\n");
 }
 fflush(stdout);
 exit(ExitCode);
}
