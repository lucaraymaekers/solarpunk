/* date = November 21st 2026 */

#ifndef ENGINE_DEFAULTS_H
#define ENGINE_DEFAULTS_H

// NOTE(L4): Every engine constant lives here. DefaultConfig() (engine_world.c) copies the
// DEFAULT_* values into engine_config so they can be tuned at runtime; engine code reads
// the config, never these defines directly (except the fixed capacities).

//~ Fixed capacities (array sizes; changing them changes the world struct layout)
#define MAX_ISLANDS     16
#define MAX_RESOURCES    8
#define MAX_NEIGHBORS    6
#define MAX_MAKERS      16   // named makers per island (not all villagers)
#define MAX_PODS       256   // static pod pool, free-list by Status
#define MAX_QUEUE       12   // per island
#define MAX_POD_PATH        MAX_ISLANDS
#define MAX_CLAIMS      MAX_ISLANDS
#define MAX_EVENTS    1024   // ring buffer, oldest overwritten

//~ Fixed timestep
#define ENGINE_TICK_SECONDS 0.05f

//~ Open-decision placeholders (02_DECISIONS_TBD.md). These are NOT decisions: they only pick
// a value so the engine runs. Change them here (or at runtime via engine_config).
#define DEFAULT_NEED_MODEL         NeedModel_AbstractCadence       // D1: spec 3.3 says Option A until decided
#define DEFAULT_SYNC_MODE          SyncMode_MeshGossip             // D2: placeholder
#define DEFAULT_MID_FLIGHT_POLICY  MidFlight_PassiveMechanicalLanding // D3: placeholder
#define DEFAULT_BORROW_POLICY      Borrow_FluidOnly                // D4: only option built (spec 8)
#define DEFAULT_RESOURCE_COUNT     MAX_RESOURCES                   // D5: catalog size placeholder
#define DEFAULT_ISLAND_COUNT       12                              // D13: placeholder
#define DEFAULT_REBIND_ON_SATISFIED 0                              // D16: off by default (spec 6.4)
#define DEFAULT_ENABLE_SPOILAGE     0                              // D10
#define DEFAULT_ENABLE_WEIGHT       0                              // D10
#define DEFAULT_ENABLE_ROBOTS       0                              // D9

//~ Simulation (spec section 9)
#define DEFAULT_SEED                     42
#define DEFAULT_DAY_LENGTH_SECONDS       120.0f
#define DEFAULT_GOSSIP_INTERVAL_SECONDS  4.0f
#define DEFAULT_RESERVED_QTY             8
#define DEFAULT_FLUID_QTY                2
#define DEFAULT_BATCH_SIZE               10
#define DEFAULT_FLUID_CLAIM_THRESHOLD    46.0f
#define DEFAULT_DEST_NEED_MIN            30.0f
#define DEFAULT_MIN_LAUNCH_RESERVE       12.0f
#define DEFAULT_LAUNCH_COOLDOWN_SECONDS  20.0f
#define DEFAULT_SCOUT_COOLDOWN_SECONDS   30.0f
#define DEFAULT_SCOUT_QTY                2
#define DEFAULT_WARMUP_DAYS              0
#define DEFAULT_MAX_QUEUE_CAPACITY       MAX_QUEUE
#define DEFAULT_POD_SPEED                0.02f
#define DEFAULT_POD_MAX_TTL_SECONDS      600.0f

//~ Physics constants (physics_constants fields)
// Solar / energy (3.1, 3.2)
#define DEFAULT_SUN_PHASE0          0.0f
#define DEFAULT_EPS_DAWN            0.05f
#define DEFAULT_E_MAX               100.0f
#define DEFAULT_E_INITIAL           50.0f   // reserve at WorldInit (not in spec 9; tune)
#define DEFAULT_K_HARVEST           3.0f
#define DEFAULT_P_IDLE              0.6f
#define DEFAULT_P_NIGHT             0.8f
#define DEFAULT_K_HOLD              0.05f
#define DEFAULT_P_STORM             1.0f
#define DEFAULT_C_DAY               3.0f
#define DEFAULT_C_NIGHT             6.0f
#define DEFAULT_ALPHA               1.5f
// Edge cost and scoring (3.4, 3.5, 6.2)
#define DEFAULT_W_D                 20.0f
#define DEFAULT_W_HOP               3.0f
#define DEFAULT_W_E                 25.0f
#define DEFAULT_W_S                 15.0f
#define DEFAULT_W_PATH              1.0f    // 3.5 w_path (not in spec 9; tune)
#define DEFAULT_E_SAFE              30.0f
#define DEFAULT_M_SLEEP             80.0f
#define DEFAULT_T_STALE             60.0f
#define DEFAULT_ATTENUATION         0.8f
#define DEFAULT_SHARP_NEED          70.0f   // 6.2 split threshold
// Need model, Option A (3.3)
#define DEFAULT_W_PRO               68.0f
#define DEFAULT_GAMMA               1.45f
#define DEFAULT_LAMBDA_U            0.5f
#define DEFAULT_TAP_BOOST           60.0f
#define DEFAULT_DELTA_U             10.0f   // U reduction per claimed unit (not in spec 9; tune)
#define DEFAULT_BETA                0.35f
#define DEFAULT_TAU_MIN             60.0f
#define DEFAULT_TAU_MAX             600.0f
#define DEFAULT_TAP_TAU_NUDGE       0.9f
#define DEFAULT_OWN_SHARE           0.5f
#define DEFAULT_TAU_TRUE_MIN        90.0f   // hidden demand cadence range (not in spec 9; tune)
#define DEFAULT_TAU_TRUE_MAX        360.0f
#define DEFAULT_DEMAND_JITTER       0.2f    // +/- fraction on each demand interval
#define DEFAULT_PRODUCTION_PER_MAKER 0.05f  // units per sim-second per maker (not in spec 9; tune)
// Need model, Option B (3.3)
#define DEFAULT_TARGET_COVER_DAYS   3.0f
#define DEFAULT_SURPLUS_COVER_DAYS  5.0f
#define DEFAULT_PER_CAPITA          0.01f   // units per person per sim-day (tune)
#define DEFAULT_PER_CAPITA_BETA     0.2f
#define DEFAULT_TAP_PER_CAPITA_NUDGE 1.1f
#define DEFAULT_CONSUME_PER_EVENT   1.0f    // units a household demand event takes from stock
// Topology (10)
#define DEFAULT_POPULATION          100
#define DEFAULT_Y_MIN               0.15f
#define DEFAULT_Y_MAX               0.85f
#define DEFAULT_X_JITTER            0.02f
#define TOPOLOGY_K_NEAREST          3       // spec 10: links to the k nearest by wrap distance
#define TOPOLOGY_MIN_WRAP_EDGES     2       // spec 10: East-West links across the seam

//~ Resource names: placeholders until D5 is decided. Index = resource_id.
// NOTE(L4): Colours and shapes are UI concerns (L2/L3), not engine data.
#define ENGINE_RESOURCE_NAMES { "Resource 0", "Resource 1", "Resource 2", "Resource 3", \
"Resource 4", "Resource 5", "Resource 6", "Resource 7" }

#endif // ENGINE_DEFAULTS_H
