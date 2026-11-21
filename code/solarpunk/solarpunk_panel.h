/* date = November 21st 2026 3:45 pm */

#ifndef SOLARPUNK_PANEL_H
#define SOLARPUNK_PANEL_H

//~ Types
enum panel_tab
{
 PanelTab_Shortages,
 PanelTab_Islands,
 PanelTab_Selected,
 PanelTab_Count
};
typedef enum panel_tab panel_tab;

// NOTE: The islands panel on the right, with three tabs.
typedef struct island_panel_state island_panel_state;
struct island_panel_state
{
 b32 Hidden;
 panel_tab Tab;         // zero = Shortages
 s32 SelectedPlusOne;   // 0 = none, otherwise island index + 1
 s32 RowHoverPlusOne;   // island under the mouse in the panel, highlighted in 3D next frame
 f32 Scroll;            // pixels scrolled down in the tab content
 v4 LastRect;           // where the panel was drawn last frame (zero when hidden)
};

#endif //SOLARPUNK_PANEL_H
