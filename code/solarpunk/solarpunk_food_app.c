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

// NOTE(L4): Plain-words lines for the island box, per food the island has or wants:
//   "Fish: 34, 12 go bad in 3 rounds"   (SoonColor when within Food_SoonRounds)
//   "  thrown away 10, missing 50"      (only the parts that are not zero)
// "thrown away" = went bad before anyone ate it; "missing" = wanted to eat it but had none.
internal void
FoodAppendLines(app_state *App, island *Island, str8 *Lines, v4 *Colors, s32 *LineCount, s32 Cap,
                v4 Color, v4 SoonColor)
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
    if(Qty > 0 || Counts->Wasted > 0 || Counts->Shortage > 0)
    {
     str8 Name = ProductNames[Kind];
     if(Stock->LotCount > 0)
     {
      food_lot *Next = &Stock->Lots[0];
      u64 Left = (Next->ExpiresRound > Round) ? Next->ExpiresRound - Round : 0;
      Lines[*LineCount] = Str8Fmt("%.*s: %u, %u go bad in %llu round%s", (s32)Name.Size, Name.Data,
                                  Qty, Next->Qty, Left, (Left == 1) ? "" : "s");
      Colors[*LineCount] = (Next->ExpiresRound <= Round + Food_SoonRounds) ? SoonColor : Color;
     }
     else
     {
      Lines[*LineCount] = Str8Fmt("%.*s: none", (s32)Name.Size, Name.Data);
      Colors[*LineCount] = Color;
     }
     *LineCount += 1;
     
     if(Counts->Wasted > 0 && Counts->Shortage > 0)
     {
      Lines[*LineCount] = Str8Fmt("  thrown away %llu, missing %llu", Counts->Wasted, Counts->Shortage);
     }
     else if(Counts->Wasted > 0)
     {
      Lines[*LineCount] = Str8Fmt("  thrown away %llu", Counts->Wasted);
     }
     else if(Counts->Shortage > 0)
     {
      Lines[*LineCount] = Str8Fmt("  missing %llu", Counts->Shortage);
     }
     if(Counts->Wasted > 0 || Counts->Shortage > 0)
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
