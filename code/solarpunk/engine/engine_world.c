//-
// World: WorldInit, WorldStep pipeline (spec 5.1), dispatch, events, conservation, MockWorld.
// Lane L4. This is the simulator (physics and time), not part of the network: it owns the
// pod pool and links, and only talks to nodes through the node API in engine.h.
//-

//~ Config
internal engine_config
DefaultConfig(void)
{
 engine_config Result = {0};

 Result.Seed = DEFAULT_SEED;
 Result.DayLengthSeconds = DEFAULT_DAY_LENGTH_SECONDS;
 Result.NeedModel = DEFAULT_NEED_MODEL;
 Result.SyncMode = DEFAULT_SYNC_MODE;
 Result.GossipIntervalSeconds = DEFAULT_GOSSIP_INTERVAL_SECONDS;
 Result.MidFlightPolicy = DEFAULT_MID_FLIGHT_POLICY;
 Result.EmergencyBorrowPolicy = DEFAULT_BORROW_POLICY;
 Result.ResourceCount = DEFAULT_RESOURCE_COUNT;
 Result.IslandCount = DEFAULT_ISLAND_COUNT;
 Result.Population = DEFAULT_POPULATION;
 Result.EnableSpoilage = DEFAULT_ENABLE_SPOILAGE;
 Result.EnableWeightLimits = DEFAULT_ENABLE_WEIGHT;
 Result.EnableRobotIslands = DEFAULT_ENABLE_ROBOTS;
 Result.RebindOnSatisfied = DEFAULT_REBIND_ON_SATISFIED;
 Result.DefaultReservedQty = DEFAULT_RESERVED_QTY;
 Result.DefaultFluidQty = DEFAULT_FLUID_QTY;
 Result.BatchSize = DEFAULT_BATCH_SIZE;
 Result.FluidClaimNeedThreshold = DEFAULT_FLUID_CLAIM_THRESHOLD;
 Result.DestNeedMin = DEFAULT_DEST_NEED_MIN;
 Result.MinLaunchReserve = DEFAULT_MIN_LAUNCH_RESERVE;
 Result.LaunchCooldownSeconds = DEFAULT_LAUNCH_COOLDOWN_SECONDS;
 Result.ScoutCooldownSeconds = DEFAULT_SCOUT_COOLDOWN_SECONDS;
 Result.ScoutQty = DEFAULT_SCOUT_QTY;
 Result.WarmupDays = DEFAULT_WARMUP_DAYS;
 Result.MaxQueueCapacity = DEFAULT_MAX_QUEUE_CAPACITY;
 Result.PodSpeed = DEFAULT_POD_SPEED;
 Result.PodMaxTtlSeconds = DEFAULT_POD_MAX_TTL_SECONDS;

 physics_constants *K = &Result.K;
 K->SunPhase0 = DEFAULT_SUN_PHASE0;
 K->EpsDawn = DEFAULT_EPS_DAWN;
 K->EMax = DEFAULT_E_MAX;
 K->EInitial = DEFAULT_E_INITIAL;
 K->KHarvest = DEFAULT_K_HARVEST;
 K->PIdle = DEFAULT_P_IDLE;
 K->PNight = DEFAULT_P_NIGHT;
 K->KHold = DEFAULT_K_HOLD;
 K->PStorm = DEFAULT_P_STORM;
 K->CDay = DEFAULT_C_DAY;
 K->CNight = DEFAULT_C_NIGHT;
 K->Alpha = DEFAULT_ALPHA;
 K->Wd = DEFAULT_W_D;
 K->WHop = DEFAULT_W_HOP;
 K->We = DEFAULT_W_E;
 K->Ws = DEFAULT_W_S;
 K->WPath = DEFAULT_W_PATH;
 K->ESafe = DEFAULT_E_SAFE;
 K->MSleep = DEFAULT_M_SLEEP;
 K->TStale = DEFAULT_T_STALE;
 K->Attenuation = DEFAULT_ATTENUATION;
 K->SharpNeed = DEFAULT_SHARP_NEED;
 K->WPro = DEFAULT_W_PRO;
 K->Gamma = DEFAULT_GAMMA;
 K->LambdaU = DEFAULT_LAMBDA_U;
 K->TapBoost = DEFAULT_TAP_BOOST;
 K->DeltaU = DEFAULT_DELTA_U;
 K->Beta = DEFAULT_BETA;
 K->TauMin = DEFAULT_TAU_MIN;
 K->TauMax = DEFAULT_TAU_MAX;
 K->TapTauNudge = DEFAULT_TAP_TAU_NUDGE;
 K->OwnShare = DEFAULT_OWN_SHARE;
 K->TauTrueMin = DEFAULT_TAU_TRUE_MIN;
 K->TauTrueMax = DEFAULT_TAU_TRUE_MAX;
 K->DemandJitter = DEFAULT_DEMAND_JITTER;
 K->ProductionPerMaker = DEFAULT_PRODUCTION_PER_MAKER;
 K->TargetCoverDays = DEFAULT_TARGET_COVER_DAYS;
 K->SurplusCoverDays = DEFAULT_SURPLUS_COVER_DAYS;
 K->PerCapita = DEFAULT_PER_CAPITA;
 K->PerCapitaBeta = DEFAULT_PER_CAPITA_BETA;
 K->TapPerCapitaNudge = DEFAULT_TAP_PER_CAPITA_NUDGE;
 K->ConsumePerEvent = DEFAULT_CONSUME_PER_EVENT;
 K->YMin = DEFAULT_Y_MIN;
 K->YMax = DEFAULT_Y_MAX;
 K->XJitter = DEFAULT_X_JITTER;

 return Result;
}

