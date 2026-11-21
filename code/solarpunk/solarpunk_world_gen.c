//-
// SolarPunk world generation: a seeded world of island clusters (archipelagos).
// Islands inside a cluster are densely linked; clusters are joined by a few long
// "bridge" links, so news spreads fast inside a cluster and slowly between them.
//-

//~ Constants
#define WorldGen_IslandCount    48
#define WorldGen_Seed           20261121
#define WorldGen_MaxIslands     64

#define WorldGen_ClusterCount   7
#define WorldGen_ClusterRadius  .155f // in 0..1 map units, for a cluster of 8 islands
#define WorldGen_MinIslandGap   .07f  // islands closer than this are re-rolled
#define WorldGen_PlaceAttempts  64
#define WorldGen_ExtraLinkChance .7f  // chance of an extra link inside a cluster

#define WorldGen_MinCost        1
#define WorldGen_MaxCost        4
#define WorldGen_MinBridgeCost  6
#define WorldGen_MaxBridgeCost  9

// NOTE: Cluster centres in 0..1 map units.
global_variable v2 WorldGenClusterCenters[WorldGen_ClusterCount] =
{
 {.17f, .18f}, {.52f, .12f}, {.86f, .22f},
 {.14f, .60f}, {.50f, .50f}, {.86f, .64f},
 {.46f, .88f},
};

// NOTE: How many islands each cluster gets, relative to the others (they sum to 48,
// so with 48 islands these are the exact counts).
global_variable u32 WorldGenClusterWeights[WorldGen_ClusterCount] =
{
 10, 5, 8,
 6, 9, 4,
 6,
};

// NOTE: Cluster names, in the same order as the centres. Each cluster also has its own
// look (see W3ClusterThemes).
global_variable char *WorldGenClusterNames[WorldGen_ClusterCount] =
{
 "Verdant Reach", "Sunward Isles", "Driftwood Shoals",
 "Ember Archipelago", "Crystal Spires", "Mistral Keys",
 "Saltglass Atolls",
};

// NOTE: Bridges between clusters, as pairs of cluster indices.
global_variable u32 WorldGenBridges[][2] =
{
 {0, 1}, {1, 2}, {0, 3}, {1, 4}, {2, 5},
 {3, 4}, {4, 5}, {3, 6}, {5, 6},
};

global_variable char *WorldGenNames[] =
{
 "Verdant Atoll",  "Sunspire",     "Kelp Haven",    "Copperleaf",
 "Windward Reach", "Mossgate",     "Glasshollow",   "Fernhold",
 "Cloudmere",      "Brightwater",  "Amber Terrace", "Willowdeep",
 "Solace Isle",    "Thistledown",  "Lumen Bay",     "Rootspire",
 "Hearthbloom",    "Skyloom",      "Cedar Rest",    "Driftmoor",
 "Petal Point",    "Ironbloom",    "Mistral Key",   "Saltgarden",
 "Honeycomb Rock", "Starfall",     "Greenvault",    "Tidewell",
 "Emberfield",     "Quietcove",    "Weaver's Rise",  "Dawnhollow",
 "Gullcrest",      "Breezehaven",  "Sorrel Isle",   "Lanternfield",
 "Coralwick",      "Pinecradle",   "Sunmeadow",     "Rainvale",
 "Brackenmoor",    "Opal Ridge",   "Seedcastle",    "Hollyfen",
 "Wavecrest",      "Duskwillow",   "Marigold Key",  "Stillwater",
};

//~ Helpers
// NOTE: Index of the first island of a cluster (Cluster == ClusterCount gives the end).
// Islands are split over the clusters by WorldGenClusterWeights.
internal u32
WorldGenClusterFirst(u32 Cluster, u32 IslandCount)
{
 u32 Total = 0;
 u32 Before = 0;
 for(u32 Idx = 0; Idx < WorldGen_ClusterCount; Idx += 1)
 {
  Total += WorldGenClusterWeights[Idx];
  if(Idx < Cluster) Before += WorldGenClusterWeights[Idx];
 }
 u32 Result = (IslandCount*Before)/Total;
 return Result;
}

