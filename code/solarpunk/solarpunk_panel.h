/* date = November 21st 2026 3:45 pm */

#ifndef SOLARPUNK_PANEL_H
#define SOLARPUNK_PANEL_H

//~ Types
// NOTE: The islands panel on the right: an overview of all islands, or the details of one.
typedef struct island_panel_state island_panel_state;
struct island_panel_state
{
 b32 Hidden;
 s32 SelectedPlusOne;   // 0 = overview of all islands, otherwise island index + 1
 s32 RowHoverPlusOne;   // island under the mouse in the panel, highlighted in 3D next frame
 f32 Scroll;            // pixels scrolled down in the panel content
 v4 LastRect;           // where the panel was drawn last frame (zero when hidden)
};

#endif //SOLARPUNK_PANEL_H
