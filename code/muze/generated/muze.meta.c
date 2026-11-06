
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
 Command_SetPanelType,
 Command_PieceToggleRecording,
 Command_PieceTogglePlaying,
 Command_PieceTogglePracticing,
 Command_PieceStop,
 Command_PieceClear,
 Command_PieceToggleMetronome,
 Command_PieceToggleBarWrapping,
 Command_PieceSetBPM,
 Command_PieceSelectNote,
 Command_PieceSelectInstrument,
 Command_CloseLister,
 Command_OpenLister,
 Command_Count
};
typedef enum command_kind command_kind;

str8 CommandsNames[] = {
{(u8 *)"Null Command", 12},
 {(u8 *)"App: Set font size", 18},
 {(u8 *)"App: Select MIDI input device", 29},
 {(u8 *)"App: Select MIDI output device", 30},
 {(u8 *)"App: Select Synth output instrument", 35},
 {(u8 *)"App: Toggle MIDI input", 22},
 {(u8 *)"App: Toggle MIDI output", 23},
 {(u8 *)"App: Toggle Virtual Keyboard input", 34},
 {(u8 *)"App: Toggle Synth output", 24},
 {(u8 *)"Panel: Close", 12},
 {(u8 *)"Panel: Split on X axis", 22},
 {(u8 *)"Panel: Split on Y axis", 22},
 {(u8 *)"Panel: Select next", 18},
 {(u8 *)"Panel: Set type", 15},
 {(u8 *)"Piece: Toggle recording", 23},
 {(u8 *)"Piece: Toggle playing", 21},
 {(u8 *)"Piece: Toggle practicing", 24},
 {(u8 *)"Piece: Stop", 11},
 {(u8 *)"Piece: Clear", 12},
 {(u8 *)"Piece: Toggle metronome", 23},
 {(u8 *)"Piece: Toggle bar wrapping", 26},
 {(u8 *)"Piece: Set BPM", 14},
 {(u8 *)"Piece: Select note", 18},
 {(u8 *)"Piece: Select instrument", 24},
 {(u8 *)"Lister: Close", 13},
 {(u8 *)"Lister: Open", 12},
 
};

command_kind CommandsWithoutParam[] = {
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
 Command_SetPanelType,
 Command_PieceToggleRecording,
 Command_PieceTogglePlaying,
 Command_PieceTogglePracticing,
 Command_PieceStop,
 Command_PieceClear,
 Command_PieceToggleMetronome,
 Command_PieceToggleBarWrapping,
 Command_PieceSetBPM,
 Command_PieceSelectNote,
 Command_PieceSelectInstrument,
 Command_CloseLister,
 Command_OpenLister,
 
};

b32 CommandHasParam[] = {
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
 true,
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
 true,
 
};


enum panel_kind
{
PanelKind_Empty,
 PanelKind_Piece,
 PanelKind_Controls,
 PanelKind_Debug,
 PanelKind_Count
};
typedef enum panel_kind panel_kind;

str8 PanelTypeStrings[] = {
{(u8 *)"Empty", 5},{(u8 *)"Piece", 5},{(u8 *)"Controls", 8},{(u8 *)"Debug", 5},
};

