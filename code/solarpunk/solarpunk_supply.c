//-
// SolarPunk supply: what each island makes and needs, per day.
//   - Every cluster has a climate: the foods that grow best there. Its islands make those in
//     large amounts and 1 or 2 other foods in small amounts.
//   - Islands eat a mixed diet (any food, also their own), covered partly at home.
//   - Per food, the whole world makes Supply_BufferPercent more than it needs, and every food
//     is made by at least one island.
//   - Slow harvest waves: output swings around that average, with each cluster at a different
//     point of the wave, so stocks go up and down while the average still balances.
// Pure data: only base types, product_kind and random_series. No app_state, so SolarSim can test it.
//-

//~ Constants
#define Supply_ClusterCount      7
#define Supply_MaxIslands        64
#define Supply_MaxClimateFoods   3
#define Supply_Seed              20261122

#define Supply_BufferPercent     10   // the world makes this much more of each food than it needs
#define Supply_ClimateWeight     3    // a climate food gets about this many times the share of another food
#define Supply_ShareMin          8    // each maker's share is its weight times a roll in [ShareMin, ShareMax]
#define Supply_ShareMax          12
#define Supply_MinOtherFoods     1    // non-climate foods an island also makes
#define Supply_MaxOtherFoods     2
#define Supply_MinNeeds          3    // foods an island eats
#define Supply_MaxNeeds          5
#define Supply_NeedStep          5    // needs are multiples of this, per day
#define Supply_MinNeed           10
#define Supply_MaxNeed           50
#define Supply_PickTries         100

#define Supply_WavePeriodRounds  24   // one harvest wave, high to low and back (the demo runs about 30 rounds)
#define Supply_WaveAmplitude     .35  // output swings between 65% and 135% of the average
#define Supply_WaveIslandSpread  .25  // islands of one cluster are up to this much of a wave apart

// NOTE: The climate of each cluster: the foods it grows or catches best, in the same order
// as the clusters in solarpunk_world_gen.c. Together they must cover every food.
global_variable product_kind SupplyClimateFoods[Supply_ClusterCount][Supply_MaxClimateFoods] =
{
 {ProductKind_Apples,  ProductKind_Pears,   ProductKind_Berries},  // Verdant Reach: mild and wet forest, orchards and berries
 {ProductKind_Wheat,   ProductKind_Eggs,    ProductKind_Coconuts}, // Sunward Isles: sunny beaches, fields, hens and palms
 {ProductKind_Fish,    ProductKind_Crabs,   ProductKind_None},     // Driftwood Shoals: open sea, fish and crab pots
 {ProductKind_Meat,    ProductKind_Pears,   ProductKind_None},     // Ember Archipelago: warm grassland, herds
 {ProductKind_Carrots, ProductKind_Cabbage, ProductKind_None},     // Mistral Heights: cool windy highlands, vegetables
 {ProductKind_Fish,    ProductKind_Eggs,    ProductKind_None},     // Crystal Spires: shimmering coast, fish and hens
 {ProductKind_Fish,    ProductKind_Potatoes, ProductKind_Wheat},   // Frostfall Isles: frozen, ice fishing, potatoes and grain
};

//~ Types
typedef struct supply_plan supply_plan;
struct supply_plan
{
 u32 IslandCount;
 u32 Cluster[Supply_MaxIslands];
 u64 Make[Supply_MaxIslands][ProductKind_Count];  // per day, average over a harvest wave
 u64 Need[Supply_MaxIslands][ProductKind_Count];  // per day
};

//~ Helpers
internal b32
SupplyIsClimateFood(u32 Cluster, product_kind Kind)
{
 b32 Result = false;
 if(Cluster < Supply_ClusterCount && Kind != ProductKind_None)
 {
  for(u32 Idx = 0; Idx < Supply_MaxClimateFoods; Idx += 1)
  {
   Result = Result || (SupplyClimateFoods[Cluster][Idx] == Kind);
  }
 }
 return Result;
}

internal u32
SupplyRandomInt(random_series *Series, u32 Min, u32 Max)
{
 u32 Result = Min + RandomNext(Series) % (Max - Min + 1);
 return Result;
}

internal product_kind
SupplyRandomKind(random_series *Series)
{
 // NOTE: Skip ProductKind_None.
 product_kind Result = (product_kind)SupplyRandomInt(Series, 1, ProductKind_Count - 1);
 return Result;
}

// NOTE: The island with the fewest non-zero entries in Row (lowest index on ties).
internal u32
SupplyFewest(u64 Rows[][ProductKind_Count], u32 IslandCount)
{
 u32 Result = 0;
 u32 BestCount = ProductKind_Count + 1;
 for(u32 I = 0; I < IslandCount; I += 1)
 {
  u32 Count = 0;
  for(u32 K = 1; K < ProductKind_Count; K += 1) Count += (Rows[I][K] > 0);
  if(Count < BestCount)
  {
   BestCount = Count;
   Result = I;
  }
 }
 return Result;
}

