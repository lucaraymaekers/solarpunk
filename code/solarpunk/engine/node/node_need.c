//-
// Node need model. Spec 3.3, 4.4. Decision D1 is OPEN: every function switches on
// Config->NeedModel. ISOLATED node file: only Self, Config and plain data.
//-

// TODO(L4): Option A and Option B bodies come after the energy work. Minimal bodies for now.

internal void
NeedInit(island_node *Self, const engine_config *Config)
{
 for EachIndex(R, MAX_RESOURCES)
 {
  need_state *Need = &Self->Need[R];
  MemoryZero(Need);
  Need->TauHat = 0.5f*(Config->K.TauMin + Config->K.TauMax);
 }
}

internal void
NeedTick(island_node *Self, const engine_config *Config, f32 Dt, f32 SimTime)
{
 switch(Config->NeedModel)
 {
  case NeedModel_AbstractCadence:
  {
   // TODO(L4)
  } break;
  case NeedModel_StockBased:
  {
   // TODO(L4)
  } break;
 }
}

internal f32
NeedScore(const island_node *Self, const engine_config *Config, resource_id R)
{
 f32 Result = 0.0f;
 if(R >= 0 && R < MAX_RESOURCES)
 {
  Result = Self->Need[R].Score;
 }
 return Result;
}

internal void
NeedOnDemandEvent(island_node *Self, const engine_config *Config, resource_id R, f32 Interval, f32 SimTime)
{
 // TODO(L4)
}

internal void
NeedOnDelivery(island_node *Self, const engine_config *Config, resource_id R, s32 Qty, f32 SimTime, b32 IsFinal)
{
 // TODO(L4)
}

internal void
NeedOnFluidClaim(island_node *Self, const engine_config *Config, resource_id R, s32 Qty)
{
 // TODO(L4)
}

internal void
NeedOnTap(island_node *Self, const engine_config *Config, resource_id R, f32 SimTime)
{
 // TODO(L4)
}

internal s32
NeedDrainToShed(island_node *Self, const engine_config *Config, resource_id R)
{
 // TODO(L4)
 s32 Result = 0;
 return Result;
}
