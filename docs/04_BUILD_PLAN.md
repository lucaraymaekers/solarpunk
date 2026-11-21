# Build Plan (4 people, no fixed owners, ~20h of dev)

> Times are **hours of active dev (H0 = first coding minute)**, not clock times. Sleep and meals are outside this plan.
> "Lanes" below are work streams, not owners. Anyone can pick up any lane; pair up or rotate whenever you like. The only rule: **one person per lane at a time**, so two people do not edit the same files.
> Stack: C in Luca's codebase. Build: `code\build.bat -SolarPunk` (Windows) or `./code/build.sh -SolarPunk` (Linux). See `CLAUDE.md` for conventions.

## 1. Principles
1. **Contract first.** `engine_types.h` (world, pod, island_node, sim_event, villager_action, chaos_action) and `MockWorld()` are written first, together, in the first hour. After that the UI and the engine can be built in parallel.
2. **Thin vertical slice early.** By about H4 there should be 3 to 4 islands, one pod, a day/night cycle, drawn on screen, even if ugly. Widen after that.
3. **`SolarSim` is a first-class tool.** A console target (`code/solarpunk/sim/solar_sim.c`) runs `test` and `sim` with no window. It is how we tune the energy constants and prove scenarios work without clicking.
4. **Undecided things stay behind config fields** (`02_DECISIONS_TBD.md`). Do not stall waiting for the D1 meeting.
5. **Feature freeze at about H15.** Last hours are polish and rehearsal, not new features.
6. **Hot reload is our friend:** tune colours, layout and constants while the app runs. It only works if all state lives in `app_state` (no pointers into code, no `malloc`).

## 2. Lanes

| Lane | Scope | Files (only edit your own) | Can start | Needs |
|---|---|---|---|---|
| **L1: Routing and node logic** | `node_router.c` (belief graph, Dijkstra, destination scoring), `node.c` (pod building, `NodeDecideNextHop`, arrival, Fluid claim), scout pods, sync modes (gossip, sneakernet merge) | `engine/node/node.c`, `node_router.c`, `node_store.c` | after contract | contract types |
| **L2: World render** | Sky + terminator, islands, energy halos, links (dots), pods, trace highlight, hit-testing | `ui/ui_world.c` | after contract | `MockWorld()` (does **not** need the engine) |
| **L3: Villager tablet + control bar** | Tablet (drop, running low, craft, rhythms) with `rl_ui`, top bar (speed, sun scrubber, chaos, sync toggle, KPIs), pod manifest panel, narrative bar | `ui/ui_tablet.c`, `ui/ui_controls.c` | after contract | `MockWorld()`, action structs |
| **L4: Energy and world engine** (the 4th person) | `engine_world.c` (`WorldInit`, `WorldStep` pipeline, transport, storms, dispatch), `node_energy.c` (power state machine, launch guard), `node_need.c` (need model switch), `engine_topology.c` (layout + biconnectivity), conservation check, `engine_scenarios.c` | `engine/engine_world.c`, `engine_topology.c`, `engine_scenarios.c`, `node/node_energy.c`, `node/node_need.c` | after contract | contract types |
| **Shared: tests, tuning, demo** | `solar_sim.c` (the `test` and `sim` commands), constant tuning, scenarios numbers, KPI numbers, pitch material, art direction (D12), README | `sim/solar_sim.c`, docs | after contract | engine (for tuning) |

Notes:
- The old plan had a separate "tests and tuning" lane; with four people it is **shared**: whoever finishes a piece of the engine writes its test in `solar_sim.c` (small, append-only file; merge conflicts are trivial).
- L1 and L4 both touch the engine but different files; their meeting point is the `NodeDecideNextHop`, `NodeOnPodArrived`, `NodeStep` signatures in `engine.h`. Agree on those in H0 to H1 (decision D18).
- When L2 or L3 finish early, they move to polish or help L1/L4 (engine integration is the usual bottleneck).

## 3. Schedule

| Hours | Goal | Done when |
|---|---|---|
| **H0 to H1** | **Team sync + contract.** Quick pass over `02_DECISIONS_TBD.md` (at least D1, D2, D5, D12, D17). Add `SolarSim` target to `cling.c`. Write `engine_types.h`, `engine_defaults.h`, `engine.h`, `engine_unity.c`, `MockWorld()`. | `build.bat -SolarPunk` and `-SolarSim` both build; mock world draws a rect per island. |
| **H1 to H4** | **Vertical slice.** L4: islands, solar cycle, energy, `WorldStep`. L1: one pod hop-by-hop, store-and-forward. L2: sky, islands, pods from mock. L3: tablet skeleton wired to `WorldDispatchVillager`. Shared: `SolarSim test` skeleton, tests 4 and 7 first. | 3 to 4 islands, day/night, one pod crossing the dark side, on screen. |
| **H4 to H9** | **Widen.** Full topology + biconnectivity check, need model (chosen option), pod creation + destination scoring, Fluid claims, gossip + sneakernet, storms, rerouting. UI: halos, trace crate, manifest, chaos button. | Tests 1 to 6 pass; 12-island world runs for several days without losing cargo. |
| **H9 to H12** | **Integrate + tune.** Replace `MockWorld()` with the real world in the UI. Tune constants so night bridges emerge. Build the scenario presets. | Scenario A runs end-to-end on screen. |
| **H12 to H15** | **Feature completion.** KPIs/sustainability panel, narrative bar, tablet polish, rhythms view, optional flags (spoilage/robots only if decided and time allows). | **Feature freeze at H15.** |
| **H15 to H18** | **Polish.** Art direction applied (D12), animations, 60 FPS, resolution check for the projector, record a **fallback demo video**. | Looks like the pitch deck. |
| **H18 to H20** | **Rehearsal.** At least 3 full 5-minute run-throughs, one with a deliberate failure. Final README. Build a **Release** build (`-Release`) and test it on the demo laptop. | Demo is boringly reliable. |