//~ Events
internal void
WorldPushEvent(world *World, sim_event Event)
{
 Event.T = World->SimTime;
 if(World->EventHead - World->EventTail >= MAX_EVENTS)
 {
  World->EventTail += 1; // NOTE(L4): full, overwrite the oldest
 }
 World->Events[World->EventHead % MAX_EVENTS] = Event;
 World->EventHead += 1;
}

internal b32
WorldPopEvent(world *World, sim_event *Event)
{
 b32 Result = (World->EventTail != World->EventHead);
 if(Result)
 {
  *Event = World->Events[World->EventTail % MAX_EVENTS];
  World->EventTail += 1;
 }
 return Result;
}

internal sim_event
WorldEvent(sim_event_kind Kind, island_id A, island_id B, pod_id Pod)
{
 sim_event Result = {0};
 Result.Kind = Kind;
 Result.A = A;
 Result.B = B;
 Result.Pod = Pod;
 Result.Resource = -1;
 return Result;
}

//~ Read-only helpers
internal b32
WorldIsValidIsland(const world *World, island_id Island)
{
 b32 Result = (Island >= 0 && Island < World->IslandCount);
 return Result;
}

internal b32
WorldIsAwake(const world *World, island_id Island)
{
 b32 Result = (WorldIsValidIsland(World, Island) && EnergyIsAwake(&World->Islands[Island]));
 return Result;
}

internal f32
WorldAvgKnowledgeAge(const world *World, island_id Island)
{
 f32 Result = 0.0f;
 if(WorldIsValidIsland(World, Island))
 {
  const island_node *Node = &World->Islands[Island];
  f32 Sum = 0.0f;
  s32 Count = 0;
  for(s32 I = 0; I < World->IslandCount; I += 1)
  {
   if(I != Island && Node->Knowledge[I].Valid)
   {
    Sum += World->SimTime - Node->Knowledge[I].UpdatedAt;
    Count += 1;
   }
  }
  Result = (Count > 0) ? Sum/(f32)Count : 0.0f;
 }
 return Result;
}

internal s32
WorldUnitsInShed(const world *World)
{
 s32 Result = 0;
 for(s32 I = 0; I < World->IslandCount; I += 1)
 {
  for EachIndex(R, MAX_RESOURCES)
  {
   Result += World->Islands[I].Shed[R];
  }
 }
 return Result;
}

internal s32
WorldUnitsInPods(const world *World, pod_status Status)
{
 s32 Result = 0;
 for EachIndex(P, MAX_PODS)
 {
  const pod *Pod = &World->Pods[P];
  if(Pod->Status == Status)
  {
   Result += Pod->ReservedQty + Pod->FluidQty;
  }
 }
 return Result;
}

// NOTE(L4): Spec 0.6. Produced counts only units that entered a shed (drain + villager drops).
internal b32
WorldCheckConservation(const world *World)
{
 const world_stats *S = &World->Stats;
 s32 Accounted = (WorldUnitsInShed(World) +
                  WorldUnitsInPods(World, PodStatus_Queued) +
                  WorldUnitsInPods(World, PodStatus_InTransit) +
                  S->UnitsDelivered + S->UnitsClaimedEnRoute + S->UnitsExpired);
 b32 Result = (Accounted == S->UnitsProduced);
 return Result;
}

//~ Init
internal power_state
WorldPowerStateFromSun(const engine_config *Config, f32 Sun, f32 Energy)
{
 power_state Result = PowerState_SolarHarvesting;
 if(Sun <= Config->K.EpsDawn)
 {
  Result = (Energy > 0.0f) ? PowerState_NightReserve : PowerState_Sleeping;
 }
 return Result;
}

internal void
WorldInit(world *World, const engine_config *Config)
{
 MemorySet(World, 0, sizeof(*World));
 World->Config = *Config;
 engine_config *C = &World->Config;
 C->ResourceCount = Clamp(1, C->ResourceCount, MAX_RESOURCES);
 C->IslandCount = Clamp(3, C->IslandCount, MAX_ISLANDS);
 C->MaxQueueCapacity = Clamp(1, C->MaxQueueCapacity, MAX_QUEUE);

 RandomSeed(&World->Rng, C->Seed);
 World->SunPhase = C->K.SunPhase0;
 World->NextGossipAt = C->GossipIntervalSeconds;

 TopologyBuildDefault(World);

 for(s32 I = 0; I < World->IslandCount; I += 1)
 {
  island_node *Node = &World->Islands[I];
  RandomSeed(&Node->Rng, C->Seed ^ (0x9E3779B97F4A7C15ULL*(u64)(I + 1)));
  Node->Population = C->Population;
  Node->Energy = C->K.EInitial;
  Node->Sun = EngineSunIntensity(Node->X, World->SunPhase);
  Node->PowerState = WorldPowerStateFromSun(C, Node->Sun, Node->Energy);

  // TODO(L4): placeholder makers until scenarios define crafts (D5 catalog is open).
  for EachIndex(M, 2)
  {
   maker *Maker = &Node->Makers[M];
   Maker->Used = true;
   Maker->VillagerId = M;
   Maker->Craft = (I + M) % C->ResourceCount;
  }

  NeedInit(Node, C);
  NodeInit(Node, C);

  for(s32 R = 0; R < C->ResourceCount; R += 1)
  {
   f32 Tau = RandomBetween(&World->Rng, C->K.TauTrueMin, C->K.TauTrueMax);
   World->DemandTauTrue[I][R] = Tau;
   World->DemandNextAt[I][R] = Tau*RandomUnilateral(&World->Rng);
   World->DemandLastAt[I][R] = -1.0f;
  }
 }
}

