//-
// SolarSim: headless console tool. `SolarSim test` runs the spec section 12 tests and exits with
// the failure count. `SolarSim sim --seed 42 --days 5 --scenario A` prints a tuning report.
// Shared file: whoever finishes an engine piece appends its test here.
//-

//~ Includes
#define BASE_CONSOLE_APPLICATION 1
#define BASE_FORCE_THREADS_COUNT 1
#include "base/base.h"
#include "base/base.c"

#include <stdlib.h>

#include "solarpunk/engine/engine_unity.c"

//~ Test harness
global_variable s32 GlobalCheckCount;
global_variable s32 GlobalFailCount;

#define Check(Cond, Message) CheckAt((Cond) ? 1 : 0, (Message), __FILE__, __LINE__)

internal void
CheckAt(b32 Passed, char *Message, char *File, int Line)
{
 GlobalCheckCount += 1;
 if(!Passed)
 {
  GlobalFailCount += 1;
  Log("  FAIL %s(%d): %s\n", File, Line, Message);
 }
}

internal world *
SimAllocWorld(void)
{
 // NOTE: OS_Allocate returns zeroed pages, so two worlds start byte-identical (test 7).
 world *Result = (world *)OS_Allocate(sizeof(world));
 Assert(Result);
 return Result;
}

//~ Tests
// Test 1 (static part): node files must not mention the world.
internal void
TestIsolation(cmd_line *CmdLine)
{
 char *NodeFiles[] = { "node.c", "node_energy.c", "node_need.c", "node_router.c", "node_store.c" };
 char *Forbidden[] = { "world", "World", "Islands[", "engine_world" };

 str8 ExeDir = S8CutFromChar(CmdLine->ExePath, OS_SlashChar, false);
 for EachIndex(F, ArrayCount(NodeFiles))
 {
  str8 Path = Str8Fmt("%S/../code/solarpunk/engine/node/%s", ExeDir, NodeFiles[F]);
  str8 File = OS_ReadEntireFileIntoMemory(CSS8(Path));
  Check(File.Size > 0, NodeFiles[F]);
  for EachIndex(W, ArrayCount(Forbidden))
  {
   str8 Needle = S8FromCString(Forbidden[W]);
   b32 Found = false;
   for(u64 At = 0; At + Needle.Size <= File.Size; At += 1)
   {
    if(memcmp(File.Data + At, Needle.Data, Needle.Size) == 0)
    {
     Found = true;
     Log("  node/%s mentions '%s'\n", NodeFiles[F], Forbidden[W]);
     break;
    }
   }
   Check(!Found, "node file mentions a forbidden symbol");
  }
  if(File.Size)
  {
   OS_FreeFileMemory(File);
  }
 }
}

// Test 9: biconnectivity and wrap edges.
internal void
TestBiconnectivity(void)
{
 world *World = SimAllocWorld();
 engine_config Config = DefaultConfig();
 WorldInit(World, &Config);
 Check(TopologyIsBiconnected(World), "default topology is biconnected");
 Check(TopologyWrapEdgeCount(World) >= 2, "default topology has >= 2 wrap edges");

 for(s32 Count = 5; Count <= MAX_ISLANDS; Count += 1)
 {
  for(u64 Seed = 1; Seed <= 4; Seed += 1)
  {
   RandomSeed(&World->Rng, Seed);
   TopologyGenerate(World, Count);
   Check(TopologyIsBiconnected(World), "generated topology is biconnected");
   Check(TopologyWrapEdgeCount(World) >= 2, "generated topology has >= 2 wrap edges");
   for(s32 I = 0; I < World->IslandCount; I += 1)
   {
    Check(World->Islands[I].NeighborCount <= MAX_NEIGHBORS, "neighbour cap");
   }
  }
 }

 // The check must actually catch a cut vertex: a plain ring with one chord removed is fine,
 // but a "bowtie" (two triangles sharing island 2) is not biconnected.
 TopologyClear(World, 5);
 TopologyAddLink(World, 0, 1); TopologyAddLink(World, 1, 2); TopologyAddLink(World, 2, 0);
 TopologyAddLink(World, 2, 3); TopologyAddLink(World, 3, 4); TopologyAddLink(World, 4, 2);
 Check(!TopologyIsBiconnected(World), "bowtie graph is detected as not biconnected");
}

// Contract: MockWorld is conservation-correct and its events drain.
internal void
TestMockWorld(void)
{
 world *World = SimAllocWorld();
 MockWorld(World);
 Check(WorldCheckConservation(World), "MockWorld conserves cargo");
 Check(WorldUnitsInPods(World, PodStatus_InTransit) > 0, "MockWorld has pods in flight");
 Check(WorldUnitsInPods(World, PodStatus_Queued) > 0, "MockWorld has queued pods");
 sim_event Event;
 s32 EventCount = 0;
 while(WorldPopEvent(World, &Event))
 {
  EventCount += 1;
 }
 Check(EventCount > 0, "MockWorld has events");
}

// Test 8 (skeleton): conservation holds at every tick, villager drops included.
internal void
TestConservation(void)
{
 world *World = SimAllocWorld();
 engine_config Config = DefaultConfig();
 WorldInit(World, &Config);
 random_series Rng;
 RandomSeed(&Rng, 7);

 b32 AllOk = true;
 for EachIndex(Tick, 2000)
 {
  if((Tick % 50) == 0)
  {
   villager_action Action = {0};
   Action.Kind = VillagerAction_DropSurplus;
   Action.IslandId = (island_id)(RandomNext(&Rng) % (u32)World->IslandCount);
   Action.Resource = (resource_id)(RandomNext(&Rng) % (u32)Config.ResourceCount);
   Action.Qty = 1 + (s32)(RandomNext(&Rng) % 10);
   WorldDispatchVillager(World, &Action);
  }
  WorldStep(World, ENGINE_TICK_SECONDS);
  AllOk = AllOk && WorldCheckConservation(World);
 }
 Check(AllOk, "conservation identity holds at every tick");
}

