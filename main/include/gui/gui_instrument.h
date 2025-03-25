#ifndef GUI_INSTRUMENT_H
#define GUI_INSTRUMENT_H

#include "gui_common.h"

// Instrument list and editor
void instrument_menu();
void instrument_option_page();
void sequence_editor(instrument_t *inst);
void quick_gen_sequ(sequences_t *sequ, uint32_t type, uint32_t index);

#endif // GUI_INSTRUMENT_H