//~ Dispatch
internal void
WorldDispatchVillager(world *World, const villager_action *Action)
{
 if(WorldIsValidIsland(World, Action->IslandId))
 {
  island_node *Node = &World->Islands[Action->IslandId];
  b32 ValidResource = (Action->Resource >= 0 && Action->Resource < World->Config.ResourceCount);
  switch(Action->Kind)
  {
   case VillagerAction_DropSurplus:
   {
    if(ValidResource && Action->Qty > 0)
    {
     Node->Shed[Action->Resource] += Action->Qty;
     Node->Dropped[Action->Resource] += Action->Qty;
     World->Stats.UnitsProduced += Action->Qty;
    }
   } break;

   case VillagerAction_TapRunningLow:
   {
    if(ValidResource)
    {
     NeedOnTap(Node, &World->Config, Action->Resource, World->SimTime);
     sim_event Event = WorldEvent(SimEvent_NeedRaised, Node->Id, -1, -1);
     Event.Resource = Action->Resource;
     WorldPushEvent(World, Event);
    }
   } break;

   case VillagerAction_SetCraft:
   {
    if(ValidResource)
    {
     NodeOnVillagerSetCraft(Node, Action->VillagerId, Action->Resource, World->SimTime);
    }
   } break;
  }
 }
}

internal void
WorldDispatchChaos(world *World, const chaos_action *Action)
{
 switch(Action->Kind)
 {
  case ChaosAction_Storm:
  {
   for(s32 I = 0; I < Min(Action->IslandCount, MAX_ISLANDS); I += 1)
   {
    island_id Id = Action->Islands[I];
    if(WorldIsValidIsland(World, Id) && Action->DurationSec > 0.0f)
    {
     World->Islands[Id].StormUntil = World->SimTime + Action->DurationSec;
     WorldPushEvent(World, WorldEvent(SimEvent_StormStarted, Id, -1, -1));
    }
   }
  } break;

  case ChaosAction_EndAllStorms:
  {
   for(s32 I = 0; I < World->IslandCount; I += 1)
   {
    if(World->Islands[I].StormUntil > 0.0f)
    {
     // NOTE(L4): Expired on the next tick by the storm check in WorldStep.
     World->Islands[I].StormUntil = World->SimTime;
    }
   }
  } break;

  case ChaosAction_SetSyncMode:
  {
   World->Config.SyncMode = Action->Mode;
  } break;

  case ChaosAction_SetConfig:
  {
   // NOTE(L4): Structural fields (Seed, IslandCount) only take effect on the next WorldInit.
   engine_config Patch = Action->ConfigPatch;
   Patch.Seed = World->Config.Seed;
   Patch.IslandCount = World->Config.IslandCount;
   Patch.ResourceCount = Clamp(1, Patch.ResourceCount, MAX_RESOURCES);
   Patch.MaxQueueCapacity = Clamp(1, Patch.MaxQueueCapacity, MAX_QUEUE);
   World->Config = Patch;
  } break;
 }
}

//~ Pipeline helpers
// NOTE(L4): What a node may perceive about its direct neighbours (spec 4.2 neighbor_obs). Awake
// is visible (beacon); the reserve estimate needs a radio/optical link, so not in sneakernet.
internal s32
WorldBuildObs(const world *World, island_id Id, neighbor_obs *Obs)
{
 const island_node *Node = &World->Islands[Id];
 b32 Radio = (World->Config.SyncMode != SyncMode_Sneakernet);
 for(s32 N = 0; N < Node->NeighborCount; N += 1)
 {
  const neighbor_link *Link = &Node->Neighbors[N];
  const island_node *Other = &World->Islands[Link->TargetId];
  neighbor_obs *O = &Obs[N];
  MemorySet(O, 0, sizeof(*O));
  O->NeighborId = Link->TargetId;
  O->IsReachable = EnergyIsAwake(Other);
  O->ObservedPowerState = Other->PowerState;
  O->HasReserveEstimate = (Radio && O->IsReachable && EnergyIsAwake(Node));
  O->EstimatedReserve = O->HasReserveEstimate ? Other->Energy : 0.0f;
  O->Distance = Link->Distance;
  O->WrapsGlobe = Link->WrapsGlobe;
 }
 return Node->NeighborCount;
}

internal s32
WorldNeighborIndex(const island_node *Node, island_id Other)
{
 s32 Result = -1;
 for(s32 N = 0; N < Node->NeighborCount; N += 1)
 {
  if(Node->Neighbors[N].TargetId == Other)
  {
   Result = N;
   break;
  }
 }
 return Result;
}

internal pod_id
WorldAllocPod(world *World)
{
 pod_id Result = -1;
 for EachIndex(P, MAX_PODS)
 {
  if(World->Pods[P].Status == PodStatus_Free)
  {
   Result = P;
   MemorySet(&World->Pods[P], 0, sizeof(pod));
   break;
  }
 }
 return Result;
}

internal b32
WorldQueueHasRoom(const world *World, island_id At)
{
 b32 Result = (World->Islands[At].QueueCount < World->Config.MaxQueueCapacity);
 return Result;
}

