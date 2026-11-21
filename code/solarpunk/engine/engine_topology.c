//-
// Topology: island layout, neighbour graph, biconnectivity check. Spec 10. Lane L4.
//-

//~ Shared math
internal f32
EngineWrapDx(f32 X0, f32 X1)
{
 f32 Dx = fabsf(X0 - X1);
 f32 Result = Min(Dx, 1.0f - Dx);
 return Result;
}

internal f32
EngineWrapDistance(f32 X0, f32 Y0, f32 X1, f32 Y1)
{
 f32 Dx = EngineWrapDx(X0, X1);
 f32 Dy = Y1 - Y0;
 f32 Result = sqrtf(Square(Dx) + Square(Dy));
 return Result;
}

internal f32
EngineSunIntensity(f32 X, f32 SunPhase)
{
 f32 Result = cosf(2.0f*Pi32*(X - SunPhase));
 return Result;
}

//~ Links
internal b32
TopologyHasLink(const world *World, island_id A, island_id B)
{
 b32 Result = false;
 for(s32 L = 0; L < World->LinkCount; L += 1)
 {
  const world_link *Link = &World->Links[L];
  if((Link->A == A && Link->B == B) || (Link->A == B && Link->B == A))
  {
   Result = true;
   break;
  }
 }
 return Result;
}

internal b32
TopologyAddLink(world *World, island_id A, island_id B)
{
 b32 Result = false;
 island_node *NodeA = &World->Islands[A];
 island_node *NodeB = &World->Islands[B];
 if(A != B &&
    !TopologyHasLink(World, A, B) &&
    NodeA->NeighborCount < MAX_NEIGHBORS &&
    NodeB->NeighborCount < MAX_NEIGHBORS &&
    World->LinkCount < (s32)ArrayCount(World->Links))
 {
  f32 Distance = EngineWrapDistance(NodeA->X, NodeA->Y, NodeB->X, NodeB->Y);
  b32 Wraps = (fabsf(NodeA->X - NodeB->X) > 0.5f);

  world_link *Link = &World->Links[World->LinkCount++];
  Link->A = A;
  Link->B = B;
  Link->Distance = Distance;
  Link->WrapsGlobe = Wraps;
  Link->Active = true;

  neighbor_link *ToB = &NodeA->Neighbors[NodeA->NeighborCount++];
  ToB->TargetId = B; ToB->Distance = Distance; ToB->WrapsGlobe = Wraps;
  neighbor_link *ToA = &NodeB->Neighbors[NodeB->NeighborCount++];
  ToA->TargetId = A; ToA->Distance = Distance; ToA->WrapsGlobe = Wraps;

  Result = true;
 }
 return Result;
}

internal void
TopologyClear(world *World, s32 IslandCount)
{
 Assert(IslandCount > 0 && IslandCount <= MAX_ISLANDS);
 World->IslandCount = IslandCount;
 World->LinkCount = 0;
 for EachIndex(I, IslandCount)
 {
  World->Islands[I].Id = I;
  World->Islands[I].NeighborCount = 0;
 }
}


//~ Checks
// NOTE(L4): Counts islands reachable from Start without passing through Skip (-1 = none).
internal s32
TopologyReachableCount(const world *World, island_id Start, island_id Skip)
{
 b32 Seen[MAX_ISLANDS] = {0};
 island_id Stack[MAX_ISLANDS];
 s32 StackCount = 0;
 s32 Result = 0;

 Stack[StackCount++] = Start;
 Seen[Start] = true;
 while(StackCount > 0)
 {
  island_id At = Stack[--StackCount];
  Result += 1;
  const island_node *Node = &World->Islands[At];
  for(s32 N = 0; N < Node->NeighborCount; N += 1)
  {
   island_id Next = Node->Neighbors[N].TargetId;
   if(Next != Skip && !Seen[Next])
   {
    Seen[Next] = true;
    Stack[StackCount++] = Next;
   }
  }
 }
 return Result;
}

// NOTE(L4): Real check (spec 10): remove each island in turn and verify the rest stays connected.
internal b32
TopologyIsBiconnected(const world *World)
{
 s32 Count = World->IslandCount;
 b32 Result = (Count >= 3 && TopologyReachableCount(World, 0, -1) == Count);
 for(island_id Skip = 0; Result && Skip < Count; Skip += 1)
 {
  island_id Start = (Skip == 0) ? 1 : 0;
  if(TopologyReachableCount(World, Start, Skip) != Count - 1)
  {
   Result = false;
  }
 }
 return Result;
}

// NOTE(L4): 0 exactly when the graph is biconnected (Count >= 3). Counts islands missed by the
// plain reachability check and by each remove-one-island check. Adding a link never raises it.
internal s32
TopologyDefect(const world *World)
{
 s32 Count = World->IslandCount;
 s32 Result = Count - TopologyReachableCount(World, 0, -1);
 for(island_id Skip = 0; Skip < Count; Skip += 1)
 {
  island_id Start = (Skip == 0) ? 1 : 0;
  Result += (Count - 1) - TopologyReachableCount(World, Start, Skip);
 }
 return Result;
}

internal s32
TopologyWrapEdgeCount(const world *World)
{
 s32 Result = 0;
 for(s32 L = 0; L < World->LinkCount; L += 1)
 {
  Result += (World->Links[L].WrapsGlobe ? 1 : 0);
 }
 return Result;
}