// Test 7 (skeleton): deterministic replay, byte-identical worlds.
internal void
TestDeterminism(void)
{
 world *Worlds[2];
 Worlds[0] = SimAllocWorld();
 Worlds[1] = SimAllocWorld();
 engine_config Config = DefaultConfig();
 Config.Seed = 42;
 for EachIndex(W, 2)
 {
  world *World = Worlds[W];
  WorldInit(World, &Config);
  for EachIndex(Tick, 3200)
  {
   if(Tick == 500)
   {
    chaos_action Storm = {0};
    Storm.Kind = ChaosAction_Storm;
    Storm.Islands[0] = 1; Storm.Islands[1] = 2; Storm.IslandCount = 2;
    Storm.DurationSec = 30.0f;
    WorldDispatchChaos(World, &Storm);
   }
   if(Tick == 900)
   {
    villager_action Drop = {0};
    Drop.Kind = VillagerAction_DropSurplus;
    Drop.IslandId = 0; Drop.Resource = 0; Drop.Qty = 40;
    WorldDispatchVillager(World, &Drop);
   }
   if(Tick >= 900 && (Tick % 60) == 0)
   {
    // Pods in flight, relays and holds, so the replay covers the transport path too.
    island_id To = World->Islands[0].Neighbors[(Tick/60) % World->Islands[0].NeighborCount].TargetId;
    pod_id Id = WorldSpawnPod(World, 0, To, 0, 3, 1);
    if(Id >= 0) WorldTestLaunch(World, Id, To);
   }
   WorldStep(World, ENGINE_TICK_SECONDS);
  }
 }
 Check(memcmp(Worlds[0], Worlds[1], sizeof(world)) == 0, "same seed + same inputs = byte-identical world");
}

internal void
TestSunCycle(void)
{
 world *World = SimAllocWorld();
 engine_config Config = DefaultConfig();
 WorldInit(World, &Config);
 s32 TicksPerDay = (s32)(Config.DayLengthSeconds/ENGINE_TICK_SECONDS + 0.5f);
 for EachIndex(Tick, TicksPerDay)
 {
  WorldStep(World, ENGINE_TICK_SECONDS);
 }
 f32 Drift = EngineWrapDx(World->SunPhase, Config.K.SunPhase0);
 Check(Drift < 0.001f, "sun returns to its start phase after one day");
 Check(fabsf(EngineSunIntensity(0.25f, 0.25f) - 1.0f) < 0.0001f, "noon intensity is 1");
}

//- Energy (spec 3.2, 5.2, section 9 sanity check)
typedef struct energy_day energy_day;
struct energy_day
{
 f32 DuskE, DawnE;
 b32 Slept;
 s32 Launched;
};

// NOTE(L4): One node, one full day starting just after dawn. Optionally tries a night launch
// every 8 s from sunset until NightLaunches have gone out. Node-level only, no world.
internal energy_day
SimEnergyDay(const engine_config *Config, f32 StartE, s32 NightLaunches, f32 Distance)
{
 energy_day Result = {0};
 island_node Node = {0};
 Node.Energy = StartE;
 Node.PowerState = PowerState_SolarHarvesting;

 f32 Theta0 = -acosf(Config->K.EpsDawn);
 s32 Ticks = (s32)(Config->DayLengthSeconds/ENGINE_TICK_SECONDS + 0.5f);
 f32 NextLaunchAt = 0.0f;
 for(s32 Tick = 0; Tick < Ticks; Tick += 1)
 {
  f32 SimTime = (f32)(Tick + 1)*ENGINE_TICK_SECONDS;
  power_state Before = Node.PowerState;
  f32 EnergyBefore = Node.Energy;
  Node.Sun = cosf(Theta0 + 2.0f*Pi32*SimTime/Config->DayLengthSeconds);
  EnergyTick(&Node, Config, SimTime, ENGINE_TICK_SECONDS);
  if(Before == PowerState_SolarHarvesting && Node.PowerState != PowerState_SolarHarvesting)
  {
   Result.DuskE = EnergyBefore;
   NextLaunchAt = SimTime;
  }
  Result.Slept = Result.Slept || (Node.PowerState == PowerState_Sleeping);
  if(Node.PowerState == PowerState_NightReserve && Result.Launched < NightLaunches && SimTime >= NextLaunchAt)
  {
   if(EnergyTryLaunch(&Node, Config, Distance))
   {
    Result.Launched += 1;
    NextLaunchAt = SimTime + 8.0f;
   }
  }
 }
 Result.DawnE = Node.Energy;
 return Result;
}

