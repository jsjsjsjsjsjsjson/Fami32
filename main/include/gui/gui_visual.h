#ifndef GUI_VISUAL_H
#define GUI_VISUAL_H

#include "gui_common.h"

#define limit_value(value, min, max) (value < min) ? min : (value > max) ? max : value

// Visualization displays (oscilloscope and custom visualization)
void osc_menu();
void visual_menu();

#endif // GUI_VISUAL_H
