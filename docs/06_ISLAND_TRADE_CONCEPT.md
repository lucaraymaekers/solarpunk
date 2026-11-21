# 06: Island Trade Concept (products, needs, announcements, cables, buses)

> **Status: PROPOSAL v0 (2026-11-21), from Denis's interview. Nothing in this file is decided.**
> A point becomes binding only when it moves to the "Decided" table in `02_DECISIONS_TBD.md`.
> Until then `03_TECH_SPEC.md` stays the engine source of truth.
> Where this file conflicts with docs 01 to 05, the conflict is listed in section 3. **Do not silently pick a side.** Ask.

---

## 0. Rules for AI agents reading this file

1. **Notation in Denis's prompts.**
   - Text in `( )` is an instruction to the AI. It is not part of the concept.
   - Text in `' '` is a **future option**. Record it here, but never implement it before the core demo works end to end.
   - Future options are tagged **[FUTURE]** below.
2. **This file is a living spec.** When Denis asks for a change here, that change becomes the new basis of the concept. In the same change:
   - update the "Decided" table in `02_DECISIONS_TBD.md` if the team agreed;
   - update `03_TECH_SPEC.md` if the engine is affected;
   - if teammates have not agreed yet, add a row to the conflict table (section 3) instead.
3. **The project must always compile. Never push a commit that does not build.**
   - On macOS the code cannot build natively. Use the Docker check in section 7. Paste its result in your report.
   - This file changes only docs, but the rule still applies to every push.
4. **Never hardcode an answer to an open question** (section 8). Put it behind an `engine_config` field, as `03_TECH_SPEC.md` already requires.
5. **Interview mode.** When Denis describes something new, critique it like a hostile jury before writing it down. Ask the questions nobody thinks about, and add them to section 8.

---

## 1. The concept, as Denis described it (normalized, nothing added)

- **World:** floating islands "based on the clouds" in a solarpunk world.
- **Links:** islands are interconnected by **electricity cables**, used "just as routing/switching".
- **No centre:** the connections are not centralized. Each island communicates individually; no island is in charge.
- **Production:** every island produces one or more things (food, clothing, ...) and **announces** them to its **neighbour** islands.
- **The core of the project is the communication between islands.**
- **Needs:** an island needs a **minimum of 7 product types to live**, and can use **up to 10 types**. Denis asked for this number to be verified and questioned (section 4.2).
- **Randomness:** what each island produces is **randomized**. "Nothing will be thought by us."
- **Catalog:** the list of possible products is to be built through interview (section 5 is the draft).
- **Transport:** products are carried by **flying buses**, always along the **closest route**.
- **[FUTURE] Seasons:**
  - Each island produces between 1 and 10 products per season.
  - What an island can produce depends on the season (winter, summer, ...), so every season brings other products.
  - Denis wrote this part in single quotes, so it is a future option. *Interpretation to confirm: Q4.*

---

## 2. Solarpunk grounding (researched; use for lore, art and pitch wording)

> Sources are in section 9. In the pitch, say "inspired by", not "implements". **[U]** = could not be fully verified.

