//-
// SolarPunk food: lots with expiry rounds, eating oldest first, waste counting.
// Pure data: only base types and product_kind. No app_state, so SolarSim can test it.
//-

//~ Queries
internal b32
FoodIsPerishable(product_kind Kind)
{
 b32 Result = (Kind > ProductKind_None && Kind < ProductKind_Count && FoodShelfLifeDays[Kind] > 0.f);
 return Result;
}

internal u64
FoodShelfLifeRounds(product_kind Kind)
{
 u64 Result = 0;
 if(FoodIsPerishable(Kind))
 {
  Result = (u64)(FoodShelfLifeDays[Kind]*(f32)Food_RoundsPerDay + .5f);
  Result = Max(Result, 1);
 }
 return Result;
}

internal b32
FoodIsValid(u32 Island, product_kind Kind)
{
 b32 Result = (Island < Food_MaxIslands && FoodIsPerishable(Kind));
 return Result;
}

internal u32
FoodStockQty(food_state *Food, u32 Island, product_kind Kind)
{
 u32 Result = 0;
 if(FoodIsValid(Island, Kind))
 {
  food_stock *Stock = &Food->Stock[Island][Kind];
  for(u32 L = 0; L < Stock->LotCount; L += 1)
  {
   Result += Stock->Lots[L].Qty;
  }
 }
 return Result;
}

// Units that go bad within Food_SoonRounds after Round (what an island should share first).
internal u32
FoodExpiringSoon(food_state *Food, u32 Island, product_kind Kind, u64 Round)
{
 u32 Result = 0;
 if(FoodIsValid(Island, Kind))
 {
  food_stock *Stock = &Food->Stock[Island][Kind];
  for(u32 L = 0; L < Stock->LotCount && Stock->Lots[L].ExpiresRound <= Round + Food_SoonRounds; L += 1)
  {
   Result += Stock->Lots[L].Qty;
  }
 }
 return Result;
}

//~ Lots
internal void
FoodRemoveFront(food_stock *Stock, u32 Count)
{
 Assert(Count <= Stock->LotCount);
 for(u32 L = Count; L < Stock->LotCount; L += 1)
 {
  Stock->Lots[L - Count] = Stock->Lots[L];
 }
 Stock->LotCount -= Count;
}

// NOTE(L4): Keeps lots sorted. Merging always keeps the EARLIER expiry, so merged food never
// lasts longer than it really would.
internal void
FoodInsertLot(food_stock *Stock, food_lot Lot)
{
 if(Lot.Qty > 0)
 {
  // Same day-ish as the newest lot: merge (production adds one lot every round).
  if(Stock->LotCount > 0)
  {
   food_lot *Newest = &Stock->Lots[Stock->LotCount - 1];
   if(Lot.ExpiresRound >= Newest->ExpiresRound && Lot.ExpiresRound - Newest->ExpiresRound < Food_MergeRounds)
   {
    Newest->Qty += Lot.Qty;
    return;
   }
  }

  // Exact expiry already present: merge.
  for(u32 L = 0; L < Stock->LotCount; L += 1)
  {
   if(Stock->Lots[L].ExpiresRound == Lot.ExpiresRound)
   {
    Stock->Lots[L].Qty += Lot.Qty;
    return;
   }
  }

  if(Stock->LotCount < Food_MaxLots)
  {
   u32 At = Stock->LotCount;
   while(At > 0 && Stock->Lots[At - 1].ExpiresRound > Lot.ExpiresRound)
   {
    Stock->Lots[At] = Stock->Lots[At - 1];
    At -= 1;
   }
   Stock->Lots[At] = Lot;
   Stock->LotCount += 1;
  }
  else
  {
   // Full: merge into the lot with the closest expiry, keeping the earlier date.
   u32 Best = 0;
   u64 BestGap = (u64)-1;
   for(u32 L = 0; L < Stock->LotCount; L += 1)
   {
    u64 Expiry = Stock->Lots[L].ExpiresRound;
    u64 Gap = (Expiry > Lot.ExpiresRound) ? Expiry - Lot.ExpiresRound : Lot.ExpiresRound - Expiry;
    if(Gap < BestGap)
    {
     BestGap = Gap;
     Best = L;
    }
   }
   Stock->Lots[Best].Qty += Lot.Qty;
   Stock->Lots[Best].ExpiresRound = Min(Stock->Lots[Best].ExpiresRound, Lot.ExpiresRound);
  }
 }
}

// Takes up to Qty units, soonest expiry first. Returns the units taken.
internal u32
FoodTakeFront(food_stock *Stock, u32 Qty, food_lot *Out, u32 OutCap, u32 *OutCount)
{
 u32 Result = 0;
 u32 Used = 0;
 while(Result < Qty && Stock->LotCount > 0 && (!Out || Used < OutCap))
 {
  food_lot *Front = &Stock->Lots[0];
  u32 Take = Min(Front->Qty, Qty - Result);
  if(Out)
  {
   Out[Used].Qty = Take;
   Out[Used].ExpiresRound = Front->ExpiresRound;
   Used += 1;
  }
  Front->Qty -= Take;
  Result += Take;
  if(Front->Qty == 0)
  {
   FoodRemoveFront(Stock, 1);
  }
 }
 if(OutCount)
 {
  *OutCount = Used;
 }
 return Result;
}

