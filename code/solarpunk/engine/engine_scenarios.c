//-
// Scenario presets (05_DEMO_PITCH.md section 3). Lane L4. Each preset uses a fixed seed so it
// replays identically, and has a headless smoke test in SolarSim test.
//-

global_variable const char *ScenarioNames[Scenario_Count] =
{
 "None", "A: Night relay", "B: Storm heals", "C: Fluid saves", "D: Sync compare",
};

internal void
ScenarioLoad(world *World, scenario_kind Kind)
{
 engine_config Config = DefaultConfig();
 // TODO(L4): per-scenario layout, crafts, needs, scripted drops/storms and WarmupDays.
 WorldInit(World, &Config);
}
