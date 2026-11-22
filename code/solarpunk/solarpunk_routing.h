/* date = November 22nd 2026 */

#ifndef SOLARPUNK_ROUTING_H
#define SOLARPUNK_ROUTING_H

// NOTE: Link-state routing between the islands (solarpunk_routing.c). Every island is a router:
//   1. Hellos    : each round an island says hello on every link, listing who it heard.
//                  A neighbour is up once the hello is two-way, down after Route_DeadRounds
//                  rounds of silence.
//   2. LSAs      : each island floods its piece of the map (its up neighbours and the cable
//                  costs) with a sequence number. Pieces move one link per round, only newer
//                  ones are kept, so every island builds its own copy of the whole map.
//   3. Dijkstra  : each island computes the cheapest path to every other island on its own
//                  copy of the map, giving a routing table (next hop + cost).
//   4. Goods     : food travels hop by hop, one link per round; every island it passes looks up
//                  the next hop in its OWN routing table.
// Plain data indexed by island index (no pointers), so it survives a hot reload, and it only
// changes once per round, so replaying rounds gives the same history. Only Down[] is a setting
// from the UI: it is kept when the simulation restarts.

//~ Constants
#define Route_MaxIslands         64     // the neighbour masks are u64, one bit per island
#define Route_None               0xFF   // no next hop / no parent
#define Route_Infinity           0xFFFFFFFFu
#define Route_DeadRounds         3      // no hello for this many rounds = the neighbour is down
#define Route_RefreshRounds      10     // an island re-sends its LSA this often, even unchanged
#define Route_MaxAgeRounds       40     // LSAs older than this are thrown away (dead islands fade out)
#define Route_MaxShipments       256
#define Route_MaxShipmentLots    4
#define Route_MaxHops            64     // a shipment that travelled this many links is dropped (loop guard)
#define Route_WantRounds         3.f    // an island orders a food when it holds less than this many rounds of its need
#define Route_KeepRounds         2.f    // a supplier always keeps this many rounds of its own need

//~ Types
// NOTE: One island's piece of the map, as flooded to everyone.
typedef struct route_lsa route_lsa;
struct route_lsa
{
 b32 Valid;
 u32 Seq;                       // higher = newer
 u64 Born;                      // round the origin made it; Round - Born is its age
 u64 NeighbourMask;             // bit I set = the origin has a two-way link to island I
 u8 Cost[Route_MaxIslands];     // cable cost to each neighbour in the mask
};

typedef struct route_shipment route_shipment;
struct route_shipment
{
 b32 Active;
 u32 Kind;                      // product_kind
 u32 From;                      // supplier
 u32 Dest;                      // the island that ordered it
 u32 At;                        // island it is on now
 u32 Hops;
 u32 Qty;
 u32 LotCount;
 food_lot Lots[Route_MaxShipmentLots];   // keeps ageing in transit
 u64 MovedRound;                // round of its last hop (the 3D view draws a train for it)
 u32 MovedFrom;
 u32 MovedTo;
};

// NOTE: What an island thinks of one of its neighbours.
enum route_neighbour_state
{
 RouteNeighbour_Never,     // never heard a hello
 RouteNeighbour_OneWay,    // hears them, but their hello does not list us yet
 RouteNeighbour_Up,        // two-way: used for flooding and routing
 RouteNeighbour_Dead,      // silent for Route_DeadRounds rounds
};
typedef enum route_neighbour_state route_neighbour_state;

typedef struct route_state route_state;
struct route_state
{
 u32 IslandCount;
 b32 Down[Route_MaxIslands];     // set from the panel; a down island sends and forwards nothing

 //- Hellos, seen from island Me: LastHello[Me][Them] = round Me last heard Them (0 = never),
 // TheyHearMe[Me][Them] = Them's last hello listed Me.
 u64 LastHello[Route_MaxIslands][Route_MaxIslands];
 u8 TheyHearMe[Route_MaxIslands][Route_MaxIslands];
 u64 HeardMask[Route_MaxIslands];  // heard within the dead interval (sent in the next hello)
 u64 UpMask[Route_MaxIslands];     // two-way and alive: the island's own links in its LSA

 //- LSAs
 u32 OwnSeq[Route_MaxIslands];
 u64 LastOriginated[Route_MaxIslands];
 b32 ForceOriginate[Route_MaxIslands];
 route_lsa Lsdb[Route_MaxIslands][Route_MaxIslands];      // Lsdb[Me][Origin]: Me's copy of the map
 route_lsa Snapshot[Route_MaxIslands][Route_MaxIslands];  // start of round copy, so LSAs move one link per round
 b32 Dirty[Route_MaxIslands];                             // map changed: rerun Dijkstra

 //- Routing tables (Dijkstra on Lsdb[Me])
 u32 Dist[Route_MaxIslands][Route_MaxIslands];
 u8 NextHop[Route_MaxIslands][Route_MaxIslands];
 u8 Parent[Route_MaxIslands][Route_MaxIslands];

 //- Goods
 route_shipment Shipments[Route_MaxShipments];
 u64 DeliveredUnits;
 u64 LostUnits;
 u64 DeliveredTo[Route_MaxIslands];
};

#endif //SOLARPUNK_ROUTING_H