internal void
TestEnergy(void)
{
 engine_config Config = DefaultConfig();

 // Section 9 sanity: from an empty reserve at dawn an idle island fills to ~70-80 by dusk,
 // loses ~50 over the night and survives.
 energy_day Idle = SimEnergyDay(&Config, 0.0f, 0, 0.0f);
 Log("  idle from 0: dusk %.1f, dawn %.1f\n", Idle.DuskE, Idle.DawnE);
 Check(Idle.DuskE >= 70.0f && Idle.DuskE <= 85.0f, "idle island reaches ~70-85 at dusk");
 Check(Idle.DuskE - Idle.DawnE >= 45.0f && Idle.DuskE - Idle.DawnE <= 55.0f, "night costs ~50");
 Check(!Idle.Slept && Idle.DawnE > 0.0f, "idle island survives the night");

 // ~5 night launches from the same start: spends ~35-45 more and sleeps before dawn.
 energy_day Busy = SimEnergyDay(&Config, 0.0f, 5, 0.3f);
 Log("  busy from 0: dusk %.1f, dawn %.1f, launched %d, slept %d\n",
     Busy.DuskE, Busy.DawnE, Busy.Launched, Busy.Slept);
 Check(Busy.Slept, "busy island sleeps before dawn");
 Check(Busy.Launched >= 3, "busy island still launches several pods at night");

 // A full island at dusk is a night bridge: 5 launches and still awake at dawn.
 energy_day Bridge = SimEnergyDay(&Config, 50.0f, 5, 0.3f);
 Log("  bridge from 50: dusk %.1f, dawn %.1f, launched %d, slept %d\n",
     Bridge.DuskE, Bridge.DawnE, Bridge.Launched, Bridge.Slept);
 Check(Bridge.Launched == 5 && !Bridge.Slept, "charged island relays 5 pods through the night");

 // Launch guard (3.2).
 island_node Node = {0};
 f32 Cost = 0.0f;
 Node.PowerState = PowerState_SolarHarvesting;
 Cost = EnergyLaunchCost(&Node, &Config, 0.5f);
 Check(fabsf(Cost - Config.K.CDay*(1.0f + Config.K.Alpha*0.5f)) < 0.001f, "day launch cost");
 Node.Energy = Cost;
 Check(EnergyTryLaunch(&Node, &Config, 0.5f) && Node.Energy < 0.001f, "day launch needs only E >= cost");

 Node.PowerState = PowerState_NightReserve;
 Cost = EnergyLaunchCost(&Node, &Config, 0.5f);
 Check(fabsf(Cost - Config.K.CNight*(1.0f + Config.K.Alpha*0.5f)) < 0.001f, "night launch cost");
 Node.Energy = Cost + Config.MinLaunchReserve - 0.01f;
 Check(!EnergyTryLaunch(&Node, &Config, 0.5f), "night launch keeps the reserve floor");
 Node.Energy = Cost + Config.MinLaunchReserve;
 Check(EnergyTryLaunch(&Node, &Config, 0.5f), "night launch allowed at the floor");
 Check(fabsf(Node.Energy - Config.MinLaunchReserve) < 0.001f, "night launch deducts the cost");

 Node.Energy = 100.0f;
 Node.PowerState = PowerState_Sleeping;
 Check(!EnergyCanLaunch(&Node, &Config, 0.1f), "sleeping island cannot launch");
 Node.PowerState = PowerState_StormOutage;
 Check(!EnergyCanLaunch(&Node, &Config, 0.1f), "stormed island cannot launch");

 // Storm state machine (5.2): storm at noon, then it ends and the sun decides again.
 MemorySet(&Node, 0, sizeof(Node));
 Node.Energy = 40.0f;
 Node.Sun = 1.0f;
 Node.PowerState = PowerState_SolarHarvesting;
 Node.StormUntil = 1.0f;
 EnergyTick(&Node, &Config, 0.5f, ENGINE_TICK_SECONDS);
 Check(Node.PowerState == PowerState_StormOutage, "storm -> StormOutage");
 Check(fabsf(Node.EDelta + Config.K.PStorm) < 0.001f, "storm drains P_storm, no solar");
 EnergyTick(&Node, &Config, 1.5f, ENGINE_TICK_SECONDS);
 Check(Node.PowerState == PowerState_SolarHarvesting, "storm over in daylight -> SolarHarvesting");
 Node.Sun = -1.0f;
 Node.StormUntil = 3.0f;
 EnergyTick(&Node, &Config, 2.5f, ENGINE_TICK_SECONDS);
 EnergyTick(&Node, &Config, 3.5f, ENGINE_TICK_SECONDS);
 Check(Node.PowerState == PowerState_NightReserve, "storm over at night with E > 0 -> NightReserve");
 Node.Energy = 0.01f;
 EnergyTick(&Node, &Config, 4.0f, ENGINE_TICK_SECONDS);
 Check(Node.PowerState == PowerState_Sleeping && Node.Energy == 0.0f, "E hits 0 at night -> Sleeping");
 Node.Sun = 0.5f;
 EnergyTick(&Node, &Config, 4.5f, ENGINE_TICK_SECONDS);
 Check(Node.PowerState == PowerState_SolarHarvesting, "sunrise wakes a sleeping island");
}

// World-level: day/night states follow the terminator and IslandState events fire.
internal void
TestWorldPower(void)
{
 world *World = SimAllocWorld();
 engine_config Config = DefaultConfig();
 WorldInit(World, &Config);

 b32 StatesMatchSun = true;
 b32 EnergyInRange = true;
 s32 StateEvents = 0;
 s32 Ticks = (s32)(2.0f*Config.DayLengthSeconds/ENGINE_TICK_SECONDS);
 for(s32 Tick = 0; Tick < Ticks; Tick += 1)
 {
  WorldStep(World, ENGINE_TICK_SECONDS);
  for(s32 I = 0; I < World->IslandCount; I += 1)
  {
   island_node *Node = &World->Islands[I];
   b32 Day = (Node->Sun > Config.K.EpsDawn);
   StatesMatchSun = StatesMatchSun && (Day == (Node->PowerState == PowerState_SolarHarvesting));
   EnergyInRange = EnergyInRange && Node->Energy >= 0.0f && Node->Energy <= Config.K.EMax;
  }
  sim_event Event;
  while(WorldPopEvent(World, &Event))
  {
   StateEvents += (Event.Kind == SimEvent_IslandState) ? 1 : 0;
  }
 }
 Check(StatesMatchSun, "SolarHarvesting exactly when the sun is up (no storms)");
 Check(EnergyInRange, "energy stays within [0, EMax]");
 Check(StateEvents >= 2*World->IslandCount, "IslandState events at sunrise and sunset");

 chaos_action Storm = {0};
 Storm.Kind = ChaosAction_Storm;
 Storm.Islands[0] = 0;
 Storm.IslandCount = 1;
 Storm.DurationSec = 10.0f;
 WorldDispatchChaos(World, &Storm);
 WorldStep(World, ENGINE_TICK_SECONDS);
 Check(World->Islands[0].PowerState == PowerState_StormOutage, "chaos storm puts island in StormOutage");
 for(s32 Tick = 0; Tick < (s32)(11.0f/ENGINE_TICK_SECONDS); Tick += 1)
 {
  WorldStep(World, ENGINE_TICK_SECONDS);
 }
 Check(World->Islands[0].PowerState != PowerState_StormOutage && World->Islands[0].StormUntil == 0.0f,
       "storm expires after its duration");
}

//- Transport (spec 5.1 steps 3-7). Uses WorldSpawnPod/WorldTestLaunch, so no L1 routing needed.

