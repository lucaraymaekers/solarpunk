//-
// SolarPunk routing: link-state routing between the islands (see solarpunk_routing.h).
// Island index = position in App's island list, the same index the broadcast and food use.
// Included before solarpunk_broadcast.c, which calls RouteRound once per round.
//-

//~ Helpers
#define RouteBit(Idx) ((u64)1 << (Idx))

internal b32
RouteIsDown(app_state *App, u32 Idx)
{
 b32 Result = (Idx < Route_MaxIslands && App->Route.Down[Idx]);
 return Result;
}

internal void
RouteClearTables(route_state *Route, u32 Idx)
{
 MemorySet(Route->Dist[Idx], 0xFF, sizeof(Route->Dist[Idx]));
 MemorySet(Route->NextHop[Idx], 0xFF, sizeof(Route->NextHop[Idx]));
 MemorySet(Route->Parent[Idx], 0xFF, sizeof(Route->Parent[Idx]));
}

// NOTE: Forget everything a router knows, like a reboot. The sequence number starts again
// at 0 too: the island learns its old number back from its neighbours (see RouteFlood).
internal void
RouteForgetIsland(route_state *Route, u32 Idx)
{
 MemorySet(Route->LastHello[Idx], 0, sizeof(Route->LastHello[Idx]));
 MemorySet(Route->TheyHearMe[Idx], 0, sizeof(Route->TheyHearMe[Idx]));
 MemorySet(Route->Lsdb[Idx], 0, sizeof(Route->Lsdb[Idx]));
 Route->HeardMask[Idx] = 0;
 Route->UpMask[Idx] = 0;
 Route->OwnSeq[Idx] = 0;
 Route->LastOriginated[Idx] = 0;
 Route->ForceOriginate[Idx] = false;
 Route->Dirty[Idx] = false;
 RouteClearTables(Route, Idx);
}

internal void
RouteLoseShipment(route_state *Route, route_shipment *Shipment)
{
 Route->LostUnits += Shipment->Qty;
 Shipment->Active = false;
}

// NOTE: Resets the simulation; which islands are down is a UI setting and is kept.
internal void
RouteReset(app_state *App)
{
 route_state *Route = &App->Route;
 b32 Down[Route_MaxIslands];
 MemoryCopyArray(Down, Route->Down);
 MemorySet(Route, 0, sizeof(*Route));
 MemoryCopyArray(Route->Down, Down);
 for(u32 Idx = 0; Idx < Route_MaxIslands; Idx += 1)
 {
  RouteClearTables(Route, Idx);
 }
}

// NOTE: Shuts an island down (it stops sending hellos and forwarding, and forgets its map) or
// starts it again (it comes back empty and has to say hello and relearn the map).
internal void
RouteSetDown(app_state *App, u32 Idx, b32 Down)
{
 route_state *Route = &App->Route;
 if(Idx < Route_MaxIslands)
 {
  Route->Down[Idx] = Down;
  RouteForgetIsland(Route, Idx);
  if(Down)
  {
   // Goods sitting on it are gone with it.
   for(u32 S = 0; S < Route_MaxShipments; S += 1)
   {
    route_shipment *Shipment = &Route->Shipments[S];
    if(Shipment->Active && Shipment->At == Idx)
    {
     RouteLoseShipment(Route, Shipment);
    }
   }
  }
 }
}

