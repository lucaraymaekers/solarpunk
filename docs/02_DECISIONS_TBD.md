# Decisions Tracker (Still To Be Decided)

> **Rule for everyone (humans and Claude Code):** nothing in this file is decided until it is moved to the "Decided" table at the bottom. Until then the engine must support the options through `engine_config` fields or the need-model switch, never hardcode a guess.
>
> **Suggested team meeting:** one 20-minute sit-down, go top to bottom through Section A. Section B can be settled in chat. Nobody needs to read anything else first.

---

## A. Must decide before (or very early in) engine work

### D1. How does an island know what it "needs"? (the inventory question)
**Status: OPEN, to be discussed as a team. Nothing assumed.**

This is the biggest open design question. Right now the specs only have an abstract "need score". The options:

| | Option A: Abstract cadence | Option B: Stock-based | Option C: Hybrid |
|---|---|---|---|
| **Idea** | Each island has a hidden rhythm per item ("needs shirts roughly every 4 days"). No real stock is tracked. | Each island tracks real stock per item. Villagers consume it, makers produce it. Need = "how many days of cover are left". | Stock tracked only for a few vital items (e.g. food, medicine); rest abstract. |
| **What judges see** | Need bars pulse and get satisfied by deliveries. Simple, clean. | Real shortages and abundance: stock bars rise and fall, you can *see* an island running out. | Mix. |
| **Learning ("learns rhythm")** | Learn from villagers' need events and taps. | Learn consumption rate per person from stock drop. | Both. |
| **Build cost** | Lowest | Medium (+3 to 5h incl. balancing) | Highest (two models to maintain) |
| **Risk** | Feels "fake": nothing is really consumed, so *why* did they need shirts? | Needs balancing so the world doesn't collapse or sit idle. | Complexity. |
| **"Own needs first, then share" story** | Fixed share of production stays home (`OwnShare`). | Naturally true: surplus = stock above safe cover. | Mixed. |

**How the code handles not knowing yet:** the node talks to a need model through one small set of functions (`NeedModelTick`, `NeedScore`, `NeedOnDelivery`, ... see tech spec section 3.3 and 4.4) that switches on `Config.NeedModel`. Option A and B are both small implementations behind it, so the team can start the shared parts (energy, routing, DTN, rendering) today and decide this at the meeting without losing work. Until decided, the default config uses Option A.

**Questions to answer in the meeting:**
- Do we want judges to *see* real stock levels on the tablet, or only need signals?
- Is "running out of apples" something we want visible and dramatic in the demo?
- How much time can we spend on balancing numbers?

### D2. How do islands learn about each other's needs? Which mode is the star of the pitch?
**Status: OPEN.**

Islands have no central server, so Island A must learn that Island Z needs something *somehow*. Three modes are implemented behind `Config.SyncMode`:

1. **`SyncMode_MeshGossip`:** awake neighbouring islands quietly exchange small status packets (low-power radio or light). Fast, but needs a little radio energy.
2. **`SyncMode_Sneakernet`:** **no radio at all.** Every delivery pod carries a small copy of what its launching island knows. When it lands, the receiving island merges it. The packages *are* the network. Slower to spread, but the purest, lowest-energy story.
3. **`SyncMode_Hybrid`:** gossip while neighbours are awake, pods carry updates across sleeping or stormy regions.

**The decision is two parts:**
- (a) **Default mode when the demo starts.**
- (b) **What we tell judges is the headline.** Option: "no servers, no internet, no radios: the packages are the network" (sneakernet as the star, with a live toggle to show gossip for comparison). Alternative: lead with resilience and treat sync as a detail.

**Lore note (if sneakernet is used):** islands still need a way to *see* whether a neighbour is awake without radio. Idea: every awake island shows a glowing lantern/beacon visible to neighbours. Purely visual, zero data.

