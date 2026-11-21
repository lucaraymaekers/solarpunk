/* date = November 21st 2026 */

#ifndef ENGINE_H
#define ENGINE_H

// NOTE(L4): The single include for the engine. The UI includes ONLY this.
// Expects base/base.h and rl/rl_random.h to be included first (unity build).
// UI -> engine only through: reading `const world *`, WorldDispatchVillager,
// WorldDispatchChaos, WorldStep and WorldPopEvent.

#include "solarpunk/engine/engine_defaults.h"
#include "solarpunk/engine/engine_types.h"

//~ Shared math (pure, no state)
internal f32 EngineWrapDx(f32 X0, f32 X1);                        // shortest |dx| on the cylinder
internal f32 EngineWrapDistance(f32 X0, f32 Y0, f32 X1, f32 Y1); // spec 3.4 d_ab
internal f32 EngineSunIntensity(f32 X, f32 SunPhase);             // spec 3.1 I = cos(2pi(x - phi))

//~ World API (engine_world.c, lane L4)
internal engine_config DefaultConfig(void);
internal void WorldInit(world *World, const engine_config *Config);  // zeroes World, builds topology
internal void WorldStep(world *World, f32 Dt);                       // Dt must be ENGINE_TICK_SECONDS
internal void WorldDispatchVillager(world *World, const villager_action *Action);
internal void WorldDispatchChaos(world *World, const chaos_action *Action);
internal b32  WorldPopEvent(world *World, sim_event *Event);         // false when empty
internal void MockWorld(world *World);                               // fake but contract-correct state for UI work

//- Read-only helpers for the UI and tests
internal b32 WorldIsAwake(const world *World, island_id Island);
internal f32 WorldAvgKnowledgeAge(const world *World, island_id Island);
internal s32 WorldUnitsInShed(const world *World);
internal s32 WorldUnitsInPods(const world *World, pod_status Status);
internal b32 WorldCheckConservation(const world *World);             // spec 0.6 identity

//~ Topology (engine_topology.c, lane L4)
internal void TopologyBuildDefault(world *World);                   // demo preset layout
internal void TopologyGenerate(world *World, s32 IslandCount);      // N islands, seeded from World->Rng
internal b32  TopologyIsBiconnected(const world *World);
internal s32  TopologyWrapEdgeCount(const world *World);

//~ Scenarios (engine_scenarios.c, lane L4)
enum scenario_kind
{
 Scenario_None,
 Scenario_A_NightRelay,
 Scenario_B_StormHeals,
 Scenario_C_FluidSaves,
 Scenario_D_SyncCompare,
 Scenario_Count,
};
typedef enum scenario_kind scenario_kind;
internal void ScenarioLoad(world *World, scenario_kind Kind);

//~ Node layer (engine/node/). ISOLATED: these never receive the world, only the node,
// plain-data neighbour observations and the pod being handled.

//- Energy (node_energy.c, lane L4)
internal void  EnergyTick(island_node *Self, const engine_config *Config, f32 SimTime, f32 Dt); // E, EDelta, PowerState from Self->Sun/StormUntil
internal b32   EnergyIsAwake(const island_node *Self);
internal f32   EnergyLaunchCost(const island_node *Self, const engine_config *Config, f32 Distance);
internal b32   EnergyCanLaunch(const island_node *Self, const engine_config *Config, f32 Distance);
internal b32   EnergyTryLaunch(island_node *Self, const engine_config *Config, f32 Distance); // guard + deduct; false = hold (LowEnergy)

//- Need model (node_need.c, lane L4). Each switches on Config->NeedModel.
internal void NeedInit(island_node *Self, const engine_config *Config);
internal void NeedTick(island_node *Self, const engine_config *Config, f32 Dt, f32 SimTime);
internal f32  NeedScore(const island_node *Self, const engine_config *Config, resource_id R);   // 0..100
internal void NeedOnDemandEvent(island_node *Self, const engine_config *Config, resource_id R, f32 Interval, f32 SimTime); // Option A
internal void NeedOnDelivery(island_node *Self, const engine_config *Config, resource_id R, s32 Qty, f32 SimTime, b32 IsFinal);
internal void NeedOnFluidClaim(island_node *Self, const engine_config *Config, resource_id R, s32 Qty);
internal void NeedOnTap(island_node *Self, const engine_config *Config, resource_id R, f32 SimTime);
internal s32  NeedDrainToShed(island_node *Self, const engine_config *Config, resource_id R);  // adds to Shed, returns units added

//- Node logic (node.c, node_router.c, node_store.c, lane L1)
internal void NodeInit(island_node *Self, const engine_config *Config);  // own Knowledge[Id], cooldowns
internal void NodeStep(island_node *Self, const engine_config *Config, const neighbor_obs *Obs, s32 ObsCount, f32 SimTime);
// Pod is a zeroed free pool slot. Fill it and take units from Self->Shed. Return false to build nothing.
internal b32  NodeTryBuildPod(island_node *Self, const engine_config *Config, resource_id R,
                              const neighbor_obs *Obs, s32 ObsCount, pod *Pod, f32 SimTime);
internal hop_decision NodeDecideNextHop(island_node *Self, const engine_config *Config, pod *Pod,
                                        const neighbor_obs *Obs, s32 ObsCount, f32 SimTime);
// Merge Pod->Carried; on final destination call NeedOnDelivery; else maybe claim Fluid (6.3).
internal arrival_result NodeOnPodArrived(island_node *Self, const engine_config *Config, pod *Pod, f32 SimTime);
// Gossip: Incoming is a copy of a neighbour's whole knowledge table (MAX_ISLANDS entries).
internal void NodeMergeKnowledge(island_node *Self, const engine_config *Config, const knowledge_entry *Incoming, f32 SimTime);
internal void NodeOnVillagerSetCraft(island_node *Self, s32 VillagerId, resource_id Craft, f32 SimTime); // LWW craft table

#endif // ENGINE_H