//~ Dijkstra
// NOTE: Cheapest path from Me to every island, using only Me's own copy of the map.
// A link is used only when BOTH ends list it (two-way check): when an island dies, its
// neighbours drop the link from their LSAs and the dead island's stale LSA no longer counts.
// NextHop[Me][D] is the first island on the way to D; Parent[Me][D] the one just before D.
internal void
RouteDijkstra(route_state *Route, u32 Me, u32 Count)
{
 u32 *Dist = Route->Dist[Me];
 u8 *NextHop = Route->NextHop[Me];
 u8 *Parent = Route->Parent[Me];
 RouteClearTables(Route, Me);

 b32 Done[Route_MaxIslands] = {0};
 Dist[Me] = 0;
 Parent[Me] = (u8)Me;

 for(;;)
 {
  //- The closest island not done yet (the candidate list is just a scan: 64 islands at most)
  u32 U = Count;
  u32 Best = Route_Infinity;
  for(u32 V = 0; V < Count; V += 1)
  {
   if(!Done[V] && Dist[V] < Best)
   {
    Best = Dist[V];
    U = V;
   }
  }
  if(U == Count) break;
  Done[U] = true;

  //- Relax U's links
  route_lsa *Lsa = &Route->Lsdb[Me][U];
  if(!Lsa->Valid) continue;
  for(u32 V = 0; V < Count; V += 1)
  {
   if(!(Lsa->NeighbourMask & RouteBit(V))) continue;
   route_lsa *Back = &Route->Lsdb[Me][V];
   if(!Back->Valid || !(Back->NeighbourMask & RouteBit(U))) continue;

   u32 Alt = Dist[U] + Lsa->Cost[V];
   if(Alt < Dist[V])
   {
    Dist[V] = Alt;
    Parent[V] = (u8)U;
    NextHop[V] = (U == Me) ? (u8)V : NextHop[U];
   }
  }
 }
}

//~ Round steps
// NOTE: Every live island says hello on every link. The hello carries who the sender heard
// last round, so a neighbour knows the link works both ways when it finds itself in it.
internal void
RouteHellos(route_state *Route, u8 Cost[][Route_MaxIslands], u32 Count, u64 Round)
{
 u64 Hello[Route_MaxIslands];
 MemoryCopyArray(Hello, Route->HeardMask);

 for(u32 Me = 0; Me < Count; Me += 1)
 {
  if(Route->Down[Me]) continue;
  for(u32 Them = 0; Them < Count; Them += 1)
  {
   if(Cost[Me][Them] && !Route->Down[Them])
   {
    Route->LastHello[Me][Them] = Round;
    Route->TheyHearMe[Me][Them] = ((Hello[Them] & RouteBit(Me)) != 0);
   }
  }
 }

 for(u32 Me = 0; Me < Count; Me += 1)
 {
  if(Route->Down[Me]) continue;
  u64 Heard = 0;
  u64 Up = 0;
  for(u32 Them = 0; Them < Count; Them += 1)
  {
   u64 Last = Route->LastHello[Me][Them];
   if(Cost[Me][Them] && Last && Round - Last < Route_DeadRounds)
   {
    Heard |= RouteBit(Them);
    if(Route->TheyHearMe[Me][Them]) Up |= RouteBit(Them);
   }
  }
  Route->HeardMask[Me] = Heard;
  if(Up != Route->UpMask[Me])
  {
   // A neighbour came up or went silent: tell everyone with a new LSA.
   Route->UpMask[Me] = Up;
   Route->ForceOriginate[Me] = true;
  }
 }
}

// NOTE: An island makes a new LSA when its up neighbours changed, and every
// Route_RefreshRounds rounds anyway, so its LSA never expires while it is alive.
internal void
RouteOriginate(route_state *Route, u8 Cost[][Route_MaxIslands], u32 Count, u64 Round)
{
 for(u32 Me = 0; Me < Count; Me += 1)
 {
  if(Route->Down[Me]) continue;
  route_lsa *Own = &Route->Lsdb[Me][Me];
  b32 Refresh = (!Own->Valid || Round - Route->LastOriginated[Me] >= Route_RefreshRounds);
  if(Route->ForceOriginate[Me] || Refresh)
  {
   Route->OwnSeq[Me] += 1;
   MemorySet(Own, 0, sizeof(*Own));
   Own->Valid = true;
   Own->Seq = Route->OwnSeq[Me];
   Own->Born = Round;
   Own->NeighbourMask = Route->UpMask[Me];
   for(u32 Them = 0; Them < Count; Them += 1)
   {
    if(Own->NeighbourMask & RouteBit(Them)) Own->Cost[Them] = Cost[Me][Them];
   }
   Route->LastOriginated[Me] = Round;
   Route->ForceOriginate[Me] = false;
   Route->Dirty[Me] = true;
  }
 }
}