// NOTE(L4): Custody moves to At. Caller checks WorldQueueHasRoom first.
internal void
WorldQueuePush(world *World, island_id At, pod_id Id)
{
 island_node *Node = &World->Islands[At];
 Assert(Node->QueueCount < MAX_QUEUE);
 Node->Queue[Node->QueueCount++] = Id;
 pod *Pod = &World->Pods[Id];
 Pod->Status = PodStatus_Queued;
 Pod->CustodianId = At;
 Pod->ToId = -1;
 Pod->Progress = 0.0f;
}

internal void
WorldQueueRemove(world *World, island_id At, pod_id Id)
{
 island_node *Node = &World->Islands[At];
 s32 Write = 0;
 for(s32 Read = 0; Read < Node->QueueCount; Read += 1)
 {
  if(Node->Queue[Read] != Id)
  {
   Node->Queue[Write++] = Node->Queue[Read];
  }
 }
 Node->QueueCount = Write;
}

// NOTE(L4): Shared by step 3 and WorldSpawnPod. The units already left the shed.
internal void
WorldFinishNewPod(world *World, island_id Origin, pod_id Id)
{
 pod *Pod = &World->Pods[Id];
 Pod->OriginId = Origin;
 Pod->FromId = Origin;
 Pod->InitialReservedQty = Pod->ReservedQty;
 Pod->InitialFluidQty = Pod->FluidQty;
 Pod->CreatedAt = World->SimTime;
 Pod->ExpiresAt = World->SimTime + World->Config.PodMaxTtlSeconds;
 Pod->HoldReason = HoldReason_None;
 Pod->Visited[0] = Origin;
 Pod->VisitedCount = 1;
 WorldQueuePush(World, Origin, Id);

 sim_event Event = WorldEvent(SimEvent_PodCreated, Origin, Pod->DestId, Id);
 Event.Resource = Pod->Resource;
 Event.Qty = Pod->ReservedQty;
 Event.Qty2 = Pod->FluidQty;
 WorldPushEvent(World, Event);
}

// NOTE(L4): Stats and events only when the reason changes, not every tick a pod waits.
internal void
WorldMarkHeld(world *World, pod_id Id, hold_reason Reason)
{
 pod *Pod = &World->Pods[Id];
 if(Reason == HoldReason_NeighborsAsleep || Reason == HoldReason_Storm)
 {
  Pod->SawOutage = true;
 }
 if(Pod->HoldReason != Reason)
 {
  if(Pod->HoldReason == HoldReason_None)
  {
   World->Stats.PodsHeldDtn += 1;
  }
  Pod->HoldReason = Reason;
  sim_event Event = WorldEvent(SimEvent_PodHeldDtn, Pod->CustodianId, Pod->DestId, Id);
  Event.Resource = Pod->Resource;
  Event.Reason = Reason;
  WorldPushEvent(World, Event);
 }
}

// NOTE(L4): D18. The hop was chosen elsewhere; this checks the link and the receiver, then runs
// the energy guard (EnergyTryLaunch deducts). Returns HoldReason_None when the pod is airborne.
internal hold_reason
WorldLaunch(world *World, island_id At, pod_id Id, island_id To)
{
 hold_reason Result = HoldReason_None;
 island_node *Node = &World->Islands[At];
 pod *Pod = &World->Pods[Id];
 s32 N = WorldIsValidIsland(World, To) ? WorldNeighborIndex(Node, To) : -1;

 if(N < 0)
 {
  Result = HoldReason_NeighborsAsleep; // NOTE(L4): not a neighbour, so nowhere to go
 }
 else if(!EnergyIsAwake(&World->Islands[To]))
 {
  Result = ((World->Islands[To].PowerState == PowerState_StormOutage) ?
            HoldReason_Storm : HoldReason_NeighborsAsleep);
 }
 else
 {
  f32 Distance = Node->Neighbors[N].Distance;
  f32 Cost = EnergyLaunchCost(Node, &World->Config, Distance);
  b32 AtNight = (Node->PowerState == PowerState_NightReserve);
  if(!EnergyTryLaunch(Node, &World->Config, Distance))
  {
   Result = HoldReason_LowEnergy;
  }
  else
  {
   World->Stats.EnergySpentTotal += Cost;
   WorldQueueRemove(World, At, Id);
   Pod->Status = PodStatus_InTransit;
   Pod->CustodianId = At;                    // custody stays with the sender until landing
   Pod->FromId = At;
   Pod->ToId = To;
   Pod->Distance = Distance;
   Pod->WrapsGlobe = Node->Neighbors[N].WrapsGlobe;
   Pod->Progress = 0.0f;
   Pod->HoldReason = HoldReason_None;
   if(Pod->LaunchedAt <= 0.0f)
   {
    Pod->LaunchedAt = World->SimTime;
   }
   if(AtNight && At != Pod->OriginId)
   {
    Pod->UsedNightRelay = true;
   }
   MemoryCopy(Pod->Carried, Node->Knowledge, sizeof(Pod->Carried));

   sim_event Event = WorldEvent(SimEvent_PodLaunched, At, To, Id);
   Event.Resource = Pod->Resource;
   Event.Qty = Pod->ReservedQty;
   Event.Qty2 = Pod->FluidQty;
   WorldPushEvent(World, Event);
  }
 }
 return Result;
}

internal void
WorldExpirePod(world *World, pod_id Id)
{
 pod *Pod = &World->Pods[Id];
 if(Pod->Status == PodStatus_Queued)
 {
  WorldQueueRemove(World, Pod->CustodianId, Id);
 }
 World->Stats.UnitsExpired += Pod->ReservedQty + Pod->FluidQty;
 Pod->Status = PodStatus_Expired;
 sim_event Event = WorldEvent(SimEvent_PodExpired, Pod->CustodianId, Pod->DestId, Id);
 Event.Resource = Pod->Resource;
 Event.Qty = Pod->ReservedQty;
 Event.Qty2 = Pod->FluidQty;
 WorldPushEvent(World, Event);
}