// NOTE: Which cluster an island belongs to (see WorldGenClusterFirst).
internal u32
WorldGenClusterOf(u32 IslandIdx, u32 IslandCount)
{
 u32 Result = 0;
 for(u32 Cluster = 0; Cluster < WorldGen_ClusterCount; Cluster += 1)
 {
  if(IslandIdx >= WorldGenClusterFirst(Cluster, IslandCount))
  {
   Result = Cluster;
  }
 }
 return Result;
}

internal u32
WorldGenRandomInt(random_series *Series, u32 Min, u32 Max)
{
 u32 Result = Min + RandomNext(Series) % (Max - Min + 1);
 return Result;
}

internal product_kind
WorldGenRandomKind(random_series *Series)
{
 // NOTE: Skip ProductKind_None.
 product_kind Result = (product_kind)WorldGenRandomInt(Series, 1, ProductKind_Count - 1);
 return Result;
}

internal f32
WorldGenDistanceSq(v2 A, v2 B)
{
 f32 DX = A.X - B.X;
 f32 DY = A.Y - B.Y;
 f32 Result = DX*DX + DY*DY;
 return Result;
}

internal void
WorldGenLink(app_state *App, random_series *Series, island *A, island *B, u32 MinCost, u32 MaxCost)
{
 s32 Cost = (s32)WorldGenRandomInt(Series, MinCost, MaxCost);
 PushLink(App, A, B, Cost);
}

internal b32 WorldGenIsLinked(island *A, island *B);

// NOTE: Index (into Islands) of the island in [First, OnePastLast) closest to Islands[Self],
// skipping Self and, if SkipLinked, islands already linked to it. Returns OnePastLast if none.
internal u32
WorldGenClosest(island **Islands, u32 First, u32 OnePastLast, u32 Self, b32 SkipLinked)
{
 u32 Result = OnePastLast;
 f32 BestDistanceSq = 1e30f;
 v2 Pos = Islands[Self]->MapPos;
 for(u32 Idx = First; Idx < OnePastLast; Idx += 1)
 {
  if(Idx != Self && !(SkipLinked && WorldGenIsLinked(Islands[Self], Islands[Idx])))
  {
   f32 DistanceSq = WorldGenDistanceSq(Islands[Idx]->MapPos, Pos);
   if(DistanceSq < BestDistanceSq)
   {
    BestDistanceSq = DistanceSq;
    Result = Idx;
   }
  }
 }
 return Result;
}

internal b32
WorldGenIsLinked(island *A, island *B)
{
 b32 Result = false;
 for(link *Link = A->FirstLink; Link && !Result; Link = Link->Next)
 {
  Result = (Link->Dest == B);
 }
 return Result;
}

// NOTE: What each cluster grows or catches, in the same order as the clusters. Islands
// only make their cluster's foods and only need foods their cluster does not make, so
// every need is traded in from another cluster.
#define WorldGen_MaxSpecialties 3
global_variable product_kind WorldGenClusterFoods[WorldGen_ClusterCount][WorldGen_MaxSpecialties] =
{
 {ProductKind_Apples,  ProductKind_Pears,   ProductKind_Cabbage}, // Verdant Reach: orchards
 {ProductKind_Wheat,   ProductKind_Eggs,    ProductKind_None},    // Sunward Isles: fields and hens
 {ProductKind_Fish,    ProductKind_None,    ProductKind_None},    // Driftwood Shoals: fishing
 {ProductKind_Meat,    ProductKind_Pears,   ProductKind_None},    // Ember Archipelago: herds
 {ProductKind_Carrots, ProductKind_Cabbage, ProductKind_None},    // Crystal Spires: vegetables
 {ProductKind_Fish,    ProductKind_Eggs,    ProductKind_None},    // Mistral Keys: fishing, hens
 {ProductKind_Fish,    ProductKind_Carrots, ProductKind_Wheat},   // Saltglass Atolls
};

