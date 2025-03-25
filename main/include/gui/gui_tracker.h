#ifndef GUI_TRACKER_H
#define GUI_TRACKER_H

#include "gui_common.h"

// Tracker (multi-channel) view and global editing options
void tracker_menu();
void main_option_page();
void clipboard_page();
void copy_data(int startRow, int endRow, int channel);
void paste_data(int startRow, int channel);
void clear_clipboard();
void fast_inst_sel_menu();

#endif // GUI_TRACKER_H