// NOTE(L4): Every live pod is in exactly one place: Queued in its custodian's queue, or InTransit.
internal b32
SimPodsAccounted(world *World)
{
 b32 Result = true;
 s32 SeenInQueues[MAX_PODS] = {0};
 for(s32 I = 0; I < World->IslandCount; I += 1)
 {
  island_node *Node = &World->Islands[I];
  for(s32 Q = 0; Q < Node->QueueCount; Q += 1)
  {
   pod_id Id = Node->Queue[Q];
   Result = Result && (Id >= 0 && Id < MAX_PODS);
   if(Result)
   {
    SeenInQueues[Id] += 1;
    Result = Result && World->Pods[Id].Status == PodStatus_Queued && World->Pods[Id].CustodianId == I;
   }
  }
 }
 for(s32 P = 0; P < MAX_PODS; P += 1)
 {
  pod *Pod = &World->Pods[P];
  if(Pod->Status == PodStatus_Queued)
  {
   Result = Result && SeenInQueues[P] == 1;
  }
  else
  {
   Result = Result && SeenInQueues[P] == 0;
  }
  if(Pod->Status == PodStatus_InTransit)
  {
   Result = Result && Pod->ToId >= 0 && Pod->ToId < World->IslandCount;
  }
  if(Pod->Status != PodStatus_Free)
  {
   Result = Result && Pod->ReservedQty == Pod->InitialReservedQty; // invariant 3
  }
 }
 return Result;
}

// A world where island A and its neighbour B are both in daylight with plenty of energy.
typedef struct sim_pair sim_pair;
struct sim_pair
{
 island_id A, B, C;   // C = another neighbour of B (not A), for relay tests
};

internal sim_pair
SimSetupPair(world *World, engine_config *Config)
{
 sim_pair Result = { -1, -1, -1 };
 Config->K.EInitial = 90.0f;
 WorldInit(World, Config);
 for(s32 I = 0; I < World->IslandCount && Result.A < 0; I += 1)
 {
  island_node *Node = &World->Islands[I];
  if(Node->PowerState != PowerState_SolarHarvesting) continue;
  for(s32 N = 0; N < Node->NeighborCount; N += 1)
  {
   island_id B = Node->Neighbors[N].TargetId;
   if(World->Islands[B].PowerState == PowerState_SolarHarvesting)
   {
    island_node *NodeB = &World->Islands[B];
    for(s32 M = 0; M < NodeB->NeighborCount; M += 1)
    {
     if(NodeB->Neighbors[M].TargetId != I)
     {
      Result.A = I; Result.B = B; Result.C = NodeB->Neighbors[M].TargetId;
      break;
     }
    }
    if(Result.A >= 0) break;
   }
  }
 }
 Assert(Result.A >= 0);

 // Give A a shed full of resource 0 through the real villager gesture.
 villager_action Drop = {0};
 Drop.Kind = VillagerAction_DropSurplus;
 Drop.IslandId = Result.A;
 Drop.Resource = 0;
 Drop.Qty = 60;
 WorldDispatchVillager(World, &Drop);
 return Result;
}

// Steps until Pod reaches one of the given statuses (or MaxSeconds); checks invariants each tick.
internal b32
SimStepUntil(world *World, pod_id Pod, pod_status Want, f32 MaxSeconds, b32 *InvariantsOk)
{
 b32 Result = false;
 s32 Ticks = (s32)(MaxSeconds/ENGINE_TICK_SECONDS);
 for(s32 T = 0; T < Ticks && !Result; T += 1)
 {
  WorldStep(World, ENGINE_TICK_SECONDS);
  *InvariantsOk = *InvariantsOk && WorldCheckConservation(World) && SimPodsAccounted(World);
  Result = (World->Pods[Pod].Status == Want);
 }
 return Result;
}

internal void
TestTransport(void)
{
 world *World = SimAllocWorld();
 engine_config Config = DefaultConfig();
 sim_pair Pair = SimSetupPair(World, &Config);
 b32 Ok = true;

 //- Direct delivery A -> B
 {
  pod_id Id = WorldSpawnPod(World, Pair.A, Pair.B, 0, 8, 2);
  Check(Id >= 0, "spawn pod");
  Check(World->Islands[Pair.A].Shed[0] == 50, "spawn takes units from the shed");
  f32 EnergyBefore = World->Islands[Pair.A].Energy;
  f32 Cost = EnergyLaunchCost(&World->Islands[Pair.A], &World->Config,
                              World->Islands[Pair.A].Neighbors[WorldNeighborIndex(&World->Islands[Pair.A], Pair.B)].Distance);
  Check(WorldTestLaunch(World, Id, Pair.B), "launch to an awake neighbour");
  Check(fabsf(EnergyBefore - Cost - World->Islands[Pair.A].Energy) < 0.001f, "launch deducts its energy cost");
  Check(World->Pods[Id].Status == PodStatus_InTransit && World->Pods[Id].CustodianId == Pair.A,
        "in flight, custody stays with the sender");
  Check(SimStepUntil(World, Id, PodStatus_Delivered, 60.0f, &Ok), "pod is delivered at its destination");
  Check(World->Stats.UnitsDelivered == 10 && World->Stats.DeliveriesCompleted == 1, "delivery stats");
  Check(World->Pods[Id].ReservedQty == 8 && World->Pods[Id].FluidQty == 2, "cargo intact on delivery");
  WorldStep(World, ENGINE_TICK_SECONDS);
  Check(World->Pods[Id].Status == PodStatus_Free, "delivered pod is recycled after one tick");
 }

 //- Relay: A -> B with destination C. B is not final, so the pod waits in B's queue.
 {
  pod_id Id = WorldSpawnPod(World, Pair.A, Pair.C, 0, 8, 2);
  Check(WorldTestLaunch(World, Id, Pair.B), "launch toward a relay");
  Check(SimStepUntil(World, Id, PodStatus_Queued, 60.0f, &Ok), "pod lands at the relay");
  pod *Pod = &World->Pods[Id];
  Check(Pod->CustodianId == Pair.B && Pod->VisitedCount == 2 && Pod->Visited[1] == Pair.B,
        "relay takes custody and is recorded as visited");
  Check(Pod->HoldReason != HoldReason_None, "relay holds it (L1 placeholder router holds everything)");
 }

 //- Low energy: the guard holds the pod with HoldReason_LowEnergy.
 {
  pod_id Id = WorldSpawnPod(World, Pair.A, Pair.B, 0, 8, 2);
  World->Islands[Pair.A].Energy = 0.5f;
  s32 HeldBefore = World->Stats.PodsHeldDtn;
  Check(!WorldTestLaunch(World, Id, Pair.B), "launch refused on low energy");
  Check(World->Pods[Id].HoldReason == HoldReason_LowEnergy, "hold reason LowEnergy");
  Check(World->Stats.PodsHeldDtn == HeldBefore + 1, "PodsHeldDtn counts the hold once");
  WorldTestLaunch(World, Id, Pair.B);
  Check(World->Stats.PodsHeldDtn == HeldBefore + 1, "same hold reason is not counted twice");
  World->Islands[Pair.A].Energy = 90.0f;
 }

 //- Stormed receiver: hold with HoldReason_Storm.
 {
  pod_id Id = WorldSpawnPod(World, Pair.A, Pair.B, 0, 8, 2);
  chaos_action Storm = {0};
  Storm.Kind = ChaosAction_Storm;
  Storm.Islands[0] = Pair.B;
  Storm.IslandCount = 1;
  Storm.DurationSec = 5.0f;
  WorldDispatchChaos(World, &Storm);
  WorldStep(World, ENGINE_TICK_SECONDS);
  Check(!WorldTestLaunch(World, Id, Pair.B), "no launch into a storm");
  Check(World->Pods[Id].HoldReason == HoldReason_Storm && World->Pods[Id].SawOutage, "hold reason Storm");
  for(s32 T = 0; T < (s32)(6.0f/ENGINE_TICK_SECONDS); T += 1) WorldStep(World, ENGINE_TICK_SECONDS);
  Check(WorldTestLaunch(World, Id, Pair.B), "launches once the storm is over");
  Check(SimStepUntil(World, Id, PodStatus_Delivered, 60.0f, &Ok), "and is delivered");
  Check(World->Stats.DeliveriesCompletedDuringOutage >= 1, "counted as delivered during an outage");
 }

 Check(Ok, "conservation + every pod accounted for, every tick");
}

