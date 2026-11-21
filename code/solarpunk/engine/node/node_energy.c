//-
// Node energy: power state machine, harvest/drain, launch guard. Spec 3.1, 3.2, 5.2.
// ISOLATED node file: only Self, Config and plain data.
//-

internal b32
EnergyIsAwake(const island_node *Self)
{
 b32 Result = (Self->PowerState == PowerState_SolarHarvesting ||
               Self->PowerState == PowerState_NightReserve);
 return Result;
}

internal b32
EnergyInStorm(const island_node *Self, f32 SimTime)
{
 b32 Result = (Self->StormUntil > 0.0f && SimTime < Self->StormUntil);
 return Result;
}

// NOTE(L4): Spec 5.2. Self->Sun (I) is set by the caller before this runs.
//   SolarHarvesting --I<=eps--> NightReserve --E==0--> Sleeping
//   Sleeping / NightReserve --I>eps, no storm--> SolarHarvesting
//   (any) --storm--> StormOutage --storm ends--> recompute from sun and E
internal void
EnergyTick(island_node *Self, const engine_config *Config, f32 SimTime, f32 Dt)
{
 const physics_constants *K = &Config->K;
 b32 Daylight = (Self->Sun > K->EpsDawn);
 b32 Storm = EnergyInStorm(Self, SimTime);

 //- State transitions (before integrating, so this tick drains by the new state)
 power_state State = Self->PowerState;
 if(Storm)
 {
  State = PowerState_StormOutage;
 }
 else if(Daylight)
 {
  State = PowerState_SolarHarvesting;
 }
 else
 {
  // Night. Coming out of a storm, or at sunset, the reserve decides.
  if(State == PowerState_SolarHarvesting || State == PowerState_StormOutage)
  {
   State = (Self->Energy > 0.0f) ? PowerState_NightReserve : PowerState_Sleeping;
  }
  // NOTE(L4): A Sleeping island stays asleep until sunrise, even if E would allow otherwise.
 }

 //- Integrate dE/dt (spec 3.2)
 f32 Rate = 0.0f;
 switch(State)
 {
  case PowerState_SolarHarvesting:
  {
   // Live solar first covers the idle load; only the leftover is stored.
   Rate = K->KHarvest*Self->Sun - K->PIdle;
  } break;
  case PowerState_NightReserve:
  {
   Rate = -K->PNight - K->KHold*(f32)Self->QueueCount;
  } break;
  case PowerState_StormOutage:
  {
   Rate = -K->PStorm;
  } break;
  case PowerState_Sleeping:
  {
   Rate = 0.0f;
  } break;
 }

 Self->Energy = Clamp(0.0f, Self->Energy + Rate*Dt, K->EMax);
 Self->EDelta = Rate;

 if(State == PowerState_NightReserve && Self->Energy <= 0.0f)
 {
  State = PowerState_Sleeping;
 }
 Self->PowerState = State;
}

//~ Launch guard (spec 3.2)
internal f32
EnergyLaunchCost(const island_node *Self, const engine_config *Config, f32 Distance)
{
 // NOTE(L4): Sunlit launches cost c_day; launches paid from the stored reserve cost c_night.
 b32 Sunlit = (Self->PowerState == PowerState_SolarHarvesting);
 f32 C = (Sunlit ? Config->K.CDay : Config->K.CNight);
 f32 Result = C*(1.0f + Config->K.Alpha*Distance);
 return Result;
}

internal b32
EnergyCanLaunch(const island_node *Self, const engine_config *Config, f32 Distance)
{
 b32 Result = false;
 f32 Cost = EnergyLaunchCost(Self, Config, Distance);
 switch(Self->PowerState)
 {
  case PowerState_SolarHarvesting:
  {
   // No floor: live sun refills the reserve.
   Result = (Self->Energy >= Cost);
  } break;
  case PowerState_NightReserve:
  {
   // Keep a floor so the island does not put itself to sleep.
   Result = (Self->Energy >= Cost + Config->MinLaunchReserve);
  } break;
  case PowerState_Sleeping:
  case PowerState_StormOutage:
  {
   Result = false;
  } break;
 }
 return Result;
}

internal b32
EnergyTryLaunch(island_node *Self, const engine_config *Config, f32 Distance)
{
 b32 Result = EnergyCanLaunch(Self, Config, Distance);
 if(Result)
 {
  Self->Energy -= EnergyLaunchCost(Self, Config, Distance);
  Self->Energy = Max(0.0f, Self->Energy);
 }
 return Result;
}