## 4. Cut list (use when behind)
Drop in this order, without guilt:
1. Optional flags: robot islands, spoilage, weight limits.
2. Hybrid sync as a separate mode (keep gossip + sneakernet).
3. Rhythms/learning visualisation on the tablet (keep the learning in the engine).
4. Multi-scenario polish: keep Scenario A and B only.
5. Local store beyond the minimal craft table + drop counters.
6. Island count 12 down to 8.
7. Dotted link drawing (D17): rely on pod motion only.

**Never cut:** isolation invariant, cargo conservation test, store-and-forward, Reserved/Fluid, the storm button, trace crate, tablet driving the live engine.

## 5. Working with Claude Code (4 people, no owners)
- **One repo, short-lived branches** (`feat/<topic>`), small commits, merge to `main` often. Avoid two people editing the same file; the lanes above exist for that.
- **Every Claude Code session starts by reading `CLAUDE.md`** (which points to the docs). Say which lane you are on.
- **Do not edit Luca's layers** (`code/base`, `code/rl`, `code/cling`, other apps) without asking him. The one expected change is the `SolarSim` target in `cling.c`.
- Typical session prompts:
  - Engine: *"Lane L4. Implement `node_energy.c` (power state machine, launch guard) per spec sections 3.1, 3.2 and 5.2. Add tests to `solar_sim.c`. Do not touch `ui/`."*
  - Render: *"Lane L2. Build the island and pod drawing in `ui_world.c` against `MockWorld()`. Include only `engine/engine.h`."*
- Ask Claude Code to run the build, `SolarSim test`, and for engine work `SolarSim sim` before declaring anything done.
- When something in the spec is wrong or ambiguous, **fix the spec in the same change**; the docs are the shared brain.

### Adding the `SolarSim` target (do this in H0)
In `code/cling/cling.c`, next to the other `AddHelpAndBuildSource` lines (for example after "Linux Serial Downloader"), add:

```c
  //~ SolarSim (headless tests + tuning)
  AddHelpAndBuildSource(CmdLine, &Help, &BuildOptions, &DidWork, S8("SolarSim"), S8("../code/solarpunk/sim/solar_sim.c"));
```
and give `solar_sim.c` the same console-app header as the other tools (see `code/pp/pp.c` and `code/compenhance/haversine_generator/haversine_generator.c`: `#define BASE_CONSOLE_APPLICATION 1`, `#define BASE_FORCE_THREADS_COUNT 1`, then `ENTRY_POINT(EntryPoint)`; `BASE_ENTRY_POINT()` is the same thing with C linkage). Then `code\build.bat -SolarSim` produces `build\solar_sim.exe` (exe name comes from the source file name). **Done (L4, 2026-11-21).** Note: `build.bat` only builds `cling.exe` when it is missing, so delete `build\cling.exe` once after changing `cling.c`.

## 6. `SolarSim` commands
- `SolarSim test`: runs the mandatory tests from spec section 12, prints pass/fail, exits with the failure count.
- `SolarSim sim --seed 42 --days 5 --scenario A`: prints per day: deliveries, deliveries during outage, reserve of each island at dusk and dawn, islands that slept early, pods held, total energy spent, energy per unit delivered. Use it to answer: *do quiet islands actually become night bridges? Do busy islands sleep early? Does a cross-globe pod arrive in under about one sim-day?*

## 7. Risks and mitigations

| Risk | Mitigation |
|---|---|
| Energy balance is wrong, so nothing interesting emerges | `SolarSim sim` tuning from H4; constants in one place (`engine_defaults.h`); budget 2h. |
| Engine/UI integration surprises at hour 12 | Contract + `MockWorld()` from H1; vertical slice at H4. |
| Over-engineering (CRDT, BPv7, 3 sync modes) | Scope table in spec section 8; cut list above. |
| D1 (need model) undecided too long | Switch in `node_need.c`; default Option A; decide by H4 at the latest. |
| Rect-only renderer limits the look (no lines, no icons) | Decide D12/D17 in H0; design everything from rounded rects, glow and dots; prototype one island early (H2). |
| C memory bugs (pool overrun, bad index) | Fixed arrays + `Assert` on every index; build with `-Asan` regularly (`build.bat -SolarPunk -Asan`); `-1` ids checked. |
| Hot reload corrupts state | No pointers in `world`; nothing static that must survive; if weird, restart the app. |
| Demo breaks on stage | Seeded deterministic scenarios (replay identically), fallback video, one-click reset, Release build tested on the demo machine. |
| Fatigue and sleep debt | Hard stop times agreed up front; never code the last 2h before rehearsal. |
| Judges cannot read a 12-island map on a projector | Large labels, high-contrast day/night, narrative bar explaining events in plain words. |
