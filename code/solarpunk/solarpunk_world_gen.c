//-
// SolarPunk world generation: a seeded world of island clusters (archipelagos).
// Islands inside a cluster are densely linked; clusters are joined by a few long
// "bridge" links, so news spreads fast inside a cluster and slowly between them.
//-

//~ Constants
#define WorldGen_IslandCount    32
#define WorldGen_Seed           20261121
#define WorldGen_MaxIslands     64

#define WorldGen_ClusterCount   4
#define WorldGen_ClusterRadius  .17f  // in 0..1 map units
#define WorldGen_MinIslandGap   .07f  // islands closer than this are re-rolled
#define WorldGen_PlaceAttempts  64
#define WorldGen_ExtraLinkChance .7f  // chance of an extra link inside a cluster

#define WorldGen_MinCost        1
#define WorldGen_MaxCost        4
#define WorldGen_MinBridgeCost  6
#define WorldGen_MaxBridgeCost  9

// NOTE: Cluster centres in 0..1 map units, one per quadrant.
global_variable v2 WorldGenClusterCenters[WorldGen_ClusterCount] =
{
 {.22f, .27f}, {.76f, .24f}, {.26f, .76f}, {.78f, .73f},
};

// NOTE: Cluster names, in the same order as the centres (top left, top right,
// bottom left, bottom right).
global_variable char *WorldGenClusterNames[WorldGen_ClusterCount] =
{
 "Verdant Reach", "Sunward Isles", "Driftwood Shoals", "Ember Archipelago",
};

// NOTE: Bridges between clusters, as pairs of cluster indices. They form a ring.
global_variable u32 WorldGenBridges[][2] =
{
 {0, 1}, {1, 3}, {3, 2}, {2, 0},
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
 "Emberfield",     "Quietcove",    "Weaver's Rise", "Dawnhollow",
};

//~ Helpers
// NOTE: Which cluster an island belongs to. Cluster C owns the islands with index in
// [IslandCount*C/ClusterCount, IslandCount*(C + 1)/ClusterCount), as built below.
internal u32
WorldGenClusterOf(u32 IslandIdx, u32 IslandCount)
{
 u32 Result = 0;
 for(u32 Cluster = 0; Cluster < WorldGen_ClusterCount; Cluster += 1)
 {
  if(IslandIdx >= (IslandCount*Cluster)/WorldGen_ClusterCount)
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

internal void
WorldGenProductsAndNeeds(app_state *App, random_series *Series, island *Island)
{
 // Production: 1 or 2 different kinds.
 b32 Produces[ProductKind_Count] = {0};
 u32 ProduceCount = WorldGenRandomInt(Series, 1, 2);
 for(u32 Produced = 0; Produced < ProduceCount; )
 {
  product_kind Kind = WorldGenRandomKind(Series);
  if(!Produces[Kind])
  {
   Produces[Kind] = true;
   PushProduct(App, Island, Kind, 10*(u64)WorldGenRandomInt(Series, 2, 20));
   Produced += 1;
  }
 }

 // Needs: 2 or 3 kinds it does not produce itself, so every island must trade.
 b32 Needs[ProductKind_Count] = {0};
 u32 NeedCount = WorldGenRandomInt(Series, 2, 3);
 for(u32 Needed = 0; Needed < NeedCount; )
 {
  product_kind Kind = WorldGenRandomKind(Series);
  if(!Produces[Kind] && !Needs[Kind])
  {
   Needs[Kind] = true;
   PushRequirement(App, Island, Kind, 5*(u64)WorldGenRandomInt(Series, 1, 12));
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
  ClusterFirst[Cluster] = (IslandCount*Cluster)/WorldGen_ClusterCount;
 }

 //- Islands: position inside their cluster, production and needs
 for(u32 Cluster = 0; Cluster < WorldGen_ClusterCount; Cluster += 1)
 {
  v2 Center = WorldGenClusterCenters[Cluster];

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
    f32 Distance = WorldGen_ClusterRadius*(f32)SqrtF64((f64)RandomUnilateral(&Series));
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

   WorldGenProductsAndNeeds(App, &Series, Island);
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
