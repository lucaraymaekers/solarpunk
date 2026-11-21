# CLAUDE.md

Project: solarpunk sky-island logistics simulator, built **in C** inside Luca's codebase ("Unknown"). Working title SolarWeave; the name is undecided, do not hardcode it.

The hackathon docs live in `docs/`:
1. `docs/01_OVERVIEW.md`: what we are building and the locked world rules.
2. `docs/03_TECH_SPEC.md`: **source of truth** for the engine: structs, math, tick order, tests.
3. `docs/02_DECISIONS_TBD.md`: **open decisions. Never guess or hardcode one.** Support options through `engine_config` fields or the need-model switch.
4. `docs/04_BUILD_PLAN.md`: work lanes, schedule, cut list.
5. `docs/05_DEMO_PITCH.md`: the 5-minute demo the code must support.

Read 01, 03 and 02 fully before writing code. Read 04 to know your lane, 05 when working on the UI or scenarios.

## How to work with the user
- **Ask, do not guess.** When anything is unclear (spec, ownership, an open decision, a codebase convention), ask with the question tool. Before writing any code in a new session, say in 10 lines or fewer what you understood and list your questions.
- Tell the user which lane you are on. Stay inside it. Do not edit other lanes' files.
- Keep answers short and concrete. Small steps; show results (build, tests) before moving on.
- If the spec is wrong or ambiguous: say so, propose a fix, and update `03_TECH_SPEC.md` (or `02_DECISIONS_TBD.md`) in the same change. Only those two docs must be kept current; 01, 04 and 05 are onboarding docs.

## The codebase (read before coding)
- Language: **C** (MSVC on Windows, clang/gcc on Linux). No C++. No external libraries beyond what is in `code/lib`. Build with `code\build.bat -SolarPunk` (or `./code/build.sh -SolarPunk`). The build is `cling` (`code/cling/cling.c`), a self-hosting build script plus a Metadesk metaprogram that turns `.mdesk` files into `generated/*.meta.c`. Run `build\solarpunk.exe`.
- App entry: `code/solarpunk/solarpunk.c` exports `UpdateAndRender(app_memory *, app_offscreen_buffer *, app_input *)` and is loaded from a DLL by the platform layer (`code/rl/rl_platform.c`). **It hot-reloads**: the DLL can be replaced while the app runs.
  - Therefore **all state lives in `app_state`**, carved out of `Memory->Memory` with arenas. No `malloc`, no mutable file-scope globals that must survive a reload, **no function pointers or pointers into code/static data inside persistent state.**
  - `Memory->Initialized` is false only on first start. `Memory->Reloaded` is true after a reload.
- Base layer (`code/base`): types `s32 u32 u64 f32 f64 b32 v2 v4 str8`, `arena` with `PushArray`/`PushArrayZero`/`Scratch`, macros `internal`, `global_variable`, `Assert`, `Min/Max/Clamp`, `ArrayCount`, `EachIndex`, `KB/MB`. Use them; do not reinvent.
- Random numbers: `random_series` in `code/rl/rl_random.h` (PCG: `RandomSeed`, `RandomNext`, `RandomF32`, `RandomBetween`). The engine uses only this. Never `rand()`.
- Renderer (`code/rl`): only **rectangles** (`DrawRect(Dest, Color, CornerRadius, BorderThickness, Softness)`): rounded corners, borders, soft edges, per-corner gradient colours (`Color0..3` in `rect_instance`), clip stack, and font-atlas glyphs. A **circle is a square with `CornerRadius` = half its size**; a ring is the same with a border. There are **no lines, arcs, ellipses or rotation**; build them from many small rects (dots) or extend the shader (ask first).
- UI toolkit (`code/rl/rl_ui.*`, `rl_widgets.c`): box tree with `UI_Row()`, `UI_Column()`, buttons, checkbox, semantic sizes. Use it for the village tablet and the control bar.
- Style (match existing files): 1-space indentation; Allman braces; `PascalCase` functions and variables; result variable named `Result`; `typedef struct name name; struct name {...};`; `internal` for functions; comments as `// NOTE(name):` / `// TODO(name):`; `//~` and `//-` section markers; single translation unit ("unity build": `.c` files are `#include`d from `solarpunk.c`).
- Do **not** edit `code/base`, `code/rl`, `code/cling` or other apps without asking. The only expected change there is adding build targets in `cling.c` (one `AddHelpAndBuildSource` line, see `04_BUILD_PLAN.md`).

## Non-negotiable engine rules
- Engine code lives in `code/solarpunk/engine/`. It may include **only** `base/` and `rl/rl_random.h`. Never `rl_renderer`, `rl_ui`, `rl_platform`, OpenGL or `windows.h`. No `Date`/clock reads, no `rand()`.
- Deterministic: same seed + same input sequence = same history (same build). Fixed timestep. No iteration over unordered data.
- **Strict node isolation:** functions in `engine/node/` take an `island_node *Self`, plain-data observations of direct neighbours, and arriving pods. They never receive or include the `world`. A test scans the source to enforce this.
- **Reserved cargo is never reduced by intermediate islands. Cargo is conserved at every tick.**
- No central server, no admin role, no global scheduler inside the node layer.
- The UI reads the `world` and changes it only through `WorldDispatchVillager`, `WorldDispatchChaos` and `WorldStep`.
- Every constant lives in `engine/engine_defaults.h` or `engine_config`; no scattered magic numbers.

## Definition of done
Before saying a task is finished: it builds without warnings, `SolarSim test` passes (target `SolarSim`, see `04_BUILD_PLAN.md`; build it with `code\build.bat -SolarSim`), and for engine work `SolarSim sim` shows the expected behaviour. Say what you ran and what it printed.
