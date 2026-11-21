# Technical, Protocol & Architecture Specification (v3, C)

> **Audience:** Claude Code and the 4 humans. This is the engineering source of truth.
> **v3 changes:** ported from TypeScript to C inside Luca's codebase. Math, protocol and tests are unchanged from v2.1; types, memory model, file layout, build targets and tests are new.
> **Open decisions** live in `02_DECISIONS_TBD.md`. **Never hardcode an open decision; expose it through `engine_config` or the need-model switch.**
> Project name is a placeholder; do not hardcode it in identifiers (`world`, `island_node`, `pod` are fine; `SolarWeave*` is not).

---

## 0. Hard Architectural Invariants (non-negotiable)

1. **Zero God-Mode (strict node isolation).** Code in `engine/node/` may only use: (a) the `island_node` it was called on, (b) the `neighbor_obs` array handed in by the world, (c) data carried by an arriving `pod`. All of these are plain structs copied by value (or `const *` to a stack copy the world made for the call). No node function receives, includes or references `world`, and no node holds a pointer to another node.
2. **Engine/UI decoupling.** `engine/**` is pure deterministic C. It includes only `base/` and `rl/rl_random.h`. No renderer, UI, platform, OpenGL, `windows.h`, clock reads, `rand()`, or `malloc`. All randomness goes through `random_series`. Same seed + same input sequence = identical history **on the same build** (float results can differ across compilers/optimisation flags; do not promise cross-build identical replays).
3. **Reserved cargo conservation.** For any pod, `ReservedQty` is never decremented by an intermediate island; intermediates may only claim from `FluidQty`.
4. **No central server, no island admin.** No singleton coordinator, global scheduler, or privileged role inside the node layer. Terminal actions (drop, tap, craft change) are conflict-free local operations. (`WorldStep` is the *simulator's* loop, standing in for physics and time; it is not part of the network.)
5. **Parameterize every open decision** via `engine_config` or the need-model switch.
6. **Conservation of cargo.** Units are never created or destroyed by routing. At every tick: `Produced = InShed + InQueues + InFlight + DeliveredToDest + ClaimedEnRoute + Expired`. `Produced` (`Stats.UnitsProduced`) counts only units that **entered a shed** (surplus drained from production plus villager drops); production kept at home (`OwnShare`, or stock below the surplus line) is outside the identity. Checked by `WorldCheckConservation`; violation hits `Assert` in dev builds and fails the test.
7. **Hot-reload safe.** The whole simulation state is one `world` struct living in `app_state` in the permanent arena. It contains **no pointers** (use indices, `-1` = none) and no function pointers, so a DLL reload or a `memcpy` of the struct is always valid.

---

## 1. Worldview (summary)
See `01_OVERVIEW.md`. Engine-relevant facts: about 100 people per island, cylindrical longitude `x in [0,1)`, no batteries (only leftover daytime energy is stored), island sleeps at `E = 0`, each island has 3 to 4 neighbours, villagers have exactly three gestures (drop, tap, craft).

---

## 2. Real-world concepts adapted (honest framing)

The project is **inspired by** these; it does not implement their wire formats. Say "DTN-inspired" in the pitch, not "implements BPv7".

| Concept | Where it shows up |
|---|---|
| **Delay/Disruption-Tolerant Networking** (bundles, store-and-forward, custody transfer) | Pods are bundles with a header; the waiting queue holds them when the next hop is asleep; custody stays with the launching island until landing. |
| **Physical Internet** (standard modular containers routed hop by hop) | Pods are uniform crates; each hop decides locally. |
| **Split-payload manifest** (this project's own extension) | `(ReservedQty, FluidQty)` per pod. |
| **Anti-entropy gossip / epidemic routing / data mules** | `Config.SyncMode`: gossip, sneakernet, hybrid. |
| **Local-first, admin-free state** (CRDT-style) | Per-island last-write-wins craft table + append-only drop counter. Deliberately tiny (about 1h, see section 8). |
| **Hop-by-hop shortest path + gradient diffusion** | `engine/node/router`. |

---

## 3. Mathematical specification

(All values `f32`. Time in simulated seconds. `Pi32` from `base_core.h`; use `cosf`.)

### 3.1 Solar cycle
Island `i` at longitude `x_i`; sun longitude `phi(t) = fmodf(phi0 + t / T_day, 1)`.
`I_i(t) = cosf(2*Pi32*(x_i - phi(t)))` in [-1, 1]. Daylight when `I_i > eps_dawn` (0.05) and no storm.

### 3.2 Battery-free surplus energy
`E_i in [0, E_max]`, `E_max = 100`. Clamp to bounds every tick.

- **Daylight, no storm:** `dE/dt = k_harvest * I - P_idle` (live solar first covers idle load; only the leftover is stored).
- **Night, `E > 0`:** `dE/dt = -P_night - k_hold * QueueCount`
- **Storm:** `dE/dt = -P_storm` (solar = 0; launches halted)

Launch cost across a link of normalized distance `d`: `dE = c * (1 + alpha * d)` with `c = c_day` when sunlit and `c = c_night` when running on stored reserve (`c_night > c_day`: spending finite stored energy). Launch guard depends on power state: in `PowerState_NightReserve` a launch needs `E >= dE + minLaunchReserve` (keep a floor so the island does not sleep itself); in `PowerState_SolarHarvesting` a launch needs only `E >= dE` (no floor: live sun refills the reserve, so an island waking at dawn with `E = 0` can launch queued pods as soon as it has accumulated enough for one flight). If `E` is momentarily too low the pod waits (`SimEvent_PodHeldDtn`, reason `HoldReason_LowEnergy`).

At night when `E <= 0` the island enters `PowerState_Sleeping` until `I > eps_dawn`.

### 3.3 Need model (pluggable, decision D1 is OPEN)

The node owns need state (`need_state`, section 4.2) and the router and sync layer only ever call `NeedScore(Node, r)` in [0,100] and the hooks below. Which implementation runs is a `switch(Config->NeedModel)` inside `engine/node/need.c`. **Two implementations are specified; the team picks (or later hybridizes) at the D1 meeting. Build the switch first; either implementation is small.**

#### Option A: `NeedModel_AbstractCadence`
There is no stock. The *world* (not the node) drives a hidden ground-truth process per `(island, resource)`: a "household demand event" fires about every `TauTrue * (1 +/- jitter)` sim-seconds (seeded). The *node* only observes these events (`NeedOnDemandEvent`) and its own taps.

- The hidden clock is **world-owned** (`world.DemandNextAt[island][r]`, `world.DemandTauTrue[island][r]`), never stored in the node, so a node cannot read its own ground truth. The world calls `NeedOnDemandEvent(Node, Config, r, Interval, SimTime)` when it fires.
- Node tracks `ElapsedSec` since the last demand event, `TauHat` (learned interval), and manual urgency `U` (from taps, decays at `lambda_U`).
- `N = Min(100, W_pro * powf(Min(1.25, ElapsedSec / TauHat), gamma) + U)`
- **Important:** `TauHat` is learned from intervals between *demand events*, **not** between deliveries. A delivery that arrives before/around the demand event "absorbs" it (no shortage recorded) but the event still updates `TauHat`. Otherwise the system would only learn how often it happened to deliver.
- On a demand event: `TauHat = Clamp(tau_min, (1-beta)*TauHat + beta*interval, tau_max)`.
- A tap sets `U += tapBoost` and multiplies `TauHat` by `tapTauNudge` (a tap means the cadence was too slow).
- A partial Fluid claim of `q` units: `ElapsedSec` reduced proportionally, `U` reduced by `delta_U * q`, `TauHat` untouched.
- Own-needs-first: a fixed fraction `OwnShare` of each maker's production stays home; the rest enters the shed.

#### Option B: `NeedModel_StockBased`
Real stock per resource.

- `Stock[r]` changes by `production(makers) - consumption(population, PerCapita[r])` per sim-second (seeded noise).
- `DaysOfCover = Stock[r] / (PerCapitaHat[r] * population)` in sim-days.
- `N = Min(100, 100 * powf(Clamp(0, (TargetCoverDays - DaysOfCover) / TargetCoverDays, 1), gamma) + U)`
- `PerCapitaHat` learned by EWMA from observed stock drops (this is the "learns rhythm" feature).
- Surplus to shed = `Stock[r]` above `SurplusCoverDays` of cover (own needs first, naturally).
- Taps add `U` and raise `PerCapitaHat` slightly.
- Needs balancing data (per-capita rates, production rates). Budget 3 to 5h; start from `engine_defaults.h`.

> Until D1 is decided, ship **Option A** as the default config (smallest) while keeping the switch clean.

**As implemented (L4, 2026-11-21, `engine/node/node_need.c`). Choices the text above leaves open:**
- **One hidden demand clock drives both options.** `world.DemandNextAt/DemandTauTrue/DemandLastAt` fire `NeedOnDemandEvent(Node, Config, r, Interval, SimTime)` per `(island, r)` every `TauTrue * (1 +/- DemandJitter)`. Option A learns `TauHat` from `Interval`. Option B consumes `ConsumePerEvent` (1) units of stock per event and learns `PerCapitaHat = EWMA(ConsumePerEvent * DayLength / (Interval * Population))`. So in B too, the node never reads its true consumption rate.
- **Option A `ElapsedSec`** = time since the last *final* delivery. A demand event never resets it. It sets `Covered = false` and counts a **shortage** (`need_state.Shortages`) if no delivery arrived since the previous event; otherwise the delivery "absorbed" it.
- **`U` decays linearly** at `lambda_U` per second (a 60-point tap fades in 120 s, one sim-day). Taps cap `U` at 100. Deliveries and claims reduce `U` by `delta_U * qty`.
- **Fluid claim (A):** `ElapsedSec *= 1 - qty/BatchSize`.
- **Production:** each maker with `Craft == r` produces `ProductionPerMaker` units/s. Option A: `(1 - OwnShare)` of it accumulates in `ProductionAccum`, and whole units drain to the shed (only while awake). Option B: production goes into `Stock`, and stock above `SurplusCoverDays` of the island's own learned use drains to the shed.
- **Option B starting state:** `Stock = TargetCoverDays * PerCapita * Population`, `PerCapitaHat = PerCapita` (the prior).

Measured with `SolarSim test` (12 sim-days, default seed, no deliveries): Option A's mean relative error of `TauHat` vs the hidden cadence drops **0.70 -> 0.05**; Option B's `PerCapitaHat` error drops **0.85 -> 0.33** (slower: one event per cadence and a lower EWMA weight). For D1 balancing: with no deliveries every Option A need saturates near 94 (the 1.25 cap), and with 2 makers per island the sheds fill at about 6 units per sim-day per maker resource.

### 3.4 Hop-by-hop edge cost
When island `u` evaluates edge `(a, b)` using its *own belief* about `b` (direct observation if `a == u`, else `u.Knowledge[b]`):

`C = w_d*d_ab + w_hop + P_state(b) + P_energy(b) + P_stale(u,b) + P_loop(b)`

- `d_ab`: cylindrical wrap distance `sqrtf(Square(Min(|dx|, 1-|dx|)) + Square(dy))`.
- `P_state(b)`: infinity if `b` is asleep/storm in the **strict pass**; in the **fallback pass** a finite `M_sleep` so a pod can advance to the edge of a dark zone and wait there (store-and-forward).
- `P_energy(b) = w_e * Square(Max(0, (E_safe - E_b_est) / E_safe))` (avoid relays about to run dry).
- `P_stale(u,b) = w_s * Min(1, age / T_stale)` (uncertainty in sneakernet mode).
- `P_loop(b)`: infinity if `b` is in `pod.Visited` unless no loop-free path exists.

Run Dijkstra on the node's local belief graph (at most `MAX_ISLANDS` vertices: use a plain O(V^2) array version, no heap needed). **Recompute at every hop.**

### 3.5 Destination scoring (for new pods)
For island `u` with shed surplus of resource `r`, over islands `j` in `u.Knowledge`:
`score_j = N_j,r * powf(attenuation, hops(u,j)) - w_path * pathCost(u,j) - Suppression(j,r)`
Pick `argmax` among `j` with `N_j,r >= destNeedMin` (default 30). `Suppression` is section 7.3 pod-sighting suppression. If none qualifies, try the scout rule (3.6); if that does not fire either, hold the surplus in the shed.

At `t = 0` every node knows **only itself** (no seeded knowledge of other islands' needs; that would be god-mode).

### 3.6 Scout / diffusion pods (cold-start deadlock fix)
Without this, sneakernet mode deadlocks: knowledge only travels on pods, but pods only launch when knowledge shows a need, so nothing ever launches.

**Trigger** (island awake, passes the 3.2 launch guard, scout cooldown elapsed for that neighbour): no island `j` meets `destNeedMin` for resource `r` **and** either
- `Shed[r] >= 2 * BatchSize` (surplus is piling up), or
- any awake neighbour's knowledge entry is missing or older than `T_stale` (we are blind there).

**Pod:** `Kind = PodKind_Scout`, `ReservedQty = 0`, `FluidQty = ScoutQty` (default 2; or `BatchSize` when triggered by the surplus-pileup condition, as a plain gift to the neighbourhood). `DestId` = awake neighbour with the oldest or missing `UpdatedAt` (tie-break: steepest known need gradient, then the island's `random_series`). It is a normal pod otherwise: it carries knowledge, can be claimed en route, is delivered on arrival, and obeys conservation.

Result: pods keep circulating knowledge in pure sneakernet mode and after long partitions, with no radio and no central bootstrapping.

---

## 4. Architecture and contracts

### 4.1 Layers and files

```
code/solarpunk/
  solarpunk.c / solarpunk.h      app entry: UpdateAndRender, app_state (includes world), input -> actions, draw
  engine/                        PURE C. includes only base/ and rl/rl_random.h
    engine.h                     the single include for the engine (types + API). The UI includes ONLY this.
    engine_types.h               all structs/enums in 4.2 and 4.3
    engine_defaults.h            all constants in section 9 as DEFAULT_* #defines (DefaultConfig() lives in engine_world.c and copies them into engine_config)
    engine_topology.c            island layout, neighbour graph, biconnectivity check
    engine_world.c               WorldInit, WorldStep (section 5), WorldDispatch*, conservation check, scenarios
    engine_scenarios.c           scenario presets (see 05_DEMO_PITCH.md)
    node/                        ISOLATED. may NOT include or mention `world` or other nodes
      node.c                     NodeStep, NodeOnPodArrived, NodeDecideNextHop, NodeTryBuildPod
      node_energy.c              power state machine, harvest/drain, launch guard
      node_router.c              belief-graph Dijkstra, edge cost, destination scoring
      node_need.c                need model switch (Option A / B)
      node_store.c               craft table (LWW), drop counter, knowledge merge
  ui/                            draws from `const world *`; may include engine.h only
    ui_world.c                   sky, islands, halos, links, pods, trace highlight
    ui_tablet.c                  villager tablet (rl_ui)
    ui_controls.c                top bar, narrative bar, KPI panel
  sim/
    solar_sim.c                  console app: `test` and `sim` commands (see section 12 and 04_BUILD_PLAN.md)
```

`node.c`, `node_router.c` and `node_store.c` (L1) were created as placeholders by L4 so the contract builds; L1 replaces their bodies.

All `.c` files are included from one translation unit (`solarpunk.c` for the app, `solar_sim.c` for the console tool), matching the rest of the codebase. `engine.h` plus the `.c` files in `engine/` form an include group that both entry points pull in with a single `#include "solarpunk/engine/engine_unity.c"` (a file that `#include`s all engine `.c` files; create it).

UI to engine only through: reading `const world *World`, `WorldDispatchVillager(World, &Action)`, `WorldDispatchChaos(World, &Action)`, `WorldStep(World, Dt)`, and draining `WorldPopEvent(World, &Event)`.

### 4.2 Memory model and core types (`engine/engine_types.h`)

> **Authoritative definitions are in `code/solarpunk/engine/engine_types.h` and `engine_defaults.h`.** The blocks below are the v3 design; differences made while writing the contract (2026-11-21):
> - `knowledge_entry` also carries the island's neighbour list (`NeighborIds`, `NeighborDistances`, `NeighborCount`) so a node can build its belief graph (3.4) without seeing the world's link table.
> - `need_state` has no `NextDemandAt` (moved to `world.DemandNextAt`, see 3.3) and gains `ProductionAccum` (fractional production between ticks).
> - `pod` gains `Distance` (length of the active link); the claim record is a named `fluid_claim` struct.
> - `engine_config` gains `ResourceCount` (D5), `IslandCount` (D13) and `Population`. `physics_constants` has one f32 per section-9 symbol plus the new ones listed there.
> - `world` gains `DemandNextAt`, `DemandTauTrue` (hidden ground truth) and `NextGossipAt`. Event ring: `EventHead` = total pushed, `EventTail` = total popped, slot = index `% MAX_EVENTS`.
> - `hold_reason` / `reroute_reason` start with a `_None = 0` value; all enums are typedef'd.
> - New result structs `arrival_result` and `hop_decision` (section 4.5).
> - Renames forced by the codebase: `MAX_PATH` is `MAX_POD_PATH` (clashes with `windows.h`), `island_node.E` is `Energy` (`E` is a macro in `base_math.h`).

Fixed capacities (all in `engine_defaults.h`):

```c
#define MAX_ISLANDS     16
#define MAX_RESOURCES    8
#define MAX_NEIGHBORS    6
#define MAX_MAKERS      16   // makers tracked per island (not all 100 villagers; see below)
#define MAX_PODS       256   // pods alive in the world (static pool, free-list by Status)
#define MAX_QUEUE       12   // per island, == maxQueueCapacity default
#define MAX_POD_PATH         MAX_ISLANDS
#define MAX_CLAIMS       MAX_ISLANDS
#define MAX_EVENTS     1024  // ring buffer, oldest overwritten if the UI does not drain
```

Identifiers are small indices; `-1` means none:

```c
typedef s32 island_id;     // 0..MAX_ISLANDS-1
typedef s32 pod_id;        // index into world.Pods
typedef s32 resource_id;   // 0..MAX_RESOURCES-1 (catalog is decision D5; names live in a const table in engine_defaults.h)

enum power_state { PowerState_SolarHarvesting, PowerState_NightReserve, PowerState_Sleeping, PowerState_StormOutage };
typedef enum power_state power_state;
enum sync_mode { SyncMode_MeshGossip, SyncMode_Sneakernet, SyncMode_Hybrid };
typedef enum sync_mode sync_mode;
enum mid_flight_policy { MidFlight_PrelaunchEnergyCheck, MidFlight_PassiveMechanicalLanding };
typedef enum mid_flight_policy mid_flight_policy;
enum emergency_borrow_policy { Borrow_FluidOnly, Borrow_NeighborPullFirst, Borrow_FluidThenNeighbor };
typedef enum emergency_borrow_policy emergency_borrow_policy;
enum need_model_kind { NeedModel_AbstractCadence, NeedModel_StockBased };
typedef enum need_model_kind need_model_kind;
enum pod_kind { PodKind_Targeted, PodKind_Scout };
typedef enum pod_kind pod_kind;
enum pod_status { PodStatus_Free = 0, PodStatus_Queued, PodStatus_InTransit, PodStatus_Delivered, PodStatus_Expired };
typedef enum pod_status pod_status;
```

`PodStatus_Free = 0` so a zeroed pool is an empty pool. Delivered/Expired pods stay in the pool for one more tick (so the UI can see them) and are then set to `Free`.

```c
typedef struct engine_config engine_config;
struct engine_config
{
 u64 Seed;
 f32 DayLengthSeconds;
 need_model_kind NeedModel;                 // D1 (OPEN)
 sync_mode SyncMode;                        // D2 (OPEN)
 f32 GossipIntervalSeconds;
 mid_flight_policy MidFlightPolicy;         // D3 (OPEN)
 emergency_borrow_policy EmergencyBorrowPolicy; // D4 (OPEN)
 b32 EnableSpoilage;                        // D10
 b32 EnableWeightLimits;                    // D10
 b32 EnableRobotIslands;                    // D9
 b32 RebindOnSatisfied;                     // D16
 s32 DefaultReservedQty, DefaultFluidQty;
 s32 BatchSize;                             // surplus units needed to build a pod
 f32 FluidClaimNeedThreshold;               // theta
 f32 DestNeedMin;
 f32 MinLaunchReserve;
 f32 LaunchCooldownSeconds;                 // per (origin, dest, resource)
 f32 ScoutCooldownSeconds;                  // per (origin, neighbour)
 s32 ScoutQty;
 s32 WarmupDays;                            // days simulated headless before a scenario starts
 s32 MaxQueueCapacity;                      // <= MAX_QUEUE
 f32 PodSpeed;                              // normalized distance per sim-second
 f32 PodMaxTtlSeconds;
 physics_constants K;                       // section 9 constants (struct of f32, one field per symbol)
};

typedef struct neighbor_link neighbor_link;
struct neighbor_link
{
 island_id TargetId;
 f32 Distance;
 b32 WrapsGlobe;
};

typedef struct knowledge_entry knowledge_entry;
struct knowledge_entry
{
 b32 Valid;                                 // false = node knows nothing about this island
 power_state PowerState;
 f32 EnergyReserve;                         // quantized 0..100
 f32 Needs[MAX_RESOURCES];
 f32 UpdatedAt;                             // sim time at the origin island
 s32 HopCount;
};
// A knowledge table is just: knowledge_entry Knowledge[MAX_ISLANDS]; indexed by island_id.

typedef struct maker maker;
struct maker
{
 b32 Used;
 s32 VillagerId;
 resource_id Craft;
 f32 UpdatedAt;                             // last-write-wins timestamp
};
```
The tablet shows "my craft" for a handful of named villagers per island, not all 100. `MAX_MAKERS = 16` named makers per island is enough for the demo; the rest of the population is the number `Population`.

```c
typedef struct need_state need_state;
struct need_state                           // per resource; fields used depend on Config.NeedModel
{
 // Option A (abstract cadence)
 f32 ElapsedSec, TauHat, U, NextDemandAt;   // NextDemandAt is world-owned ground truth, see 3.3
 // Option B (stock)
 f32 Stock, PerCapitaHat;
 // both
 f32 Score;                                 // cached 0..100
};

typedef struct pod pod;
struct pod
{
 pod_status Status;
 pod_kind Kind;
 resource_id Resource;
 island_id OriginId, DestId;
 s32 ReservedQty, FluidQty;                 // ReservedQty immutable en route
 s32 InitialReservedQty, InitialFluidQty;
 f32 Weight, Freshness;                     // only if flags on
 f32 CreatedAt, ExpiresAt;
 island_id CustodianId, FromId, ToId;
 b32 WrapsGlobe;
 f32 Progress;                              // 0..1 on active link
 island_id Visited[MAX_POD_PATH]; s32 VisitedCount;
 island_id PlannedPath[MAX_POD_PATH]; s32 PlannedPathCount;   // advisory, for UI
 s32 RerouteCount;
 struct { island_id IslandId; s32 Qty; f32 Timestamp; } FluidClaims[MAX_CLAIMS]; s32 FluidClaimCount;
 knowledge_entry Carried[MAX_ISLANDS];      // sneakernet/hybrid ledger: a COPY made at launch
 f32 LaunchedAt;                            // for delivery-time stats
 b32 SawOutage;                             // path crossed an asleep/stormed island (stats)
 b32 UsedNightRelay;
};

/* What the world lets a node perceive about a direct neighbour. By value. No pointers. */
typedef struct neighbor_obs neighbor_obs;
struct neighbor_obs
{
 island_id NeighborId;
 b32 IsReachable;                           // awake and storm-free (visible beacon, not data)
 power_state ObservedPowerState;
 b32 HasReserveEstimate;                    // only if a gossip/optical link is active
 f32 EstimatedReserve;
 f32 Distance;
 b32 WrapsGlobe;
};

typedef struct island_node island_node;
struct island_node
{
 island_id Id;
 // public-ish info (for UI and scoring)
 f32 X, Y;                                  // x wraps [0,1), y in [0.1, 0.9]
 s32 Population;
 b32 IsAutomatedRobotIsland;
 s32 BiomeIndex;
 // physics state
 f32 Sun;                                   // I(t)
 f32 Energy;                                     // reserve 0..100 (named Energy: `E` is a base_math.h macro)
 f32 EDelta;                                // last tick's dE/dt, for the UI
 power_state PowerState;
 f32 StormUntil;                            // sim time; 0 = no storm
 // makers and economy
 maker Makers[MAX_MAKERS];
 need_state Need[MAX_RESOURCES];
 s32 Shed[MAX_RESOURCES];                   // surplus waiting to become a pod
 s32 Dropped[MAX_RESOURCES];                // append-only drop counters (the "drop log")
 // network state
 neighbor_link Neighbors[MAX_NEIGHBORS]; s32 NeighborCount;
 knowledge_entry Knowledge[MAX_ISLANDS];    // this node's beliefs. Knowledge[Id] is always itself.
 pod_id Queue[MAX_QUEUE]; s32 QueueCount;   // waiting queue: indices into world.Pods (custody)
 f32 LaunchCooldownUntil[MAX_ISLANDS][MAX_RESOURCES];
 f32 ScoutCooldownUntil[MAX_ISLANDS];
 f32 SuppressUntil[MAX_ISLANDS][MAX_RESOURCES]; f32 SuppressAmount[MAX_ISLANDS][MAX_RESOURCES];
 random_series Rng;                         // per-island stream, seeded from world seed + Id
};
```
**Pods are in a world-owned pool; the node only holds indices.** A node function that needs to read or change a pod receives `pod *Pod` as an explicit argument from the world. A node must never be given the pool itself. (A pod is "data physically carried", so passing it is allowed; the pool is not.)

### 4.3 Engine/UI contract (define first so UI work runs on a mock)

The UI reads `const world *`. There is no separate snapshot copy: because the world is plain data, the UI just reads it between steps (same thread, no locking). Things the old TypeScript snapshot computed are small helper functions in `engine.h`, e.g. `WorldAvgKnowledgeAge(World, Island)`.

```c
typedef struct world_stats world_stats;
struct world_stats
{
 s32 UnitsProduced, UnitsDelivered, UnitsClaimedEnRoute, UnitsExpired;
 s32 DeliveriesCompleted;
 s32 DeliveriesCompletedDuringOutage;       // delivered while >=1 island on its path was asleep/stormed
 s32 DeliveriesViaNightRelay;
 s32 Reroutes, PodsHeldDtn;
 f32 EnergySpentTotal;
 f32 EnergyPerUnitDelivered;
 s32 FluidFlightsSaved;                     // fluid units delivered en route without an extra flight
 f32 TotalDeliverySeconds;                  // avg = TotalDeliverySeconds / DeliveriesCompleted
};

typedef struct world_link world_link;
struct world_link { island_id A, B; f32 Distance; b32 WrapsGlobe; b32 Active; };

typedef struct world world;
struct world
{
 engine_config Config;
 f32 SimTime, SunPhase;
 s32 TickCount;
 random_series Rng;                         // world-level stream (demand events, storms, jitter)
 island_node Islands[MAX_ISLANDS]; s32 IslandCount;
 pod Pods[MAX_PODS];
 world_link Links[MAX_ISLANDS*MAX_NEIGHBORS]; s32 LinkCount;
 world_stats Stats;
 sim_event Events[MAX_EVENTS]; u32 EventHead, EventTail;   // ring buffer
};
```

Events (tagged struct, not a union of many types, to keep it simple):

```c
enum sim_event_kind
{
 SimEvent_PodCreated, SimEvent_PodLaunched, SimEvent_PodRerouted, SimEvent_PodHeldDtn,
 SimEvent_PodReturned, SimEvent_FluidClaimed, SimEvent_PodDelivered, SimEvent_PodExpired,
 SimEvent_GossipPing, SimEvent_IslandState, SimEvent_StormStarted, SimEvent_StormEnded,
 SimEvent_NeedRaised, SimEvent_QueueBackpressure,
};
typedef enum sim_event_kind sim_event_kind;
enum hold_reason { HoldReason_NeighborsAsleep, HoldReason_LowEnergy, HoldReason_Storm, HoldReason_Cooldown };
enum reroute_reason { Reroute_Asleep, Reroute_Storm, Reroute_LowEnergy, Reroute_LoopAvoid };

typedef struct sim_event sim_event;
struct sim_event
{
 sim_event_kind Kind;
 f32 T;
 pod_id Pod;
 island_id A, B;                            // meaning depends on Kind (from/to, island/neighbour, ...)
 resource_id Resource;
 s32 Qty, Qty2;                             // reserved/fluid, or claimed qty
 s32 Reason;                                // hold_reason / reroute_reason / need source
 s32 FromState, ToState;                    // for IslandState
};
```
The narrative bar turns events into plain-language lines in `ui_controls.c` (e.g. "Pod to Kelp Isle is waiting out the night at Fern Isle").

Actions from the UI:

```c
enum villager_action_kind { VillagerAction_DropSurplus, VillagerAction_TapRunningLow, VillagerAction_SetCraft };
typedef struct villager_action villager_action;
struct villager_action
{
 villager_action_kind Kind;
 island_id IslandId; s32 VillagerId; resource_id Resource; s32 Qty;   // Qty only for DropSurplus; Resource = new craft for SetCraft
};

enum chaos_action_kind { ChaosAction_Storm, ChaosAction_EndAllStorms, ChaosAction_SetSyncMode, ChaosAction_SetConfig };
typedef struct chaos_action chaos_action;
struct chaos_action
{
 chaos_action_kind Kind;
 island_id Islands[MAX_ISLANDS]; s32 IslandCount; f32 DurationSec;     // Storm
 sync_mode Mode;                                                       // SetSyncMode
 engine_config ConfigPatch;                                            // SetConfig: whole config; caller copies, edits, passes
};
```

### 4.4 Need model functions (`engine/node/node_need.c`)

```c
internal void NeedInit(island_node *Self, const engine_config *Config);
internal void NeedTick(island_node *Self, const engine_config *Config, f32 Dt, f32 SimTime);
internal f32  NeedScore(const island_node *Self, const engine_config *Config, resource_id R);   // 0..100
internal void NeedOnDemandEvent(island_node *Self, const engine_config *Config, resource_id R, f32 Interval, f32 SimTime); // Option A, fired by the hidden clock
internal void NeedOnDelivery(island_node *Self, const engine_config *Config, resource_id R, s32 Qty, f32 SimTime, b32 IsFinal);
internal void NeedOnFluidClaim(island_node *Self, const engine_config *Config, resource_id R, s32 Qty);
internal void NeedOnTap(island_node *Self, const engine_config *Config, resource_id R, f32 SimTime);
internal s32  NeedDrainToShed(island_node *Self, const engine_config *Config, resource_id R);  // own-needs-first lives here; adds to Shed, returns units added
```
Each function does `switch(Config->NeedModel)`, so every one takes `const engine_config *Config`. Config is shared constants, not other nodes' state, so it does not break isolation (the isolation test does not forbid it).

### 4.5 Node API and the L1/L4 boundary (decision D18, decided 2026-11-21)

The node decides; the world acts, moves pods and emits events. Declared in `engine.h`:

```c
// Energy (node_energy.c, L4)
internal void EnergyTick(island_node *Self, const engine_config *Config, f32 SimTime, f32 Dt); // E, EDelta, PowerState from Self->Sun / StormUntil
internal b32  EnergyIsAwake(const island_node *Self);
internal f32  EnergyLaunchCost(const island_node *Self, const engine_config *Config, f32 Distance);
internal b32  EnergyCanLaunch(const island_node *Self, const engine_config *Config, f32 Distance);
internal b32  EnergyTryLaunch(island_node *Self, const engine_config *Config, f32 Distance);   // guard + deduct

// Node logic (node.c / node_router.c / node_store.c, L1)
internal void NodeInit(island_node *Self, const engine_config *Config);
internal void NodeStep(island_node *Self, const engine_config *Config, const neighbor_obs *Obs, s32 ObsCount, f32 SimTime);
internal b32  NodeTryBuildPod(island_node *Self, const engine_config *Config, resource_id R,
                              const neighbor_obs *Obs, s32 ObsCount, pod *Pod, f32 SimTime);
internal hop_decision   NodeDecideNextHop(island_node *Self, const engine_config *Config, pod *Pod,
                                          const neighbor_obs *Obs, s32 ObsCount, f32 SimTime);
internal arrival_result NodeOnPodArrived(island_node *Self, const engine_config *Config, pod *Pod, f32 SimTime);
internal void NodeMergeKnowledge(island_node *Self, const engine_config *Config, const knowledge_entry *Incoming, f32 SimTime);
internal void NodeOnVillagerSetCraft(island_node *Self, s32 VillagerId, resource_id Craft, f32 SimTime);
```

Who does what:
- **Pod creation:** the world finds a free pool slot and passes it zeroed. `NodeTryBuildPod` (L1) scores destinations or the scout rule, fills the pod and takes the units from `Self->Shed`. The world sets status and timestamps, enqueues it and emits `PodCreated`.
- **Departure:** `NodeDecideNextHop` (L1) only chooses the hop (it may update `PlannedPath`, suppression and cooldowns). The world then calls `EnergyTryLaunch` (L4), which runs the 3.2 guard and deducts the cost; if it fails, the pod is held with `HoldReason_LowEnergy`. On launch the world appends `Visited`, copies `Self->Knowledge` into `Pod->Carried`, sets `FromId/ToId/Distance/Progress` and emits events.
- **Arrival:** the world applies the mid-flight policy. Otherwise `NodeOnPodArrived` (L1) merges `Carried`, calls `NeedOnDelivery` on the final hop or claims Fluid (6.3), and reports the outcome in `arrival_result`. The world updates stats, status and the queue (queue push/pop and backpressure are world-side).
- **Gossip:** the world passes a by-value copy of a neighbour's knowledge table to `NodeMergeKnowledge` (L1).

---

## 5. Tick pipeline and state machines

### 5.1 `WorldStep(World, Dt)`, fixed sub-step 0.05 s, this exact order
The UI calls `WorldStep` with a **fixed** `Dt = 0.05f` as many times as needed per frame (accumulator in `app_state`, multiplied by the speed slider). Never pass the frame time as `Dt`.

1. **Clock and sun:** `SimTime += Dt`; update `SunPhase`.
2. **Per-island local update:** compute `I`, update `E` and `PowerState`; `NeedTick`; world fires hidden demand events (Option A) via `NeedOnDemandEvent`; refresh own `Knowledge[Id]` entry; expire storms; drain surplus to shed (if awake).
3. **Pod creation:** per awake island, per resource: if `Shed[r] >= BatchSize` and cooldown ok, run destination scoring (3.5, falling back to the scout rule 3.6) and build a pod (section 6). Add to the waiting queue.
4. **Sync:** if `SyncMode != SyncMode_Sneakernet` and the gossip timer fired, for every link whose both ends are awake and storm-free, exchange copies of knowledge and merge by newer `UpdatedAt`. Emit `SimEvent_GossipPing`.
5. **Transport:** advance in-transit pods (`Progress += PodSpeed/Distance*Dt`); spoilage if enabled; TTL expiry. On arrival run Arrival Resolution:
   - If `ToId` went offline mid-flight: `MidFlight_PassiveMechanicalLanding` puts the pod in `ToId`'s queue; `MidFlight_PrelaunchEnergyCheck` returns it to `FromId` (custody rollback, `SimEvent_PodReturned`).
   - Else `NodeOnPodArrived(Node, Pod, SimTime)`: merge `Pod->Carried`; if final destination deliver `Reserved + Fluid`, call `NeedOnDelivery`, mark Delivered; else try a Fluid claim (6.3), then enqueue for onward forwarding.
6. **Departure drain:** for each awake island with queued pods (FIFO, urgent first): build a local `neighbor_obs Obs[MAX_NEIGHBORS]`, call `NodeDecideNextHop(Node, Pod, Obs, ObsCount, SimTime)`; if a valid awake hop exists and energy suffices: deduct energy, copy knowledge into `Pod->Carried`, launch (`SimEvent_PodLaunched`, plus `SimEvent_PodRerouted` if it differs from the previous plan). Else keep it (`SimEvent_PodHeldDtn`).
7. **Conservation check** (always in debug/test builds).

Delivered/Expired pods are recycled (set to `Free`) at the **start** of the next `WorldStep` (step 0), so the UI sees them for exactly one tick.

**As implemented (L4, 2026-11-21), details the steps above leave open:**
- **Step 2 order per island:** storm expiry, sun, `EnergyTick` (+ `IslandState` event), `NeedTick`, hidden demand events (Option A only; world RNG), then, if awake, `NodeStep` and `NeedDrainToShed` (adds to `Stats.UnitsProduced`). `NeedOnDemandEvent` gets `Interval = -1` for the first event of each `(island, r)`.
- **Step 3:** per awake island, per resource with `Shed >= BatchSize`: stop if the queue is full or the pool is empty (surplus stays in the shed). `SimEvent_QueueBackpressure` fires once when an island becomes backpressured, not every tick (`world.Backpressured`).
- **Step 4:** both ends of a link must be awake; each side merges a by-value snapshot of the other's table. Links are processed in `Links[]` order.
- **Step 5:** TTL expiry applies to Queued and InTransit pods (an expired queued pod leaves its queue). A pod arriving at a relay whose queue is full **hovers** at `Progress = 1` and retries every tick; the same applies when the mid-flight target queue is full. It is never dropped. On arrival the world asserts `ReservedQty` unchanged and that the Fluid decrease equals `arrival_result.FluidClaimedQty`.
- **Step 6:** FIFO over the queue (L1 may reorder `Self->Queue` in `NodeStep` for "urgent first"). A hold is counted in `Stats.PodsHeldDtn` and emitted as `PodHeldDtn` only when the pod's `HoldReason` changes, not every tick. `UsedNightRelay` is set when a non-origin island launches the pod while in `NightReserve`. `SawOutage` is set by a NeighborsAsleep/Storm hold or a mid-flight outage.
- New fields: `pod.HoldReason`, `world.Backpressured[]`, `world.DemandLastAt[][]`.
- **Test/scenario helpers** (not for the UI): `WorldSpawnPod` (takes units from the origin shed, queues a pod) and `WorldTestLaunch` (launches a queued pod to a given neighbour through the same code as step 6, bypassing `NodeDecideNextHop`). The transport tests use these, so they do not depend on L1 routing. Spec test 4 (DTN) and tests 2, 3, 5, 6 still need L1's router.

### 5.2 Island power state machine
```
SolarHarvesting --sunset (I<=0.05)--> NightReserve --E==0--> Sleeping
Sleeping / NightReserve --sunrise (I>0.05) & no storm--> SolarHarvesting
(any) --storm--> StormOutage --storm ends--> (recompute from sun and E)
```

### 5.3 Pod lifecycle
`created -> Queued (custody at island) -> InTransit -> arrives -> {final dest ? Delivered : [Fluid check] -> Queued}`; any state may go `Expired` on TTL. Custody returns to sender on rollback.

---

## 6. Production, pod creation, Reserved/Fluid

### 6.1 Who creates pods and when
Pod creation is a **local decision of the island** (no scheduler). When `Shed[r] >= BatchSize`, the launch cooldown for `(origin,dest,r)` has elapsed, and 3.5 yields a destination, the island builds one pod from `BatchSize` units. Villager `DropSurplus` adds directly to `Shed[r]` (and `Dropped[r]`).

### 6.2 Sizing the split
- Sharp need at destination (`N_dest >= 70`): `Fluid = Min(DefaultFluidQty, BatchSize / 5)`, rest Reserved.
- Weak/gradient need (`N_dest < 70`): `Fluid = BatchSize / 2`, rest Reserved. (Common staples drift generously; urgent items stay protected.)

### 6.3 Fluid claim rule at intermediate islands
Claim only if `Pod->FluidQty > 0` and `NeedScore(r) >= FluidClaimNeedThreshold`.
`Claim = Min(Pod->FluidQty, 1 + (s32)((N - theta)/(100 - theta) * Pod->InitialFluidQty))`.
Never touch `ReservedQty`. Record in `FluidClaims`, emit `SimEvent_FluidClaimed`, call `NeedOnFluidClaim`.

### 6.4 Destination already satisfied on arrival
Default (`RebindOnSatisfied = false`): deliver anyway (the island keeps it). If the team enables the flag: an intermediate holding fresher `Carried` knowledge may re-target the whole pod (never splitting Reserved) to a needier island. (Decision D16, low priority.)

---

## 7. Edge cases and guarantees
1. **Hemisphere blackout / partition:** never drop pods or assert. Pods wait in the queue (bounded by `MaxQueueCapacity`, emit `SimEvent_QueueBackpressure` beyond) and drain when the terminator wakes a neighbour. If the queue is full, the island stops building new pods (surplus stays in the shed); it never discards a queued pod.
2. **Ping-pong loops:** exclude `Pod->Visited`; if every awake neighbour is visited, clear all but the immediate previous hop or hold.
3. **Thundering herd:** local launch cooldown per `(origin,dest,resource)`; seeded jitter on dispatch; **pod-sighting suppression** (an island that forwards a pod of `r` bound for `D` lowers its own belief of `N_D,r` so it does not launch a duplicate right behind it).
4. **Mid-flight outage:** honour `MidFlightPolicy`; a pod never vanishes.
5. **No single point of failure:** topology must pass the real biconnectivity check (section 10).
6. **Energy exhaustion mid-night:** islands with `E <= 0` stop launching but still *receive* pods passively under `MidFlight_PassiveMechanicalLanding`.
7. **Pool exhaustion:** if `MAX_PODS` live pods exist, no new pod is created (the shed keeps the surplus). Assert in tests that this never happens in presets.

---

## 8. Scope guidance (what to keep small)
| Item | Guidance |
|---|---|
| Local store ("CRDT") | The craft table in `Makers[]` (last-write-wins by `UpdatedAt`) plus `Dropped[]` counters. About 1h. Do not build a general CRDT library. |
| Sync modes | Implement gossip and sneakernet; hybrid = both active. |
| Spoilage, weight, robot islands | Behind flags, **off**, only after the core works. |
| Edge cost | Only the five terms in 3.4. |
| `Borrow_NeighborPull*` | Not built unless the team decides to build request messages (D4). |

---

## 9. Physical constants: starting values (all tunable; expect to tune)

Put these in `engine_defaults.h` (as `physics_constants` fields filled by `DefaultConfig()`) and expose them in a debug panel. They are **starting points chosen to make the night-bridge effect plausible**, not validated. Tune with `SolarSim sim` (see `04_BUILD_PLAN.md`). Never write these numbers anywhere else in the code.

| Symbol | Default | Meaning |
|---|---|---|
| `DayLengthSeconds` | 120 | 1 full day in sim seconds (speed slider 1x to 8x) |
| `eps_dawn` | 0.05 | daylight threshold |
| `E_max` | 100 | max stored reserve |
| `k_harvest` | 3.0 | solar gain per second at I=1 |
| `P_idle` | 0.6 | live idle load in daylight |
| `P_night` | 0.8 | night drain per second |
| `k_hold` | 0.05 | extra drain per queued pod per second |
| `P_storm` | 1.0 | drain per second during storm |
| `c_day` / `c_night` | 3 / 6 | base launch cost |
| `alpha` | 1.5 | distance multiplier in launch cost |
| `MinLaunchReserve` | 12 | reserve left after a night launch |
| `PodSpeed` | 0.02 | distance units per sim second |
| `PodMaxTtlSeconds` | 600 | pod expiry |
| `MaxQueueCapacity` | 12 | pods per island |
| `GossipIntervalSeconds` | 4 | |
| `BatchSize` | 10 | units per pod |
| `DefaultReservedQty` / `DefaultFluidQty` | 8 / 2 | for a 10-unit batch |
| `FluidClaimNeedThreshold` (theta) | 46 | |
| `DestNeedMin` | 30 | |
| `LaunchCooldownSeconds` | 20 | |
| `ScoutCooldownSeconds` / `ScoutQty` | 30 / 2 | scout pods (3.6) |
| `WarmupDays` | 0 | demo presets set this (e.g. 8) so learned rhythms are already visible when the demo starts, instead of shrinking `tau_*` |
| `w_d, w_hop, w_e, w_s` | 20, 3, 25, 15 | edge-cost weights |
| `E_safe` | 30 | reserve considered safe |
| `M_sleep` | 80 | fallback-pass penalty for sleeping nodes |
| `T_stale` | 60 | seconds for knowledge-age penalty to saturate |
| `W_pro`, `gamma` | 68, 1.45 | need curve |
| `lambda_U` / `tapBoost` | 0.5 / 60 | tap urgency decay/impulse |
| `beta`, `tau_min`, `tau_max` | 0.35, 60, 600 | cadence learning |
| `tapTauNudge` | 0.9 | multiply `TauHat` on a tap |
| `attenuation` | 0.8 | need ripple decay per hop |
| `OwnShare` | 0.5 | fraction of production kept home (Option A) |

Added while writing the contract (not in v3; starting values, tune):

| Symbol | Default | Meaning |
|---|---|---|
| `EInitial` | 50 | reserve at `WorldInit` |
| `w_path` | 1 | 3.5 path-cost weight in destination scoring |
| `SharpNeed` | 70 | 6.2 split threshold |
| `delta_U` | 10 | `U` reduction per Fluid unit claimed (3.3) |
| `TauTrueMin` / `TauTrueMax` | 90 / 360 | range of the hidden true demand cadence (Option A) |
| `DemandJitter` | 0.2 | +/- fraction on each hidden demand interval |
| `ProductionPerMaker` | 0.05 | units per sim-second per maker |
| `TargetCoverDays` / `SurplusCoverDays` | 3 / 5 | Option B |
| `PerCapita` / `PerCapitaBeta` / `TapPerCapitaNudge` | 0.01 / 0.2 / 1.1 | Option B |
| `YMin` / `YMax` / `XJitter` | 0.15 / 0.85 / 0.02 | topology layout |

Open-decision placeholders (D1 to D5, D9, D10, D13, D16) are grouped at the top of `engine_defaults.h`. They are not decisions.

Sanity check behind these (verify with `SolarSim sim`): an idle island fills toward about 70 to 76 reserve over a day and loses about 50 over the night, so it survives; an island launching about 5 pods at night spends about 40 more and sleeps before dawn.

**Measured (L4, 2026-11-21, `SolarSim test` energy group, default constants, launch distance 0.3):**
- Idle, starting the day at E = 0: dusk 80.2, dawn 30.7 (night costs ~49.5). Survives.
- Same start, night launches every 8 s: the reserve floor allows only 4 launches (34.8 energy), then the island sleeps before dawn.
- Starting the day at E = 50: hits the cap (dusk 99.4), relays all 5 night launches and wakes with 6.3. This is the night-bridge effect.
- With no traffic every island settles at dusk ~99 / dawn ~50, so all idle islands look the same; differences only appear once pods fly. If the halo needs more variety, lower `k_harvest` or raise `E_max`.

State-machine details (5.2) as implemented: a `Sleeping` island stays asleep until sunrise; at dawn an island with E = 0 is `SolarHarvesting` (E stays near 0 until `k_harvest * I > P_idle`, i.e. I > 0.2); the island's state at `WorldInit` comes from the sun with `EInitial = 50`.

---

## 10. Topology
- Default: 12 islands (decision D13 open; hard cap `MAX_ISLANDS = 16`). `x` spread evenly with small seeded jitter, `y` in [0.15, 0.85].
- Neighbours: `k = 3` nearest by wrap distance, then repair edges until **biconnected** (graph stays connected after removing any one island) and at least **2 East-West wrap edges** exist. Never exceed `MAX_NEIGHBORS`.
- A test must **actually verify** biconnectivity (remove each vertex, check connectivity with a small DFS). "Every degree >= 2" is necessary but not sufficient.
- Provide a hand-tuned preset layout for the demo and a generator for N islands.

**As implemented (L4, 2026-11-21, `engine_topology.c`):**
- `TopologyConnect`: (1) each island links to its `TOPOLOGY_K_NEAREST` (3) nearest by wrap distance (skipping full sides); (2) greedily adds the shortest missing link that lowers `TopologyDefect` (islands missed by the reachability check and by each remove-one-island check; 0 means biconnected) until biconnected; (3) adds the shortest seam-crossing links until `TOPOLOGY_MIN_WRAP_EDGES` (2). Pairs are sorted by distance with index tie-break, so the result is deterministic.
- `TopologyGenerate(N)`: x evenly spread plus `XJitter`, y cycles through three bands of `[YMin, YMax]` so islands next to each other in x do not stack. Seeded from `world.Rng`.
- `TopologyBuildDefault`: if `IslandCount == 12`, uses the hand-tuned demo positions (`TopologyDemoLayout`, positions only; links come from `TopologyConnect`); otherwise it uses the generator. D13 is still open.
- Measured: demo layout has 21 links (avg degree 3.5) and 2 wrap edges. Generated N = 3..16 x 8 seeds: all biconnected, avg degree 3.53 (min 2, max 6), >= 2 wrap edges from N = 5 (N = 3 and 4 have too few seam-crossing pairs).

---

## 11. Rendering and UI notes (for lanes L2/L3)

The renderer (`code/rl/rl_renderer.h`) draws **rectangles only** with: rounded corners (`CornerRadius`), border (`BorderThickness`), edge softness (`Softness`), four per-corner colours (gradients), a clip stack, and font-atlas glyphs. Consequences:

- **Circle** = square with `CornerRadius = Size/2`. **Ring** = same with `BorderThickness`. **Glow/halo** = a large circle with high `Softness` and low alpha. **Sky** = a full-screen rect with 4 corner colours; the day/night terminator can be two rects with a gradient edge.
- **Lines, arcs, ellipses, rotation:** not available (decision D17). Default: draw links as a chain of small dots. Ask before touching `rl_render.mdesk`.
- **Capacity:** `MaxRectsCount = KB(64)` rects per frame. 16 islands x ~12 rects plus ~100 dots per link x 40 links is under 8k: fine, but do not draw per-villager rects.
- **No images/icons.** Resource icons are small rect compositions or a font glyph; the font atlas and its codepoint range are set in `FontPaths` / `FontCodepointCounts` (check before using non-ASCII symbols).
- **UI toolkit:** the tablet and control bar use `rl_ui` (`UI_Row()`, `UI_Column()`, `UI_Padding`, buttons, `UI_Checkbox`). World-space drawing (islands, pods) uses `DrawRect` directly with world-to-screen math.
- **Input:** `app_input` gives mouse position/buttons per frame; hit-test islands and pods yourself (`IsInsideRectV2`).
- **Frame budget:** keep `UpdateAndRender` at 60 FPS: fixed engine steps per frame are capped (e.g. at 8 steps at 1x speed; at higher speeds scale the sub-steps but cap total work per frame and carry the remainder).

---

## 12. Mandatory automated tests (Definition of Done)

Tests live in `code/solarpunk/sim/solar_sim.c` and run with `SolarSim test`. A tiny macro `Check(Cond, "message")` counts failures and prints file/line; exit code is the failure count (so it can be scripted). No external framework.

1. **Isolation:** the test reads every `code/solarpunk/engine/node/*.c` file from disk and asserts none contains the substrings `world`, `World`, `Islands[` (other than `Self`/`Node` access through the passed pointer), `engine_world`, or includes `engine_world.c`. A runtime check also verifies node functions work when given a *copy* of the node placed at a different address.
2. **Reserved conservation:** 50 pods through a mesh where every intermediate has `NeedScore = 100`; every delivered pod has `ReservedQty == InitialReservedQty`; `InitialFluidQty - FluidQty == sum(FluidClaims.Qty)`.
3. **Storm rerouting:** strike a path node before/during the journey; pod reroutes via an alternate awake path (including a wrap edge if needed); `RerouteCount` increases.
4. **DTN store-and-forward:** put all neighbours of `A` to sleep, dispatch from `A`; pod stays `PodStatus_Queued` (not dropped); advance time until dawn; it launches and reaches `DestId`.
5. **Loop safety:** 10 sim-minutes under sneakernet with random storms: no pod visits an island more than twice; **no pod is ever lost**: every pod is delivered, expired by TTL, or still in custody at the end.
6. **Sneakernet convergence and cold start:** start with **empty knowledge** (each node knows only itself) and `SyncMode_Sneakernet`; pods must still launch (scouts, 3.6) and not deadlock. Then raise need on `Z`; traffic flows; knowledge of `Z`'s need reaches distant `A` solely through `Carried`.
7. **Deterministic replay:** two worlds, `Seed = 42`, at least **one full day plus night** (>= 3000 ticks at the default day length), identical scripted actions and storms: the two `world` structs are byte-identical (`memcmp`), after both were created from a zeroed arena. (Same build only.)
8. **Cargo conservation:** randomized stress (seeded); the 0.6 identity holds at every tick.
9. **Biconnectivity:** the default and generated topologies are biconnected and have >= 2 wrap edges.
10. **Scenario smoke tests:** each preset in `engine_scenarios.c` (see `05_DEMO_PITCH.md`) runs headless and meets its stated outcome, so the demo cannot silently break.

`SolarSim sim --seed 42 --days 5 --scenario A` is the tuning tool; see `04_BUILD_PLAN.md` section 6.

---

## 13. Build order
See `04_BUILD_PLAN.md`: contract and mock first, thin vertical slice early, then widen. The gate for merging is "`SolarSim test` passes and the app builds"; UI is built in parallel against `MockWorld()` (a function in `engine_world.c` that fills a `world` with a fake but contract-correct state, no stepping needed).
