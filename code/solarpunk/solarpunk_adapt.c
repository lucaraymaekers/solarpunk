//-
// SolarPunk supply correction: producers react to shortages.
//   - Every WorldGen_StormEvery rounds a storm hits one cluster: its harvest drops for a few
//     rounds (WorldGenStormFactor). The first round of a storm is a surprise, so the world
//     dips below what it needs.
//   - Every round, for every food, the makers plan to cover what the world needs plus a
//     little surplus (Adapt_Target). They know the seasons (harvest waves) and storms that
//     are already raging, but growing more takes time: output can only rise by Adapt_MaxRise
//     per round, while cutting back is immediate.
//   - So a storm shows as a dip, then a climb back, then a small surplus again.
// The boost is per food (all its makers grow more). The state lives in broadcast_state, so
// a restart clears it and replaying rounds (the timeline) rebuilds the exact same history.
//-

//~ Constants
#define Adapt_Target   1.05f   // aim for 5% more than the world needs
#define Adapt_MaxRise  .25f    // growing more takes time: at most this much more per round
#define Adapt_MinBoost .5f
#define Adapt_MaxBoost 2.2f

//~ Queries
internal f32
AdaptBoost(app_state *App, product_kind Kind)
{
 f32 Result = 1.f;
 if((u32)Kind < ProductKind_Count)
 {
  Result = 1.f + App->Broadcast.Adapt.BoostDelta[Kind];
 }
 return Result;
}

// NOTE: World balance of a food in a round, as a fraction: made/needed - 1 (0 = exactly
// enough, -.2 = 20% short). Kind = ProductKind_None gives all foods together.
// Valid for the last Adapt_HistoryRounds rounds.
internal f32
AdaptBalance(app_state *App, u64 Round, product_kind Kind)
{
 f32 Result = App->Broadcast.Adapt.Balance[Round % Adapt_HistoryRounds][(u32)Kind % ProductKind_Count];
 return Result;
}

//~ Round
// NOTE: Called once per round, right after Broadcast->Round += 1 and before the food is
// produced, so this round's harvest already uses the new boost.
internal void
SupplyAdaptRound(app_state *App, u64 Round)
{
 supply_adapt *Adapt = &App->Broadcast.Adapt;
 u32 IslandCount = App->Broadcast.IslandCount;
 
 f32 Need[ProductKind_Count] = {0};
 f32 Expected[ProductKind_Count] = {0};   // what the makers count on: seasons + storms they know about
 f32 Actual[ProductKind_Count] = {0};     // what the harvest really is, before the boost
 
 u32 Idx = 0;
 for(island *Island = App->FirstIsland; Island && Idx < IslandCount; Island = Island->Next)
 {
  f32 Harvest = WorldGenHarvestFactor(App, Idx, IslandCount, Round);
  f32 Known = Harvest;
  if(App->HasMapLayout && WorldGenStormIsNew(WorldGenClusterOf(Idx, IslandCount), Round))
  {
   // A storm in its first round has not been noticed yet.
   Known = Harvest/WorldGenStormFactor(WorldGenClusterOf(Idx, IslandCount), Round);
  }
  for(product *Product = Island->FirstProduct; Product; Product = Product->Next)
  {
   Expected[Product->Kind] += Known*(f32)Product->Amount;
   Actual[Product->Kind] += Harvest*(f32)Product->Amount;
  }
  for(product *Req = Island->FirstRequirement; Req; Req = Req->Next)
  {
   Need[Req->Kind] += (f32)Req->Amount;
  }
  Idx += 1;
 }
 
 f32 AllMade = 0.f, AllNeed = 0.f;
 for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
 {
  f32 Old = 1.f + Adapt->BoostDelta[Kind];
  f32 New = Old;
  if(Expected[Kind] > 0.f && Need[Kind] > 0.f)
  {
   f32 Desired = Adapt_Target*Need[Kind]/Expected[Kind];
   New = Min(Desired, Old + Adapt_MaxRise);
   New = Clamp(Adapt_MinBoost, New, Adapt_MaxBoost);
  }
  Adapt->BoostDelta[Kind] = New - 1.f;
  
  f32 Made = New*Actual[Kind];
  Adapt->Balance[Round % Adapt_HistoryRounds][Kind] = (Need[Kind] > 0.f) ? Made/Need[Kind] - 1.f : 0.f;
  AllMade += Made;
  AllNeed += Need[Kind];
 }
 Adapt->Balance[Round % Adapt_HistoryRounds][ProductKind_None] = (AllNeed > 0.f) ? AllMade/AllNeed - 1.f : 0.f;
}