//~ Round
internal void
FoodReset(food_state *Food)
{
 MemorySet(Food, 0, sizeof(*Food));
}

// NOTE(L4): One round for one (island, kind), in this order:
//   1. throw away lots that are bad by now (waste)
//   2. produce: ProducePerDay/RoundsPerDay units, as a new lot expiring after the shelf life
//   3. eat: NeedPerDay/RoundsPerDay units, soonest expiry first; what is missing is a shortage
// Whole units only; fractions carry over to the next round.
internal void
FoodIslandRound(food_state *Food, u32 Island, product_kind Kind, f32 ProducePerDay, f32 NeedPerDay, u64 Round)
{
 if(FoodIsValid(Island, Kind))
 {
  food_stock *Stock = &Food->Stock[Island][Kind];
  food_counts *Counts = &Food->Counts[Island][Kind];

  //- 1. Spoil
  u32 Bad = 0;
  while(Bad < Stock->LotCount && Stock->Lots[Bad].ExpiresRound <= Round)
  {
   Counts->Wasted += Stock->Lots[Bad].Qty;
   Bad += 1;
  }
  FoodRemoveFront(Stock, Bad);

  //- 2. Produce
  Stock->ProduceCarry += Max(ProducePerDay, 0.f)/(f32)Food_RoundsPerDay;
  u32 Made = (u32)Stock->ProduceCarry;
  Stock->ProduceCarry -= (f32)Made;
  if(Made > 0)
  {
   food_lot Lot;
   Lot.Qty = Made;
   Lot.ExpiresRound = (u32)(Round + FoodShelfLifeRounds(Kind));
   FoodInsertLot(Stock, Lot);
   Counts->Produced += Made;
  }

  //- 3. Eat
  Stock->EatCarry += Max(NeedPerDay, 0.f)/(f32)Food_RoundsPerDay;
  u32 Want = (u32)Stock->EatCarry;
  Stock->EatCarry -= (f32)Want;
  u32 Ate = FoodTakeFront(Stock, Want, 0, 0, 0);
  Counts->Eaten += Ate;
  Counts->Shortage += Want - Ate;
 }
}

//~ Routing engine API
// NOTE(L4): For whoever builds transport. FoodTake removes up to Qty units from an island,
// soonest-to-expire first, and writes them as lots (with their expiry) into Out. The food keeps
// ageing in transit: hand the same lots to FoodGive on arrival. Returns the units taken.
internal u32
FoodTake(food_state *Food, u32 Island, product_kind Kind, u32 Qty, food_lot *Out, u32 OutCap, u32 *OutCount)
{
 u32 Result = 0;
 if(OutCount)
 {
  *OutCount = 0;
 }
 if(FoodIsValid(Island, Kind) && Out && OutCap > 0)
 {
  Result = FoodTakeFront(&Food->Stock[Island][Kind], Qty, Out, OutCap, OutCount);
  Food->Counts[Island][Kind].Sent += Result;
 }
 return Result;
}

// Hands lots to an island. Lots that are already bad at Round are counted as waste there.
internal void
FoodGive(food_state *Food, u32 Island, product_kind Kind, food_lot *Lots, u32 LotCount, u64 Round)
{
 if(FoodIsValid(Island, Kind))
 {
  food_stock *Stock = &Food->Stock[Island][Kind];
  food_counts *Counts = &Food->Counts[Island][Kind];
  for(u32 L = 0; L < LotCount; L += 1)
  {
   Counts->Received += Lots[L].Qty;
   if(Lots[L].ExpiresRound <= Round)
   {
    Counts->Wasted += Lots[L].Qty;
   }
   else
   {
    FoodInsertLot(Stock, Lots[L]);
   }
  }
 }
}

//~ Totals
internal food_counts
FoodTotals(food_state *Food, u32 IslandCount)
{
 food_counts Result = {0};
 for(u32 I = 0; I < Min(IslandCount, (u32)Food_MaxIslands); I += 1)
 {
  for(u32 K = 0; K < ProductKind_Count; K += 1)
  {
   food_counts *C = &Food->Counts[I][K];
   Result.Produced += C->Produced;
   Result.Eaten += C->Eaten;
   Result.Wasted += C->Wasted;
   Result.Shortage += C->Shortage;
   Result.Sent += C->Sent;
   Result.Received += C->Received;
  }
 }
 return Result;
}

// Produced + Received == Eaten + Wasted + Sent + InStock, for one (island, kind).
internal b32
FoodIsConserved(food_state *Food, u32 Island, product_kind Kind)
{
 b32 Result = true;
 if(FoodIsValid(Island, Kind))
 {
  food_counts *C = &Food->Counts[Island][Kind];
  u64 In = C->Produced + C->Received;
  u64 Out = C->Eaten + C->Wasted + C->Sent + FoodStockQty(Food, Island, Kind);
  Result = (In == Out);
 }
 return Result;
}
