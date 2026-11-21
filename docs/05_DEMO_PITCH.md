# Demo & Pitch Plan (5 minutes)

> Judged on **creativity, overall idea, execution**. The demo has to make the idea obvious in the first 30 seconds and prove it is real by the end.
> Some details depend on open decisions (D1 need model, D2 sync headline, D11 transport look, D12 art). Marked **[TBD]**.
> The demo is a **native app** (C, `build\solarpunk.exe`), run on one laptop (D15). Drawing is rectangle-based (see `03_TECH_SPEC.md` section 11), so the look is flat, geometric and glowing, not illustrated.

## 1. What the judges should remember
1. A bright, hopeful network that **works without a server, without an admin and without batteries**.
2. It **sleeps with the sun**, and still delivers.
3. It is **real**: you can break it live (storm) and watch it heal.

## 2. Five-minute script

| Time | What happens on screen | What we say (gist) |
|---|---|---|
| 0:00 to 0:30 | World map, islands glowing, day/night terminator slowly sweeping. No UI clutter. | "In a solarpunk world, 100 people on an island can't make everything. So islands share." One sentence on the web of drone crates. |
| 0:30 to 1:15 | Open one island's **villager tablet**. Drop 10 apples. Pod appears and lifts off. | "Villagers don't choose routes or destinations. They drop surplus; the software handles the rest. No admin, anyone can change what they make." |
| 1:15 to 2:15 | **Scenario A: cross-globe delivery through a night relay.** Click the pod: **Trace crate** shows its hops, the Reserved/Fluid split, the lit hemisphere moving. A night-bridge island with a full reserve forwards it. | "Each island only knows its neighbours. Each hop is decided locally. Half the planet is dark, so a calm island that saved daytime energy carries it through." |
| 2:15 to 3:00 | **Fluid claim en route.** An intermediate island with a need takes the Fluid share; the Reserved share continues untouched. | "Nobody starves the destination. Islands along the way can take from the free share only." |
| 3:00 to 4:00 | **Chaos button:** storm over a cluster on the pod's path. Pod reroutes (or holds safely and resumes at dawn). Narrative bar explains in words. | "No central server to fail. Storm, night, dead energy reserve: the cargo waits or goes around. Nothing is lost." |
| 4:00 to 4:30 | Show **sustainability panel**: deliveries completed during outages, energy per unit delivered, units never lost. **[TBD: sync toggle moment if D2 picks sneakernet as headline: "no radios at all"]** | "Tech that gets out of the way: it uses only leftover sun." |
| 4:30 to 5:00 | Pull back to the whole planet, calm. Close on the real-world angle. | One-line vision: off-grid and island communities, mutual aid, disaster relief. Thanks. |

## 3. Presets to build (`engine_scenarios.c`, each with a headless smoke test in `SolarSim test`)
- **A. Cross-Globe Delivery Through a Night Relay** (main story). Seeded so it replays identically.
- **B. Storm Heals the Route.** Storm struck on the planned path mid-journey; pod reroutes or holds, then delivers.
- **C. Fluid Share Saves a Flight.** Intermediate island takes a share; destination still gets its Reserved amount.
- **D. (optional) Sneakernet vs Gossip.** Same world, toggle sync mode, show how knowledge spreads with no radios. **[only if D2 headline = sneakernet or time allows]**

Every scenario needs: one-click start, one-click reset, a fixed seed, and a plain-words caption in the narrative bar.

## 4. On-screen KPIs (all come from `WorldStats`)
- Deliveries completed / completed **during outage**
- Deliveries carried through a **night relay**
- Units lost: **0** (cargo conservation)
- **Energy per unit delivered** and total energy used
- Fluid shares that **saved an extra flight**
- Islands asleep right now / awake

**Honesty rule for comparisons:** if we show "vs a centralized system", label it clearly as a *simulated illustrative baseline* and define the baseline on the slide (for example, direct flights from origin through one central hub). Do not claim real-world savings we did not measure.

## 5. Judges' likely questions (prepare short answers)
- *"Is the network real or an animation?"* Each island runs an isolated node that only sees its neighbours and arriving pods; we have tests proving it and a headless replay with fixed seeds.
- *"Why not a central server?"* The point of the world: no admin, no single point of failure, no always-on infrastructure.
- *"What if everyone is asleep?"* Pods wait safely in the last island's queue and resume at dawn. Nothing is dropped.
- *"What stops one island taking everything?"* Reserved share is protected; intermediates only take from the Fluid share.
- *"How does it know what an island needs?"* **[TBD, depends on D1]**
- *"Is this realistic?"* Inspired by delay-tolerant networking (used for space communication) and physical-internet logistics; the energy model is simplified.
- *"Where would this be used?"* Off-grid communities, island archipelagos, disaster relief and mutual-aid supply, as inspiration; we make no claims beyond the simulation.

## 6. Failure plan
- **Fallback video** recorded at about H16 to H18 of the full 5 minutes.
- **Release build** (`-Release`) copied with its `data/` folder to the demo laptop and tested there; do not demo from the hot-reload debug build.
- **Deterministic seeds** so a restart replays the same story.
- **Reset button** on screen and one rehearsed recovery line.
- Test the projector resolution and font sizes in the rehearsal slot, not at the last minute.
- Optional pitch line, only if we measure it: the whole simulation runs in a fixed memory block with no runtime allocation. Do not quote watts or CPU numbers we did not measure.

## 7. Art & tone checklist **[TBD, decision D12]**
Bright and hopeful, the opposite of neon-dark cyberpunk. Warm daylight palette, plants and wind on the islands, soft glow for stored energy (D7), calm motion. Large readable labels for a projector. Pick palette, fonts and island style as a team **before** the render work polishes anything.