### D3. Mid-flight depletion: what if the destination runs out of stored energy while a pod is flying to it?
**Status: OPEN** (engine supports both via `Config.MidFlightPolicy`).
- Option 1 `MidFlight_PrelaunchEnergyCheck`: only launch if the receiver will be awake on arrival; on a surprise storm the pod returns to sender.
- Option 2 `MidFlight_PassiveMechanicalLanding`: pods can land on a sleeping island and wait for sunrise.

Suggestion for discussion (not a decision): Option 2 is the more solarpunk story ("the island does not need power to catch a pod"), Option 1 is simpler to reason about.

### D4. Emergency borrowing: what can an island in urgent need take?
**Status: OPEN** (field `Config.EmergencyBorrowPolicy`).
- `Borrow_FluidOnly`: may only take the Fluid part of passing pods. Needs no extra messages. Works in every sync mode.
- `Borrow_NeighborPullFirst` / `Borrow_FluidThenNeighbor`: also request stock from awake neighbours. **This needs a new "request" message type that does not exist in the spec yet and cannot travel in sneakernet mode without riding on pods.**

Note: unless the team wants to build request messages, `Borrow_FluidOnly` is the only option that costs no extra work.

### D5. Starter item catalog
**Status: OPEN.** The two Gemini docs disagree:
- Doc 2: Apples, Grain, Shirts, Medicine, Solar Glass, Tools, Wood, Water Filters (8)
- Doc 3: apples, shirts, herbs, glass, tools, wood, water_filters (7)

Pick 6 to 8 (the engine caps the catalog at `MAX_RESOURCES = 8`). Fewer items = a cleaner demo. Each item needs: name, colour, and a **shape drawn from rectangles** (the renderer has no image/icon loading; see D12). Perishability only if spoilage is on, D10.

### D6. Project name
**Status: OPEN.** "SolarWeave" is Gemini's working title only. It is used in the docs as a placeholder, nothing is hardcoded to it. The build target is called `SolarPunk` (already in `cling.c`) and does not need to change. Candidates are welcome here:
-

---

## B. Check with the whole team (can be settled in chat)

