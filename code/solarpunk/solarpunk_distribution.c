//-
// SolarPunk distribution: islands that PRODUCE a food send what they can spare to the islands
// that NEED it. Built on the existing pieces:
//   - what an island knows about the others comes from the broadcast (Known[Me][Other]: what
//     they need and hold),
//   - where it can deliver comes from its own routing table (solarpunk_routing.c),
//   - the food itself moves with FoodTakeFresh / FoodGive (solarpunk_food.c) as routed
//     shipments: the freshest food travels, the producer eats its oldest itself.
// Included right after solarpunk_routing.c, which calls RouteDistribute once per round.
//-

//~ Helpers
// NOTE: Links on the path From -> To, read off From's own shortest-path tree.
internal u32
RouteHopCount(route_state *Route, u32 From, u32 To)
{
 u32 Result = 0;
 u32 At = To;
 while(At != From && Result < Route_MaxIslands)
 {
  u8 Parent = Route->Parent[From][At];
  if(Parent == Route_None) return Route_Infinity;
  At = Parent;
  Result += 1;
 }
 return Result;
}

// NOTE: *Cursor remembers where the last search stopped, so one round's sends scan the pool once.
internal route_shipment *
RouteFreeShipment(route_state *Route, u64 Round, u32 *Cursor)
{
 route_shipment *Result = 0;
 // Not a slot that is still drawing its last hop this round (the 3D trains).
 for(; *Cursor < Route_MaxShipments && !Result; *Cursor += 1)
 {
  route_shipment *Slot = &Route->Shipments[*Cursor];
  if(!Slot->Active && Slot->MovedRound != Round) Result = Slot;
 }
 return Result;
}

typedef struct route_candidate route_candidate;
struct route_candidate
{
 u32 Island;
 u32 Missing;      // units to bring it up to its target
 f32 RoundsLeft;   // how long what it has (and what is on the way) lasts: lower = more urgent
 u32 Dist;
};