//~ Plan
// NOTE: Fills Plan for IslandCount islands; ClusterOf[I] is island I's cluster.
// Deterministic: the same inputs always give the same plan.
internal void
SupplyMakePlan(supply_plan *Plan, u32 IslandCount, u32 *ClusterOf)
{
 MemorySet(Plan, 0, sizeof(*Plan));
 IslandCount = Min(IslandCount, (u32)Supply_MaxIslands);
 Plan->IslandCount = IslandCount;
 if(IslandCount == 0) return;

 random_series Series = {0};
 RandomSeed(&Series, Supply_Seed);

 // Share of each maker before scaling: climate foods large, other foods small.
 u64 Weight[Supply_MaxIslands][ProductKind_Count] = {0};

 for(u32 I = 0; I < IslandCount; I += 1)
 {
  u32 Cluster = Min(ClusterOf[I], (u32)Supply_ClusterCount - 1);
  Plan->Cluster[I] = Cluster;

  //- Makes: every climate food of its cluster
  for(u32 Idx = 0; Idx < Supply_MaxClimateFoods; Idx += 1)
  {
   product_kind Kind = SupplyClimateFoods[Cluster][Idx];
   if(Kind != ProductKind_None)
   {
    Weight[I][Kind] = Supply_ClimateWeight*(u64)SupplyRandomInt(&Series, Supply_ShareMin, Supply_ShareMax);
   }
  }

  //- Makes: 1 or 2 other foods, in small amounts
  u32 OtherCount = SupplyRandomInt(&Series, Supply_MinOtherFoods, Supply_MaxOtherFoods);
  for(u32 Made = 0, Tries = 0; Made < OtherCount && Tries < Supply_PickTries; Tries += 1)
  {
   product_kind Kind = SupplyRandomKind(&Series);
   if(!SupplyIsClimateFood(Cluster, Kind) && Weight[I][Kind] == 0)
   {
    Weight[I][Kind] = SupplyRandomInt(&Series, Supply_ShareMin, Supply_ShareMax);
    Made += 1;
   }
  }

  //- Needs: a mixed diet of any foods, its own included
  u32 NeedCount = SupplyRandomInt(&Series, Supply_MinNeeds, Supply_MaxNeeds);
  for(u32 Needed = 0, Tries = 0; Needed < NeedCount && Tries < Supply_PickTries; Tries += 1)
  {
   product_kind Kind = SupplyRandomKind(&Series);
   if(Plan->Need[I][Kind] == 0)
   {
    Plan->Need[I][Kind] = Supply_NeedStep*(u64)SupplyRandomInt(&Series, Supply_MinNeed/Supply_NeedStep, Supply_MaxNeed/Supply_NeedStep);
    Needed += 1;
   }
  }
 }

 for(u32 K = 1; K < ProductKind_Count; K += 1)
 {
  //- Every food is eaten and made somewhere
  u64 TotalNeed = 0;
  u64 TotalWeight = 0;
  for(u32 I = 0; I < IslandCount; I += 1)
  {
   TotalNeed += Plan->Need[I][K];
   TotalWeight += Weight[I][K];
  }
  if(TotalNeed == 0)
  {
   u32 I = SupplyFewest(Plan->Need, IslandCount);
   Plan->Need[I][K] = Supply_MinNeed;
   TotalNeed = Supply_MinNeed;
  }
  if(TotalWeight == 0)
  {
   u32 I = SupplyFewest(Weight, IslandCount);
   Weight[I][K] = Supply_ShareMin;
   TotalWeight = Supply_ShareMin;
  }

  //- Split the target over the makers by weight; whole units, the sum is exact
  // Target = TotalNeed*(100 + Buffer)/100, rounded up.
  u64 Target = (TotalNeed*(100 + Supply_BufferPercent) + 99)/100;
  u64 Given = 0;
  for(u32 I = 0; I < IslandCount; I += 1)
  {
   Plan->Make[I][K] = (Target*Weight[I][K])/TotalWeight;
   Given += Plan->Make[I][K];
  }
  // The units lost to rounding go to the makers with the largest leftover fraction.
  b32 GotExtra[Supply_MaxIslands] = {0};
  for(; Given < Target; Given += 1)
  {
   u32 Best = IslandCount;
   u64 BestLeft = 0;
   for(u32 I = 0; I < IslandCount; I += 1)
   {
    u64 Left = (Target*Weight[I][K]) % TotalWeight;
    if(Weight[I][K] > 0 && !GotExtra[I] && (Best == IslandCount || Left > BestLeft))
    {
     Best = I;
     BestLeft = Left;
    }
   }
   Assert(Best < IslandCount);
   GotExtra[Best] = true;
   Plan->Make[Best][K] += 1;
  }
 }
}

//~ Harvest waves
// NOTE: How much of its average an island makes in this round: 1 + Amplitude*sin(...).
// Each cluster sits at its own point of the wave, and islands inside a cluster a little apart.
// Over any Supply_WavePeriodRounds rounds in a row it averages to 1.
internal f32
SupplyHarvestFactor(u32 Cluster, u32 Island, u64 Round)
{
 f64 IslandSpread = (f64)Island*0.6180339887498949;
 IslandSpread -= (f64)(u64)IslandSpread;
 f64 Phase = (f64)Cluster/(f64)Supply_ClusterCount + Supply_WaveIslandSpread*IslandSpread;
 f64 T = (f64)(Round % Supply_WavePeriodRounds)/(f64)Supply_WavePeriodRounds + Phase;
 f32 Result = (f32)(1.0 + Supply_WaveAmplitude*SinF64(2.0*3.14159265358979323846*T));
 return Result;
}