// NOTE(L4): Spec 5.1 step 5, Arrival Resolution. If the target queue is full the pod hovers at
// Progress 1 and retries next tick: it never vanishes (7.1).
internal void
WorldArrive(world *World, pod_id Id)
{
 engine_config *Config = &World->Config;
 pod *Pod = &World->Pods[Id];
 island_id To = Pod->ToId;
 island_node *ToNode = &World->Islands[To];

 if(!EnergyIsAwake(ToNode))
 {
  //- Receiver went offline mid-flight (D3)
  Pod->SawOutage = true;
  b32 Passive = (Config->MidFlightPolicy == MidFlight_PassiveMechanicalLanding);
  island_id Target = Passive ? To : Pod->FromId;
  if(WorldQueueHasRoom(World, Target))
  {
   if(Passive && Pod->VisitedCount < MAX_POD_PATH)
   {
    Pod->Visited[Pod->VisitedCount++] = To;
   }
   WorldQueuePush(World, Target, Id);
   if(!Passive)
   {
    sim_event Event = WorldEvent(SimEvent_PodReturned, Pod->FromId, To, Id);
    Event.Resource = Pod->Resource;
    WorldPushEvent(World, Event);
   }
  }
 }
 else if(Pod->DestId != To && !WorldQueueHasRoom(World, To))
 {
  // Hover: wait for room at the relay.
 }
 else
 {
  //- Hand the pod to the node (L1)
  s32 ReservedBefore = Pod->ReservedQty;
  s32 FluidBefore = Pod->FluidQty;
  if(Pod->VisitedCount < MAX_POD_PATH)
  {
   Pod->Visited[Pod->VisitedCount++] = To;
  }
  Pod->CustodianId = To;
  arrival_result Arrival = NodeOnPodArrived(ToNode, Config, Pod, World->SimTime);

  // Invariant 3: intermediates never touch Reserved. The Fluid change must match the report.
  Assert(Pod->ReservedQty == ReservedBefore);
  s32 Claimed = FluidBefore - Pod->FluidQty;
  Assert(Claimed >= 0 && Claimed == Arrival.FluidClaimedQty);
  if(Claimed > 0)
  {
   World->Stats.UnitsClaimedEnRoute += Claimed;
   World->Stats.FluidFlightsSaved += Claimed;
   sim_event Event = WorldEvent(SimEvent_FluidClaimed, To, Pod->DestId, Id);
   Event.Resource = Pod->Resource;
   Event.Qty = Claimed;
   WorldPushEvent(World, Event);
  }

  if(Arrival.IsFinal)
  {
   world_stats *S = &World->Stats;
   S->UnitsDelivered += Pod->ReservedQty + Pod->FluidQty;
   S->DeliveriesCompleted += 1;
   S->DeliveriesCompletedDuringOutage += Pod->SawOutage ? 1 : 0;
   S->DeliveriesViaNightRelay += Pod->UsedNightRelay ? 1 : 0;
   S->TotalDeliverySeconds += World->SimTime - Pod->CreatedAt;
   S->EnergyPerUnitDelivered = ((S->UnitsDelivered > 0) ?
                                S->EnergySpentTotal/(f32)S->UnitsDelivered : 0.0f);
   Pod->Status = PodStatus_Delivered;
   Pod->ToId = -1;

   sim_event Event = WorldEvent(SimEvent_PodDelivered, Pod->OriginId, To, Id);
   Event.Resource = Pod->Resource;
   Event.Qty = Pod->ReservedQty;
   Event.Qty2 = Pod->FluidQty;
   WorldPushEvent(World, Event);
  }
  else
  {
   WorldQueuePush(World, To, Id);
  }
 }
}

//~ Scenario / test helpers
internal pod_id
WorldSpawnPod(world *World, island_id Origin, island_id Dest, resource_id R, s32 Reserved, s32 Fluid)
{
 pod_id Result = -1;
 if(WorldIsValidIsland(World, Origin) && WorldIsValidIsland(World, Dest) && Origin != Dest &&
    R >= 0 && R < World->Config.ResourceCount && Reserved >= 0 && Fluid >= 0 &&
    World->Islands[Origin].Shed[R] >= Reserved + Fluid && WorldQueueHasRoom(World, Origin))
 {
  Result = WorldAllocPod(World);
  if(Result >= 0)
  {
   pod *Pod = &World->Pods[Result];
   Pod->Kind = PodKind_Targeted;
   Pod->Resource = R;
   Pod->DestId = Dest;
   Pod->ReservedQty = Reserved;
   Pod->FluidQty = Fluid;
   World->Islands[Origin].Shed[R] -= Reserved + Fluid;
   WorldFinishNewPod(World, Origin, Result);
  }
 }
 return Result;
}

internal b32
WorldTestLaunch(world *World, pod_id Id, island_id To)
{
 b32 Result = false;
 if(Id >= 0 && Id < MAX_PODS && World->Pods[Id].Status == PodStatus_Queued)
 {
  island_id At = World->Pods[Id].CustodianId;
  if(EnergyIsAwake(&World->Islands[At]))
  {
   hold_reason Hold = WorldLaunch(World, At, Id, To);
   if(Hold != HoldReason_None)
   {
    WorldMarkHeld(World, Id, Hold);
   }
   Result = (Hold == HoldReason_None);
  }
 }
 return Result;
}

