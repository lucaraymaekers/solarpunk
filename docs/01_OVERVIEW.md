# Project Overview: Solarpunk Sky-Island Logistics

> **Working title:** SolarWeave (placeholder, see `02_DECISIONS_TBD.md` D6)
> **Hackathon theme:** Solarpunk, coding a sustainable/green future. *The opposite of dark cyberpunk: a bright, hopeful future where simple, low-energy tech helps nature and people thrive together.*
> **Team:** 4 people, no fixed roles. **Demo:** 5 minutes. **Judged on:** creativity, overall idea, execution.
> **Time budget:** about 20h of active dev inside a 30h window (sleep and meals protected).
> **Stack:** C, inside Luca's codebase (custom base layer, OpenGL rectangle renderer, `rl_ui` toolkit, hot reload, `cling` build). See `CLAUDE.md`.

Read order for newcomers: this file, then `02_DECISIONS_TBD.md`, then `04_BUILD_PLAN.md`. `03_TECH_SPEC.md` is the engineering source of truth for Claude Code. `05_DEMO_PITCH.md` is the 5-minute script.

---

## 1. The idea in one paragraph

In a solarpunk world, people live in small autonomous communities of about 100 on islands. Everyone does what they love (picking apples, weaving shirts, shaping solar glass), and people switch freely. But 100 people cannot make everything, so islands share surplus through a **web of drone-carried crates** that hop island to island across the globe. There is **no central server and no island admin**. Islands run on solar power with **no batteries**, so half the world is always asleep, and the network must keep working anyway. Villagers never route anything: they drop surplus into a shed and the software does the rest.

## 2. Why this is interesting (pitch angles)

- **The opposite of cyberpunk by construction:** no servers, no cloud, no surveillance, no admin. Tech that gets out of the way.
- **The network sleeps with the sun.** Resilience to night and storms is the core of the design, not an afterthought.
- **Battery-free energy:** only unused daytime energy is stored (gravity, flywheels, etc.), and when it runs out the island rests until sunrise. Busy islands tire early; calm islands become **night bridges** for the others. This behaviour *emerges*, it is not scripted.
- **Altruism built into the logistics:** crates carry a protected **Reserved** share for the far destination and a **Fluid** share that islands along the way can take if they need it. Nobody starves the destination.
- **The software learns each island's rhythm** so supplies arrive *before* people run out. (Exact method depends on decision D1.)
- **Real engine, not an animation:** every island runs isolated and may only use what it can physically observe or what arrives in a pod. This is enforced by a test.
- **Light on the machine too:** a native C program with a fixed memory block and no runtime allocation, which fits the "low-energy tech" theme. (Only claim this if we measure it; see `05_DEMO_PITCH.md`.)

## 3. Locked-in world rules

1. **Islands:** about 100 people each, shown as floating sky islands. The engine does not care whether they are islands or ground villages.
2. **Work:** no admin. Anyone can pick or change their craft at any time on the shared terminal.
3. **Surplus first:** the community covers itself, then only the surplus enters the outbound shed.
4. **Energy:** solar only, no chemical batteries. Leftover daytime energy is stored mechanically (visual TBD, D7). At zero stored energy the island sleeps until sunrise.
5. **Globe:** islands wrap around a cylinder of longitudes, so roughly half the world is always in daylight.
6. **Topology:** each island links to only a few neighbours (3 to 4), never everyone.
7. **Villager UX has exactly three gestures:** drop surplus, tap "We're running low on X", update my craft. No destinations, no routes, no chat between islands.

## 4. How a crate travels (plain language)

1. Villagers drop surplus apples in the shed.
2. The island's software decides where they are most needed, based only on what it knows (its own state plus what has reached it).
3. A pod is built carrying `Reserved` units (locked for the destination) plus a few `Fluid` units (up for grabs on the way).
4. At each island on the path, that island's software decides the next hop **using only local knowledge**, steering around sleeping or stormed islands and tired night relays.
5. If the next island is asleep, the pod **waits safely** at the current island (store-and-forward) until sunrise or until another path opens.
6. A passing island in real need may take some Fluid units. The Reserved units are untouchable and arrive intact.

## 5. Demo concept: 3-in-1

| Part | What it is |
|---|---|
| **Main stage (about 75% width)** | Floating-island world map: day/night sweep, each island's stored-energy halo, mesh links, flying pods. Drawn with rounded rectangles (see `03_TECH_SPEC.md` section 11 for the renderer's limits). |
| **Right drawer (about 25%)** | Click any island to open *its* villager tablet (built with `rl_ui`): drop surplus, tap "running low", set craft, view learned rhythms. Changes the live engine immediately. |
| **Top control bar (for the pitch)** | Pause / speed / sun scrubber, **Chaos button** (storm over a cluster), sync-mode toggle, KPI counters. |
| **Trace crate mode** | Click any pod to see its hop-by-hop path and its Reserved/Fluid split. |

See `05_DEMO_PITCH.md` for the 5-minute script.

## 6. Glossary

- **Pod / crate / bundle:** a drone-carried box moving between islands.
- **Reserved / Fluid:** locked-for-destination vs. free-for-passing-islands share of a crate.
- **Store-and-forward (DTN):** hold cargo safely when the next hop is unavailable, forward later.
- **Night bridge:** an island that entered night with a full reserve and relays crates while others sleep.
- **Sneakernet:** islands learn about each other only through what pods physically carry (no radio).
- **Need score:** 0 to 100 signal of how much an island needs an item (how it is computed depends on D1).
- **Tick:** one fixed engine step of 0.05 simulated seconds.

## 7. Open questions
Everything unresolved lives in `02_DECISIONS_TBD.md`. Do not duplicate decisions in this file.