// D3: what happens when the receiver goes down while the pod is in the air.
internal void
TestMidFlight(mid_flight_policy Policy)
{
 world *World = SimAllocWorld();
 engine_config Config = DefaultConfig();
 Config.MidFlightPolicy = Policy;
 sim_pair Pair = SimSetupPair(World, &Config);
 b32 Ok = true;

 pod_id Id = WorldSpawnPod(World, Pair.A, Pair.C, 0, 8, 2);
 Check(WorldTestLaunch(World, Id, Pair.B), "launch");
 chaos_action Storm = {0};
 Storm.Kind = ChaosAction_Storm;
 Storm.Islands[0] = Pair.B;
 Storm.IslandCount = 1;
 Storm.DurationSec = 200.0f;
 WorldDispatchChaos(World, &Storm);
 Check(SimStepUntil(World, Id, PodStatus_Queued, 60.0f, &Ok), "pod is caught, not lost");

 pod *Pod = &World->Pods[Id];
 if(Policy == MidFlight_PassiveMechanicalLanding)
 {
  Check(Pod->CustodianId == Pair.B, "passive landing: waits at the stormed island");
 }
 else
 {
  Check(Pod->CustodianId == Pair.A, "prelaunch policy: returned to the sender");
  b32 SawReturned = false;
  sim_event Event;
  while(WorldPopEvent(World, &Event)) SawReturned = SawReturned || Event.Kind == SimEvent_PodReturned;
  Check(SawReturned, "PodReturned event");
 }
 Check(Pod->SawOutage, "outage recorded on the pod");
 Check(Ok, "conservation + every pod accounted for, every tick");
}

internal void
TestExpiryAndQueue(void)
{
 world *World = SimAllocWorld();
 engine_config Config = DefaultConfig();
 Config.PodMaxTtlSeconds = 5.0f;
 Config.MaxQueueCapacity = 3;
 sim_pair Pair = SimSetupPair(World, &Config);
 b32 Ok = true;

 pod_id Ids[3];
 for(s32 I = 0; I < 3; I += 1) Ids[I] = WorldSpawnPod(World, Pair.A, Pair.B, 0, 5, 5);
 Check(Ids[2] >= 0, "three pods fit a queue of three");
 Check(WorldSpawnPod(World, Pair.A, Pair.B, 0, 5, 5) < 0, "a full queue refuses new pods (7.1)");
 Check(SimStepUntil(World, Ids[0], PodStatus_Expired, 10.0f, &Ok), "queued pod expires on TTL");
 Check(World->Islands[Pair.A].QueueCount == 0, "expired pods leave the queue");
 Check(World->Stats.UnitsExpired == 30, "expired units counted");
 Check(Ok, "conservation holds through expiry");
}

internal void
TestGossipTimer(void)
{
 world *World = SimAllocWorld();
 engine_config Config = DefaultConfig();
 for(s32 Mode = 0; Mode < 2; Mode += 1)
 {
  Config.SyncMode = (Mode == 0) ? SyncMode_MeshGossip : SyncMode_Sneakernet;
  WorldInit(World, &Config);
  s32 Pings = 0;
  for(s32 T = 0; T < (s32)(20.0f/ENGINE_TICK_SECONDS); T += 1)
  {
   WorldStep(World, ENGINE_TICK_SECONDS);
   sim_event Event;
   while(WorldPopEvent(World, &Event)) Pings += (Event.Kind == SimEvent_GossipPing) ? 1 : 0;
  }
  if(Mode == 0) Check(Pings > 0, "gossip pings between awake neighbours");
  else          Check(Pings == 0, "no radio in sneakernet mode");
 }
}

