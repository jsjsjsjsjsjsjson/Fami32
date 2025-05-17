#include "gui_common.h"
#include "gui_tracker.h"
#include "gui_channel.h"
#include "gui_menu.h"
#include "gui_frames.h"
#include "gui_info.h"
#include "gui_instrument.h"
#include "gui_visual.h"

// Definition of global variables
uint8_t main_menu_pos = 0;
int8_t channel_sel_pos = 0;
int inst_sel_pos = 0;
uint8_t g_octv = 3;
extern int g_vol;         // `g_vol` is defined in the configuration module
bool touch_note = true;
extern bool edit_mode;    // `edit_mode` is defined in the configuration module
bool setting_change = false;
int copy_start = 0;
bool copy_mode = false;
extern bool _midi_output; // `_midi_output` flag for MIDI output defined elsewhere
extern const char *config_path; // Config file path defined in configuration

std::vector<unpk_item_t> clipboard_data;  // will be used for copy/paste pattern data
const char ch_name[5][10] = {
    "PULSE1", "PULSE2", "TRIANGLE", "NOISE", "DPCM"
};

// Pause audio playback and clear audio buffer
void pause_sound() {
    player.stop_play();
}

// Start/resume audio playback
void start_sound() {
    player.start_play();
}

// Draw a centered popup box with a message (blocking message display)
void drawPopupBox(const char* message) {
    display.setTextWrap(false);
    display.setTextColor(1);

    // Calculate text bounds to size the box
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    display.getTextBounds(message, 0, 0, &x1, &y1, &textWidth, &textHeight);

    int boxWidth = textWidth + 4;
    int boxHeight = textHeight + 4;
    int boxX = (128 - boxWidth) / 2;
    int boxY = (64 - boxHeight) / 2;

    // Draw popup background and border
    display.fillRect(boxX + 1, boxY + 1, boxWidth - 2, boxHeight - 2, 0);
    display.drawRect(boxX - 1, boxY - 1, boxWidth + 2, boxHeight + 2, 0);
    display.drawRect(boxX, boxY, boxWidth, boxHeight, 1);

    // Draw text inside box
    display.setCursor(boxX + 2, boxY + 2);
    display.setTextColor(1);
    display.print(message);
    display.display();
}

// The main GUI task loop that dispatches to different GUI modules based on main_menu_pos.
void gui_task(void *arg) {
    // Ensure the currently selected channel uses the selected instrument (especially when entering edit mode)
    player.channel[channel_sel_pos].set_inst(inst_sel_pos);
    for (;;) {
        switch (main_menu_pos) {
            case 0:
                tracker_menu();
                break;
            case 1:
                channel_menu();
                break;
            case 2:
                frames_menu();
                break;
            case 3:
                instrument_menu();
                break;
            case 4:
                song_info_menu();
                break;
            case 5:
                osc_menu();
                break;
            case 6:
                visual_menu();
                break;
            default:
                // If main_menu_pos is out of range, just loop
                break;
        }
        // Loop continuously; each sub-menu function will return when the user exits it
    }
}
