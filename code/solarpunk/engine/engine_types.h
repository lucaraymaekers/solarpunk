/* date = November 21st 2026 */

#ifndef ENGINE_TYPES_H
#define ENGINE_TYPES_H

// NOTE(L4): Contract types, spec 03_TECH_SPEC.md sections 4.2 and 4.3.
// Rules: no pointers anywhere in these structs (indices, -1 = none), no function pointers.
// The whole world is plain data so a DLL reload or a memcpy of it is always valid.

//~ Identifiers
typedef s32 island_id;     // 0..MAX_ISLANDS-1, -1 = none
typedef s32 pod_id;        // index into world.Pods, -1 = none
typedef s32 resource_id;   // 0..MAX_RESOURCES-1, -1 = none

//~ Enums
enum power_state
{
 PowerState_SolarHarvesting,
 PowerState_NightReserve,
 PowerState_Sleeping,
 PowerState_StormOutage,
};
typedef enum power_state power_state;

enum sync_mode
{
 SyncMode_MeshGossip,
 SyncMode_Sneakernet,
 SyncMode_Hybrid,
};
typedef enum sync_mode sync_mode;

enum mid_flight_policy
{
 MidFlight_PrelaunchEnergyCheck,
 MidFlight_PassiveMechanicalLanding,
};
typedef enum mid_flight_policy mid_flight_policy;

enum emergency_borrow_policy
{
 Borrow_FluidOnly,
 Borrow_NeighborPullFirst,
 Borrow_FluidThenNeighbor,
};
typedef enum emergency_borrow_policy emergency_borrow_policy;

enum need_model_kind
{
 NeedModel_AbstractCadence,
 NeedModel_StockBased,
};
typedef enum need_model_kind need_model_kind;

enum pod_kind
{
 PodKind_Targeted,
 PodKind_Scout,
};
typedef enum pod_kind pod_kind;

// NOTE(L4): Free = 0 so a zeroed pool is an empty pool. Delivered/Expired pods stay one tick
// (so the UI can see them), then the world sets them back to Free.
enum pod_status
{
 PodStatus_Free = 0,
 PodStatus_Queued,
 PodStatus_InTransit,
 PodStatus_Delivered,
 PodStatus_Expired,
};
typedef enum pod_status pod_status;

enum hold_reason
{
 HoldReason_None = 0,
 HoldReason_NeighborsAsleep,
 HoldReason_LowEnergy,
 HoldReason_Storm,
 HoldReason_Cooldown,
};
typedef enum hold_reason hold_reason;

enum reroute_reason
{
 Reroute_None = 0,
 Reroute_Asleep,
 Reroute_Storm,
 Reroute_LowEnergy,
 Reroute_LoopAvoid,
};
typedef enum reroute_reason reroute_reason;

//~ Config
// NOTE(L4): One f32 per section-9 symbol. Filled by DefaultConfig().
typedef struct physics_constants physics_constants;
struct physics_constants
{
 // Solar / energy
 f32 SunPhase0, EpsDawn, EMax, EInitial;
 f32 KHarvest, PIdle, PNight, KHold, PStorm;
 f32 CDay, CNight, Alpha;
 // Edge cost / scoring
 f32 Wd, WHop, We, Ws, WPath;
 f32 ESafe, MSleep, TStale, Attenuation, SharpNeed;
 // Need model A
 f32 WPro, Gamma, LambdaU, TapBoost, DeltaU;
 f32 Beta, TauMin, TauMax, TapTauNudge, OwnShare;
 f32 TauTrueMin, TauTrueMax, DemandJitter, ProductionPerMaker;
 // Need model B
 f32 TargetCoverDays, SurplusCoverDays, PerCapita, PerCapitaBeta, TapPerCapitaNudge;
 // Topology
 f32 YMin, YMax, XJitter;
};

typedef struct engine_config engine_config;
struct engine_config
{
 u64 Seed;
 f32 DayLengthSeconds;
 need_model_kind NeedModel;                     // D1 (OPEN)
 sync_mode SyncMode;                            // D2 (OPEN)
 f32 GossipIntervalSeconds;
 mid_flight_policy MidFlightPolicy;             // D3 (OPEN)
 emergency_borrow_policy EmergencyBorrowPolicy; // D4 (OPEN)
 s32 ResourceCount;                             // D5 (OPEN), <= MAX_RESOURCES
 s32 IslandCount;                               // D13 (OPEN), <= MAX_ISLANDS
 s32 Population;                                // per island
 b32 EnableSpoilage;                            // D10
 b32 EnableWeightLimits;                        // D10
 b32 EnableRobotIslands;                        // D9
 b32 RebindOnSatisfied;                         // D16
 s32 DefaultReservedQty, DefaultFluidQty;
 s32 BatchSize;                                 // surplus units needed to build a pod
 f32 FluidClaimNeedThreshold;                   // theta
 f32 DestNeedMin;
 f32 MinLaunchReserve;
 f32 LaunchCooldownSeconds;                     // per (origin, dest, resource)
 f32 ScoutCooldownSeconds;                      // per (origin, neighbour)
 s32 ScoutQty;
 s32 WarmupDays;                                // days simulated headless before a scenario starts
 s32 MaxQueueCapacity;                          // <= MAX_QUEUE
 f32 PodSpeed;                                  // normalized distance per sim-second
 f32 PodMaxTtlSeconds;
 physics_constants K;
};