// Test 8: randomized stress with drops, spawns, launches to random neighbours and storms.
internal void
TestConservationStress(void)
{
 world *World = SimAllocWorld();
 engine_config Config = DefaultConfig();
 WorldInit(World, &Config);
 random_series Rng;
 RandomSeed(&Rng, 1234);
 b32 Ok = true;
 s32 Launches = 0;

 for(s32 Tick = 0; Tick < 6000; Tick += 1)
 {
  island_id I = (island_id)(RandomNext(&Rng) % (u32)World->IslandCount);
  if((Tick % 20) == 0)
  {
   villager_action Drop = {0};
   Drop.Kind = VillagerAction_DropSurplus;
   Drop.IslandId = I;
   Drop.Resource = (resource_id)(RandomNext(&Rng) % (u32)Config.ResourceCount);
   Drop.Qty = 1 + (s32)(RandomNext(&Rng) % 12);
   WorldDispatchVillager(World, &Drop);
   island_id Dest = (island_id)(RandomNext(&Rng) % (u32)World->IslandCount);
   s32 Fluid = (s32)(RandomNext(&Rng) % 3);
   WorldSpawnPod(World, I, Dest, Drop.Resource, Drop.Qty - Fluid, Fluid);
  }
  if((Tick % 7) == 0 && World->Islands[I].QueueCount > 0)
  {
   island_node *Node = &World->Islands[I];
   island_id To = Node->Neighbors[RandomNext(&Rng) % (u32)Node->NeighborCount].TargetId;
   Launches += WorldTestLaunch(World, Node->Queue[0], To) ? 1 : 0;
  }
  if((Tick % 400) == 0)
  {
   chaos_action Storm = {0};
   Storm.Kind = ChaosAction_Storm;
   Storm.Islands[0] = I;
   Storm.IslandCount = 1;
   Storm.DurationSec = 15.0f;
   WorldDispatchChaos(World, &Storm);
  }
  WorldStep(World, ENGINE_TICK_SECONDS);
  Ok = Ok && WorldCheckConservation(World) && SimPodsAccounted(World);
 }
 Log("  stress: %d launches, %d delivered, %d expired units, %d held\n", Launches,
     World->Stats.DeliveriesCompleted, World->Stats.UnitsExpired, World->Stats.PodsHeldDtn);
 Check(Launches > 50 && World->Stats.DeliveriesCompleted > 0, "stress actually moves pods");
 Check(Ok, "conservation + every pod accounted for at every tick under stress");
}

//- Need model (spec 3.3). Node-level, both D1 options.
internal void
SimNeedTicks(island_node *Node, const engine_config *Config, f32 Seconds)
{
 s32 Ticks = (s32)(Seconds/ENGINE_TICK_SECONDS + 0.5f);
 for(s32 T = 0; T < Ticks; T += 1)
 {
  NeedTick(Node, Config, ENGINE_TICK_SECONDS, 0.0f);
 }
}

internal void
TestNeedCadence(void)
{
 engine_config Config = DefaultConfig();
 Config.NeedModel = NeedModel_AbstractCadence;
 const physics_constants *K = &Config.K;
 island_node Node = {0};
 Node.Population = 100;
 Node.Makers[0].Used = true;
 Node.Makers[0].Craft = 1;
 Node.Makers[1].Used = true;
 Node.Makers[1].Craft = 1;
 NeedInit(&Node, &Config);
 need_state *Need = &Node.Need[0];

 Check(NeedScore(&Node, &Config, 0) < 0.01f, "fresh island has no need");
 SimNeedTicks(&Node, &Config, Need->TauHat);
 Check(fabsf(NeedScore(&Node, &Config, 0) - K->WPro) < 1.5f, "after one learned cadence N ~ W_pro");
 SimNeedTicks(&Node, &Config, Need->TauHat);
 f32 Capped = K->WPro*powf(1.25f, K->Gamma);
 Check(fabsf(NeedScore(&Node, &Config, 0) - Capped) < 0.5f, "N saturates at the 1.25 cadence cap");

 NeedOnDelivery(&Node, &Config, 0, 10, 0.0f, true);
 Check(NeedScore(&Node, &Config, 0) < 0.01f && Need->Covered, "final delivery satisfies the need");
 NeedOnDemandEvent(&Node, &Config, 0, -1.0f, 0.0f);
 Check(Need->Shortages == 0, "a delivery absorbs the next demand event");
 NeedOnDemandEvent(&Node, &Config, 0, -1.0f, 0.0f);
 Check(Need->Shortages == 1, "a demand event with nothing delivered is a shortage");

 f32 TauBefore = Need->TauHat;
 for(s32 I = 0; I < 10; I += 1) NeedOnDemandEvent(&Node, &Config, 0, 100.0f, 0.0f);
 Check(Need->TauHat < TauBefore && fabsf(Need->TauHat - 100.0f) < 5.0f, "TauHat learns the demand interval");

 Need->ElapsedSec = 80.0f;
 TauBefore = Need->TauHat;
 NeedOnFluidClaim(&Node, &Config, 0, Config.BatchSize/2);
 Check(fabsf(Need->ElapsedSec - 40.0f) < 0.01f && Need->TauHat == TauBefore,
       "Fluid claim of half a batch halves ElapsedSec, TauHat untouched");

 NeedOnTap(&Node, &Config, 0, 0.0f);
 Check(fabsf(Need->U - K->TapBoost) < 0.01f, "tap adds TapBoost urgency");
 Check(fabsf(Need->TauHat - Clamp(K->TauMin, TauBefore*K->TapTauNudge, K->TauMax)) < 0.01f, "tap nudges TauHat down");
 Check(NeedScore(&Node, &Config, 0) >= K->TapBoost, "tap shows up in the score at once");
 SimNeedTicks(&Node, &Config, 60.0f);
 Check(fabsf(Need->U - (K->TapBoost - 60.0f*K->LambdaU)) < 0.1f, "urgency decays linearly at lambda_U");

 // Own needs first: OwnShare of 2 makers x 100 s stays home, the rest drains to the shed.
 NeedDrainToShed(&Node, &Config, 1);           // empty what the earlier ticks produced
 Node.Shed[1] = 0;
 Node.Need[1].ProductionAccum = 0.0f;
 SimNeedTicks(&Node, &Config, 100.0f);
 f32 Expected = 2.0f*K->ProductionPerMaker*100.0f*(1.0f - K->OwnShare);
 f32 Accum = Node.Need[1].ProductionAccum;
 s32 Drained = NeedDrainToShed(&Node, &Config, 1);
 Check(Node.Shed[1] == Drained && Drained >= (s32)Expected - 1 && Drained <= (s32)Expected, "surplus drains to the shed");
 Check(fabsf((f32)Drained + Node.Need[1].ProductionAccum - Accum) < 0.001f, "drain keeps the fraction for later");
 Check(NeedDrainToShed(&Node, &Config, 0) == 0, "no makers, no surplus");
}