// NOTE: Every live island sends what it knew at the start of the round to its up neighbours;
// the receiver keeps only LSAs newer than its own copy. Sending the whole database every round
// also does the database sync of a neighbour that just came up.
internal void
RouteFlood(route_state *Route, u32 Count)
{
 MemoryCopyArray(Route->Snapshot, Route->Lsdb);

 for(u32 From = 0; From < Count; From += 1)
 {
  if(Route->Down[From]) continue;
  for(u32 To = 0; To < Count; To += 1)
  {
   // Sent to a neighbour that died but is not detected yet: lost on the way.
   if(!(Route->UpMask[From] & RouteBit(To)) || Route->Down[To]) continue;

   for(u32 Origin = 0; Origin < Count; Origin += 1)
   {
    route_lsa *Offer = &Route->Snapshot[From][Origin];
    if(!Offer->Valid) continue;

    if(Origin == To)
    {
     // Our own LSA came back with a HIGHER number than ours: we rebooted and the network
     // still has our old LSA. Jump past it, or our new LSAs would look old. (Equal is just
     // our current LSA coming back, which is normal.)
     if(Offer->Seq > Route->OwnSeq[To])
     {
      Route->OwnSeq[To] = Offer->Seq;
      Route->ForceOriginate[To] = true;
     }
     continue;
    }

    route_lsa *Have = &Route->Lsdb[To][Origin];
    if(!Have->Valid || Offer->Seq > Have->Seq)
    {
     *Have = *Offer;
     Route->Dirty[To] = true;
    }
   }
  }
 }
}

// NOTE: LSAs that were not refreshed for Route_MaxAgeRounds come from islands that are gone.
internal void
RouteAge(route_state *Route, u32 Count, u64 Round)
{
 for(u32 Me = 0; Me < Count; Me += 1)
 {
  if(Route->Down[Me]) continue;
  for(u32 Origin = 0; Origin < Count; Origin += 1)
  {
   route_lsa *Lsa = &Route->Lsdb[Me][Origin];
   if(Origin != Me && Lsa->Valid && Round - Lsa->Born >= Route_MaxAgeRounds)
   {
    Lsa->Valid = false;
    Route->Dirty[Me] = true;
   }
  }
 }
}

//~ Goods
internal u32
RouteInFlight(route_state *Route, u32 Dest, u32 Kind)
{
 u32 Result = 0;
 for(u32 S = 0; S < Route_MaxShipments; S += 1)
 {
  route_shipment *Shipment = &Route->Shipments[S];
  if(Shipment->Active && Shipment->Dest == Dest && Shipment->Kind == Kind) Result += Shipment->Qty;
 }
 return Result;
}

