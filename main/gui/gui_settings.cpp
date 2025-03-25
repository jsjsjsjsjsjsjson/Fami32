#include "gui_settings.h"
#include "gui_menu.h"     // for menu(), num_set_menu_int

// Volume setting page (separate quick adjust for master volume)
void vol_set_page() {
    int last_vol = g_vol;
    num_set_menu_int("VOLUME", 0, 64, 1, &g_vol, 0, 0, 68, 32);
    if (g_vol != last_vol) {
        drawPopupBox("SAVE CONFIG...");
        display.display();
        set_config_value("VOLUME", CONFIG_INT, &g_vol);
        write_config(config_path);
    }
}

// Adjust sample rate
void samp_rate_set() {
    num_set_menu_int("SAMPLE RATE", 10800, 192000, 6000, &SAMP_RATE, 0, 0, 128, 32);
    setting_change = true;
    if (set_config_value("SAMPLE_RATE", CONFIG_INT, &SAMP_RATE) == CONFIG_SUCCESS) {
        printf("Updated 'SAMPLE_RATE' to %d\n", SAMP_RATE);
    }
}

// Adjust audio engine speed (ticks per frame)
void eng_speed_set() {
    num_set_menu_int("ENGINE SPEED", 1, 240, 1, &ENG_SPEED, 0, 0, 128, 32);
    setting_change = true;
    if (set_config_value("ENGINE_SPEED", CONFIG_INT, &ENG_SPEED) == CONFIG_SUCCESS) {
        printf("Updated 'ENGINE_SPEED' to %d\n", ENG_SPEED);
    }
}

// Adjust low-pass filter cutoff
void low_pass_set() {
    num_set_menu_int("LOW PASS", 1000, SAMP_RATE / 2, 1000, &LPF_CUTOFF, 0, 0, 128, 32);
    setting_change = true;
    if (set_config_value("LPF_CUTOFF", CONFIG_INT, &LPF_CUTOFF) == CONFIG_SUCCESS) {
        printf("Updated 'LPF_CUTOFF' to %d\n", LPF_CUTOFF);
    }
}

// Adjust high-pass filter cutoff
void high_pass_set() {
    num_set_menu_int("HIGH PASS", 0, 1000, 10, &HPF_CUTOFF, 0, 0, 128, 32);
    setting_change = true;
    if (set_config_value("HPF_CUTOFF", CONFIG_INT, &HPF_CUTOFF) == CONFIG_SUCCESS) {
        printf("Updated 'HPF_CUTOFF' to %d\n", HPF_CUTOFF);
    }
}

// Adjust base frequency fine-tune
void finetune_set() {
    num_set_menu_int("FINETUNE", 400, 500, 1, &BASE_FREQ_HZ, 0, 0, 128, 32);
    if (set_config_value("BASE_FREQ_HZ", CONFIG_INT, &BASE_FREQ_HZ) == CONFIG_SUCCESS) {
        printf("Updated 'BASE_FREQ_HZ' to %d\n", BASE_FREQ_HZ);
    }
}

// Adjust audio oversampling rate
void over_sample_set() {
    num_set_menu_int("OVER SAMPLE", 1, 8, 1, &OVER_SAMPLE, 0, 0, 100, 32);
    if (set_config_value("OVER_SAMPLE", CONFIG_INT, &OVER_SAMPLE) == CONFIG_SUCCESS) {
        printf("Updated 'OVER_SAMPLE' to %d\n", OVER_SAMPLE);
    }
}

// Adjust MIDI output enable/disable
void midi_out_set() {
    static const char *midiOpt[2] = {"OFF", "ON"};
    int choice = menu("MIDI OUT", midiOpt, 2, nullptr, 60, 29, 0, 0, _midi_output);
    if (choice != -1) {
        _midi_output = choice;
    }
}

// Reset (erase) configuration option
void erase_config_set() {
    static const char *confirm_str[2] = {"NO", "YES"};
    int ret = menu("ARE YOU SURE!?", confirm_str, 2, nullptr, 68, 29);
    if (ret == 1) {
        display.setFont(&rismol57);
        drawPopupBox("ERASE CONFIG...");
        display.display();
        remove(config_path);
        esp_restart();
    }
}

void reboot_page() {
    static const char *menu_str[3] = {"NORMAL", "StoreProhibit", "LoadProhibit"};
    int ret = menu("REBOOT...", menu_str, 3, NULL, 60, 37);
    if (ret == 0) {
        display.setFont(&rismol57);
        drawPopupBox("REBOOTING...");
        display.display();
        esp_restart();
    } else if (ret == 1) {
        int *test = 0;
        *test = rand();
    } else if (ret == 2) {
        int *test = 0;
        printf("%d", *test);
    }
}

// Settings menu page that consolidates various engine settings
void settings_page() {
    static const char *settings_items[9] = {
        "SAMPLE RATE", "ENGINE SPEED", "LOW PASS", "HIGH PASS", 
        "FINETUNE", "OVER SAMPLE", "VOLUME", "MIDI OUT", "RESET CONFIG"
    };
    // Corresponding action functions for each setting
    void (*settings_actions[9])(void) = {
        samp_rate_set, eng_speed_set, low_pass_set, high_pass_set,
        finetune_set, over_sample_set, vol_set_page, midi_out_set, erase_config_set
    };
    // Display a full-screen settings menu
    fullScreenMenu("SETTINGS", settings_items, 9, settings_actions, 0);
    // After returning from menu, save config if needed and possibly reboot
    drawPopupBox("SAVE CONFIG...");
    display.display();
    write_config(config_path);
    if (setting_change) {
        drawPopupBox("Requires reboot to apply changes.");
        display.display();
        vTaskDelay(1024);
        esp_restart();
    }
    display.setFont(&rismol35);
}