//~ Neighbour graph (spec 10)
// NOTE(L4): Undo the most recent TopologyAddLink. It was appended last everywhere.
internal void
TopologyPopLastLink(world *World)
{
 Assert(World->LinkCount > 0);
 world_link *Link = &World->Links[--World->LinkCount];
 World->Islands[Link->A].NeighborCount -= 1;
 World->Islands[Link->B].NeighborCount -= 1;
 MemorySet(Link, 0, sizeof(*Link));
}

typedef struct topology_pair topology_pair;
struct topology_pair
{
 island_id A, B;
 f32 Distance;
};

// All island pairs sorted by wrap distance (ties by index), for deterministic greedy picks.
internal s32
TopologySortedPairs(const world *World, topology_pair *Pairs)
{
 s32 Result = 0;
 for(island_id A = 0; A < World->IslandCount; A += 1)
 {
  for(island_id B = A + 1; B < World->IslandCount; B += 1)
  {
   const island_node *NodeA = &World->Islands[A];
   const island_node *NodeB = &World->Islands[B];
   topology_pair Pair;
   Pair.A = A;
   Pair.B = B;
   Pair.Distance = EngineWrapDistance(NodeA->X, NodeA->Y, NodeB->X, NodeB->Y);
   s32 At = Result++;
   while(At > 0 && Pairs[At - 1].Distance > Pair.Distance)
   {
    Pairs[At] = Pairs[At - 1];
    At -= 1;
   }
   Pairs[At] = Pair;
  }
 }
 return Result;
}

// NOTE(L4): Positions must be set. 1) every island links to its TOPOLOGY_K_NEAREST nearest;
// 2) add the shortest links that reduce TopologyDefect until biconnected; 3) add the shortest
// wrap-crossing links until there are TOPOLOGY_MIN_WRAP_EDGES. Never exceeds MAX_NEIGHBORS.
internal void
TopologyConnect(world *World)
{
 topology_pair Pairs[MAX_ISLANDS*(MAX_ISLANDS - 1)/2];
 s32 PairCount = TopologySortedPairs(World, Pairs);

 //- 1. k nearest
 for(island_id I = 0; I < World->IslandCount; I += 1)
 {
  s32 Picked = 0;
  for(s32 P = 0; P < PairCount && Picked < TOPOLOGY_K_NEAREST; P += 1)
  {
   if(Pairs[P].A == I || Pairs[P].B == I)
   {
    island_id Other = (Pairs[P].A == I) ? Pairs[P].B : Pairs[P].A;
    TopologyAddLink(World, I, Other);  // no-op if it exists or a side is full
    Picked += 1;
   }
  }
 }

 //- 2. Repair to biconnected
 s32 Defect = TopologyDefect(World);
 while(Defect > 0)
 {
  b32 Improved = false;
  for(s32 P = 0; P < PairCount && !Improved; P += 1)
  {
   if(TopologyAddLink(World, Pairs[P].A, Pairs[P].B))
   {
    s32 NewDefect = TopologyDefect(World);
    if(NewDefect < Defect)
    {
     Defect = NewDefect;
     Improved = true;
    }
    else
    {
     TopologyPopLastLink(World);
    }
   }
  }
  if(!Improved)
  {
   break; // NOTE(L4): only possible if neighbour caps block every fix; the tests catch it
  }
 }

 //- 3. Enough East-West wrap edges
 for(s32 P = 0; P < PairCount && TopologyWrapEdgeCount(World) < TOPOLOGY_MIN_WRAP_EDGES; P += 1)
 {
  const island_node *A = &World->Islands[Pairs[P].A];
  const island_node *B = &World->Islands[Pairs[P].B];
  if(fabsf(A->X - B->X) > 0.5f)
  {
   TopologyAddLink(World, Pairs[P].A, Pairs[P].B);
  }
 }
}

//~ Layouts
// NOTE(L4): Hand-tuned demo layout (positions only; links come from TopologyConnect). Used when
// IslandCount == ArrayCount(TopologyDemoLayout). D13 (island count) is still open.
global_variable const v2 TopologyDemoLayout[] =
{
 {0.04f, 0.30f}, {0.12f, 0.70f}, {0.21f, 0.45f}, {0.29f, 0.20f},
 {0.37f, 0.62f}, {0.46f, 0.38f}, {0.54f, 0.78f}, {0.62f, 0.25f},
 {0.71f, 0.55f}, {0.79f, 0.80f}, {0.88f, 0.35f}, {0.96f, 0.65f},
};

internal void
TopologyGenerate(world *World, s32 IslandCount)
{
 TopologyClear(World, IslandCount);
 const physics_constants *K = &World->Config.K;
 for(s32 I = 0; I < IslandCount; I += 1)
 {
  island_node *Node = &World->Islands[I];
  f32 X = ((f32)I + 0.5f)/(f32)IslandCount + K->XJitter*RandomBilateral(&World->Rng);
  // Cycle through three height bands so neighbours in x do not stack on top of each other.
  f32 Band = (f32)(I % 3) + RandomUnilateral(&World->Rng);
  Node->X = X - floorf(X);
  Node->Y = K->YMin + (Band/3.0f)*(K->YMax - K->YMin);
 }
 TopologyConnect(World);
}

internal void
TopologyBuildDefault(world *World)
{
 s32 Count = World->Config.IslandCount;
 if(Count == (s32)ArrayCount(TopologyDemoLayout))
 {
  TopologyClear(World, Count);
  for(s32 I = 0; I < Count; I += 1)
  {
   World->Islands[I].X = TopologyDemoLayout[I].X;
   World->Islands[I].Y = TopologyDemoLayout[I].Y;
  }
  TopologyConnect(World);
 }
 else
 {
  TopologyGenerate(World, Count);
 }
}
