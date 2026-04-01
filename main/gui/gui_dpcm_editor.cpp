#include "gui_instrument.h"
#include "gui_menu.h"      // for menu(), drawPinstripe
#include "gui_input.h"     // for set_channel_sel_pos()
#include "gui_tracker.h"   // for menu_navi()
#include "gui_channel.h"   // for channel_menu

void sample_editor_menu() {
    static int selected_sample = 0;
    static int pageStart = 0;
    const int itemsPerPage = 6;

    for (;;) {
        display.clearDisplay();

        display.fillRect(0, 0, 128, 9, 1);
        display.setTextColor(0);
        display.setFont(&rismol57);
        display.setCursor(1, 1);
        display.print("SAMPLE EDITOR ");
        display.setFont(&font4x6);
        display.printf("(%02X/%02X)", selected_sample, ftm.dpcm_block.sample_num - 1);

        display.drawFastHLine(0, 9, player.channel[channel_sel_pos].get_rel_vol() / 2, 1);
        display.drawFastHLine(0, 62, 128, 1);

        display.setFont(&rismol35);
        display.setTextColor(1);

        if (selected_sample < pageStart) {
            pageStart = selected_sample;
        } else if (selected_sample >= pageStart + itemsPerPage) {
            pageStart = selected_sample - itemsPerPage + 1;
        }
        int pageEnd = (pageStart + itemsPerPage >= ftm.dpcm_block.sample_num) ? ftm.dpcm_block.sample_num : pageStart + itemsPerPage;

        for (uint8_t i = pageStart; i < pageEnd; i++) {
            int displayIndex = i - pageStart;
            int itemYPos = 9 + (displayIndex * 8);

            dpcm_sample_t *dpcm_sample = &ftm.dpcm_samples[i];

            if (i == selected_sample) {
                display.fillRect(0, itemYPos + 1, 128, 7, 1);
                display.setTextColor(0);
                display.setCursor(2, itemYPos + 2);
            } else {
                display.setTextColor(1);
                display.setCursor(2, itemYPos + 2);
            }

            if (dpcm_sample->name_len > 18) {
                display.printf("%02X>%.15s...", dpcm_sample->index, dpcm_sample->name);
            } else {
                display.printf("%02X>%s", dpcm_sample->index, dpcm_sample->name);
            }
        }

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OK) {
                    break;
                } else if (e.bit.KEY == KEY_NAVI) {
                    menu_navi();
                    break;
                } else if (e.bit.KEY == KEY_MENU) {
                    instrument_option_page();
                } else if (e.bit.KEY == KEY_BACK) {
                    // BACK
                } else if (e.bit.KEY == KEY_UP) {
                    selected_sample--;
                    if (selected_sample < 0) {
                        selected_sample = ftm.inst_block.inst_num - 1;
                        pageStart = (ftm.inst_block.inst_num / itemsPerPage) * itemsPerPage;
                    }
                } else if (e.bit.KEY == KEY_DOWN) {
                    selected_sample++;
                    if (selected_sample >= ftm.inst_block.inst_num) {
                        selected_sample = 0;
                        pageStart = 0;
                    }
                } else if (e.bit.KEY == KEY_OCTU) {
                    g_octv++;
                } else if (e.bit.KEY == KEY_OCTD) {
                    g_octv--;
                }
            }
        }

        display.display();
        vTaskDelay(4);
    }
}