// NOTE: Scarce foods: made in smaller amounts than the others, so the world as a whole
// does not have enough of them and islands needing them turn red.
internal b32
WorldGenIsScarce(product_kind Kind)
{
 b32 Result = (Kind == ProductKind_Meat || Kind == ProductKind_Pears);
 return Result;
}

internal b32
WorldGenClusterMakes(u32 Cluster, product_kind Kind)
{
 b32 Result = false;
 for(u32 Idx = 0; Idx < WorldGen_MaxSpecialties; Idx += 1)
 {
  Result = Result || (WorldGenClusterFoods[Cluster][Idx] == Kind && Kind != ProductKind_None);
 }
 return Result;
}

internal void
WorldGenProductsAndNeeds(app_state *App, random_series *Series, island *Island, u32 Cluster)
{
 //- Makes: 1 or 2 of its cluster's foods
 u32 FoodCount = 0;
 for(u32 Idx = 0; Idx < WorldGen_MaxSpecialties; Idx += 1)
 {
  if(WorldGenClusterFoods[Cluster][Idx] != ProductKind_None) FoodCount += 1;
 }
 b32 Produces[ProductKind_Count] = {0};
 u32 ProduceCount = Min(WorldGenRandomInt(Series, 1, 2), FoodCount);
 for(u32 Produced = 0; Produced < ProduceCount; )
 {
  product_kind Kind = WorldGenClusterFoods[Cluster][WorldGenRandomInt(Series, 0, FoodCount - 1)];
  if(!Produces[Kind])
  {
   Produces[Kind] = true;
   u64 Amount = 10*(u64)WorldGenRandomInt(Series, 6, 16);
   if(WorldGenIsScarce(Kind)) Amount = 10*(u64)WorldGenRandomInt(Series, 2, 5);
   PushProduct(App, Island, Kind, Amount);
   Produced += 1;
  }
 }
 
 //- Needs: 2 or 3 foods its cluster does not make, so they come from other clusters
 b32 Needs[ProductKind_Count] = {0};
 u32 NeedCount = WorldGenRandomInt(Series, 2, 3);
 for(u32 Needed = 0, Tries = 0; Needed < NeedCount && Tries < 100; Tries += 1)
 {
  product_kind Kind = WorldGenRandomKind(Series);
  if(!WorldGenClusterMakes(Cluster, Kind) && !Needs[Kind])
  {
   Needs[Kind] = true;
   PushRequirement(App, Island, Kind, 5*(u64)WorldGenRandomInt(Series, 2, 10));
   Needed += 1;
  }
 }
}

