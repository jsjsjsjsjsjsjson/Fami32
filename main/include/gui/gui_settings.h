#ifndef GUI_SETTINGS_H
#define GUI_SETTINGS_H

#include "gui_common.h"

// System settings and configuration menu
void settings_page();
// Individual setting adjustment functions (as needed by settings_page menu)
void vol_set_page();
void samp_rate_set();
void eng_speed_set();
void low_pass_set();
void high_pass_set();
void finetune_set();
void over_sample_set();
void midi_out_set();
void debug_print_set();
void erase_config_set();
void reboot_page();

#endif // GUI_SETTINGS_H
