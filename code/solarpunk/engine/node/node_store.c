//-
// Node store: craft table (last-write-wins), drop counters, knowledge merge. Spec 2, 8.
// OWNER: lane L1. This file is a placeholder created by L4 so the contract builds;
// L1 replaces the bodies. ISOLATED node file: only Self, Config and plain data.
//-

internal void
NodeMergeKnowledge(island_node *Self, const engine_config *Config, const knowledge_entry *Incoming, f32 SimTime)
{
 // TODO(L1): merge by newer UpdatedAt, never overwrite Knowledge[Self->Id].
}

internal void
NodeOnVillagerSetCraft(island_node *Self, s32 VillagerId, resource_id Craft, f32 SimTime)
{
 // TODO(L1): last-write-wins on Makers[] by UpdatedAt.
}