| ID | Decision | Options / notes | Status |
|---|---|---|---|
| D7 | Storage lore + visual (how islands store leftover daytime energy without batteries) | gravity weights hanging under the island, spinning stone flywheels, pumped water towers, thermal sand, or just a glowing halo (a soft-edged rect with `Softness`) | OPEN |
| D8 | Human-to-human messaging between islands | Currently assumed **none** (fully invisible logistics). Confirm. | OPEN: confirm |
| D9 | Robot / automated islands (`Config.EnableRobotIslands`) | produce unpopular-but-vital items when global supply drops; and/or nudge humans via tablet; or skip | OPEN |
| D10 | Physical limits | Spoilage (`EnableSpoilage`), pod weight (`EnableWeightLimits`): in MVP or "future vision" slide? Storms are in already (Chaos button). | OPEN |
| D11 | Transport look | solar gliders, floating crates, mini-zeppelins, glowing seed pods. Must be drawable from rects (a glider = a few rects, a seed pod = a rounded rect with soft glow). | OPEN |
| D12 | Art direction | palette, font, island style, mood. Needed *before* rendering work so it does not get redone. **Constraint:** the renderer only draws rectangles and font glyphs (see D17). The existing UI palette (`ColorU32(Night3)` etc.) is a dark Nord theme; the solarpunk look needs its own bright palette. | OPEN |
| D13 | Island count and layout | Default plan: 12 handcrafted. Fewer is more legible on a projector, more looks impressive. Engine hard cap: `MAX_ISLANDS = 16`. | OPEN |
| D14 | Which demo scenarios to polish | See `05_DEMO_PITCH.md`. | OPEN |
| D15 | Hosting / delivery | The app is a **native Windows/Linux program**, so a "hosted link for judges" is not possible with this stack. Options: run on one laptop (default), ship the `.exe` plus `data/` folder zipped, plus a recorded video. Confirm. | OPEN: confirm |
| D16 | What happens when a pod arrives and the destination's need is already met | Default: deliver anyway. Option (`RebindOnSatisfied`, off by default): redirect to a needier island. | OPEN (low priority) |
| D17 | How to draw mesh links, orbits and the sun arc | The renderer has **no lines, arcs or ellipses**. Options: (a) chains of small dots/rects along a link (zero renderer changes, looks dotted, which suits "supply route" well); (b) a small shader change in `rl_render.mdesk` for rotated rects (clean lines; touches Luca's code, ask first); (c) skip links and rely on pod motion. | OPEN |
| D18 | Where the 4th lane (energy and world engine) draws the line with the routing lane | Boundary between `engine/node/energy` and `engine/node/router`. See `04_BUILD_PLAN.md` section 2. | OPEN: confirm |

---

## C. Post-MVP (only if ahead of schedule)
- Villagers create brand-new item types.
- Judges scan a QR code and control an island from their phone (multi-device). Not possible with the native stack without a lot of extra work: probably drop.
- Real-world translation slide with deeper research (inter-island shipping, off-grid LoRa meshes, disaster relief).
- Robot islands, spoilage, weight limits (if not decided into MVP).

---

## Decided

| ID | Decision | Date | Notes |
|---|---|---|---|
| L1 | Islands of about 100 people, shown as floating sky islands (engine is topology-agnostic) | before 2026-11-21 | |
| L2 | Zero admins; anyone can change craft any time | before 2026-11-21 | |
| L3 | Surplus-first: own community first, only surplus goes out | before 2026-11-21 | |
| L4 | No chemical batteries; only leftover daytime energy is stored; island sleeps at 0 | before 2026-11-21 | |
| L5 | Villagers never choose destinations; they drop surplus and tap "running low" | before 2026-11-21 | |
| L6 | 3-in-1 demo: real engine + island world sim + clickable villager tablet | before 2026-11-21 | |
| L7 | **Stack: C inside Luca's codebase** (base layer, OpenGL rect renderer `rl`, `rl_ui`, hot reload, `cling` build). Replaces the earlier TypeScript + React plan. | 2026-11-21 | Decided by the team lead; engine is plain C in `code/solarpunk/engine/`. |
| L8 | Demo slot is 5 minutes; judged on creativity, idea, execution | 2026-11-21 | |
| L9 | Team of 4, no fixed owners; work together | 2026-11-21 | |
| L11 | **No solar energy, no day/night, no energy reserve.** Food spoils instead: food and seeds have an expiry date, and the network should share food before it goes bad. Time = the app's rounds and timeline slider on `main`. | 2026-11-21 | Team decision. Tech spec section 13. Energy sections of the spec are obsolete. |
| L12 | Spoilage model: lots with an expiry round, eaten soonest-expiring first. Perishable: every product in the food catalog (Apples, Pears, Cabbage, Carrots, Wheat, Fish, Meat, Eggs). Product Amounts are per day (`Food_RoundsPerDay` rounds). | 2026-11-21 | Shelf lives are starting values, still to tune. |
| L13 | Routing/transport of goods is built by another teammate; the food module only offers `FoodTake`/`FoodGive` and "expiring soon" in announcements. | 2026-11-21 | |
| L14 | The 3D view shows a **visual-only** day/night cycle (sky colours, sun and moon; island lighting does not change): one round = one hour, 24 rounds per sky day, round 1 = 06:00; the sky moves smoothly through each round and pauses with the sim; a "Day N, hh:00" card bottom-left. No effect on the simulation, so L11 still holds. Note: this sky day is not the food day (`Food_RoundsPerDay` = 10 rounds). | 2026-11-21 | Code: `BroadcastDayOfRound`, `R3BuildSky`, `V3DrawClock`. |
| L10 | Engine uses fixed-capacity arrays and arenas only (no `malloc`), all state in one `world` struct inside `app_state`, so hot reload keeps working and replays are deterministic | 2026-11-21 | Follows from L7. |
