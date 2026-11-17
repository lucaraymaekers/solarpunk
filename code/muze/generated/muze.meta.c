
enum command_kind
{
Command_None,
 Command_SetFontSize,
 Command_SelectInputDevice,
 Command_SelectOutputDevice,
 Command_SelectInstrument,
 Command_ToggleMIDIInput,
 Command_ToggleMIDIOutput,
 Command_ToggleVirtualKeyboardInput,
 Command_ToggleOutputSynth,
 Command_ClosePanel,
 Command_SplitSelectedPanelX,
 Command_SplitSelectedPanelY,
 Command_SelectNextPanel,
 Command_SelectPanelType,
 Command_SetPanelType,
 Command_PieceToggleRecording,
 Command_PieceTogglePlaying,
 Command_PieceStartPlaying,
 Command_PieceStopPlaying,
 Command_PieceTogglePracticing,
 Command_PieceStop,
 Command_PieceClear,
 Command_PieceToggleMetronome,
 Command_PieceToggleBarWrapping,
 Command_PieceToggleStrictTiming,
 Command_PieceToggleLooping,
 Command_PieceSetBPM,
 Command_PieceSelectNote,
 Command_PieceDeselectAllNotes,
 Command_PieceDeselectNote,
 Command_PieceDeleteNote,
 Command_PieceSetNoteLength,
 Command_PieceSelectInstrument,
 Command_PieceAddStaff,
 Command_PieceTrimAtFirstNote,
 Command_PieceSetPracticeRangeFromSelection,
 Command_CloseLister,
 Command_OpenLister,
 Command_SelectCommand,
 Command_Count
};
typedef enum command_kind command_kind;


  typedef struct command_meta_info command_meta_info;
 struct command_meta_info
 {
  command_kind Command;
  b32 NeedsParam;
  str8 Tag;
  str8 Description;
  
  b32 Control;
  b32 Shift;
  b32 Alt;
  platform_key Key;
  
  f32 Min;
  f32 Max;
  f32 Step;
  str8 SliderFmt;
 };
 
command_meta_info CommandsMetaInfo[] =
{

 {
  .Command = Command_None,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"", 0},
  .Description = {(u8 *)"Null Command", 12},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_SetFontSize,
  .NeedsParam = true,
  
  .Tag = {(u8 *)"App", 3},
  .Description = {(u8 *)"Set font size", 13},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_SelectInputDevice,
  .NeedsParam = true,
  
  .Tag = {(u8 *)"App", 3},
  .Description = {(u8 *)"Select MIDI input device", 24},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_SelectOutputDevice,
  .NeedsParam = true,
  
  .Tag = {(u8 *)"App", 3},
  .Description = {(u8 *)"Select MIDI output device", 25},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_SelectInstrument,
  .NeedsParam = true,
  
  .Tag = {(u8 *)"App", 3},
  .Description = {(u8 *)"Select Synth output instrument", 30},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_ToggleMIDIInput,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"App", 3},
  .Description = {(u8 *)"Toggle MIDI input", 17},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 1.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_ToggleMIDIOutput,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"App", 3},
  .Description = {(u8 *)"Toggle MIDI output", 18},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 1.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_ToggleVirtualKeyboardInput,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"App", 3},
  .Description = {(u8 *)"Toggle Virtual Keyboard input", 29},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 1.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_ToggleOutputSynth,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"App", 3},
  .Description = {(u8 *)"Toggle Synth output", 19},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 1.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_ClosePanel,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Panel", 5},
  .Description = {(u8 *)"Close", 5},
  
  .Key = PlatformKey_P,
  .Shift = 1,
  .Control = 1,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_SplitSelectedPanelX,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Panel", 5},
  .Description = {(u8 *)"Split on X axis", 15},
  
  .Key = PlatformKey_P,
  .Shift = 0,
  .Control = 1,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_SplitSelectedPanelY,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Panel", 5},
  .Description = {(u8 *)"Split on Y axis", 15},
  
  .Key = PlatformKey_Minus,
  .Shift = 0,
  .Control = 1,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_SelectNextPanel,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Panel", 5},
  .Description = {(u8 *)"Select next", 11},
  
  .Key = PlatformKey_Comma,
  .Shift = 0,
  .Control = 1,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_SelectPanelType,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Panel", 5},
  .Description = {(u8 *)"Select panel type", 17},
  
  .Key = PlatformKey_T,
  .Shift = 0,
  .Control = 1,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_SetPanelType,
  .NeedsParam = true,
  
  .Tag = {(u8 *)"Panel", 5},
  .Description = {(u8 *)"Set type", 8},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceToggleRecording,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Toggle recording", 16},
  
  .Key = PlatformKey_Space,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 1.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceTogglePlaying,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Toggle playing", 14},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 1.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceStartPlaying,
  .NeedsParam = true,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Start playing", 13},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceStopPlaying,
  .NeedsParam = true,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Stop playing", 12},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceTogglePracticing,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Toggle practicing", 17},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 1.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceStop,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Stop everything", 15},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceClear,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Clear", 5},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceToggleMetronome,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Toggle metronome", 16},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 1.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceToggleBarWrapping,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Toggle bar wrapping", 19},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 1.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceToggleStrictTiming,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Toggle strict timing practice", 29},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 1.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceToggleLooping,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Toggle looping", 14},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 1.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceSetBPM,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Set BPM", 7},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 300.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"%3.0f", 5},
 },
 
 {
  .Command = Command_PieceSelectNote,
  .NeedsParam = true,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Select note", 11},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceDeselectAllNotes,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Deselect all notes", 18},
  
  .Key = PlatformKey_Escape,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceDeselectNote,
  .NeedsParam = true,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Deselect note", 13},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceDeleteNote,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Delete note", 11},
  
  .Key = PlatformKey_Delete,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceSetNoteLength,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Set note length", 15},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceSelectInstrument,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Select synth instrument", 23},
  
  .Key = PlatformKey_S,
  .Shift = 0,
  .Control = 1,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 1.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceAddStaff,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Add bass staff", 14},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceTrimAtFirstNote,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Trim from start to first note", 29},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_PieceSetPracticeRangeFromSelection,
  .NeedsParam = false,
  
  .Tag = {(u8 *)"Piece", 5},
  .Description = {(u8 *)"Set practice range from selection", 33},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_CloseLister,
  .NeedsParam = true,
  
  .Tag = {(u8 *)"Lister", 6},
  .Description = {(u8 *)"Close", 5},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_OpenLister,
  .NeedsParam = true,
  
  .Tag = {(u8 *)"Lister", 6},
  .Description = {(u8 *)"Open", 4},
  
  .Key = PlatformKey_None,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
 {
  .Command = Command_SelectCommand,
  .NeedsParam = true,
  
  .Tag = {(u8 *)"Lister", 6},
  .Description = {(u8 *)"Commands", 8},
  
  .Key = PlatformKey_F1,
  .Shift = 0,
  .Control = 0,
  .Alt = 0,
  
  .Min = 0.f,
  .Max = 0.f,
  .Step = 1.f,
  
  .SliderFmt = {(u8 *)"", 0},
 },
 
};


enum panel_kind
{
PanelKind_Empty,
 PanelKind_Debug,
 PanelKind_Piece,
 PanelKind_Controls,
 PanelKind_Piano,
 PanelKind_Count
};
typedef enum panel_kind panel_kind;

str8 PanelTypeStrings[] =
{
{(u8 *)"Empty", 5},{(u8 *)"Debug", 5},{(u8 *)"Piece", 5},{(u8 *)"Controls", 8},{(u8 *)"Piano", 5},
};

