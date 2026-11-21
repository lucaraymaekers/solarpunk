//-
// Node need model. Spec 3.3, 4.4. Decision D1 is OPEN: every function switches on
// Config->NeedModel. ISOLATED node file: only Self, Config and plain data.
//
// Both models are driven by the same hidden household demand clock: the caller fires
// NeedOnDemandEvent when a household uses an item. The node never sees the true cadence.
//
// Option A (abstract cadence): no stock. ElapsedSec = time since the last final delivery.
//   N = Min(100, W_pro * Min(1.25, ElapsedSec/TauHat)^gamma + U). TauHat is learned from the
//   intervals between demand events (not between deliveries).
// Option B (stock): Stock per resource. Production adds, demand events consume.
//   N = Min(100, 100 * Clamp01((Target - Cover)/Target)^gamma + U), Cover in sim-days.
//   PerCapitaHat is learned from the observed consumption rate.
//-

internal s32
NeedMakerCount(const island_node *Self, resource_id R)
{
 s32 Result = 0;
 for EachIndex(M, MAX_MAKERS)
 {
  Result += (Self->Makers[M].Used && Self->Makers[M].Craft == R) ? 1 : 0;
 }
 return Result;
}

// Units per sim-day the island expects to use, from its own learned rate (Option B).
internal f32
NeedDailyUse(const island_node *Self, const need_state *Need)
{
 f32 Result = Need->PerCapitaHat*(f32)Max(Self->Population, 1);
 return Result;
}

internal f32
NeedComputeScore(const island_node *Self, const engine_config *Config, resource_id R)
{
 const physics_constants *K = &Config->K;
 const need_state *Need = &Self->Need[R];
 f32 Base = 0.0f;
 switch(Config->NeedModel)
 {
  case NeedModel_AbstractCadence:
  {
   f32 Ratio = Min(1.25f, Need->ElapsedSec/Max(Need->TauHat, 1.0f));
   Base = K->WPro*powf(Ratio, K->Gamma);
  } break;
  case NeedModel_StockBased:
  {
   f32 Daily = NeedDailyUse(Self, Need);
   f32 Cover = (Daily > 0.0f) ? Need->Stock/Daily : K->TargetCoverDays;
   f32 Target = Max(K->TargetCoverDays, 0.001f);
   f32 Gap = Clamp(0.0f, (Target - Cover)/Target, 1.0f);
   Base = 100.0f*powf(Gap, K->Gamma);
  } break;
 }
 f32 Result = Clamp(0.0f, Base + Need->U, 100.0f);
 return Result;
}

internal void
NeedRefresh(island_node *Self, const engine_config *Config, resource_id R)
{
 Self->Need[R].Score = NeedComputeScore(Self, Config, R);
}

internal b32
NeedIsValid(const engine_config *Config, resource_id R)
{
 b32 Result = (R >= 0 && R < Config->ResourceCount);
 return Result;
}

//~ API
internal void
NeedInit(island_node *Self, const engine_config *Config)
{
 const physics_constants *K = &Config->K;
 for EachIndex(R, MAX_RESOURCES)
 {
  need_state *Need = &Self->Need[R];
  MemoryZero(Need);
  Need->TauHat = 0.5f*(K->TauMin + K->TauMax);
  Need->PerCapitaHat = K->PerCapita;
  Need->Stock = K->TargetCoverDays*NeedDailyUse(Self, Need);
 }
 for(s32 R = 0; R < Config->ResourceCount; R += 1)
 {
  NeedRefresh(Self, Config, R);
 }
}

internal void
NeedTick(island_node *Self, const engine_config *Config, f32 Dt, f32 SimTime)
{
 const physics_constants *K = &Config->K;
 for(s32 R = 0; R < Config->ResourceCount; R += 1)
 {
  need_state *Need = &Self->Need[R];
  f32 Produced = (f32)NeedMakerCount(Self, R)*K->ProductionPerMaker*Dt;

  // NOTE(L4): Linear decay: a tap of TapBoost fades over TapBoost/LambdaU sim-seconds.
  Need->U = Max(0.0f, Need->U - K->LambdaU*Dt);

  switch(Config->NeedModel)
  {
   case NeedModel_AbstractCadence:
   {
    Need->ElapsedSec += Dt;
    // Own needs first: OwnShare of production stays home, the rest waits for the shed.
    Need->ProductionAccum += Produced*(1.0f - K->OwnShare);
   } break;
   case NeedModel_StockBased:
   {
    Need->Stock += Produced;
   } break;
  }
  NeedRefresh(Self, Config, R);
 }
}

internal f32
NeedScore(const island_node *Self, const engine_config *Config, resource_id R)
{
 f32 Result = NeedIsValid(Config, R) ? Self->Need[R].Score : 0.0f;
 return Result;
}

