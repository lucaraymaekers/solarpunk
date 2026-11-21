//-
// Engine unity include. Both entry points (solarpunk.c, sim/solar_sim.c) pull the whole engine
// in with one #include of this file, after base/base.h + base/base.c.
//-

#include "rl/rl_random.h"
#include "solarpunk/engine/engine.h"

//~ Node layer (isolated)
#include "solarpunk/engine/node/node_energy.c"
#include "solarpunk/engine/node/node_need.c"
#include "solarpunk/engine/node/node_store.c"
#include "solarpunk/engine/node/node_router.c"
#include "solarpunk/engine/node/node.c"

//~ Simulator
#include "solarpunk/engine/engine_topology.c"
#include "solarpunk/engine/engine_world.c"
#include "solarpunk/engine/engine_scenarios.c"