internal void
TestNeedStock(void)
{
 engine_config Config = DefaultConfig();
 Config.NeedModel = NeedModel_StockBased;
 const physics_constants *K = &Config.K;
 island_node Node = {0};
 Node.Population = 100;
 NeedInit(&Node, &Config);
 need_state *Need = &Node.Need[0];

 Check(fabsf(Need->Stock - K->TargetCoverDays*K->PerCapita*100.0f) < 0.001f, "starts with target cover");
 Check(NeedScore(&Node, &Config, 0) < 0.01f, "full cover means no need");

 // Households use 1 unit per 60 s: 0.02 units/person/sim-day at 100 people, 120 s days.
 for(s32 I = 0; I < 12; I += 1) NeedOnDemandEvent(&Node, &Config, 0, 60.0f, 0.0f);
 f32 TrueRate = K->ConsumePerEvent*Config.DayLengthSeconds/(60.0f*100.0f);
 Check(fabsf(Need->PerCapitaHat - TrueRate) < 0.002f, "PerCapitaHat learns the consumption rate");
 Check(Need->Stock == 0.0f && Need->Shortages > 0, "stock runs out and shortages are counted");
 Check(NeedScore(&Node, &Config, 0) > 99.0f, "empty stock means maximum need");

 f32 Daily = Need->PerCapitaHat*100.0f;
 s32 Fill = (s32)ceilf(K->SurplusCoverDays*Daily);
 NeedOnDelivery(&Node, &Config, 0, Fill, 0.0f, true);
 Check(NeedScore(&Node, &Config, 0) < 0.01f, "delivery restores cover");
 Check(NeedDrainToShed(&Node, &Config, 0) == 0, "stock below the surplus line stays home");
 NeedOnDelivery(&Node, &Config, 0, 5, 0.0f, true);
 Check(NeedDrainToShed(&Node, &Config, 0) == 5 && Node.Shed[0] == 5, "only stock above SurplusCoverDays leaves");

 f32 RateBefore = Need->PerCapitaHat;
 NeedOnTap(&Node, &Config, 0, 0.0f);
 Check(fabsf(Need->PerCapitaHat - RateBefore*K->TapPerCapitaNudge) < 0.0001f && Need->U > 0.0f,
       "tap raises PerCapitaHat and urgency");
}

// "Learns each island's rhythm": run the world and compare the learned estimate with the hidden truth.
internal void
TestNeedLearning(need_model_kind Model)
{
 world *World = SimAllocWorld();
 engine_config Config = DefaultConfig();
 Config.NeedModel = Model;
 WorldInit(World, &Config);

 f32 ErrorStart = 0.0f, ErrorEnd = 0.0f;
 b32 ScoresOk = true;
 s32 Ticks = (s32)(12.0f*Config.DayLengthSeconds/ENGINE_TICK_SECONDS);
 for(s32 Pass = 0; Pass < 2; Pass += 1)
 {
  f32 Error = 0.0f;
  for(s32 I = 0; I < World->IslandCount; I += 1)
  {
   for(s32 R = 0; R < Config.ResourceCount; R += 1)
   {
    need_state *Need = &World->Islands[I].Need[R];
    f32 TrueTau = World->DemandTauTrue[I][R];
    if(Model == NeedModel_AbstractCadence)
    {
     Error += fabsf(Need->TauHat - TrueTau)/TrueTau;
    }
    else
    {
     f32 TrueRate = Config.K.ConsumePerEvent*Config.DayLengthSeconds/(TrueTau*(f32)World->Islands[I].Population);
     Error += fabsf(Need->PerCapitaHat - TrueRate)/TrueRate;
    }
   }
  }
  Error /= (f32)(World->IslandCount*Config.ResourceCount);
  if(Pass == 0)
  {
   ErrorStart = Error;
   for(s32 T = 0; T < Ticks; T += 1)
   {
    WorldStep(World, ENGINE_TICK_SECONDS);
    for(s32 I = 0; I < World->IslandCount; I += 1)
    {
     for(s32 R = 0; R < Config.ResourceCount; R += 1)
     {
      f32 N = World->Islands[I].Need[R].Score;
      ScoresOk = ScoresOk && N >= 0.0f && N <= 100.0f && World->Islands[I].Need[R].Stock >= 0.0f;
     }
    }
    ScoresOk = ScoresOk && WorldCheckConservation(World);
   }
  }
  else
  {
   ErrorEnd = Error;
  }
 }
 Log("  %s: mean relative error of learned rhythm %.2f -> %.2f after 12 days, %d units produced\n",
     (Model == NeedModel_AbstractCadence) ? "Option A TauHat" : "Option B PerCapitaHat",
     ErrorStart, ErrorEnd, World->Stats.UnitsProduced);
 Check(ErrorEnd < 0.5f*ErrorStart, "islands learn the hidden rhythm");
 Check(ScoresOk, "scores stay in [0,100], stock >= 0, conservation holds");
}

internal s32
RunTests(cmd_line *CmdLine)
{
 struct { char *Name; s32 Kind; } Tests[] =
 {
  {"1 isolation (static)", 1}, {"9 biconnectivity", 9}, {"contract: MockWorld", 100},
  {"sun cycle", 101}, {"energy: node state machine + launch guard", 102},
  {"energy: world day/night + storms", 103},
  {"transport: launch, flight, relay, delivery, holds", 104},
  {"transport: mid-flight passive landing (D3 option 2)", 105},
  {"transport: mid-flight return to sender (D3 option 1)", 106},
  {"transport: TTL expiry + queue capacity", 107},
  {"sync: gossip timer vs sneakernet", 108},
  {"need: Option A abstract cadence", 110}, {"need: Option B stock", 111},
  {"need: Option A learns the hidden rhythm (world)", 112},
  {"need: Option B learns the hidden rhythm (world)", 113},
  {"8 conservation (drops)", 8}, {"8 conservation (stress with pods)", 109},
  {"7 determinism (storms + pods)", 7},
 };

 for EachIndex(T, ArrayCount(Tests))
 {
  s32 FailsBefore = GlobalFailCount;
  switch(Tests[T].Kind)
  {
   case 1:   TestIsolation(CmdLine); break;
   case 9:   TestBiconnectivity(); break;
   case 100: TestMockWorld(); break;
   case 101: TestSunCycle(); break;
   case 102: TestEnergy(); break;
   case 103: TestWorldPower(); break;
   case 104: TestTransport(); break;
   case 105: TestMidFlight(MidFlight_PassiveMechanicalLanding); break;
   case 106: TestMidFlight(MidFlight_PrelaunchEnergyCheck); break;
   case 107: TestExpiryAndQueue(); break;
   case 108: TestGossipTimer(); break;
   case 109: TestConservationStress(); break;
   case 110: TestNeedCadence(); break;
   case 111: TestNeedStock(); break;
   case 112: TestNeedLearning(NeedModel_AbstractCadence); break;
   case 113: TestNeedLearning(NeedModel_StockBased); break;
   case 8:   TestConservation(); break;
   case 7:   TestDeterminism(); break;
  }
  Log("[%s] %s\n", (GlobalFailCount == FailsBefore) ? "PASS" : "FAIL", Tests[T].Name);
 }

 Log("\n%d checks, %d failed. TODO: tests 2-6 and 10 need the node layer (L1) and transport.\n",
     GlobalCheckCount, GlobalFailCount);
 return GlobalFailCount;
}