internal void
NeedOnDemandEvent(island_node *Self, const engine_config *Config, resource_id R, f32 Interval, f32 SimTime)
{
 if(NeedIsValid(Config, R))
 {
  const physics_constants *K = &Config->K;
  need_state *Need = &Self->Need[R];
  switch(Config->NeedModel)
  {
   case NeedModel_AbstractCadence:
   {
    // A delivery since the last event absorbs this one; otherwise the household went short.
    if(!Need->Covered)
    {
     Need->Shortages += 1;
    }
    Need->Covered = false;
    if(Interval > 0.0f)
    {
     f32 Tau = (1.0f - K->Beta)*Need->TauHat + K->Beta*Interval;
     Need->TauHat = Clamp(K->TauMin, Tau, K->TauMax);
    }
   } break;
   case NeedModel_StockBased:
   {
    if(Need->Stock >= K->ConsumePerEvent)
    {
     Need->Stock -= K->ConsumePerEvent;
    }
    else
    {
     Need->Stock = 0.0f;
     Need->Shortages += 1;
    }
    if(Interval > 0.0f)
    {
     // Observed use: ConsumePerEvent units per Interval seconds, as units/person/sim-day.
     f32 Observed = K->ConsumePerEvent*Config->DayLengthSeconds/(Interval*(f32)Max(Self->Population, 1));
     Need->PerCapitaHat = (1.0f - K->PerCapitaBeta)*Need->PerCapitaHat + K->PerCapitaBeta*Observed;
    }
   } break;
  }
  NeedRefresh(Self, Config, R);
 }
}

internal void
NeedOnDelivery(island_node *Self, const engine_config *Config, resource_id R, s32 Qty, f32 SimTime, b32 IsFinal)
{
 if(NeedIsValid(Config, R) && Qty > 0)
 {
  const physics_constants *K = &Config->K;
  need_state *Need = &Self->Need[R];
  Need->U = Max(0.0f, Need->U - K->DeltaU*(f32)Qty);
  switch(Config->NeedModel)
  {
   case NeedModel_AbstractCadence:
   {
    if(IsFinal)
    {
     Need->ElapsedSec = 0.0f;
     Need->Covered = true;
    }
   } break;
   case NeedModel_StockBased:
   {
    Need->Stock += (f32)Qty;
   } break;
  }
  NeedRefresh(Self, Config, R);
 }
}

internal void
NeedOnFluidClaim(island_node *Self, const engine_config *Config, resource_id R, s32 Qty)
{
 if(NeedIsValid(Config, R) && Qty > 0)
 {
  const physics_constants *K = &Config->K;
  need_state *Need = &Self->Need[R];
  Need->U = Max(0.0f, Need->U - K->DeltaU*(f32)Qty);
  switch(Config->NeedModel)
  {
   case NeedModel_AbstractCadence:
   {
    // Partial cover: ElapsedSec shrinks by the claimed share of a batch. TauHat untouched.
    f32 Share = Clamp(0.0f, (f32)Qty/(f32)Max(Config->BatchSize, 1), 1.0f);
    Need->ElapsedSec *= (1.0f - Share);
   } break;
   case NeedModel_StockBased:
   {
    Need->Stock += (f32)Qty;
   } break;
  }
  NeedRefresh(Self, Config, R);
 }
}

internal void
NeedOnTap(island_node *Self, const engine_config *Config, resource_id R, f32 SimTime)
{
 if(NeedIsValid(Config, R))
 {
  const physics_constants *K = &Config->K;
  need_state *Need = &Self->Need[R];
  Need->U = Min(100.0f, Need->U + K->TapBoost);
  switch(Config->NeedModel)
  {
   case NeedModel_AbstractCadence:
   {
    // A tap means the learned cadence was too slow.
    Need->TauHat = Clamp(K->TauMin, Need->TauHat*K->TapTauNudge, K->TauMax);
   } break;
   case NeedModel_StockBased:
   {
    Need->PerCapitaHat *= K->TapPerCapitaNudge;
   } break;
  }
  NeedRefresh(Self, Config, R);
 }
}

internal s32
NeedDrainToShed(island_node *Self, const engine_config *Config, resource_id R)
{
 s32 Result = 0;
 if(NeedIsValid(Config, R))
 {
  const physics_constants *K = &Config->K;
  need_state *Need = &Self->Need[R];
  switch(Config->NeedModel)
  {
   case NeedModel_AbstractCadence:
   {
    Result = (s32)floorf(Need->ProductionAccum);
    Need->ProductionAccum -= (f32)Result;
   } break;
   case NeedModel_StockBased:
   {
    // Own needs first, naturally: only stock above SurplusCoverDays of cover leaves.
    f32 Keep = K->SurplusCoverDays*NeedDailyUse(Self, Need);
    Result = (s32)floorf(Max(0.0f, Need->Stock - Keep));
    Need->Stock -= (f32)Result;
   } break;
  }
  Self->Shed[R] += Result;
  if(Result > 0)
  {
   NeedRefresh(Self, Config, R);
  }
 }
 return Result;
}