//~ Topology and knowledge
typedef struct neighbor_link neighbor_link;
struct neighbor_link
{
 island_id TargetId;
 f32 Distance;
 b32 WrapsGlobe;
};

// NOTE(L4): Neighbour list added to knowledge_entry (not in the v3 spec) so a node can build
// its belief graph for Dijkstra (3.4) without ever seeing the world's link table: an island's
// links travel with its knowledge, by gossip or on pods. Static data, so it never goes stale.
typedef struct knowledge_entry knowledge_entry;
struct knowledge_entry
{
 b32 Valid;                                     // false = node knows nothing about this island
 power_state PowerState;
 f32 EnergyReserve;                             // quantized 0..100
 f32 Needs[MAX_RESOURCES];
 f32 UpdatedAt;                                 // sim time at the origin island
 s32 HopCount;
 island_id NeighborIds[MAX_NEIGHBORS];
 f32 NeighborDistances[MAX_NEIGHBORS];
 s32 NeighborCount;
};

typedef struct maker maker;
struct maker
{
 b32 Used;
 s32 VillagerId;
 resource_id Craft;
 f32 UpdatedAt;                                 // last-write-wins timestamp
};

// NOTE(L4): Option A's hidden ground truth (next demand event time) is NOT here; it lives in
// world.DemandNextAt so a node cannot read it.
typedef struct need_state need_state;
struct need_state                               // per resource; fields used depend on Config.NeedModel
{
 // Option A (abstract cadence)
 f32 ElapsedSec, TauHat, U;
 f32 ProductionAccum;                           // fractional production carried between ticks
 // Option B (stock)
 f32 Stock, PerCapitaHat;
 // both
 f32 Score;                                     // cached 0..100
};

//~ Pods
typedef struct fluid_claim fluid_claim;
struct fluid_claim
{
 island_id IslandId;
 s32 Qty;
 f32 Timestamp;
};

typedef struct pod pod;
struct pod
{
 pod_status Status;
 pod_kind Kind;
 resource_id Resource;
 island_id OriginId, DestId;
 s32 ReservedQty, FluidQty;                     // ReservedQty immutable en route
 s32 InitialReservedQty, InitialFluidQty;
 f32 Weight, Freshness;                         // only if flags on
 f32 CreatedAt, ExpiresAt;
 island_id CustodianId, FromId, ToId;
 b32 WrapsGlobe;
 f32 Distance;                                  // length of the active link
 f32 Progress;                                  // 0..1 on active link
 island_id Visited[MAX_POD_PATH]; s32 VisitedCount;
 island_id PlannedPath[MAX_POD_PATH]; s32 PlannedPathCount;   // advisory, for UI
 s32 RerouteCount;
 fluid_claim FluidClaims[MAX_CLAIMS]; s32 FluidClaimCount;
 knowledge_entry Carried[MAX_ISLANDS];          // sneakernet/hybrid ledger: a COPY made at launch
 f32 LaunchedAt;                                // for delivery-time stats
 b32 SawOutage;                                 // path crossed an asleep/stormed island (stats)
 b32 UsedNightRelay;
};

//~ Node
/* What the world lets a node perceive about a direct neighbour. By value. No pointers. */
typedef struct neighbor_obs neighbor_obs;
struct neighbor_obs
{
 island_id NeighborId;
 b32 IsReachable;                               // awake and storm-free (visible beacon, not data)
 power_state ObservedPowerState;
 b32 HasReserveEstimate;                        // only if a gossip/optical link is active
 f32 EstimatedReserve;
 f32 Distance;
 b32 WrapsGlobe;
};

typedef struct island_node island_node;
struct island_node
{
 island_id Id;
 // public-ish info (for UI and scoring)
 f32 X, Y;                                      // x wraps [0,1), y in [YMin, YMax]
 s32 Population;
 b32 IsAutomatedRobotIsland;
 s32 BiomeIndex;
 // physics state
 f32 Sun;                                       // I(t)
 f32 Energy;                                         // reserve 0..EMax
 f32 EDelta;                                    // last tick's dE/dt, for the UI
 power_state PowerState;
 f32 StormUntil;                                // sim time; 0 = no storm
 // makers and economy
 maker Makers[MAX_MAKERS];
 need_state Need[MAX_RESOURCES];
 s32 Shed[MAX_RESOURCES];                       // surplus waiting to become a pod
 s32 Dropped[MAX_RESOURCES];                    // append-only drop counters (the "drop log")
 // network state
 neighbor_link Neighbors[MAX_NEIGHBORS]; s32 NeighborCount;
 knowledge_entry Knowledge[MAX_ISLANDS];        // this node's beliefs. Knowledge[Id] is always itself.
 pod_id Queue[MAX_QUEUE]; s32 QueueCount;       // waiting queue: indices into the pod pool (custody)
 f32 LaunchCooldownUntil[MAX_ISLANDS][MAX_RESOURCES];
 f32 ScoutCooldownUntil[MAX_ISLANDS];
 f32 SuppressUntil[MAX_ISLANDS][MAX_RESOURCES];
 f32 SuppressAmount[MAX_ISLANDS][MAX_RESOURCES];
 random_series Rng;                             // per-island stream, seeded from world seed + Id
};

