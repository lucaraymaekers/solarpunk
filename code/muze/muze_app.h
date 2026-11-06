/* date = January 27th 2026 7:06 pm */

#ifndef MUZE_APP_H
#define MUZE_APP_H

//~ Types

//- Muze 
typedef enum piece_note_kind piece_note_kind;
enum piece_note_kind
{
 PieceNoteKind_Silence,
 PieceNoteKind_Pitch,
};

typedef struct piece_note piece_note;
struct piece_note
{
 piece_note_kind Kind;
 f32 Start;
 f32 Length;
 
 s32 Pitch;
};

typedef struct piece piece; 
struct piece 
{
 piece *Next;
 piece *Prev;
 
 str8 Name;
 
 s32 Channel;
 s32 Preset;
 
 f32 BPM;
 f32 TimeSigNum;
 f32 TimeSigDen;
 note_pitch Key;
 b32 Major;
 
 u64 NoteCount;
 piece_note Notes[KB(1)];
 u64 SelectedNoteIdx;
 
 b32 IsRecording;
 b32 IsPlaying;
 b32 IsPracticing;
 f32 RecordLength;
 f32 PlayPos;
 f32 PracticeStart;
 f32 PracticeEnd;
 f32 PracticePos;
 
 b32 Metronome;
 b32 IsLooping;
 
 // UI
 v2 PrevPlayPos; // For animating
 v2 PrevPracticePos; // For animating
 b32 BarWrapping;
 f32 BarPadding;
 f32 BeatWidth;
 f32 NoteSize;
 f32 StaffLineWidth;
 f32 TailHeight;
 
 s32 Octave; // Only for recording
};

typedef enum note_kind note_kind;
enum note_kind
{
 NoteKind_Note,
 NoteKind_Pedal
};

typedef struct rec_note rec_note;
struct rec_note
{
 rec_note *Next;
 rec_note *Prev;
 
 u8 Pitch;
 u8 Velocity;
 u8 Controller;
 f32 Timestamp;
 f32 Duration;
 
 note_kind Kind;
};
raddbg_type_view(note, rows(Note, (s32)Pitch, no_char((note_pitch)(Pitch%Note_Count)), no_char(Velocity), omit($, Pitch, Velocity)));

typedef struct rec_note_node rec_note_node;
struct rec_note_node
{
 rec_note *Value;
 rec_note_node *Next;
 rec_note_node *Prev;
};
#define EachRecNoteNode(Index, First) \
(rec_note_node *_node = (First); _node; _node = _node->Next) \
for (rec_note *Index = _node->Value, *_note = _node->Value; _note; _note = 0)
#define EachRecNote(Index, First) (rec_note *Index = First; !IsNilNote(Index); Note = Note->Next)
#define EachRecNoteBack(Index, Last) (rec_note *Index = Last; !IsNilNote(Index); Note = Note->Prev)

typedef struct voice voice;
struct voice
{
 rec_note *FirstNote;
 rec_note *LastNote;
 s64 NoteCount;
 
 b32 IsRecording;
 b32 IsPlaying;
 b32 AutoScroll;
 
 f32 RecordStart;
 f32 RecordLength;
 f32 PlayPos;
 
 u8 MaxPitch;
 u8 MinPitch;
 
 f32 NoteSetLength;
 
 rec_note_node *NoteSel;
 
 int Channel;
 int PresetIdx;
 f32 Volume;
 f32 MIDISoftness;
 
 f32 SheetScrollX;
 
 arena *Arena;
};

//- Panels

typedef struct panel panel;
struct panel
{
 // NOTE(luca): This has to be the first field for the raddbg type view to work.
 panel *First;
 panel *Last;
 panel *Next;
 panel *Prev;
 panel *Parent;
 
 axis2 Axis;
 f32 ParentPct;
 
 v4 Region;
 
 panel_kind Kind;
 voice *Voice;
 v2 Scroll;
 piece *Piece;
};
#if 0
raddbg_type_view(panel, 
                 rows($,
                      (&First == NilPanel || &First == 0),
                      ParentPct,
                      Kind,
                      Axis,
                      omit(ParentPct, Kind, Axis)));
#endif

#define EachChildPanel(Child, Parent) \
(panel *Child = Parent->First; !IsNilPanel(Child); Child = Child->Next)

typedef struct panel_rec panel_rec;
struct panel_rec
{
 panel *Next;
 s32 PushCount;
 s32 PopCount;
};

typedef struct drag_data drag_data;
struct drag_data
{
 s32 StartPos;
 f32 StartPct;
 f32 NextStartPct;
};

