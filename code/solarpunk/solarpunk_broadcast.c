//-
// SolarPunk broadcast: every island announces what it produces and what it needs.
// Islands only talk to their direct neighbours (links). Each round, every island
// passes everything it knows one link further, so far islands learn a few rounds later.
//-

//~ Helpers
internal u32
BroadcastIslandIndex(app_state *App, island *Target)
{
 u32 Result = 0;
 for(island *Island = App->FirstIsland; Island && Island != Target; Island = Island->Next)
 {
  Result += 1;
 }
 return Result;
}

internal u32
BroadcastCountIslands(app_state *App)
{
 u32 Result = 0;
 for(island *Island = App->FirstIsland; Island; Island = Island->Next)
 {
  Result += 1;
 }
 return Result;
}

internal island_ad
BroadcastMakeOwnAd(island *Island, u64 Round)
{
 island_ad Result = {0};
 Result.Valid = true;
 Result.HopCount = 0;
 Result.Round = Round;

 for(product *Product = Island->FirstProduct; Product; Product = Product->Next)
 {
  Assert(Product->Kind < ProductKind_Count);
  Result.Produces[Product->Kind] += Product->Amount;
 }

 for(product *Req = Island->FirstRequirement; Req; Req = Req->Next)
 {
  Assert(Req->Kind < ProductKind_Count);
  Result.Needs[Req->Kind] += Req->Amount;
 }

 return Result;
}

// NOTE: An offer replaces what the receiver has when it is newer, or equally new but
// came along a shorter path.
internal b32
BroadcastOfferIsBetter(island_ad *Offer, island_ad *Have)
{
 b32 Result = false;

 if(Offer->Valid)
 {
  if(!Have->Valid)
  {
   Result = true;
  }
  else if(Offer->Round > Have->Round)
  {
   Result = true;
  }
  else if(Offer->Round == Have->Round && Offer->HopCount + 1 < Have->HopCount)
  {
   Result = true;
  }
 }

 return Result;
}

//~ Round
internal void
BroadcastRound(app_state *App)
{
 broadcast_state *Broadcast = &App->Broadcast;

 u32 IslandCount = BroadcastCountIslands(App);
 Assert(IslandCount <= Broadcast_MaxIslands);
 IslandCount = Min(IslandCount, (u32)Broadcast_MaxIslands);
 Broadcast->IslandCount = IslandCount;
 Broadcast->Round += 1;

 //- Food: spoil, produce, eat (solarpunk_food.c). Before the ads, so they carry this round's food.
 FoodRoundFromApp(App, Broadcast->Round);

 //- Routing: hellos, LSAs, Dijkstra, and goods moving one link (solarpunk_routing.c).
 RouteRound(App, Broadcast->Round);

 //- Every island refreshes its own announcement.
 {
  u32 Idx = 0;
  for(island *Island = App->FirstIsland; Island && Idx < IslandCount; Island = Island->Next)
  {
   Broadcast->Known[Idx][Idx] = BroadcastMakeOwnAd(Island, Broadcast->Round);
   FoodFillAd(&App->Food, Idx, &Broadcast->Known[Idx][Idx], Broadcast->Round);
   Idx += 1;
  }
 }

 //- Every island passes what it knew at the start of the round to its neighbours.
 // NOTE: The first round only seeds each island's own announcement, so the view
 // starts with every island knowing only itself.
 b32 IsFirstRound = (Broadcast->Round == 1);
 MemoryCopyArray(Broadcast->Snapshot, Broadcast->Known);

 u32 FromIdx = 0;
 for(island *From = App->FirstIsland; From && FromIdx < IslandCount && !IsFirstRound; From = From->Next)
 {
  // A shut-down island (solarpunk_routing.c) neither sends nor receives news.
  for(link *Link = From->FirstLink; Link && !RouteIsDown(App, FromIdx); Link = Link->Next)
  {
   u32 ToIdx = BroadcastIslandIndex(App, Link->Dest);
   if(ToIdx < IslandCount && !RouteIsDown(App, ToIdx))
   {
    for(u32 OriginIdx = 0; OriginIdx < IslandCount; OriginIdx += 1)
    {
     island_ad *Offer = &Broadcast->Snapshot[FromIdx][OriginIdx];
     island_ad *Have = &Broadcast->Known[ToIdx][OriginIdx];

     if(BroadcastOfferIsBetter(Offer, Have))
     {
      // Light up the link only when the receiver learns about a new island.
      if(!Have->Valid)
      {
       Broadcast->Flash[FromIdx][ToIdx] = Broadcast_FlashSeconds;
      }

      *Have = *Offer;
      Have->HopCount += 1;
     }
    }
   }
  }

  FromIdx += 1;
 }
}

