//-
// SolarPunk food glue: connects the food core to the app's islands, rounds and announcements.
// Island index = position in App's island list, the same index the broadcast uses.
// Included before solarpunk_broadcast.c, which calls FoodRoundFromApp and FoodFillAd.
//-

internal u32 BroadcastIslandIndex(app_state *App, island *Target);

// NOTE(L4): Called once per round, right after Broadcast->Round += 1. Product and requirement
// Amounts are per day; FoodIslandRound turns them into per-round units.
internal void
FoodRoundFromApp(app_state *App, u64 Round)
{
 u32 Idx = 0;
 for(island *Island = App->FirstIsland; Island && Idx < Food_MaxIslands; Island = Island->Next)
 {
  f32 Produce[ProductKind_Count] = {0};
  f32 Need[ProductKind_Count] = {0};
  for(product *Product = Island->FirstProduct; Product; Product = Product->Next)
  {
   Produce[Product->Kind] += (f32)Product->Amount;
  }
  for(product *Req = Island->FirstRequirement; Req; Req = Req->Next)
  {
   Need[Req->Kind] += (f32)Req->Amount;
  }
  for(u32 Kind = 0; Kind < ProductKind_Count; Kind += 1)
  {
   FoodIslandRound(&App->Food, Idx, (product_kind)Kind, Produce[Kind], Need[Kind], Round);
  }
  Idx += 1;
 }
}

// Adds the island's food picture to its own announcement, so neighbours (and the routing
// engine) can see what it holds and what goes bad soon.
internal void
FoodFillAd(food_state *Food, u32 Island, island_ad *Ad, u64 Round)
{
 for(u32 Kind = 0; Kind < ProductKind_Count; Kind += 1)
 {
  Ad->FoodStock[Kind] = FoodStockQty(Food, Island, (product_kind)Kind);
  Ad->FoodExpiringSoon[Kind] = FoodExpiringSoon(Food, Island, (product_kind)Kind, Round);
 }
}

// One line for the island box in the graph view, e.g. "food 120 (14 soon) wasted 30".
internal str8
FoodStatusLine(app_state *App, island *Island)
{
 u32 Idx = BroadcastIslandIndex(App, Island);
 u64 Round = App->Broadcast.Round;
 u32 Stock = 0, Soon = 0;
 u64 Wasted = 0;
 if(Idx < Food_MaxIslands)
 {
  for(u32 Kind = 0; Kind < ProductKind_Count; Kind += 1)
  {
   Stock += FoodStockQty(&App->Food, Idx, (product_kind)Kind);
   Soon += FoodExpiringSoon(&App->Food, Idx, (product_kind)Kind, Round);
   Wasted += App->Food.Counts[Idx][Kind].Wasted;
  }
 }
 str8 Result = Str8Fmt("food %u (%u soon) wasted %llu", Stock, Soon, Wasted);
 return Result;
}