//~ State 
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
 
 //- Voice 
 Command_AddVoice,
 Command_DeleteVoice,
 Command_SetPanelVoice,
 Command_PlayAllVoices,
 Command_StopAllVoices,
 
 Command_Enqueue,
 Command_Dequeue,
 
 Command_ToggleRecording,
 Command_TogglePlaying,
 Command_ToggleAutoScroll,
 
 Command_Trim,
 Command_Stop,
 Command_Reset,
 Command_PlayFromStart,
 Command_PlayFromNote,
 
 Command_Round,
 Command_Sync,
 Command_GuessBPM,
 Command_SetLength,
 Command_ClearSelection,
 Command_DeleteSelection,
 
 //- Lister
 Command_CloseLister,
 Command_OpenLister,
 
 Command_ClosePanel,
 Command_SplitSelectedPanelX,
 Command_SplitSelectedPanelY,
 Command_SelectNextPanel,
 Command_SetPanelType,
 
 //- Piece 
 Command_PieceRecord,
 Comamnd_PiecePlay,
 Command_PiecePause,
 Command_PieceStop,
 Command_PieceClear,
 Command_PieceToggleMetronome,
 Command_PieceToggleBarWrapping,
 Command_PieceSetBPM,
 Command_PieceSetInstrument,
 Command_PieceTogglePracticing,
 Command_PieceSelectNote,
 //-
};
typedef enum command_kind command_kind;

enum lister_kind
{
 ListerKind_Instruments,
 ListerKind_MIDIInput,
 ListerKind_MIDIOutput,
 ListerKind_DebugInfo,
 ListerKind_PanelTypes,
};
typedef enum lister_kind lister_kind;

typedef struct command command;
struct command
{
 command_kind Kind;
 
 panel *Panel;
 panel_kind PanelKind;
 voice *Voice;
 piece *Piece;
 platform_midi_device *Device;
 lister_kind ListerKind;
 axis2 Axis;
 
 s32 S32;
 f32 F32; 
 u64 U64;
 
 // TODO(luca): For undo and redo
 ms_serializer Serializer;
};

typedef struct app_state app_state;
struct app_state
{
 //- UI 
 arena *UIArena;
 font Font;
 font IconsFont;
 font_atlas FontAtlas;
 arena *FontAtlasArena; 
 f32 PreviousHeightPx;
 f32 HeightPx;
 
 //- Panels 
 panel *SelectedPanel;
 panel *FirstPanel;
 panel *FreePanel;
 arena *PanelArena;
 panel *DebugPanel;
 
 //- Misc. 
 u64 FrameIdx;
 arena *ReadOnlyArena;
 
 //- Muze 
 
 // TSF 
 tsf *TrackerForTSF;
 arena *TSFArena;
 s32 NextChannelIdx;
 s32 MetronomeChannel;
 s32 MetronomePreset;
 
 s32 InstrumentCount;
 str8 *InstrumentNames;
 
 // UI 
 ui_box *TopBox;
 ui_box *ListerBox;
 f32 ListerScroll;
 b32 ListerOpened;
 b32 ListerReturnPressed;
 lister_kind ListerKind;
 
 u64 MaxTextSize;
 str8 Text;
 f32 CursorAnimTime;
 
 platform_midi_device In;
 platform_midi_device Out;
 
 b32 OutputMIDIEnabled;
 b32 OutputSynthEnabled;
 b32 InputMIDIEnabled;
 b32 InputVirtualKeyboardEnabled;
 
 s64 MaxVoiceCount;
 s64 VoiceCount;
 voice *Voices;
 voice *SelectedVoice;
 
 arena *PieceArena;
 piece *FirstPiece;
 piece *LastPiece;
 u64 PieceCount;
 u64 MaxPieceCount;
 piece *NextFreePiece;
 
 s32 TimeSig;
 f32 BPM;
 
 // Application memory
 arena *Arena;
 rec_note_node *FirstFreeNode;
 
 // NOTE(luca): PushBuffer
 umm MaxCommandCount;
 umm CommandCount;
 command *Commands;
 
 // Debug
 u64 MaxDebugStringCount;
 u64 DebugStringCount;
 str8 *DebugStrings;
 
 //- Rendering 
 gl_render_state Render;
};

typedef struct lister_item lister_item;
struct lister_item
{
 str8 Name;
 u64 Idx;
};

//- UI 
typedef struct piece_box_data piece_box_data;
struct piece_box_data
{
 ui_box *Box;
 piece *Piece;
 app_state *App;
};

typedef struct muze_box_data muze_box_data;
struct muze_box_data
{
 ui_box *Box;
 app_state *App;
 voice *Voice;
};

typedef struct custom_draw__single_line_text_input_params custom_draw__single_line_text_input_params;
struct custom_draw__single_line_text_input_params
{
 ui_box *Box;
 str8 Text;
 f32 CursorAnimTime;
};

//~ Globals
#define DefaultHeightPx 20

#endif //MUZE_APP_H