//~ Distribution
// NOTE: Once per round, for every live island S and every food K it produces:
//   Spare  = S's stock - Route_KeepRounds rounds of S's own need
//   For every island R that S can reach and that, as far as S has heard, needs K:
//     Target = (Route_WantRounds + rounds on the way) rounds of R's need
//     Have   = R's stock in its last announcement - eaten since + delivered since + on the way
//   Islands are served most urgent first (fewest rounds of food left), then closest. Food is
//   only sent if it still keeps Route_MinFreshRounds when it arrives, and never more than the
//   receiver can eat before it goes bad. Gaps smaller than
//   Route_MinShipment wait until they are worth a trip.
// Producers go in island order and see each other's shipments as "on the way", so two
// producers do not both fill the same gap.
internal void
RouteDistribute(app_state *App, f32 MakePerDay[][ProductKind_Count], f32 NeedPerDay[][ProductKind_Count], u32 Count)
{
 route_state *Route = &App->Route;
 broadcast_state *Broadcast = &App->Broadcast;
 food_state *Food = &App->Food;
 u64 Round = Broadcast->Round;
 
 // What is already on the way to each island, per food (kept up to date as shipments start).
 static u32 InFlight[Route_MaxIslands][ProductKind_Count];
 MemorySet(InFlight, 0, sizeof(InFlight));
 for(u32 Slot = 0; Slot < Route_MaxShipments; Slot += 1)
 {
  route_shipment *Shipment = &Route->Shipments[Slot];
  if(Shipment->Active && Shipment->Dest < Route_MaxIslands && Shipment->Kind < ProductKind_Count)
  {
   InFlight[Shipment->Dest][Shipment->Kind] += Shipment->Qty;
  }
 }
 u32 FreeCursor = 0;
 
 for(u32 S = 0; S < Count; S += 1)
 {
  if(Route->Down[S]) continue;
  for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
  {
   if(MakePerDay[S][Kind] <= 0.f) continue;
   
   //- What S can spare
   u32 Keep = (u32)CeilF32(Route_KeepRounds*NeedPerDay[S][Kind]/(f32)Food_RoundsPerDay);
   u32 Stock = FoodStockQty(Food, S, (product_kind)Kind);
   if(Stock <= Keep) continue;
   u32 Spare = Stock - Keep;
   // The freshest food S has: what it ships (FoodTakeFresh).
   food_stock *FoodStock = &Food->Stock[S][Kind];
   u64 FreshExpiry = (FoodStock->LotCount > 0) ? FoodStock->Lots[FoodStock->LotCount - 1].ExpiresRound : 0;
   
   //- Who needs it, as far as S knows
   route_candidate Candidates[Route_MaxIslands];
   u32 CandidateCount = 0;
   for(u32 R = 0; R < Count; R += 1)
   {
    island_ad *Ad = &Broadcast->Known[S][R];
    if(R == S || !Ad->Valid || Ad->Needs[Kind] == 0) continue;
    if(Route->Dist[S][R] == Route_Infinity) continue;
    
    u32 Hops = RouteHopCount(Route, S, R);
    if(Hops == Route_Infinity) continue;
    // Only send food that still keeps a while when it gets there, and no more than R can
    // eat before it goes bad.
    u64 Arrive = Round + Hops;
    if(FreshExpiry <= Arrive + Route_MinFreshRounds) continue;
    f32 KeepsFor = (f32)(FreshExpiry - Arrive);
    
    f32 NeedPerRound = (f32)Ad->Needs[Kind]/(f32)Food_RoundsPerDay;
    u32 Target = (u32)CeilF32((Min(Route_WantRounds, KeepsFor) + (f32)Hops)*NeedPerRound);
    // What R has now, as far as S can tell: its announced stock, minus what it has eaten
    // since that announcement, plus what was delivered since and what is on the way.
    f32 EatenSince = (f32)(Round - Min(Ad->Round, Round))*NeedPerRound;
    f32 StockNow = Max(0.f, (f32)Ad->FoodStock[Kind] - EatenSince);
    u32 Have = (u32)StockNow + RouteDeliveredSince(Route, R, Kind, Ad->Round) + InFlight[R][Kind];
    if(Have >= Target) continue;
    
    route_candidate Candidate;
    Candidate.Island = R;
    Candidate.Missing = Target - Have;
    Candidate.RoundsLeft = (f32)Have/NeedPerRound;
    Candidate.Dist = Route->Dist[S][R];
    
    // Insertion sort: most urgent first, then closest, then lowest index.
    u32 At = CandidateCount++;
    while(At > 0)
    {
     route_candidate *Prev = &Candidates[At - 1];
     b32 Before = (Candidate.RoundsLeft < Prev->RoundsLeft ||
                   (Candidate.RoundsLeft == Prev->RoundsLeft && Candidate.Dist < Prev->Dist));
     if(!Before) break;
     Candidates[At] = *Prev;
     At -= 1;
    }
    Candidates[At] = Candidate;
   }
   
   //- Send, most urgent first, until the spare food or the shipments run out
   u32 Sent = 0;
   for(u32 Idx = 0; Idx < CandidateCount && Spare > 0 && Sent < Route_MaxSendsPerKind; Idx += 1)
   {
    u32 Give = Min(Candidates[Idx].Missing, Spare);
    if(Give < Route_MinShipment) continue;
    
    route_shipment *Shipment = RouteFreeShipment(Route, Round, &FreeCursor);
    if(!Shipment) return;
    MemorySet(Shipment, 0, sizeof(*Shipment));
    Shipment->Qty = FoodTakeFresh(Food, S, (product_kind)Kind, Give, Shipment->Lots, Route_MaxShipmentLots, &Shipment->LotCount);
    if(Shipment->Qty > 0)
    {
     Shipment->Active = true;
     Shipment->Kind = Kind;
     Shipment->From = S;
     Shipment->Dest = Candidates[Idx].Island;
     Shipment->At = S;
     Spare -= Min(Spare, Shipment->Qty);
     InFlight[Shipment->Dest][Kind] += Shipment->Qty;
     Sent += 1;
    }
   }
  }
 }
}