internal s32
BroadcastSpeedIdx(broadcast_state *Broadcast)
{
 s32 Result = Clamp(0, Broadcast_DefaultSpeedIdx + Broadcast->SpeedOffset, (s32)ArrayCount(BroadcastSpeeds) - 1);
 return Result;
}

internal f32
BroadcastSpeed(broadcast_state *Broadcast)
{
 f32 Result = BroadcastSpeeds[BroadcastSpeedIdx(Broadcast)];
 return Result;
}

// NOTE: Resets the simulation only; speed and UI state are kept.
internal void
BroadcastRestart(app_state *App)
{
 broadcast_state *Broadcast = &App->Broadcast;

 Broadcast->Round = 0;
 Broadcast->Timer = 0.f;
 Broadcast->IslandCount = 0;
 MemorySet(Broadcast->Known, 0, sizeof(Broadcast->Known));
 MemorySet(Broadcast->Snapshot, 0, sizeof(Broadcast->Snapshot));
 MemorySet(Broadcast->Flash, 0, sizeof(Broadcast->Flash));
 FoodReset(&App->Food);
 RouteReset(App);
}

// NOTE: Rounds are deterministic (same world, same rules), so going back in time is
// done by restarting and replaying up to the target round. Round 1 is the first round.
internal void
BroadcastJumpToRound(app_state *App, u64 TargetRound)
{
 broadcast_state *Broadcast = &App->Broadcast;

 TargetRound = Clamp(1, TargetRound, (u64)Broadcast_MaxJumpRound);
 if(TargetRound < Broadcast->Round)
 {
  BroadcastRestart(App);
 }

 while(Broadcast->Round < TargetRound)
 {
  BroadcastRound(App);
 }

 // A jump should not light up every link it passed through.
 Broadcast->Timer = 0.f;
 MemorySet(Broadcast->Flash, 0, sizeof(Broadcast->Flash));
}

internal void
BroadcastJumpBy(app_state *App, s64 Delta)
{
 s64 Target = (s64)App->Broadcast.Round + Delta;
 BroadcastJumpToRound(App, (u64)Max(Target, 1));
}

//~ Clock (visual only, see L14)
// NOTE: Day number (1 = first day) and hour (0..23) of a round, and the time of day
// (0..1, 0 = midnight). RoundFraction (0..1) is how much of the round has passed, so the
// sky can move smoothly; at 0 the time is exactly the round's hour.
internal u64
BroadcastDayOfRound(u64 Round, f32 RoundFraction, u32 *Hour, f32 *TimeOfDay)
{
 u64 RoundsDone = (Round > 0) ? Round - 1 : 0;
 u64 Hours = Broadcast_SkyStartHour + RoundsDone;
 *Hour = (u32)(Hours % Broadcast_SkyRoundsPerDay);
 *TimeOfDay = ((f32)*Hour + Clamp(0.f, RoundFraction, 1.f))/(f32)Broadcast_SkyRoundsPerDay;
 u64 Result = Hours/Broadcast_SkyRoundsPerDay + 1;
 return Result;
}

//~ Actions (used by both the keyboard and the on-screen buttons)
internal b32
BroadcastIsPaused(broadcast_state *Broadcast)
{
 b32 Result = (BroadcastSpeedIdx(Broadcast) == 0);
 return Result;
}

