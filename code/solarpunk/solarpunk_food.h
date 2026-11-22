/* date = November 21st 2026 */

#ifndef SOLARPUNK_FOOD_H
#define SOLARPUNK_FOOD_H

// NOTE(L4): Food spoils. Every unit of food is part of a lot that carries the round it goes bad.
// Lots stay sorted by expiry, so eating and sharing always use the soonest-to-expire food first.
// The state is plain data indexed by island index and product_kind (no pointers), so it
// survives a hot reload, and it only changes once per round, so replaying rounds (the
// timeline slider) gives the exact same food history.
// Moving food between islands is NOT done here: the routing engine calls FoodTake/FoodGive.

//~ Constants
#define Food_MaxIslands    64   // same as Broadcast_MaxIslands
#define Food_MaxLots       48   // lots per (island, kind); more are merged
#define Food_RoundsPerDay  10   // product Amounts are per day; one round = 1/RoundsPerDay day
#define Food_MergeRounds    1   // a new lot within this many rounds of the newest one is merged into it (1 = same round only)
#define Food_SoonRounds     3   // "goes bad soon" = within this many rounds

// NOTE(L4): Shelf life in ROUNDS per product kind; 0 = never spoils, not tracked.
// Every product in the catalog is a food (solarpunk.mdesk). Short on purpose: the demo runs
// about 30 rounds, so every food must be able to go bad inside it. Roughly in real-world
// order (fish and meat first, grain last); tune with SolarSim.
global_variable const u32 FoodShelfLife[ProductKind_Count] =
{
 [ProductKind_Fish]    =  6,
 [ProductKind_Crabs]   =  6,
 [ProductKind_Meat]    =  8,
 [ProductKind_Berries] =  9,
 [ProductKind_Eggs]    = 10,
 [ProductKind_Pears]   = 12,
 [ProductKind_Cabbage] = 14,
 [ProductKind_Apples]  = 16,
 [ProductKind_Carrots] = 18,
 [ProductKind_Coconuts] = 20,
 [ProductKind_Potatoes] = 22,
 [ProductKind_Wheat]   = 24,
};

//~ Types
typedef struct food_lot food_lot;
struct food_lot
{
 u32 Qty;
 u32 ExpiresRound;   // the lot is bad from this round on
};
// NOTE(L4): Two u32s and no padding, so copies never carry stray bytes (byte-identical replays).

typedef struct food_stock food_stock;
struct food_stock
{
 food_lot Lots[Food_MaxLots];   // sorted by ExpiresRound, soonest first
 u32 LotCount;
 f32 ProduceCarry;              // fractional units carried to the next round
 f32 EatCarry;
};

// NOTE(L4): Per (island, kind) bookkeeping. Conservation, always:
//   Produced + Received == Eaten + Wasted + Sent + InStock
typedef struct food_counts food_counts;
struct food_counts
{
 u64 Produced;
 u64 Eaten;
 u64 Wasted;      // went bad in stock (or arrived already bad)
 u64 Shortage;    // units the island wanted to eat but did not have
 u64 Sent;        // taken out by the routing engine (FoodTake)
 u64 Received;    // handed in by the routing engine (FoodGive)
};

typedef struct food_state food_state;
struct food_state
{
 food_stock Stock[Food_MaxIslands][ProductKind_Count];
 food_counts Counts[Food_MaxIslands][ProductKind_Count];
};

#endif //SOLARPUNK_FOOD_H
