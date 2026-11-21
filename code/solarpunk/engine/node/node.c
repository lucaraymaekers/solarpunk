//-
// Node logic: NodeStep, pod building, next hop, arrival. Spec 3.5, 3.6, 6, 7.
// OWNER: lane L1. This file is a placeholder created by L4 so the contract builds;
// L1 replaces the bodies. ISOLATED node file: only Self, Config, Obs and the passed pod.
//-

internal void
NodeInit(island_node *Self, const engine_config *Config)
{
 // TODO(L1): at t = 0 a node knows only itself (spec 3.5).
 knowledge_entry *Own = &Self->Knowledge[Self->Id];
 Own->Valid = true;
 Own->PowerState = Self->PowerState;
 Own->EnergyReserve = Self->Energy;
 Own->NeighborCount = Self->NeighborCount;
 for(s32 N = 0; N < Self->NeighborCount; N += 1)
 {
  Own->NeighborIds[N] = Self->Neighbors[N].TargetId;
  Own->NeighborDistances[N] = Self->Neighbors[N].Distance;
 }
}

internal void
NodeStep(island_node *Self, const engine_config *Config, const neighbor_obs *Obs, s32 ObsCount, f32 SimTime)
{
 // TODO(L1): refresh Knowledge[Self->Id] (needs, power, reserve, UpdatedAt), decay suppression.
}

internal b32
NodeTryBuildPod(island_node *Self, const engine_config *Config, resource_id R,
                const neighbor_obs *Obs, s32 ObsCount, pod *Pod, f32 SimTime)
{
 // TODO(L1): destination scoring (3.5), scout rule (3.6), split sizing (6.2).
 b32 Result = false;
 return Result;
}

internal hop_decision
NodeDecideNextHop(island_node *Self, const engine_config *Config, pod *Pod,
                  const neighbor_obs *Obs, s32 ObsCount, f32 SimTime)
{
 // TODO(L1): Dijkstra on the belief graph, two passes (3.4), loop avoidance (7.2).
 hop_decision Result = {0};
 Result.NextHop = -1;
 Result.HoldReason = HoldReason_NeighborsAsleep;
 return Result;
}

internal arrival_result
NodeOnPodArrived(island_node *Self, const engine_config *Config, pod *Pod, f32 SimTime)
{
 // TODO(L1): merge Pod->Carried, NeedOnDelivery on final, Fluid claim (6.3) otherwise.
 arrival_result Result = {0};
 Result.IsFinal = (Pod->DestId == Self->Id);
 return Result;
}
