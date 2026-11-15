
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
 Command_ToggleSynthOutput,
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
 Command_PieceDeselectNote,
 Command_PieceDeleteNote,
 Command_PieceSetNoteLength,
 Command_PieceSelectInstrument,
 Command_PieceAddStaff,
 Command_PieceTrimAtFirstNote,
 Command_CloseLister,
 Command_OpenLister,
 Command_SelectCommand,
 Command_Count
};
typedef enum command_kind command_kind;

str8 CommandsDescriptions[] =
{
{(u8 *)"Null Command", 12},
 {(u8 *)"Set font size", 13},
 {(u8 *)"Select MIDI input device", 24},
 {(u8 *)"Select MIDI output device", 25},
 {(u8 *)"Select Synth output instrument", 30},
 {(u8 *)"Toggle MIDI input", 17},
 {(u8 *)"Toggle MIDI output", 18},
 {(u8 *)"Toggle Virtual Keyboard input", 29},
 {(u8 *)"Toggle Synth output", 19},
 {(u8 *)"Close", 5},
 {(u8 *)"Split on X axis", 15},
 {(u8 *)"Split on Y axis", 15},
 {(u8 *)"Select next", 11},
 {(u8 *)"Select panel type", 17},
 {(u8 *)"Set type", 8},
 {(u8 *)"Toggle recording", 16},
 {(u8 *)"Toggle playing", 14},
 {(u8 *)"Start playing", 13},
 {(u8 *)"Stop playing", 12},
 {(u8 *)"Toggle practicing", 17},
 {(u8 *)"Stop", 4},
 {(u8 *)"Clear", 5},
 {(u8 *)"Toggle metronome", 16},
 {(u8 *)"Toggle bar wrapping", 19},
 {(u8 *)"Toggle strict timing practice", 29},
 {(u8 *)"Toggle looping", 14},
 {(u8 *)"Set BPM", 7},
 {(u8 *)"Select note", 11},
 {(u8 *)"Deselect note", 13},
 {(u8 *)"Delete note", 11},
 {(u8 *)"Set note length", 15},
 {(u8 *)"Select instrument", 17},
 {(u8 *)"Add bass staff", 14},
 {(u8 *)"Trim piece start at first note", 30},
 {(u8 *)"Close", 5},
 {(u8 *)"Open", 4},
 {(u8 *)"Commands", 8},
 
};

str8 CommandsTags[] =
{
{(u8 *)"", 0},
 {(u8 *)"App", 3},
 {(u8 *)"App", 3},
 {(u8 *)"App", 3},
 {(u8 *)"App", 3},
 {(u8 *)"App", 3},
 {(u8 *)"App", 3},
 {(u8 *)"App", 3},
 {(u8 *)"App", 3},
 {(u8 *)"Panel", 5},
 {(u8 *)"Panel", 5},
 {(u8 *)"Panel", 5},
 {(u8 *)"Panel", 5},
 {(u8 *)"Panel", 5},
 {(u8 *)"Panel", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Piece", 5},
 {(u8 *)"Lister", 6},
 {(u8 *)"Lister", 6},
 {(u8 *)"Lister", 6},
 
};

b32 CommandHasParam[] =
{
false,
 true,
 true,
 true,
 true,
 false,
 false,
 false,
 false,
 false,
 false,
 false,
 false,
 false,
 true,
 false,
 false,
 false,
 false,
 false,
 false,
 false,
 false,
 false,
 false,
 false,
 true,
 true,
 false,
 false,
 false,
 false,
 false,
 false,
 false,
 true,
 false,
 
};


  typedef struct key_bind key_bind;
 struct key_bind
 {
  b32 Control;
  b32 Shift;
  b32 Alt;
  platform_key Key;
  
  command_kind Command;
 };
 
key_bind DefaultKeyBinds[] =
{

 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_None,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_SetFontSize,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_SelectInputDevice,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_SelectOutputDevice,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_SelectInstrument,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_ToggleMIDIInput,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_ToggleMIDIOutput,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_ToggleVirtualKeyboardInput,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_ToggleSynthOutput,
 },
 {
  .Shift = 1, .Control = 1, .Alt = 0,
  .Key = PlatformKey_P,
  .Command = Command_ClosePanel,
 },
 {
  .Shift = 0, .Control = 1, .Alt = 0,
  .Key = PlatformKey_P,
  .Command = Command_SplitSelectedPanelX,
 },
 {
  .Shift = 0, .Control = 1, .Alt = 0,
  .Key = PlatformKey_Minus,
  .Command = Command_SplitSelectedPanelY,
 },
 {
  .Shift = 0, .Control = 1, .Alt = 0,
  .Key = PlatformKey_Comma,
  .Command = Command_SelectNextPanel,
 },
 {
  .Shift = 0, .Control = 1, .Alt = 0,
  .Key = PlatformKey_T,
  .Command = Command_SelectPanelType,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_SetPanelType,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_Space,
  .Command = Command_PieceToggleRecording,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceTogglePlaying,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceStartPlaying,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceStopPlaying,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceTogglePracticing,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceStop,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceClear,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceToggleMetronome,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceToggleBarWrapping,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceToggleStrictTiming,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceToggleLooping,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceSetBPM,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceSelectNote,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceDeselectNote,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceDeleteNote,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceSetNoteLength,
 },
 {
  .Shift = 0, .Control = 1, .Alt = 0,
  .Key = PlatformKey_S,
  .Command = Command_PieceSelectInstrument,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceAddStaff,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_PieceTrimAtFirstNote,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_CloseLister,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_None,
  .Command = Command_OpenLister,
 },
 {
  .Shift = 0, .Control = 0, .Alt = 0,
  .Key = PlatformKey_F1,
  .Command = Command_SelectCommand,
 },
};


enum panel_kind
{
PanelKind_Empty,
 PanelKind_Piece,
 PanelKind_Controls,
 PanelKind_Debug,
 PanelKind_Piano,
 PanelKind_Count
};
typedef enum panel_kind panel_kind;

str8 PanelTypeStrings[] =
{
{(u8 *)"Empty", 5},{(u8 *)"Piece", 5},{(u8 *)"Controls", 8},{(u8 *)"Debug", 5},{(u8 *)"Piano", 5},
};