//~ Step
internal void
WorldStep(world *World, f32 Dt)
{
 engine_config *Config = &World->Config;
 neighbor_obs Obs[MAX_NEIGHBORS];

 //- 0. Recycle pods delivered/expired during the previous tick (they stayed one tick for the UI).
 for EachIndex(P, MAX_PODS)
 {
  pod *Pod = &World->Pods[P];
  if(Pod->Status == PodStatus_Delivered || Pod->Status == PodStatus_Expired)
  {
   MemorySet(Pod, 0, sizeof(*Pod));
  }
 }

 //- 1. Clock and sun
 World->SimTime += Dt;
 World->TickCount += 1;
 {
  f32 Phase = Config->K.SunPhase0 + World->SimTime/Config->DayLengthSeconds;
  World->SunPhase = Phase - floorf(Phase);
 }

 //- 2. Per-island local update
 for(s32 I = 0; I < World->IslandCount; I += 1)
 {
  island_node *Node = &World->Islands[I];
  power_state OldState = Node->PowerState;

  if(Node->StormUntil > 0.0f && World->SimTime >= Node->StormUntil)
  {
   Node->StormUntil = 0.0f;
   WorldPushEvent(World, WorldEvent(SimEvent_StormEnded, Node->Id, -1, -1));
  }

  Node->Sun = EngineSunIntensity(Node->X, World->SunPhase);
  EnergyTick(Node, Config, World->SimTime, Dt);

  if(Node->PowerState != OldState)
  {
   sim_event Event = WorldEvent(SimEvent_IslandState, Node->Id, -1, -1);
   Event.FromState = OldState;
   Event.ToState = Node->PowerState;
   WorldPushEvent(World, Event);
  }

  NeedTick(Node, Config, Dt, World->SimTime);

  // Hidden household demand clock (ground truth for both need models, spec 3.3). World-owned.
  {
   for(s32 R = 0; R < Config->ResourceCount; R += 1)
   {
    if(World->SimTime >= World->DemandNextAt[I][R])
    {
     f32 Last = World->DemandLastAt[I][R];
     f32 Interval = (Last >= 0.0f) ? World->SimTime - Last : -1.0f;
     NeedOnDemandEvent(Node, Config, R, Interval, World->SimTime);
     f32 Gap = World->DemandTauTrue[I][R]*(1.0f + Config->K.DemandJitter*RandomBilateral(&World->Rng));
     World->DemandLastAt[I][R] = World->SimTime;
     World->DemandNextAt[I][R] = World->SimTime + Max(Gap, Dt);
    }
   }
  }

  if(EnergyIsAwake(Node))
  {
   s32 ObsCount = WorldBuildObs(World, I, Obs);
   NodeStep(Node, Config, Obs, ObsCount, World->SimTime);
   for(s32 R = 0; R < Config->ResourceCount; R += 1)
   {
    World->Stats.UnitsProduced += NeedDrainToShed(Node, Config, R);
   }
  }
 }

 //- 3. Pod creation (spec 6.1): a local decision of each awake island.
 for(s32 I = 0; I < World->IslandCount; I += 1)
 {
  island_node *Node = &World->Islands[I];
  if(EnergyIsAwake(Node))
  {
   s32 ObsCount = WorldBuildObs(World, I, Obs);
   b32 WantsPod = false;
   for(s32 R = 0; R < Config->ResourceCount; R += 1)
   {
    if(Node->Shed[R] >= Config->BatchSize)
    {
     WantsPod = true;
     if(!WorldQueueHasRoom(World, I))
     {
      break; // 7.1: full queue, the surplus stays in the shed
     }
     pod_id Id = WorldAllocPod(World);
     if(Id < 0)
     {
      break; // 7.7: pool exhausted, the surplus stays in the shed
     }
     pod *Pod = &World->Pods[Id];
     s32 ShedBefore = Node->Shed[R];
     if(NodeTryBuildPod(Node, Config, R, Obs, ObsCount, Pod, World->SimTime))
     {
      Pod->Resource = R;
      Assert(ShedBefore - Node->Shed[R] == Pod->ReservedQty + Pod->FluidQty);
      WorldFinishNewPod(World, I, Id);
     }
     else
     {
      MemorySet(Pod, 0, sizeof(*Pod));
     }
    }
   }

   b32 Backpressure = (WantsPod && !WorldQueueHasRoom(World, I));
   if(Backpressure && !World->Backpressured[I])
   {
    sim_event Event = WorldEvent(SimEvent_QueueBackpressure, I, -1, -1);
    Event.Qty = Node->QueueCount;
    WorldPushEvent(World, Event);
   }
   World->Backpressured[I] = Backpressure;
  }
 }

 //- 4. Sync: gossip between awake, storm-free neighbours (spec 5.1 step 4).
 if(Config->SyncMode != SyncMode_Sneakernet && World->SimTime >= World->NextGossipAt)
 {
  while(World->NextGossipAt <= World->SimTime)
  {
   World->NextGossipAt += Max(Config->GossipIntervalSeconds, Dt);
  }
  for(s32 L = 0; L < World->LinkCount; L += 1)
  {
   world_link *Link = &World->Links[L];
   island_node *A = &World->Islands[Link->A];
   island_node *B = &World->Islands[Link->B];
   if(Link->Active && EnergyIsAwake(A) && EnergyIsAwake(B))
   {
    // NOTE(L4): By-value snapshots, so both sides merge what the other knew before this ping.
    knowledge_entry FromA[MAX_ISLANDS];
    knowledge_entry FromB[MAX_ISLANDS];
    MemoryCopy(FromA, A->Knowledge, sizeof(FromA));
    MemoryCopy(FromB, B->Knowledge, sizeof(FromB));
    NodeMergeKnowledge(B, Config, FromA, World->SimTime);
    NodeMergeKnowledge(A, Config, FromB, World->SimTime);
    WorldPushEvent(World, WorldEvent(SimEvent_GossipPing, Link->A, Link->B, -1));
   }
  }
 }

 //- 5. Transport: TTL, flight progress, arrival resolution.
 for EachIndex(P, MAX_PODS)
 {
  pod *Pod = &World->Pods[P];
  if((Pod->Status == PodStatus_Queued || Pod->Status == PodStatus_InTransit) &&
     World->SimTime >= Pod->ExpiresAt)
  {
   WorldExpirePod(World, P);
  }
  else if(Pod->Status == PodStatus_InTransit)
  {
   // TODO(L4): spoilage when Config->EnableSpoilage (D10).
   Pod->Progress += Config->PodSpeed/Max(Pod->Distance, 0.001f)*Dt;
   if(Pod->Progress >= 1.0f)
   {
    Pod->Progress = 1.0f;
    WorldArrive(World, P);
   }
  }
 }

 //- 6. Departures: each awake island asks its node for a next hop, FIFO.
 // NOTE(L4): "urgent first" ordering is L1's call: the node may reorder Self->Queue in NodeStep.
 for(s32 I = 0; I < World->IslandCount; I += 1)
 {
  island_node *Node = &World->Islands[I];
  if(EnergyIsAwake(Node) && Node->QueueCount > 0)
  {
   s32 ObsCount = WorldBuildObs(World, I, Obs);
   s32 Q = 0;
   while(Q < Node->QueueCount)
   {
    pod_id Id = Node->Queue[Q];
    pod *Pod = &World->Pods[Id];
    hop_decision Hop = NodeDecideNextHop(Node, Config, Pod, Obs, ObsCount, World->SimTime);
    hold_reason Hold = (Hop.HoldReason != HoldReason_None) ? Hop.HoldReason : HoldReason_NeighborsAsleep;
    if(Hop.NextHop >= 0)
    {
     Hold = WorldLaunch(World, I, Id, Hop.NextHop);
    }

    if(Hold == HoldReason_None)
    {
     if(Hop.IsReroute)
     {
      Pod->RerouteCount += 1;
      World->Stats.Reroutes += 1;
      sim_event Event = WorldEvent(SimEvent_PodRerouted, I, Hop.NextHop, Id);
      Event.Resource = Pod->Resource;
      Event.Reason = Hop.RerouteReason;
      WorldPushEvent(World, Event);
     }
     // The pod left the queue, so the next one moved into slot Q.
    }
    else
    {
     WorldMarkHeld(World, Id, Hold);
     Q += 1;
    }
   }
  }
 }

 //- 7. Conservation check
 Assert(WorldCheckConservation(World));
}