// NOTE: Every shipment moves one link, to the next hop in the routing table of the island it is
// on (hop-by-hop forwarding). No route yet: it waits there. Next hop died but the island has
// not noticed yet: the goods are lost.
internal void
RouteMoveShipments(app_state *App, u8 Cost[][Route_MaxIslands], u64 Round)
{
 route_state *Route = &App->Route;
 for(u32 S = 0; S < Route_MaxShipments; S += 1)
 {
  route_shipment *Shipment = &Route->Shipments[S];
  if(!Shipment->Active) continue;

  u32 At = Shipment->At;
  if(Route->Down[At])
  {
   RouteLoseShipment(Route, Shipment);
   continue;
  }

  u8 Hop = Route->NextHop[At][Shipment->Dest];
  if(Hop == Route_None) continue;
  if(Route->Down[Hop] || !Cost[At][Hop])
  {
   RouteLoseShipment(Route, Shipment);
   continue;
  }

  Shipment->MovedFrom = At;
  Shipment->MovedTo = Hop;
  Shipment->MovedRound = Round;
  Shipment->At = Hop;
  Shipment->Hops += 1;

  if(Shipment->At == Shipment->Dest)
  {
   FoodGive(&App->Food, Shipment->Dest, (product_kind)Shipment->Kind, Shipment->Lots, Shipment->LotCount, Round);
   Route->DeliveredUnits += Shipment->Qty;
   Route->DeliveredTo[Shipment->Dest] += Shipment->Qty;
   Shipment->Active = false;
  }
  else if(Shipment->Hops >= Route_MaxHops)
  {
   RouteLoseShipment(Route, Shipment);
  }
 }
}

// NOTE: An island that holds less than Route_WantRounds rounds of a food it needs (counting
// what is already on the way) orders it from the closest island it can reach that, as far as
// it has heard (the broadcast), has that food in store. The supplier keeps Route_KeepRounds
// rounds of its own need. Orders arrive instantly; only the goods are routed.
internal void
RouteOrders(app_state *App, f32 NeedPerDay[][ProductKind_Count], u32 Count)
{
 route_state *Route = &App->Route;
 broadcast_state *Broadcast = &App->Broadcast;
 food_state *Food = &App->Food;

 for(u32 Me = 0; Me < Count; Me += 1)
 {
  if(Route->Down[Me]) continue;
  for(u32 Kind = 1; Kind < ProductKind_Count; Kind += 1)
  {
   f32 NeedPerRound = NeedPerDay[Me][Kind]/(f32)Food_RoundsPerDay;
   if(NeedPerRound <= 0.f) continue;

   u32 Want = (u32)CeilF32(Route_WantRounds*NeedPerRound);
   u32 Have = FoodStockQty(Food, Me, (product_kind)Kind) + RouteInFlight(Route, Me, Kind);
   if(Have >= Want) continue;

   //- Closest reachable supplier this island has heard of
   u32 Supplier = Count;
   u32 BestDist = Route_Infinity;
   for(u32 Other = 0; Other < Count; Other += 1)
   {
    island_ad *Ad = &Broadcast->Known[Me][Other];
    if(Other != Me && Route->Dist[Me][Other] < BestDist && Ad->Valid && Ad->FoodStock[Kind] > 0)
    {
     BestDist = Route->Dist[Me][Other];
     Supplier = Other;
    }
   }
   if(Supplier == Count || Route->Down[Supplier]) continue;

   //- What the supplier can spare
   u32 Keep = (u32)CeilF32(Route_KeepRounds*NeedPerDay[Supplier][Kind]/(f32)Food_RoundsPerDay);
   u32 Stock = FoodStockQty(Food, Supplier, (product_kind)Kind);
   if(Stock <= Keep) continue;
   u32 Give = Min(Want - Have, Stock - Keep);

   //- A free slot (not one that is still drawing its last hop this round)
   route_shipment *Shipment = 0;
   for(u32 S = 0; S < Route_MaxShipments && !Shipment; S += 1)
   {
    route_shipment *Slot = &Route->Shipments[S];
    if(!Slot->Active && Slot->MovedRound != Broadcast->Round) Shipment = Slot;
   }
   if(!Shipment) return;

   MemorySet(Shipment, 0, sizeof(*Shipment));
   Shipment->Qty = FoodTake(Food, Supplier, (product_kind)Kind, Give, Shipment->Lots, Route_MaxShipmentLots, &Shipment->LotCount);
   if(Shipment->Qty > 0)
   {
    Shipment->Active = true;
    Shipment->Kind = Kind;
    Shipment->From = Supplier;
    Shipment->Dest = Me;
    Shipment->At = Supplier;
   }
  }
 }
}