//~ Sim (tuning report)
internal void
RunSim(u64 Seed, s32 Days, scenario_kind Scenario)
{
 world *World = SimAllocWorld();
 if(Scenario == Scenario_None)
 {
  engine_config Config = DefaultConfig();
  Config.Seed = Seed;
  WorldInit(World, &Config);
 }
 else
 {
  // NOTE(L4): Presets own their seed so they replay identically; --seed is ignored here.
  ScenarioLoad(World, Scenario);
 }

 Log("SolarSim sim: seed %llu, %d days, scenario %s, %d islands, %d links\n",
     World->Config.Seed, Days, ScenarioNames[Scenario], World->IslandCount, World->LinkCount);

 char *StateNames[] = { "day", "night", "ASLEEP", "STORM" };
 s32 TicksPerDay = (s32)(World->Config.DayLengthSeconds/ENGINE_TICK_SECONDS + 0.5f);
 for(s32 Day = 1; Day <= Days; Day += 1)
 {
  // NOTE(L4): -1 = no sunset/sunrise seen for that island during this day.
  f32 DuskE[MAX_ISLANDS], DawnE[MAX_ISLANDS], MinE[MAX_ISLANDS], AsleepSec[MAX_ISLANDS];
  for(s32 I = 0; I < World->IslandCount; I += 1)
  {
   DuskE[I] = DawnE[I] = -1.0f;
   MinE[I] = World->Islands[I].Energy;
   AsleepSec[I] = 0.0f;
  }

  for EachIndex(Tick, TicksPerDay)
  {
   power_state Before[MAX_ISLANDS];
   f32 EnergyBefore[MAX_ISLANDS];
   for(s32 I = 0; I < World->IslandCount; I += 1)
   {
    Before[I] = World->Islands[I].PowerState;
    EnergyBefore[I] = World->Islands[I].Energy;
   }
   WorldStep(World, ENGINE_TICK_SECONDS);
   for(s32 I = 0; I < World->IslandCount; I += 1)
   {
    island_node *Node = &World->Islands[I];
    b32 WasDay = (Before[I] == PowerState_SolarHarvesting);
    b32 IsDay = (Node->PowerState == PowerState_SolarHarvesting);
    if(WasDay && !IsDay) DuskE[I] = EnergyBefore[I];
    if(!WasDay && IsDay) DawnE[I] = EnergyBefore[I];
    MinE[I] = Min(MinE[I], Node->Energy);
    AsleepSec[I] += (Node->PowerState == PowerState_Sleeping) ? ENGINE_TICK_SECONDS : 0.0f;
   }
  }

  world_stats *S = &World->Stats;
  s32 Asleep = 0;
  s32 SleptEarly = 0;
  for(s32 I = 0; I < World->IslandCount; I += 1)
  {
   Asleep += (World->Islands[I].PowerState == PowerState_Sleeping) ? 1 : 0;
   SleptEarly += (AsleepSec[I] > 0.0f) ? 1 : 0;
  }
  Log("\nday %d  t=%.0f  delivered %d (in outage %d, via night relay %d)  held %d  "
      "energy spent %.1f  per unit %.2f  slept-before-dawn %d  asleep-now %d\n",
      Day, World->SimTime, S->DeliveriesCompleted, S->DeliveriesCompletedDuringOutage,
      S->DeliveriesViaNightRelay, S->PodsHeldDtn, S->EnergySpentTotal, S->EnergyPerUnitDelivered,
      SleptEarly, Asleep);
  Log("   isl     x   dusk   dawn    min  asleep(s)  now\n");
  for(s32 I = 0; I < World->IslandCount; I += 1)
  {
   island_node *Node = &World->Islands[I];
   Log("   %3d  %.2f  %5.1f  %5.1f  %5.1f  %9.1f  %s (E %.1f)\n", I, Node->X, DuskE[I], DawnE[I],
       MinE[I], AsleepSec[I], StateNames[Node->PowerState], Node->Energy);
  }
 }
}

//~ Entry
internal char *
SimArgValue(int ArgCount, char **Args, char *Flag, char *Default)
{
 char *Result = Default;
 for(int A = 1; A + 1 < ArgCount; A += 1)
 {
  if(strcmp(Args[A], Flag) == 0)
  {
   Result = Args[A + 1];
  }
 }
 return Result;
}

ENTRY_POINT(EntryPoint)
{
 int ExitCode = 0;
 char *Command = (Params->ArgCount >= 2) ? Params->Args[1] : "";

 if(strcmp(Command, "test") == 0)
 {
  ExitCode = RunTests(&Params->CmdLine);
 }
 else if(strcmp(Command, "sim") == 0)
 {
  u64 Seed = (u64)atoll(SimArgValue(Params->ArgCount, Params->Args, "--seed", "42"));
  s32 Days = atoi(SimArgValue(Params->ArgCount, Params->Args, "--days", "5"));
  char *ScenarioArg = SimArgValue(Params->ArgCount, Params->Args, "--scenario", "-");
  scenario_kind Scenario = Scenario_None;
  if(ScenarioArg[0] >= 'A' && ScenarioArg[0] <= 'D')
  {
   Scenario = (scenario_kind)(Scenario_A_NightRelay + (ScenarioArg[0] - 'A'));
  }
  RunSim(Seed, Days, Scenario);
 }
 else
 {
  Log("usage: SolarSim test\n"
      "       SolarSim sim [--seed 42] [--days 5] [--scenario A|B|C|D]\n");
 }

 fflush(stdout);
 exit(ExitCode);
}
