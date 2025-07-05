#include "gui_instrument.h"
#include "gui_menu.h"      // for menu(), drawPinstripe
#include "gui_input.h"     // for set_channel_sel_pos()
#include "gui_tracker.h"   // for menu_navi()
#include "gui_channel.h"   // for channel_menu

void sample_editor_menu() {
    for (;;) {
        display.clearDisplay();

        display.fillRect(0, 0, 128, 9, 1);
        display.setTextColor(0);
        display.setFont(&rismol57);
        display.setCursor(1, 1);
        display.print("SAMPLE EDITOR");

        display.setTextColor(1);

        display.setCursor(1, 12);
        display.print(random());

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OK) {

                } else if (e.bit.KEY == KEY_NAVI) {
                    menu_navi();
                    break;
                } else if (e.bit.KEY == KEY_MENU) {

                }
            }
        }

        display.display();
        vTaskDelay(4);
    }
}