//~ Mock
internal pod_id
MockAddPod(world *World, pod_status Status, island_id From, island_id To, island_id Dest,
           resource_id R, s32 Reserved, s32 Fluid, f32 Progress)
{
 pod_id Result = -1;
 for EachIndex(P, MAX_PODS)
 {
  if(World->Pods[P].Status == PodStatus_Free)
  {
   Result = P;
   break;
  }
 }

 if(Result >= 0)
 {
  pod *Pod = &World->Pods[Result];
  island_node *FromNode = &World->Islands[From];
  Pod->Status = Status;
  Pod->Kind = PodKind_Targeted;
  Pod->Resource = R;
  Pod->OriginId = From;
  Pod->DestId = Dest;
  Pod->ReservedQty = Pod->InitialReservedQty = Reserved;
  Pod->FluidQty = Pod->InitialFluidQty = Fluid;
  Pod->CreatedAt = World->SimTime - 10.0f;
  Pod->ExpiresAt = Pod->CreatedAt + World->Config.PodMaxTtlSeconds;
  Pod->CustodianId = From;
  Pod->FromId = From;
  Pod->ToId = To;
  Pod->Visited[Pod->VisitedCount++] = From;
  Pod->PlannedPath[Pod->PlannedPathCount++] = From;
  if(To >= 0)
  {
   Pod->PlannedPath[Pod->PlannedPathCount++] = To;
   for(s32 N = 0; N < FromNode->NeighborCount; N += 1)
   {
    if(FromNode->Neighbors[N].TargetId == To)
    {
     Pod->Distance = FromNode->Neighbors[N].Distance;
     Pod->WrapsGlobe = FromNode->Neighbors[N].WrapsGlobe;
    }
   }
  }
  if(Dest != To)
  {
   Pod->PlannedPath[Pod->PlannedPathCount++] = Dest;
  }
  Pod->Progress = Progress;
  Pod->LaunchedAt = World->SimTime - 5.0f;
  MemoryCopy(Pod->Carried, FromNode->Knowledge, sizeof(Pod->Carried));

  if(Status == PodStatus_Queued && FromNode->QueueCount < MAX_QUEUE)
  {
   FromNode->Queue[FromNode->QueueCount++] = Result;
  }
 }
 return Result;
}