//~ Round
// NOTE: Called once per round from BroadcastRound, after the food round.
internal void
RouteRound(app_state *App, u64 Round)
{
 route_state *Route = &App->Route;
 if(Round == 1)
 {
  RouteReset(App);
 }

 //- The world as plain arrays: cable costs and needs per island
 u32 Count = 0;
 island *Islands[Route_MaxIslands];
 for(island *Island = App->FirstIsland; Island && Count < Route_MaxIslands; Island = Island->Next)
 {
  Islands[Count++] = Island;
 }
 Route->IslandCount = Count;

 u8 Cost[Route_MaxIslands][Route_MaxIslands];
 f32 NeedPerDay[Route_MaxIslands][ProductKind_Count];
 MemorySet(Cost, 0, sizeof(Cost));
 MemorySet(NeedPerDay, 0, sizeof(NeedPerDay));
 for(u32 Me = 0; Me < Count; Me += 1)
 {
  for(link *Link = Islands[Me]->FirstLink; Link; Link = Link->Next)
  {
   for(u32 Them = 0; Them < Count; Them += 1)
   {
    if(Islands[Them] == Link->Dest) Cost[Me][Them] = (u8)Clamp(1, Link->Cost, 255);
   }
  }
  for(product *Req = Islands[Me]->FirstRequirement; Req; Req = Req->Next)
  {
   NeedPerDay[Me][Req->Kind] += (f32)Req->Amount;
  }
 }

 //- Control plane: hellos, LSAs, flooding, ageing, Dijkstra
 RouteHellos(Route, Cost, Count, Round);
 RouteOriginate(Route, Cost, Count, Round);
 RouteFlood(Route, Count);
 RouteAge(Route, Count, Round);
 for(u32 Me = 0; Me < Count; Me += 1)
 {
  if(!Route->Down[Me] && Route->Dirty[Me])
  {
   RouteDijkstra(Route, Me, Count);
   Route->Dirty[Me] = false;
  }
 }

 //- Data plane: move the goods, then place new orders
 RouteMoveShipments(App, Cost, Round);
 RouteOrders(App, NeedPerDay, Count);
}

//~ Queries (for the panel and the 3D view)
internal route_neighbour_state
RouteNeighbourState(app_state *App, u32 Me, u32 Them, u64 *RoundsSilent)
{
 route_state *Route = &App->Route;
 route_neighbour_state Result = RouteNeighbour_Never;
 *RoundsSilent = 0;
 if(Me < Route_MaxIslands && Them < Route_MaxIslands && Route->LastHello[Me][Them])
 {
  u64 Round = App->Broadcast.Round;
  *RoundsSilent = (Round > Route->LastHello[Me][Them]) ? Round - Route->LastHello[Me][Them] : 0;
  if(*RoundsSilent >= Route_DeadRounds)           Result = RouteNeighbour_Dead;
  else if(Route->UpMask[Me] & RouteBit(Them))     Result = RouteNeighbour_Up;
  else                                            Result = RouteNeighbour_OneWay;
 }
 return Result;
}

// Islands Me has an LSA for (its own included): the size of its map.
internal u32
RouteMapSize(app_state *App, u32 Me)
{
 route_state *Route = &App->Route;
 u32 Result = 0;
 if(Me < Route_MaxIslands)
 {
  for(u32 Origin = 0; Origin < Route->IslandCount; Origin += 1) Result += (Route->Lsdb[Me][Origin].Valid != 0);
 }
 return Result;
}

// Other islands Me has a route to.
internal u32
RouteReachableCount(app_state *App, u32 Me)
{
 route_state *Route = &App->Route;
 u32 Result = 0;
 if(Me < Route_MaxIslands)
 {
  for(u32 D = 0; D < Route->IslandCount; D += 1) Result += (D != Me && Route->Dist[Me][D] != Route_Infinity);
 }
 return Result;
}
