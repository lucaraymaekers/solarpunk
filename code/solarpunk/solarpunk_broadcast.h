/* date = November 21st 2026 2:05 pm */

#ifndef SOLARPUNK_BROADCAST_H
#define SOLARPUNK_BROADCAST_H

//~ Constants
#define Broadcast_MaxIslands    64
#define Broadcast_RoundSeconds  1.5f
#define Broadcast_FlashSeconds  1.0f
#define Broadcast_TimelineRounds 30   // length of the timeline slider
#define Broadcast_MaxJumpRound  1000  // jumps further than this are clamped
// NOTE: Visual-only clock for the sky (L14): one round is one hour, round 1 starts at 12:00 (noon, full day).
#define Broadcast_SkyRoundsPerDay 24
#define Broadcast_SkyStartHour    12

// NOTE: Simulation speeds, chosen with the Up/Down arrow keys. 0 = paused.
global_variable f32 BroadcastSpeeds[] = {0.f, .25f, .5f, 1.f, 2.f, 4.f, 8.f};
#define Broadcast_DefaultSpeedIdx 3 // 1x

//~ Types
// NOTE: What one island announces: everything it produces and everything it needs.
// Indexed by product_kind so it holds no pointers and survives a hot reload.
typedef struct island_ad island_ad;
struct island_ad
{
 b32 Valid;      // false = this island has not heard from the origin yet
 u32 HopCount;   // how many links the announcement travelled
 u64 Round;      // round in which the origin made it; newer wins
 u64 Produces[ProductKind_Count];
 u64 Needs[ProductKind_Count];
 // Food (solarpunk_food.h): units in stock, and units that go bad within a day.
 u64 FoodStock[ProductKind_Count];
 u64 FoodExpiringSoon[ProductKind_Count];
};

typedef struct broadcast_state broadcast_state;
struct broadcast_state
{
 u64 Round;
 f32 Timer;
 // Offset from Broadcast_DefaultSpeedIdx, so a zeroed state starts at 1x.
 s32 SpeedOffset;
 // Speed to go back to when the Play button is pressed.
 s32 SpeedOffsetBeforePause;
 // True while the mouse is dragging the timeline slider.
 b32 TimelineDragging;
 u32 IslandCount;

 // Known[Receiver][Origin]: what Receiver believes Origin produces and needs.
 island_ad Known[Broadcast_MaxIslands][Broadcast_MaxIslands];
 // Copy of Known at the start of a round, so news moves one link per round.
 island_ad Snapshot[Broadcast_MaxIslands][Broadcast_MaxIslands];

 // Flash[From][To] > 0 while a link just delivered news, for the graph view.
 f32 Flash[Broadcast_MaxIslands][Broadcast_MaxIslands];
};

#endif //SOLARPUNK_BROADCAST_H
