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
    Drop.IslandId = 0; Drop.Resource = 0; Drop.Qty = 10;
    WorldDispatchVillager(World, &Drop);
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

internal s32
RunTests(cmd_line *CmdLine)
{
 struct { char *Name; s32 Kind; } Tests[] =
 {
  {"1 isolation (static)", 1}, {"9 biconnectivity", 9}, {"contract: MockWorld", 100},
  {"sun cycle", 101}, {"energy: node state machine + launch guard", 102},
  {"energy: world day/night + storms", 103},
  {"8 conservation (skeleton)", 8}, {"7 determinism (skeleton)", 7},
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
