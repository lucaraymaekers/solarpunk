/* date = January 27th 2026 7:06 pm */

#ifndef MUZE_APP_H
#define MUZE_APP_H

//~ Types

//- Muze 
typedef enum piece_note_kind piece_note_kind;
enum piece_note_kind
{
 PieceNoteKind_None,
 PieceNoteKind_Rest,
 PieceNoteKind_Pitch,
};

typedef struct piece_note piece_note;
struct piece_note
{
 piece_note_kind Kind;
 f32 Start;
 f32 Length;
 u8 Pitch;
 u8 Velocity;
 
 b32 Hidden;
 s32 StaffIdx;
};

typedef enum clef_kind clef_kind;
enum clef_kind
{
 ClefKind_Treble, // Sol
 ClefKind_Bass,   // Fa
 ClefKind_Alto,   // ?
 ClefKind_Tenor,  // Ut
};

typedef struct staff staff;
struct staff
{
 note_pitch Key;
 b32 Minor;
 s32 Octave;
 clef_kind ClefKind;
};

typedef struct piece_note_node piece_note_node;
struct piece_note_node
{
 piece_note *V;
 piece_note_node *Next;
 piece_note_node *Prev;
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
 
 s32 StaffCount;
 staff Staffs[128];
 
 u64 NoteCount;
 piece_note Notes[KB(1)];
 
 u64 SelectionNextIdx;
 piece_note_node Selection[KB(1)];
 piece_note_node *SelectionFirst;
 piece_note_node *SelectionLast;
 piece_note_node *SelectionFree;
 
 b32 IsRecording;
 f32 RecordLength;
 
 b32 IsPlaying;
 f32 PlayPos;
 
 b32 IsPracticing;
 f32 PracticeStart;
 f32 PracticeEnd;
 f32 PracticePos;
 b32 PracticeStrictTiming;
 u64 PracticeNoteCount;
 piece_note PracticeNotes[KB(1)];
 
 f32 MetronomeTime;
 b32 Metronome;
 b32 LoopingDisabled;
 
 // UI
 v2 PrevPlayPos; // For animating
 v2 PrevPracticePos; // For animating
 b32 BarWrapping;
 f32 BarPadding;
 f32 BeatWidth;
 f32 NoteSize;
 f32 StaffLineWidth;
 f32 TailHeight;
 f32 SetNoteLength;
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

//- Lister
enum lister_entry_kind
{
 // A bool could be an f32 that goes from 0 to 1 with step 1
 // A bool could also be a command that toggles that value
 // A command has a key bind
 
 ListerEntryKind_None,
 ListerEntryKind_B32,
 ListerEntryKind_F32,
 ListerEntryKind_Command,
};
typedef enum lister_entry_kind lister_entry_kind;

typedef struct lister_entry lister_entry;
struct lister_entry
{
 str8 Icon;
 str8 Tag;
 str8 Text;
 str8 Description;
 
 s32 S32;
 f32 F32;
 f32 Min;
 f32 Max;
 f32 Step;
 command_kind Command;
 
 // If not set, that uses the Command aboves as default.
 command_kind ButtonCommand;
 
 // Computed when filtered
 s32 Idx;
};

enum lister_kind
{
 ListerKind_Instruments,
 ListerKind_MIDIInput,
 ListerKind_MIDIOutput,
 ListerKind_DebugInfo,
 ListerKind_PanelTypes,
 ListerKind_Commands,
 ListerKind_Everything,
 ListerKind_Count,
};
typedef enum lister_kind lister_kind;

//- Commands
typedef struct command command;
struct command
{
 command_kind Kind;
 
 panel *Panel;
 piece *Piece;
 platform_midi_device *Device;
 axis2 Axis;
 
 s32 S32;
 f32 F32; 
 u64 U64;
 
 // TODO(luca): For undo and redo
 ms_serializer Serializer;
};

//- App 
typedef struct app_state app_state;
struct app_state
{
 //- UI 
 arena *UIArena;
 font TextFont;
 font IconsFont;
 font MusicFont;
 font_atlas FontAtlas;
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
 s32 MaxListerEntryCount;
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
 b32 OutputSynthDisabled;
 b32 InputMIDIDisabled;
 b32 InputVirtualKeyboardDisabled;
 
 arena *PieceArena;
 piece *FirstPiece;
 piece *LastPiece;
 u64 PieceCount;
 u64 MaxPieceCount;
 piece *NextFreePiece;
 
 // Application memory
 arena *Arena;
 
 // NOTE(luca): PushBuffer
 umm MaxCommandCount;
 umm CommandCount;
 command *Commands;
 
 // Debug
 u64 MaxDebugStringCount;
 u64 DebugStringCount;
 str8 *DebugStrings;
 str8 *DebugStringTags;
 
 //- Rendering 
 gl_render_state Render;
};

//~ UI 
typedef struct piece_box_data piece_box_data;
struct piece_box_data
{
 ui_box *Box;
 piece *Piece;
 app_state *App;
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

#define MiddleC 5*Note_Count

#endif //MUZE_APP_H