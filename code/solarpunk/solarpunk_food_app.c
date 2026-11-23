//-
// SolarPunk food glue: connects the food core to the app's islands, rounds and announcements.
// Island index = position in App's island list, the same index the broadcast uses.
// Included before solarpunk_broadcast.c, which calls FoodRoundFromApp and FoodFillAd.
//-

internal u32 BroadcastIslandIndex(app_state *App, island *Target);
internal b32 RouteIsDown(app_state *App, u32 Idx);   // solarpunk_routing.c

// NOTE: Every island starts with this many rounds of each food it needs in store, so the
// world starts without shortages while the first shipments are on their way.
#define Food_StartRounds 12.f

// NOTE(L4): Called once per round, right after Broadcast->Round += 1. Product and requirement
// Amounts are per day; FoodIslandRound turns them into per-round units. Product Amounts are
// averages: this round's harvest wave scales them (WorldGenHarvestFactor).
internal void
FoodRoundFromApp(app_state *App, u64 Round)
{
 u32 Idx = 0;
 u32 IslandCount = App->Broadcast.IslandCount;
 for(island *Island = App->FirstIsland; Island && Idx < Food_MaxIslands; Island = Island->Next)
 {
  f32 Produce[ProductKind_Count] = {0};
  f32 Need[ProductKind_Count] = {0};
  f32 Harvest = WorldGenHarvestFactor(App, Idx, IslandCount, Round);
  // A shut-down island (solarpunk_routing.c) makes nothing; its people still eat.
  if(RouteIsDown(App, Idx)) Harvest = 0.f;
  for(product *Product = Island->FirstProduct; Product; Product = Product->Next)
  {
   Produce[Product->Kind] += Harvest*AdaptBoost(App, Product->Kind)*(f32)Product->Amount;
  }
  for(product *Req = Island->FirstRequirement; Req; Req = Req->Next)
  {
   Need[Req->Kind] += (f32)Req->Amount;
  }
  //- Round 1: starting stock (counted as produced, so the books still balance)
  if(Round == 1)
  {
   for(u32 Kind = 0; Kind < ProductKind_Count; Kind += 1)
   {
    if(Need[Kind] > 0.f && FoodIsPerishable((product_kind)Kind))
    {
     food_lot Lot;
     Lot.Qty = (u32)CeilF32(Food_StartRounds*Need[Kind]/(f32)Food_RoundsPerDay);
     Lot.ExpiresRound = (u32)(Round + FoodShelfLifeRounds((product_kind)Kind));
     FoodInsertLot(&App->Food.Stock[Idx][Kind], Lot);
     App->Food.Counts[Idx][Kind].Produced += Lot.Qty;
    }
   }
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

// NOTE(L4): Plain-words lines for the island box, per food the island has or wants:
//   "Fish: 34, 12 go bad in 3 rounds"   (SoonColor when within Food_SoonRounds)
//   "  thrown away 10, missing 5"       (only the parts that are not zero)
// "thrown away" = went bad before anyone ate it (so far); "missing" = wanted to eat it but had
// none, in the last Food_RecentRounds rounds.
// NOTE: With Kinds (may be 0), Kinds[line] is the food a line is about (0 for the detail
// lines) and the name is left out of the text, so the caller can draw the food's icon instead.
internal void
FoodAppendLines(app_state *App, island *Island, str8 *Lines, v4 *Colors, s32 *LineCount, s32 Cap,
                v4 Color, v4 SoonColor, u32 *Kinds)
{
 u32 Idx = BroadcastIslandIndex(App, Island);
 u64 Round = App->Broadcast.Round;
 if(Idx < Food_MaxIslands)
 {
  for(u32 Kind = 0; Kind < ProductKind_Count && *LineCount + 2 < Cap; Kind += 1)
  {
   if(FoodIsPerishable((product_kind)Kind))
   {
    food_stock *Stock = &App->Food.Stock[Idx][Kind];
    food_counts *Counts = &App->Food.Counts[Idx][Kind];
    u32 Qty = FoodStockQty(&App->Food, Idx, (product_kind)Kind);
    // Missing = only what the island went without in the last Food_RecentRounds rounds.
    u32 Missing = FoodRecentShortage(&App->Food, Idx, (product_kind)Kind);
    if(Qty > 0 || Counts->Wasted > 0 || Missing > 0)
    {
     str8 Name = Kinds ? S8("") : ProductNames[Kind];
     char *Sep = Kinds ? "" : ": ";
     if(Kinds)
     {
      Kinds[*LineCount] = Kind;
      if(*LineCount + 1 < Cap) Kinds[*LineCount + 1] = 0;
     }
     if(Stock->LotCount > 0)
     {
      food_lot *Next = &Stock->Lots[0];
      u64 Left = (Next->ExpiresRound > Round) ? Next->ExpiresRound - Round : 0;
      Lines[*LineCount] = Str8Fmt("%.*s%s%u, %u go bad in %llu round%s", (s32)Name.Size, Name.Data, Sep,
                                  Qty, Next->Qty, Left, (Left == 1) ? "" : "s");
      Colors[*LineCount] = (Next->ExpiresRound <= Round + Food_SoonRounds) ? SoonColor : Color;
     }
     else
     {
      Lines[*LineCount] = Str8Fmt("%.*s%snone", (s32)Name.Size, Name.Data, Sep);
      Colors[*LineCount] = Color;
     }
     *LineCount += 1;
     
     if(Counts->Wasted > 0 && Missing > 0)
     {
      Lines[*LineCount] = Str8Fmt("  thrown away %llu, missing %u", Counts->Wasted, Missing);
     }
     else if(Counts->Wasted > 0)
     {
      Lines[*LineCount] = Str8Fmt("  thrown away %llu", Counts->Wasted);
     }
     else if(Missing > 0)
     {
      Lines[*LineCount] = Str8Fmt("  missing %u", Missing);
     }
     if(Counts->Wasted > 0 || Missing > 0)
     {
      Colors[*LineCount] = SoonColor;
      *LineCount += 1;
     }
    }
   }
  }
 }
}

// One line for the bottom of the screen that explains the food rules.
internal str8
FoodLegendLine(void)
{
 str8 Result = S8("Food goes bad after:");
 b32 First = true;
 for(u32 Kind = 0; Kind < ProductKind_Count; Kind += 1)
 {
  if(FoodIsPerishable((product_kind)Kind))
  {
   str8 Name = ProductNames[Kind];
   Result = Str8Fmt("%S%s %.*s %llu", Result, First ? "" : ",", (s32)Name.Size, Name.Data,
                    FoodShelfLifeRounds((product_kind)Kind));
   First = false;
  }
 }
 Result = Str8Fmt("%S rounds.  Oldest food is eaten first.  Orange = bad within %d rounds.", Result, Food_SoonRounds);
 return Result;
}

// NOTE(L4): The same rules in short lines, for the narrow islands panel. Foods are listed from
// fastest to slowest spoiling, three per line. ColorName names the colour of the warning lines.
#define Food_LegendPerLine 3
internal u32
FoodLegendShortLines(str8 *Lines, u32 Cap, char *ColorName)
{
 u32 Result = 0;
 
 //- Perishable kinds, sorted by shelf life (insertion sort, ties by kind)
 product_kind Kinds[ProductKind_Count];
 u32 KindCount = 0;
 for(u32 Kind = 0; Kind < ProductKind_Count; Kind += 1)
 {
  if(FoodIsPerishable((product_kind)Kind))
  {
   u32 At = KindCount++;
   while(At > 0 && FoodShelfLifeRounds(Kinds[At - 1]) > FoodShelfLifeRounds((product_kind)Kind))
   {
    Kinds[At] = Kinds[At - 1];
    At -= 1;
   }
   Kinds[At] = (product_kind)Kind;
  }
 }
 
 if(Result < Cap) Lines[Result++] = S8("Goes bad after (rounds):");
 for(u32 Start = 0; Start < KindCount && Result < Cap; Start += Food_LegendPerLine)
 {
  str8 Line = S8(" ");
  for(u32 Idx = Start; Idx < Min(Start + Food_LegendPerLine, KindCount); Idx += 1)
  {
   str8 Name = ProductNames[Kinds[Idx]];
   b32 Last = (Idx + 1 == KindCount);
   Line = Str8Fmt("%S %.*s %llu%s", Line, (s32)Name.Size, Name.Data, FoodShelfLifeRounds(Kinds[Idx]), Last ? "" : ",");
  }
  Lines[Result++] = Line;
 }
 if(Result < Cap) Lines[Result++] = S8("Oldest food is eaten first.");
 if(Result < Cap) Lines[Result++] = Str8Fmt("%s = goes bad within %d rounds.", ColorName, Food_SoonRounds);
 return Result;
}

// NOTE(L4): What really happened to one food across all islands so far (for the 3D view's
// shortage strip and dropdown, which otherwise only compare the per-day amounts).
internal food_counts
FoodKindTotals(app_state *App, product_kind Kind)
{
 food_counts Result = {0};
 u32 Count = Min(App->Broadcast.IslandCount, (u32)Food_MaxIslands);
 if(Kind < ProductKind_Count)
 {
  for(u32 I = 0; I < Count; I += 1)
  {
   food_counts *C = &App->Food.Counts[I][Kind];
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

// NOTE(L4): Food of one kind that goes bad, as units PER DAY (the unit of product Amounts),
// averaged since round 1: units thrown away so far * rounds per day / rounds so far.
// The 3D view subtracts it from what is made, so spoiled food counts as a shortage.
internal u64
FoodSpoiledPerDay(app_state *App, product_kind Kind)
{
 u64 Round = Max(App->Broadcast.Round, 1);
 food_counts Totals = FoodKindTotals(App, Kind);
 u64 Result = (Totals.Wasted*(u64)Food_RoundsPerDay + Round/2)/Round;
 return Result;
}