internal void
BroadcastFaster(broadcast_state *Broadcast)
{
 s32 MaxOffset = (s32)ArrayCount(BroadcastSpeeds) - 1 - Broadcast_DefaultSpeedIdx;
 Broadcast->SpeedOffset = Min(Broadcast->SpeedOffset + 1, MaxOffset);
}

internal void
BroadcastSlower(broadcast_state *Broadcast)
{
 Broadcast->SpeedOffset = Max(Broadcast->SpeedOffset - 1, -Broadcast_DefaultSpeedIdx);
}

internal void
BroadcastTogglePause(broadcast_state *Broadcast)
{
 if(BroadcastIsPaused(Broadcast))
 {
  Broadcast->SpeedOffset = Broadcast->SpeedOffsetBeforePause;
  if(BroadcastIsPaused(Broadcast))
  {
   Broadcast->SpeedOffset = 0; // 1x
  }
 }
 else
 {
  Broadcast->SpeedOffsetBeforePause = Broadcast->SpeedOffset;
  Broadcast->SpeedOffset = -Broadcast_DefaultSpeedIdx;
 }
}

internal void
BroadcastStep(app_state *App)
{
 App->Broadcast.Timer = 0.f;
 BroadcastRound(App);
}

// NOTE: Keyboard controls
//   Up / Down : faster / slower (slowest is paused)
//   Right     : advance one round now
//   Left      : restart, every island knows only itself again
internal void
BroadcastUpdate(app_state *App, app_input *Input)
{
 broadcast_state *Broadcast = &App->Broadcast;
 f32 dt = Input->dtForFrame;

 //- Controls
 {
  if(WasPressed(Input->ActionUp))    BroadcastFaster(Broadcast);
  if(WasPressed(Input->ActionDown))  BroadcastSlower(Broadcast);
  if(WasPressed(Input->ActionLeft))  BroadcastRestart(App);
  if(WasPressed(Input->ActionRight)) BroadcastStep(App);
 }

 for(u32 From = 0; From < Broadcast_MaxIslands; From += 1)
 {
  for(u32 To = 0; To < Broadcast_MaxIslands; To += 1)
  {
   Broadcast->Flash[From][To] = Max(0.f, Broadcast->Flash[From][To] - dt);
  }
 }

 Broadcast->Timer += dt*BroadcastSpeed(Broadcast);
 if(Broadcast->Round == 0 || Broadcast->Timer >= Broadcast_RoundSeconds)
 {
  Broadcast->Timer = 0.f;
  BroadcastRound(App);
 }
}

//~ Queries (for the graph view)
internal u32
BroadcastKnownCount(app_state *App, island *Island)
{
 broadcast_state *Broadcast = &App->Broadcast;

 u32 Result = 0;
 u32 Idx = BroadcastIslandIndex(App, Island);
 if(Idx < Broadcast->IslandCount)
 {
  for(u32 OriginIdx = 0; OriginIdx < Broadcast->IslandCount; OriginIdx += 1)
  {
   if(Broadcast->Known[Idx][OriginIdx].Valid)
   {
    Result += 1;
   }
  }
 }

 return Result;
}

// NOTE: 0 = quiet, 1 = just delivered news (in either direction).
internal f32
BroadcastLinkFlash(app_state *App, island *A, island *B)
{
 broadcast_state *Broadcast = &App->Broadcast;

 f32 Result = 0.f;
 u32 AIdx = BroadcastIslandIndex(App, A);
 u32 BIdx = BroadcastIslandIndex(App, B);
 if(AIdx < Broadcast_MaxIslands && BIdx < Broadcast_MaxIslands)
 {
  f32 Flash = Max(Broadcast->Flash[AIdx][BIdx], Broadcast->Flash[BIdx][AIdx]);
  Result = Flash/Broadcast_FlashSeconds;
 }

 return Result;
}