//~ World
internal void
BuildGeneratedWorld(app_state *App, u32 IslandCount)
{
 ArenaClear(App->IslandsArena);
 ArenaClear(App->ProductsArena);
 ArenaClear(App->LinksArena);
 App->FirstIsland = App->LastIsland = 0;
 App->HasMapLayout = true;

 IslandCount = Min(IslandCount, (u32)WorldGen_MaxIslands);

 // NOTE: Same seed every frame, so the world is identical each time it is rebuilt.
 random_series Series = {0};
 RandomSeed(&Series, WorldGen_Seed);

 island *Islands[WorldGen_MaxIslands] = {0};

 // NOTE: Cluster C owns the islands in [ClusterFirst[C], ClusterFirst[C + 1]).
 u32 ClusterFirst[WorldGen_ClusterCount + 1] = {0};
 for(u32 Cluster = 0; Cluster <= WorldGen_ClusterCount; Cluster += 1)
 {
  ClusterFirst[Cluster] = WorldGenClusterFirst(Cluster, IslandCount);
 }

 //- Islands: position inside their cluster, production and needs
 for(u32 Cluster = 0; Cluster < WorldGen_ClusterCount; Cluster += 1)
 {
  v2 Center = WorldGenClusterCenters[Cluster];
  // Bigger clusters spread wider, so islands keep about the same spacing.
  u32 Size = ClusterFirst[Cluster + 1] - ClusterFirst[Cluster];
  f32 ClusterRadius = WorldGen_ClusterRadius*(f32)SqrtF64((f64)Size/8.0);

  for(u32 Idx = ClusterFirst[Cluster]; Idx < ClusterFirst[Cluster + 1]; Idx += 1)
  {
   str8 Name = S8FromCString(WorldGenNames[Idx % ArrayCount(WorldGenNames)]);
   if(Idx >= ArrayCount(WorldGenNames))
   {
    Name = Str8Fmt("%.*s %u", (s32)Name.Size, Name.Data, Idx/(u32)ArrayCount(WorldGenNames) + 1);
   }

   island *Island = PushIsland(App, Name);
   Islands[Idx] = Island;

   // Random spot in the cluster's disc, re-rolled if too close to an earlier island.
   v2 Pos = Center;
   for(u32 Attempt = 0; Attempt < WorldGen_PlaceAttempts; Attempt += 1)
   {
    f32 Angle = 2.f*Pi32*RandomUnilateral(&Series);
    f32 Distance = ClusterRadius*(f32)SqrtF64((f64)RandomUnilateral(&Series));
    Pos = V2(Center.X + Distance*(f32)CosF64((f64)Angle),
             Center.Y + Distance*(f32)SinF64((f64)Angle));

    b32 TooClose = false;
    for(u32 Other = ClusterFirst[Cluster]; Other < Idx && !TooClose; Other += 1)
    {
     TooClose = (WorldGenDistanceSq(Islands[Other]->MapPos, Pos) < WorldGen_MinIslandGap*WorldGen_MinIslandGap);
    }

    if(!TooClose)
    {
     break;
    }
   }
   Island->MapPos = Pos;

   WorldGenProductsAndNeeds(App, &Series, Island, Cluster);
  }
 }

 //- Links inside each cluster
 // NOTE: Each island links to the closest island placed before it in its cluster,
 // which keeps the cluster connected. Most then get an extra link to their closest
 // island they are not linked to yet.
 for(u32 Cluster = 0; Cluster < WorldGen_ClusterCount; Cluster += 1)
 {
  u32 First = ClusterFirst[Cluster];
  u32 OnePastLast = ClusterFirst[Cluster + 1];

  for(u32 Idx = First + 1; Idx < OnePastLast; Idx += 1)
  {
   u32 Closest = WorldGenClosest(Islands, First, Idx, Idx, false);
   if(Closest < Idx)
   {
    WorldGenLink(App, &Series, Islands[Idx], Islands[Closest], WorldGen_MinCost, WorldGen_MaxCost);
   }
  }

  for(u32 Idx = First; Idx < OnePastLast; Idx += 1)
  {
   if(RandomUnilateral(&Series) < WorldGen_ExtraLinkChance)
   {
    u32 Closest = WorldGenClosest(Islands, First, OnePastLast, Idx, true);
    if(Closest < OnePastLast)
    {
     WorldGenLink(App, &Series, Islands[Idx], Islands[Closest], WorldGen_MinCost, WorldGen_MaxCost);
    }
   }
  }
 }

 //- Bridges between clusters: link the two closest islands of each cluster pair
 for EachElement(BridgeIdx, WorldGenBridges)
 {
  u32 ClusterA = WorldGenBridges[BridgeIdx][0];
  u32 ClusterB = WorldGenBridges[BridgeIdx][1];

  u32 BestA = IslandCount;
  u32 BestB = IslandCount;
  f32 BestDistanceSq = 1e30f;
  for(u32 A = ClusterFirst[ClusterA]; A < ClusterFirst[ClusterA + 1]; A += 1)
  {
   for(u32 B = ClusterFirst[ClusterB]; B < ClusterFirst[ClusterB + 1]; B += 1)
   {
    f32 DistanceSq = WorldGenDistanceSq(Islands[A]->MapPos, Islands[B]->MapPos);
    if(DistanceSq < BestDistanceSq)
    {
     BestDistanceSq = DistanceSq;
     BestA = A;
     BestB = B;
    }
   }
  }

  if(BestA < IslandCount && BestB < IslandCount)
  {
   WorldGenLink(App, &Series, Islands[BestA], Islands[BestB], WorldGen_MinBridgeCost, WorldGen_MaxBridgeCost);
  }
 }
}