//~ Node call results (D18 boundary: the node decides, the world acts and emits events)
typedef struct arrival_result arrival_result;
struct arrival_result
{
 b32 IsFinal;                                   // pod reached DestId: world marks it Delivered
 s32 FluidClaimedQty;                           // > 0: node claimed this many Fluid units
};

typedef struct hop_decision hop_decision;
struct hop_decision
{
 island_id NextHop;                             // -1 = hold in queue
 hold_reason HoldReason;                        // when NextHop == -1
 b32 IsReroute;                                 // differs from the previous plan
 reroute_reason RerouteReason;
};

//~ Events
enum sim_event_kind
{
 SimEvent_PodCreated,
 SimEvent_PodLaunched,
 SimEvent_PodRerouted,
 SimEvent_PodHeldDtn,
 SimEvent_PodReturned,
 SimEvent_FluidClaimed,
 SimEvent_PodDelivered,
 SimEvent_PodExpired,
 SimEvent_GossipPing,
 SimEvent_IslandState,
 SimEvent_StormStarted,
 SimEvent_StormEnded,
 SimEvent_NeedRaised,
 SimEvent_QueueBackpressure,
 SimEvent_Count,
};
typedef enum sim_event_kind sim_event_kind;

typedef struct sim_event sim_event;
struct sim_event
{
 sim_event_kind Kind;
 f32 T;
 pod_id Pod;
 island_id A, B;                                // meaning depends on Kind (from/to, island/neighbour, ...)
 resource_id Resource;
 s32 Qty, Qty2;                                 // reserved/fluid, or claimed qty
 s32 Reason;                                    // hold_reason / reroute_reason / need source
 s32 FromState, ToState;                        // for IslandState
};

//~ Actions from the UI
enum villager_action_kind
{
 VillagerAction_DropSurplus,
 VillagerAction_TapRunningLow,
 VillagerAction_SetCraft,
};
typedef enum villager_action_kind villager_action_kind;

typedef struct villager_action villager_action;
struct villager_action
{
 villager_action_kind Kind;
 island_id IslandId;
 s32 VillagerId;
 resource_id Resource;                          // new craft for SetCraft
 s32 Qty;                                       // only for DropSurplus
};

enum chaos_action_kind
{
 ChaosAction_Storm,
 ChaosAction_EndAllStorms,
 ChaosAction_SetSyncMode,
 ChaosAction_SetConfig,
};
typedef enum chaos_action_kind chaos_action_kind;

typedef struct chaos_action chaos_action;
struct chaos_action
{
 chaos_action_kind Kind;
 island_id Islands[MAX_ISLANDS]; s32 IslandCount; f32 DurationSec;   // Storm
 sync_mode Mode;                                                     // SetSyncMode
 engine_config ConfigPatch;                                          // SetConfig: whole config; caller copies, edits, passes
};

//~ World
typedef struct world_stats world_stats;
struct world_stats
{
 s32 UnitsProduced;                             // units that entered a shed (drain + villager drops)
 s32 UnitsDelivered, UnitsClaimedEnRoute, UnitsExpired;
 s32 DeliveriesCompleted;
 s32 DeliveriesCompletedDuringOutage;           // delivered while >=1 island on its path was asleep/stormed
 s32 DeliveriesViaNightRelay;
 s32 Reroutes, PodsHeldDtn;
 f32 EnergySpentTotal;
 f32 EnergyPerUnitDelivered;
 s32 FluidFlightsSaved;                         // fluid units delivered en route without an extra flight
 f32 TotalDeliverySeconds;                      // avg = TotalDeliverySeconds / DeliveriesCompleted
};

typedef struct world_link world_link;
struct world_link
{
 island_id A, B;
 f32 Distance;
 b32 WrapsGlobe;
 b32 Active;
};

typedef struct world world;
struct world
{
 engine_config Config;
 f32 SimTime, SunPhase;
 s32 TickCount;
 random_series Rng;                             // world-level stream (demand events, storms, jitter)
 island_node Islands[MAX_ISLANDS]; s32 IslandCount;
 pod Pods[MAX_PODS];
 world_link Links[MAX_ISLANDS*MAX_NEIGHBORS]; s32 LinkCount;
 // hidden ground truth, never shown to nodes
 f32 DemandNextAt[MAX_ISLANDS][MAX_RESOURCES];  // Option A: next household demand event
 f32 DemandTauTrue[MAX_ISLANDS][MAX_RESOURCES]; // Option A: true cadence
 f32 NextGossipAt;
 world_stats Stats;
 sim_event Events[MAX_EVENTS]; u32 EventHead, EventTail;   // ring buffer, Head = next write
};

#endif // ENGINE_TYPES_H