// NOTE(L4): Fills World with a fake but contract-correct state (conservation holds, no
// stepping needed) so L2/L3 can draw and click before the engine works.
internal void
MockWorld(world *World)
{
 engine_config Config = DefaultConfig();
 WorldInit(World, &Config);
 random_series *Rng = &World->Rng;

 World->SimTime = 37.0f;
 World->TickCount = (s32)(World->SimTime/ENGINE_TICK_SECONDS);
 World->SunPhase = 0.3f;

 for(s32 I = 0; I < World->IslandCount; I += 1)
 {
  island_node *Node = &World->Islands[I];
  Node->Sun = EngineSunIntensity(Node->X, World->SunPhase);
  Node->Energy = RandomBetween(Rng, 15.0f, 90.0f);
  Node->EDelta = RandomBetween(Rng, -1.0f, 2.0f);
  if(Node->Sun <= Config.K.EpsDawn && (I % 3) == 0)
  {
   Node->Energy = 0.0f;
  }
  Node->PowerState = WorldPowerStateFromSun(&Config, Node->Sun, Node->Energy);

  for EachIndex(R, Config.ResourceCount)
  {
   Node->Need[R].Score = RandomBetween(Rng, 0.0f, 100.0f);
   Node->Need[R].TauHat = RandomBetween(Rng, Config.K.TauMin, Config.K.TauMax);
   Node->Shed[R] = (s32)RandomBetween(Rng, 0.0f, 6.0f);
   Node->Dropped[R] = Node->Shed[R];
  }

  // Knowledge: itself and its direct neighbours, slightly stale.
  knowledge_entry *Own = &Node->Knowledge[I];
  Own->Valid = true;
  Own->PowerState = Node->PowerState;
  Own->EnergyReserve = Node->Energy;
  Own->UpdatedAt = World->SimTime;
  for EachIndex(R, Config.ResourceCount)
  {
   Own->Needs[R] = Node->Need[R].Score;
  }
 }

 // Storm over one island.
 {
  island_node *Stormed = &World->Islands[World->IslandCount/2];
  Stormed->StormUntil = World->SimTime + 20.0f;
  Stormed->PowerState = PowerState_StormOutage;
  Stormed->Knowledge[Stormed->Id].PowerState = PowerState_StormOutage;
 }

 for(s32 I = 0; I < World->IslandCount; I += 1)
 {
  island_node *Node = &World->Islands[I];
  for(s32 N = 0; N < Node->NeighborCount; N += 1)
  {
   island_id Other = Node->Neighbors[N].TargetId;
   Node->Knowledge[Other] = World->Islands[Other].Knowledge[Other];
   Node->Knowledge[Other].UpdatedAt = World->SimTime - RandomBetween(Rng, 1.0f, 30.0f);
   Node->Knowledge[Other].HopCount = 1;
  }
 }

 // Pods: some in flight along links, some waiting in queues.
 for EachIndex(L, Min(World->LinkCount, 6))
 {
  world_link *Link = &World->Links[L];
  island_id Dest = (Link->B + 1) % World->IslandCount;
  resource_id R = L % Config.ResourceCount;
  MockAddPod(World, PodStatus_InTransit, Link->A, Link->B, Dest, R, 8, 2,
             RandomBetween(Rng, 0.1f, 0.9f));
 }
 for EachIndex(I, 3)
 {
  island_id At = (3*I + 1) % World->IslandCount;
  island_id Dest = (At + 4) % World->IslandCount;
  MockAddPod(World, PodStatus_Queued, At, -1, Dest, I % Config.ResourceCount, 5, 5, 0.0f);
 }
 // One pod that had a Fluid claim en route.
 {
  pod_id Id = MockAddPod(World, PodStatus_InTransit, 2, World->Islands[2].Neighbors[0].TargetId,
                         5, 1, 8, 2, 0.5f);
  if(Id >= 0)
  {
   pod *Pod = &World->Pods[Id];
   Pod->FluidQty = 1;
   Pod->FluidClaims[Pod->FluidClaimCount].IslandId = 2;
   Pod->FluidClaims[Pod->FluidClaimCount].Qty = 1;
   Pod->FluidClaims[Pod->FluidClaimCount].Timestamp = World->SimTime - 3.0f;
   Pod->FluidClaimCount += 1;
   World->Stats.UnitsClaimedEnRoute += 1;
  }
 }

 // Stats consistent with the state above.
 {
  world_stats *S = &World->Stats;
  S->DeliveriesCompleted = 3;
  S->UnitsDelivered = 30;
  S->DeliveriesCompletedDuringOutage = 1;
  S->DeliveriesViaNightRelay = 1;
  S->Reroutes = 2;
  S->PodsHeldDtn = 4;
  S->EnergySpentTotal = 120.0f;
  S->EnergyPerUnitDelivered = S->EnergySpentTotal/(f32)S->UnitsDelivered;
  S->FluidFlightsSaved = 1;
  S->TotalDeliverySeconds = 3.0f*45.0f;
  S->UnitsProduced = (WorldUnitsInShed(World) +
                      WorldUnitsInPods(World, PodStatus_Queued) +
                      WorldUnitsInPods(World, PodStatus_InTransit) +
                      S->UnitsDelivered + S->UnitsClaimedEnRoute + S->UnitsExpired);
 }

 // A few events for the narrative bar.
 {
  sim_event Event = WorldEvent(SimEvent_PodLaunched, 0, World->Islands[0].Neighbors[0].TargetId, 0);
  Event.Qty = 8; Event.Qty2 = 2; Event.Resource = 0;
  WorldPushEvent(World, Event);
  Event = WorldEvent(SimEvent_PodHeldDtn, 1, -1, -1);
  Event.Reason = HoldReason_NeighborsAsleep;
  WorldPushEvent(World, Event);
  WorldPushEvent(World, WorldEvent(SimEvent_StormStarted, World->IslandCount/2, -1, -1));
 }

 Assert(WorldCheckConservation(World));
}