### 2.1 Where it comes from
- **2008, "From Steampunk to Solarpunk":** an anonymous post on the blog *Republic of the Bees*, inspired by a cargo ship pulled by a kite sail.
- **2012, Brazil:** the first anthology, *Solarpunk: Ecological and Fantastical Stories in a Sustainable World*.
- **2014, Tumblr:** Olivia Louise's post set the look: *"Natural colors! Art Nouveau! Handcrafted wares! … Streetcars! Airships! Stained glass window solar panels!!!"*
- **2014, Adam Flynn, "Solarpunk: Notes toward a manifesto"** (Arizona State University's Project Hieroglyph). It names four values: **ingenuity, generativity, independence, community**. It prefers "smart citizenry" to "smart cities", and calls for "infrastructure as a form of resistance".
- **"A Solarpunk Manifesto"** (re-des.org, about 2019 [U]): a "post-scarcity, post-hierarchy, post-capitalistic world"; "local solutions, local energy grids, ways of creating autonomous functioning systems"; "high-tech backends with simple, elegant outputs".
- **Key fiction:**
  - Le Guin, *The Dispossessed* (1974): the anarchist Anarres economy.
  - Kim Stanley Robinson, *Pacific Edge* (1990): town councils and water rights.
  - Becky Chambers, *A Psalm for the Wild-Built* (2021): leaderless villages, and the robot's question "What do humans need?".
  - Anthologies: *Sunvault* (2017) and *Glass and Gardens* (2018).
- **Studio Ghibli** (*Nausicaä*, *Castle in the Sky*) is cited as a precursor by Flynn and by the manifesto.

### 2.2 Values, and why "individualism" is the wrong word
- **The mainstream reading is decentralized *community* autonomy**, federated through **mutual aid** and voluntary cooperation, with commons and strong personal freedom inside it.
  - Lineage: Kropotkin → Bookchin's libertarian municipalism and "confederated councils" (Gillam 2023, *Philosophies*).
- Flynn criticises transhumanist futures as **"individualist and unsustainable"**. Andrewism rejects "the individualistic, unsustainable approaches of some futurists". Bookchin broke with anarchism partly over its individualism.
- **For this project:** every island is an autonomous **community**, and there is no centre. That matches the movement exactly. The pitch should say **"autonomy" or "decentralization"**, not "individualism". See C8 and Q5.

### 2.3 Is "announce your surplus to your neighbours" solarpunk? Yes, with one condition
- **The condition:** announcements are **offers answered by need, not price bids**.
- **Kropotkin, *The Conquest of Bread* (1892):** "no stint or limit to what the community possesses in abundance", but "equal sharing and dividing of those commodities which are scarce … a system of rations". This is a ready-made allocation rule: **take freely in abundance, ration equally in scarcity.**
- **Project Cybersyn** (Chile, 1971–73): factories reported production daily over telex. During the October 1972 strike the network kept essential goods moving with about 200 trucks. It is the real-world "announce, then route".
- **Freifunk's Pico Peering Agreement:** every node carries everyone's traffic for free, so transit islands relay for free (Q34).
- **Warning, Anarres:** its coordinating computers sat in the capital, Abbenay, and informal hierarchy crept in. **No island may hold the global state.**

### 2.4 Aesthetic
- **Look:** Art Nouveau and Arts & Crafts; bright greens, blues and light; vertical gardens; stained-glass solar panels; airships and streetcars; "a future with a human face and dirt behind its ears" (Flynn).
- **Floating islands, a trope to invert:** Swift's and Miyazaki's Laputa are **sky empires that dominate the land below**. Our design is the opposite: **many peer islands, no ruling island**. That contrast is pitch material.
- **Precedents a jury may raise:**
  - The co-op game ***Solarpunk*** (Cyberwave) is set on floating islands with airships, a 2026 release [U]. Our project is a decentralized-logistics simulation, not that game. Keep the visuals original and lead with the network.
  - The 2021 Chobani **"Dear Alice"** ad has a **flying school bus**. It is the standard example of *corporate co-optation* of solarpunk; know it before a judge mentions it.

### 2.5 Real-world systems to cite
| System | Fact | Maps to |
|---|---|---|
| **Guifi.net** (Catalonia) | 37,000+ nodes; links held as a **commons** under a Wireless Commons License | cables as a commons |
| **Freifunk** (Germany) | mesh networks; created B.A.T.M.A.N. routing, where no node holds the whole map | neighbour-only routing |
| **NYC Mesh** | 1,000+ nodes; grew **hubs** (supernodes) | warning: decentralized networks drift toward hubs (Q41) |
| **Low-tech Magazine** | solar-powered website, 95% uptime, offline when the battery is empty | islands that sleep (C1) |
| **Permacomputing** | "Keep it small", "Care for the chips" | a native C program in a fixed memory block fits the theme |

### 2.6 Critiques to be ready for
- **Greenwashing and co-optation:** green luxury condos, corporate ads.
- **"Aesthetic without politics":** the answer is to show the *mechanism* (no centre, rationing by need), not only pretty islands.
- **Vagueness and tech-optimism:** the answer is low-tech framing (permacomputing, low-energy, no batteries).
- **Colonial critique:** "pleasant green architecture means nothing if it becomes an extension of colonialist fantasy" (Wilk, e-flux).

---

## 3. Fit with docs 01 to 05: agreements and conflicts

**Agreements:**
- no central server or admin;
- neighbour-only knowledge;
- shortest-route, hop-by-hop forwarding;
- randomness from a seeded `random_series`.

**Conflicts (each one needs a team decision):**

| # | This concept says | Docs 01 to 05 say | Why it matters | Possible reconciliation (not decided) |
|---|---|---|---|---|
| C1 | Islands are linked by **electricity** cables | No batteries. Islands **sleep** when their stored energy hits 0, and "night bridge" islands are the key emergent behaviour (L4, spec 3.2) | If cables carry power, a sunlit island can keep a dark one awake. Sleeping and night bridges then disappear: **the main pitch of 01/05 breaks** | (a) Cables carry **data only** ("signal threads"). (b) Cables carry a trickle of power: enough for messages, not enough to stay awake. (c) Drop the night/sleep story and lead with trade |
| C2 | Islands **announce over cables** | D2 lists gossip, sneakernet ("no radios, the packages are the network") and hybrid | Physical cables make gossip the natural mode. They make the sneakernet headline pointless | Make cable gossip the default. Keep sneakernet as the "cable cut" fallback |
| C3 | Production is **random**: "nothing will be thought by us" | L2: villagers **pick and change their craft** on the tablet. "Update my craft" is one of the three gestures | Random production removes villager agency. Chosen crafts remove the randomness | Random **starting** crafts and random **yields**, but villagers can still switch craft |
| C4 | Islands need **7 to 10** product types | `MAX_RESOURCES = 8`. D5 proposes a 6 to 8 item catalog | An island cannot need 10 types from a catalog of 8. With a catalog of 8, every island needs nearly everything, so islands become identical | Catalog of about 12, `MAX_RESOURCES = 16`, each island needs a subset (section 4.2) |
| C5 | **Flying buses** | Drone **pods / crates** (D11 transport look is open) | A "bus" suggests **several stops and passengers getting on and off**. A pod is one destination | A bus is a pod whose free (Fluid) share is goods getting off at intermediate stops and whose Reserved share rides through. This fits the existing Reserved/Fluid mechanic well |
| C6 | **[FUTURE]** seasons | Time is a day/night cycle: 1 day = 120 sim-seconds, longitude on a cylinder, no latitude | Seasons need either a year clock (a season of several days) or latitude. A 5-minute demo shows at most one or two seasons unless a season lasts about a minute | Season = N sim-days set in config. Islands share one season, or use `y` as latitude for hemispheres |
| C7 | Supply-driven: islands **announce what they have** | Need-driven: `NeedScore`, destination scoring, "running low" taps | Push (supply) floods. Pull (need) only moves goods where they are needed. Docs 03 are pull | Announce supply over cables (push) and choose destinations by need (pull). Both are needed |
| C8 | "Solarpunk is about **individualism**" | 01 describes autonomous **communities of about 100** | Research says solarpunk is decentralized **community** autonomy and mutual aid, not individualism (section 2). A jury may notice | Pitch word: "autonomy", not "individualism" |
| C9 | Denis develops on **macOS** | Build instructions cover Windows and Linux only | The codebase does not compile on macOS (section 7). Denis cannot run the app or check the "always compiles" rule locally | Docker check (section 7). Decide who owns the demo laptop |

---

## 4. Proposed model (draft for discussion, not decided)

### 4.1 Product types versus products
- A **product type** is a need category, for example *Fresh food*.
- A **product** is a concrete item of a type, for example *strawberries*.
- An island **needs types** and **produces products**.
- This split lets seasons change the products while the list of needs stays fixed.

### 4.2 The "7 to 10 types to live" rule: critique and recommendation

**Against the rule as stated:**
- **The numbers are arbitrary.** Survival depends on *quantity* (calories, litres), not on how many categories an island holds. 2,100 kcal of apples is still malnutrition.
- **The catalog must be larger than the need count.** If the catalog has 10 types and an island needs 7 to 10, every island needs almost the same things and the map is monotone.
- **An island that produces all its own needs never trades.** With up to 10 products per island, that will happen.
- **"Minimum 7, maximum 10" mixes two ideas:** what an island *must* have, and what it *can use*.

**For the rule:**
- Real frameworks land in this range when you count only physical goods:
  - Sphere humanitarian standards have 4 to 5 sectors;
  - Doughnut Economics has 12 social dimensions, about 5 of them physical;
  - Max-Neef has 9 universal needs, with goods as *satisfiers*;
  - FAO counts 10 food groups and treats 5 or more as an adequate diet.
- 7 to 10 is also small enough for a jury to read on screen.

**Recommendation, to be confirmed in Q8 to Q12:**
- A **global catalog of about 12 types**.
- Each island gets a random **required set** of 7 to 10 types; the rest are "nice to have".
- Needs are **quantities per season** (1 unit = 1 person-season of need), not a yes/no.
- An island **produces 1 to 3 types** in any period, and is **never allowed to cover all of its own required set**, so trade is guaranteed.
- `MAX_RESOURCES` grows from 8 to 16.

### 4.3 Production (randomized)
- **Core demo:** at world creation the seeded RNG gives each island its production types and yields.
- **[FUTURE]:** production is re-rolled every season from that season's product table (section 5).
- **Determinism:** each island gets its own `random_series`, seeded from `hash(WorldSeed, IslandId)`, so the same seed replays the same world. This is the existing spec rule.

### 4.4 Announcements: "the communication", the core of the concept
This is the part Denis says the project is about, so it should be the headline of the demo.
- **What an announcement carries:** `{island, product, available = stock - reserved, expires_at, seq}`.
  - Publish what is actually available, never the raw stock.
  - An announcement is a **hint, not a promise**. Only the producer knows its true stock.
- **How far it travels:** neighbour to neighbour over cables, merging and forwarding the nearest supplier of each product. In effect each product gets its own small distance-vector table: `nearest tomatoes: cost 7, via island B`.
  - This is the idea behind Named Data Networking (NDN): route by *what* you want, not by *who* has it.
- **Pacing:** announce at once after a change, then back off while nothing changes (the Trickle algorithm, RFC 6206).
- **When two islands want the same goods:** a request is answered with a **lease**, a reservation that expires. If the requester does not confirm, the goods return to stock.
  - The fairness rule must be explicit. First-come-first-served lets well-connected hub islands win every race and starves outer islands.
- **On screen:** pulses travelling along cables, coloured by message type (announce / request / grant), plus a "nearest supply" heat map for each product.

### 4.5 Cables
- Cables are the **control plane**: information, and possibly power (C1).
- Buses are the **data plane**: goods.
- Information can be copied for free. Goods cannot: they are conserved and perishable. Every analogy to computer networking breaks at that line.
- The cable graph should be connected and survive the loss of any one cable or island. `03_TECH_SPEC.md` section 10 already requires biconnectivity.

### 4.6 Flying buses and "closest route"
- **Does a bus follow the cables, or fly straight?** If it flies straight, cables have no effect on delivery and "closest route" is just a straight line. Recommendation: **buses fly along cable corridors** (Q15).
- **"Closest" must be a cost** that adds up along the route and is never negative, for example `distance + hop penalty + penalties for sleeping or stormy islands`. That is spec 3.4. Hop count alone ignores geography.
- A **bus with stops** is the "bus" metaphor, mapped onto Reserved (through passengers) and Fluid (getting off on the way). See C5.

---

## 5. Draft product catalog by type and season (interview input; nothing chosen)

Assumes a temperate climate with greenhouse and permaculture techniques. Floating-island twists are in section 6.

| Type | Winter | Spring | Summer | Autumn | Shelf life |
|---|---|---|---|---|---|
| Water | cloud-net water (fog season peak) | rain and cloud water | low: sunny above the clouds | cloud-net water rising | sealed casks: long |
| Staple food | stored grain, potatoes, dried beans, chuño (freeze-dried potato) | **hungry gap**: stores low, little harvest | early potatoes, barley, broad beans | grain, potatoes, quinoa, squash, nuts | 4 to 8+ seasons |
| Fresh food | greenhouse kale, spinach, leeks, sprouts, mushrooms | asparagus, rhubarb, salad greens | tomatoes, berries, cherries, courgettes | apples, pears, grapes, mushrooms | 0 to 1 season |
| Protein | cheese, cured meat, stored legumes | eggs, milk | milk, insects, fish | nuts, dry beans | 1 to 3 seasons |
| Honey and sugar | stored honey | early honey | sky honey peak | last honey | does not spoil |
| Textiles and clothing | spinning, weaving, felting | wool (shearing), nettle fibre | flax linen | hemp, leather | wears out only |
| Medicine and hygiene | elderberry syrup, tinctures, soap | nettle, willow bark | calendula, chamomile, comfrey | rosehip, echinacea, valerian | dried: long |
| Building materials | timber, willow, bamboo cutting | cob/clay, mycelium bricks | hempcrete, mycelium bricks | straw, reed, hemp hurd | long |
| Tools and spare parts | forge and repair season | low | low | medium | wears out only |
| Soil and compost | curing compost | spreading, green manure | hot compost (18 to 30 days) | leaf mould, biochar | long |
| Seeds | stored seed, seed swaps | none | lettuce, brassica seed | most seed saving | 1 to 7 years |
| Energy (conflicts with C1 and L4) | wind high, solar low | balanced | solar high | wind rising | flywheel / gravity storage only, no chemical batteries |
| Knowledge and repair skills | a service, not cargo: cannot be copied by bus, *can* be sent over cables | | | | |

**Questions this table raises:** Q8 to Q14 and Q23 to Q28.

---

## 6. Floating-island specifics (lore and mechanics flavour)

- **Colder:** temperature falls about 6.5 °C per km. An island at 2 to 3 km has harsher winters and a shorter growing season.
- **More sun and UV above the clouds:** about +10% UV per 1,000 m. Solar power is better, but crops and people get sun stress.
- **Less rain above the cloud deck.** Clouds become the water source. Fog nets (CloudFisher) collect roughly 4 to 22 L per m² on foggy days.
  - *Mechanic idea:* an island can dip into the clouds to harvest water, at the cost of losing sun.
- **No ground soil:** soil has to be composted, which makes compost a precious, tradeable good.
- **Strong, steady wind** for winter energy and for kites, gliders and buses.
- **Unique products:** cloud water, chuño-style freeze-dried food (cold nights, sunny days), sky honey, high-altitude crops (quinoa, oca, barley).
- **Visual references:** *Castle in the Sky* (Laputa) style floating gardens. Keep the look **original**: rounded-rect islands with soft glow, not a recreation of any existing film or game.

---

## 7. Build verification (macOS)

Verified on 2026-11-21 on an Apple Silicon Mac:

- **Native macOS build fails.** `cling` does not compile: `__attribute__((section(".rodata")))` is invalid for Mach-O, and `base_os_linux.c` includes `<linux/limits.h>`. The platform layer only has Linux (X11/GLX/ALSA) and Windows backends.
- **arm64 Linux fails.** `base_intrinsics.h` uses x86 `rdtsc` inline assembly.
- **Debian 12 (bookworm) fails.** Its ALSA is too old: `snd_seq_event_data_t` is undeclared.
- **Debian 13 (trixie) on x86-64 succeeds.** It produces `build/cling`, `build/solarpunk_app.so` and `build/solarpunk`.

The "does it compile" check, run from the repo root:

```sh
docker run --rm --platform linux/amd64 -v "$PWD":/src -w /src debian:trixie sh -c '
  apt-get update -qq >/dev/null &&
  DEBIAN_FRONTEND=noninteractive apt-get install -y -qq clang libx11-dev libxrandr-dev \
    libgl-dev libglx-dev libasound2-dev libcurl4-openssl-dev >/dev/null 2>&1 &&
  ln -sf /usr/bin/clang /usr/local/bin/cc &&
  rm -rf build && sh code/build.sh -SolarPunk;
  test -x build/solarpunk && test -f build/solarpunk_app.so && echo BUILD_OK || { echo BUILD_FAILED; exit 1; }'
```

Notes:
- **Exit codes.** Up to commit `a309147`, cling printed `Done.` and exited 0 even when a target failed (seen with 11 errors on Debian 12).
  - Commit `3582a48` fixes this: cling now prints `Build FAILED.` and exits 1.
  - **Delete `build/cling` once after pulling.** `build.sh` only rebuilds cling when that file is missing, so an old copy keeps the old behaviour.
  - The command above also checks that the output binaries exist. Only `BUILD_OK` counts as passing.
- **Platform matrix** (all x86-64, as of `3582a48`):

  | Platform | Status |
  |---|---|
  | Debian 13 (clang) | builds |
  | Ubuntu 24.04 (clang, gcc) | builds; was broken by four newer X11 keysyms, fixed in `3582a48` |
  | Arch (clang, gcc) | builds |
  | Debian 12 | fails: ALSA too old |
  | Windows/MSVC | **untested from here** |
  | macOS | unsupported; would need a new platform layer |

- **Windows is stricter than Linux.** The Windows build treats warnings as errors (`-WX -W4`), so code that only warns under clang or gcc can still break Windows. Proposed: a GitHub Actions CI job on `ubuntu-latest` and `windows-latest` (Q3).
- This writes `build/` into your checkout; `build` is git-ignored.
- It runs under x86-64 emulation, so it is slow but works.
- It proves the **Linux/clang** build only. The team's main target is Windows/MSVC, so someone must also check `code\build.bat -SolarPunk` on Windows before a demo (Q3).
- Once the `SolarSim` target exists (`04_BUILD_PLAN.md`), add `sh code/build.sh -SolarSim && ./build/solar_sim test` to the same command.

---

## 8. Open questions: the hostile-jury list

Answer by number. Answers move into section 4 and, once agreed, into `02_DECISIONS_TBD.md`.

### A. Process and scope (blocking)
1. A teammate already pushed `CLAUDE.md` and docs 01 to 05 with a different core: day/night energy, pods, and the Reserved/Fluid split. Does this concept **replace** it, **merge into** it, or is it a **separate demo**? Who has the final word? L7 says "decided by the team lead"; who is that?
2. This is a 5-minute demo built in about 20 hours. Seasons, 10+ product types, random production, announcements, buses, energy and Reserved/Fluid will not all fit. **What is the one thing the jury must remember?** You said "the communication between islands". Is that the headline, above "the network sleeps with the sun"?
3. You are on macOS and the code builds only on Linux or Windows x86-64. **Who builds and runs the demo, on which laptop?** Who checks the Windows build before each push?
4. Is my reading right that per-season production (1 to 10 products, different each season) is a **[FUTURE]** option, and the first demos use fixed random production?

### B. World and lore
5. "Solarpunk is about individualism": research says it is about decentralized **community** autonomy and mutual aid (section 2). Do you mean **autonomy** (every island self-governing, no centre)? The pitch word matters.
6. "Islands based on the clouds": do they sit **on** a cloud layer, float **above** it, or move **through** it? This drives water, sunlight and the art.
7. How many islands, and is the map fixed (handcrafted for the demo) or generated from the seed?

### C. Needs (the "7 to 10" rule)
8. Is a need **binary** (the island has the type or not) or a **quantity** (units per season)?
9. What happens when an island lacks a needed type: death, a falling population, an unhappiness meter, or nothing visible? A jury will ask what the stakes are.
10. Why 7 and 10? What breaks at 6, and at 11? How big is the catalog compared with the need count?
11. Does every island need the **same** set, or a random subset?
12. Are all needs **equally urgent**? A water shortage kills in days, a textile shortage after years.
13. Is "food" **one type or several** (staple / fresh / protein)?
14. Are **water** and **energy** products at all, if every island has cloud nets and solar panels? And energy as a product conflicts with "no batteries" (L4).

### D. Production and randomness
15. Do buses **fly along the cables** or **straight**? If straight, what do the cables actually do for delivery?
16. Random production versus "villagers pick their craft" (L2): which wins? If production is random, what does the tablet's "update my craft" button do?
17. "1 to 10 products": product **types**, or **units**? One unit of grain a season feeds nobody.
18. Can an island produce something **it does not need**? Is it **forbidden to produce all of its own needs**? Without that rule, some islands never trade.
19. If the dice give **no island any food in winter**, everyone starves. Is that acceptable, or must the generator guarantee every type exists somewhere?
20. Is randomness rolled **once per run** (seed) or **every season**? Is the demo seed fixed, so the story replays the same way?
21. Is the random draw **weighted by the season table**, or uniform (strawberries in winter)?
22. Does output depend on island size, workers or soil, or is it pure chance?

### E. Seasons [FUTURE]
23. How long is a season in real seconds? (One day is 120 s in the spec, so a 90-day season would last 3 hours.)
24. Do all islands share one season, or are there hemispheres? Opposite seasons would make trade natural.
25. If a bus trip is longer than a product's shelf life, fresh food can never be traded. Should that be a deliberate mechanic (preserving: berries become jam)?
26. Can islands **stockpile** across seasons? Is storage limited?
27. What is the **hungry gap** (early spring) in the game: a crisis moment for the demo?
28. Do processed goods (cheese, tools, bricks) need inputs of other types, i.e. production chains?

### F. Announcements and communication (the core)
29. "Each island communicates individually with each other": with **every** island or only **direct neighbours**? How does an island five hops away learn there are tomatoes?
30. Is an announcement a **promise** or a **hint**? Island A announces 10 apples, and B, C and D each ask for 10 in the same tick. Walk through what happens.
31. Do islands announce only **supply** (push), or also **needs** (pull)? Who decides where goods go: the producer or the consumer?
32. How long is an announcement valid? What happens to goods reserved for an island that went silent?
33. Can an island **lie** (announce 1000 tomatoes it does not have)? How do the others notice? This is a real failure mode: in 2008 Pakistan Telecom wrongly announced YouTube's internet addresses and took YouTube offline worldwide.
34. Why would a **transit island** forward announcements or let buses through? Can it delay, divert or skim them?
35. A cable is cut and the network splits for a while. When it reconnects, which reservations and announcements win?

### G. Buses
36. Who **owns** the buses: every island, or a shared fleet? How many are there, and how much does one carry?
37. Does a bus fly home **empty**? Does it make **several stops**? Multi-stop routing is the vehicle routing problem, which is NP-hard.
38. "Closest route": closest by **distance, hop count, travel time or energy**? Does it change with weather or traffic? If it does, how do you stop routes flipping back and forth?
39. A bus is lost mid-flight (storm). Who notices, who takes the loss, and can a retry deliver the goods twice?
40. How many buses can share one cable corridor or dock at one island at once?

### H. Fairness and scarcity
41. An outer island with one cable loses every first-come race to the central islands. What stops it starving?
42. An island produces **nothing anyone needs**. Why would anyone give it anything? (If the answer is "gift economy and mutual aid", say so in the pitch.)
43. During a shortage, what stops islands from **over-requesting "just in case"**, like a run on a bank?
44. When total supply is below total demand, no protocol helps. Who goes short, and is that intended?
45. Is **success** measured as islands alive, fairness (how evenly needs are met), waste, or bus-kilometres? What number is on screen?

### I. Demo and engineering
46. How do we draw cables with a rectangles-only renderer (D17: dots, or a shader change in Luca's code)?
47. A new island joins mid-run: how does it learn about the world? How long does a dead island's last announcement linger?
48. Same seed, same story: who proves it? Floats, unordered iteration, wall-clock reads and threads can all break it.
49. Which real-world systems do we cite? Community mesh networks (Freifunk, Guifi.net, NYC Mesh), NDN, delay-tolerant networking, Doughnut Economics.
50. A game called *Solarpunk*, set on floating islands with airships, already exists, and a 2021 yogurt ad had a flying school bus. When a judge says "isn't this just that?", what is our one-line answer? (Suggestion: "Ours is not a game world. It is a working decentralized network: no island is in charge, and you can cut a cable live and watch it heal.")

---

## 9. Sources
- **Solarpunk:**
  - Wikipedia overview: https://en.wikipedia.org/wiki/Solarpunk
  - Flynn 2014: https://hieroglyph.asu.edu/2014/09/solarpunk-notes-toward-a-manifesto/
  - Manifesto: https://www.re-des.org/a-solarpunk-manifesto/
  - Gillam 2023: https://doi.org/10.3390/philosophies8040073
  - Andrewism, "What is Solarpunk?": https://theanarchistlibrary.org/library/saint-andrew-what-is-solarpunk.html
  - VICE on politics over aesthetics: https://www.vice.com/en/article/solarpunk-is-not-about-pretty-aesthetics-its-about-the-end-of-capitalism/
  - Wilk on ornament and solar panels (e-flux): https://www.e-flux.com/architecture/positions/191258/is-ornamenting-solar-panels-a-crime/
  - Kropotkin, *The Conquest of Bread*: https://pzacad.pitzer.edu/Anarchist_Archives/kropotkin/conquest/ch5.html
  - Anarres: https://en.wikipedia.org/wiki/Anarres
  - Project Cybersyn: https://en.wikipedia.org/wiki/Project_Cybersyn
  - Guifi.net: https://en.wikipedia.org/wiki/Guifi.net
  - NYC Mesh networking: https://docs.nycmesh.net/networking
  - Low-tech Magazine solar website: https://solar.lowtechmagazine.com/2020/01/how-sustainable-is-a-solar-powered-website/
  - Permacomputing principles: https://permacomputing.net/Principles/
  - *Solarpunk* game: https://www.pcgamer.com/solarpunk-is-a-co-op-cozy-survival-game-set-in-a-world-of-airships-and-floating-islands
  - "Dear Alice" ad: https://www.looper.com/1446464/what-is-solarpunk-dear-alice-ad-explained
- **Needs frameworks:**
  - Doughnut Economics: https://doughnuteconomics.org/doughnut/
  - Max-Neef: https://en.wikipedia.org/wiki/Manfred_Max-Neef%27s_Fundamental_human_needs
  - Sphere standards: https://interactive.spherestandards.org/
  - FAO dietary diversity: https://fantaproject.org/node/1209
- **Seasons and agriculture:**
  - Eliot Coleman, winter harvest: https://www.chelseagreen.com/2024/eliot-coleman-components-of-the-winter-harvest/
  - Flax: https://cropsandsoils.extension.wisc.edu/flax
  - Mycelium bricks: https://news.columbia.edu/news/magic-building-mushrooms
  - Andean freeze-drying: https://textilemuseum.ca/cloth_clay/explore/andes_resources.html
- **Altitude:**
  - Lapse rate: https://www.e-education.psu.edu/meteo3/print/book/export/html/1968
  - UV and altitude: https://www.who.int/uv/faq/whatisuv/en/
  - Fog harvesting: https://www.wipo.int/en/web/wipo-magazine/articles/pioneering-fog-harvesting-technology-helps-relieve-water-shortages-in-arid-regions-40409
- **Networking:**
  - Trickle: https://www.rfc-editor.org/rfc/rfc6206
  - Babel: https://www.rfc-editor.org/rfc/rfc8966
  - OSPF: https://www.rfc-editor.org/rfc/rfc2328
  - Delay-tolerant networking architecture: https://www.rfc-editor.org/rfc/rfc4838
  - Named Data Networking: https://named-data.net
  - Freifunk: https://freifunk.net
