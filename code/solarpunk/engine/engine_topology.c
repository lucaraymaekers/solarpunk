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

//~ Layouts
// TODO(L4): Placeholder for task (a): evenly spaced ring plus skip-one chords. It is biconnected
// and has 3 wrap edges, which is enough for MockWorld and the first tests. The real generator
// (k = 3 nearest by wrap distance, then repair to biconnected, spec 10) and the hand-tuned demo
// preset come later.
internal void
TopologyRingWithChords(world *World, s32 IslandCount, f32 Jitter)
{
 TopologyClear(World, IslandCount);
 const physics_constants *K = &World->Config.K;

 for EachIndex(I, IslandCount)
 {
  island_node *Node = &World->Islands[I];
  f32 X = ((f32)I + 0.5f)/(f32)IslandCount + Jitter*RandomBilateral(&World->Rng);
  X = X - floorf(X);
  f32 YMid = 0.5f*(K->YMin + K->YMax);
  f32 YAmp = 0.5f*(K->YMax - K->YMin);
  f32 Y = YMid + ((I & 1) ? YAmp : -YAmp)*(0.4f + 0.5f*RandomUnilateral(&World->Rng));
  Node->X = X;
  Node->Y = Clamp(K->YMin, Y, K->YMax);
 }

 for EachIndex(I, IslandCount)
 {
  TopologyAddLink(World, I, (I + 1) % IslandCount);
 }
 if(IslandCount >= 5)
 {
  for EachIndex(I, IslandCount)
  {
   TopologyAddLink(World, I, (I + 2) % IslandCount);
  }
 }
}

internal void
TopologyBuildDefault(world *World)
{
 TopologyRingWithChords(World, World->Config.IslandCount, 0.0f);
}

internal void
TopologyGenerate(world *World, s32 IslandCount)
{
 TopologyRingWithChords(World, IslandCount, World->Config.K.XJitter);
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
