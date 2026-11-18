//-
// TSF
//-

//~ Globals
global_variable tsf *GlobalTSF;


//~ Functions
internal void
TSFPlayNote(app_state *App, s32 Channel, s32 Key, f32 Velocity)
{
 if(!App->OutputSynthDisabled)
 {
  tsf_channel_note_on(GlobalTSF, Channel, Key, Velocity);
 }
}

internal void
TSFPlayNoteU8(app_state *App, s32 Channel, s32 Key, u8 Velocity)
{
 TSFPlayNote(App, Channel, Key, ((f32)Velocity/256.f));
}

internal s32
TSFNextChannel(app_state *App)
{
 s32 Result = App->NextChannelIdx;
 App->NextChannelIdx += 1;
 return Result;
}
