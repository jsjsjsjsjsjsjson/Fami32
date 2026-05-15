#include "gui_settings.h"

extern bool _debug_print;

namespace {

constexpr int kScreenW = 128;
constexpr int kHeaderH = 9;
constexpr int kFooterY = 56;
constexpr int kRowsPerPage = 4;
constexpr int kRowH = 11;

enum class SettingKind {
    Integer,
    Toggle,
    Action,
};

struct SettingItem {
    const char *label;
    const char *config_key;
    SettingKind kind;
    int *int_value;
    bool *bool_value;
    int min_value;
    int max_value;
    int step;
    bool needs_restart;
};

int select_option(const char *title, const char *options[], int option_count, int selected);

int clamp_int(int value, int min_value, int max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

int value_of(const SettingItem &item) {
    if (item.kind == SettingKind::Toggle && item.bool_value != nullptr) {
        return *item.bool_value ? 1 : 0;
    }

    if (item.int_value != nullptr) {
        return *item.int_value;
    }

    return 0;
}

void format_value(const SettingItem &item, char *buffer, size_t buffer_size) {
    if (item.kind == SettingKind::Toggle) {
        snprintf(buffer, buffer_size, "%s", value_of(item) ? "ON" : "OFF");
    } else if (item.kind == SettingKind::Action) {
        snprintf(buffer, buffer_size, "...");
    } else {
        snprintf(buffer, buffer_size, "%d", value_of(item));
    }
}

void draw_top_bar(const char *title) {
    display.fillRect(0, 0, kScreenW, kHeaderH, 1);
    display.setFont(&rismol57);
    display.setTextColor(0);
    display.setCursor(1, 1);
    display.print(title);
    display.setTextColor(1);
}

void draw_bottom_hint(const char *hint) {
    display.drawFastHLine(0, kFooterY, kScreenW, 1);
    display.setFont(&rismol35);
    display.setTextColor(1);
    display.setCursor(2, 58);
    display.print(hint);
}

void draw_progress_bar(int x, int y, int width, int value, int min_value, int max_value) {
    display.drawRect(x, y, width, 5, 1);

    int span = max_value - min_value;
    if (span <= 0) return;

    int fill = ((value - min_value) * (width - 2)) / span;
    fill = clamp_int(fill, 0, width - 2);
    if (fill > 0) {
        display.fillRect(x + 1, y + 1, fill, 3, 1);
    }
}

void draw_right_aligned_text(const char *text, int right_x, int y) {
    int16_t x1, y1;
    uint16_t text_width, text_height;
    display.getTextBounds(text, 0, y, &x1, &y1, &text_width, &text_height);
    display.setCursor(right_x - text_width, y);
    display.print(text);
}

void draw_settings_list(const SettingItem items[], int item_count, int selected, int page_start) {
    display.clearDisplay();
    draw_top_bar("SETTINGS");

    display.setFont(&font4x6);
    display.setTextColor(0);
    display.setCursor(98, 2);
    display.printf("%02d/%02d", selected + 1, item_count);
    display.setTextColor(1);

    for (int row = 0; row < kRowsPerPage; ++row) {
        int index = page_start + row;
        if (index >= item_count) break;

        int y = 12 + (row * kRowH);
        bool is_selected = index == selected;
        char value[16];
        format_value(items[index], value, sizeof(value));

        if (is_selected) {
            display.fillRect(0, y - 1, kScreenW, 10, 1);
            display.setTextColor(0);
        } else {
            display.setTextColor(1);
        }

        display.setFont(&rismol35);
        display.setCursor(3, y + 1);
        display.print(items[index].label);

        display.setFont(&font4x6);
        display.setCursor(91, y + 1);
        display.print(value);
    }

    draw_bottom_hint("L/R ADJ  OK EDIT  BACK");
    display.display();
}

void apply_integer_delta(const SettingItem &item, int delta) {
    if (item.int_value == nullptr) return;
    *item.int_value = clamp_int(*item.int_value + delta, item.min_value, item.max_value);
}

void update_config_for_item(const SettingItem &item) {
    if (item.config_key == nullptr) return;

    if (item.kind == SettingKind::Integer && item.int_value != nullptr &&
        set_config_value(item.config_key, CONFIG_INT, item.int_value) == CONFIG_SUCCESS) {
        printf("Updated '%s' to %d\n", item.config_key, *item.int_value);
    } else if (item.kind == SettingKind::Toggle && item.bool_value != nullptr) {
        int value = *item.bool_value ? 1 : 0;
        if (set_config_value(item.config_key, CONFIG_INT, &value) == CONFIG_SUCCESS) {
            printf("Updated '%s' to %d\n", item.config_key, value);
        }
    }
}

bool edit_integer_item(const SettingItem &item) {
    if (item.int_value == nullptr) return false;

    int original = *item.int_value;
    int value = original;

    for (;;) {
        display.clearDisplay();
        draw_top_bar(item.label);

        display.setFont(&rismol57);
        display.setTextColor(1);
        display.setCursor(3, 18);
        display.print(value);

        display.setFont(&font4x6);
        char max_value[16];
        snprintf(max_value, sizeof(max_value), "%d", item.max_value);
        display.setCursor(3, 40);
        display.printf("%d", item.min_value);
        draw_right_aligned_text(max_value, 125, 40);
        draw_progress_bar(3, 47, 122, value, item.min_value, item.max_value);

        draw_bottom_hint("L/R +/-  U/D FAST  OK");
        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_L || e.bit.KEY == KEY_S) {
                    value = clamp_int(value - item.step, item.min_value, item.max_value);
                } else if (e.bit.KEY == KEY_R || e.bit.KEY == KEY_P) {
                    value = clamp_int(value + item.step, item.min_value, item.max_value);
                } else if (e.bit.KEY == KEY_UP) {
                    value = clamp_int(value + (item.step * 2), item.min_value, item.max_value);
                } else if (e.bit.KEY == KEY_DOWN) {
                    value = clamp_int(value - (item.step * 2), item.min_value, item.max_value);
                } else if (e.bit.KEY == KEY_BACK) {
                    *item.int_value = original;
                    return false;
                } else if (e.bit.KEY == KEY_OK) {
                    *item.int_value = value;
                    if (*item.int_value != original) {
                        update_config_for_item(item);
                        if (item.needs_restart) {
                            setting_change = true;
                        }
                        return true;
                    }
                    return false;
                }
            }
        }

        vTaskDelay(4);
    }
}

bool toggle_item(const SettingItem &item) {
    if (item.bool_value == nullptr) return false;
    *item.bool_value = !*item.bool_value;
    update_config_for_item(item);
    return true;
}

bool edit_toggle_item(const SettingItem &item) {
    if (item.bool_value == nullptr) return false;

    static const char *options[2] = {"OFF", "ON"};
    bool original = *item.bool_value;
    int choice = select_option(item.label, options, 2, original ? 1 : 0);
    if (choice == -1) {
        return false;
    }

    *item.bool_value = choice == 1;
    if (*item.bool_value != original) {
        update_config_for_item(item);
    }
    return *item.bool_value != original;
}

int select_option(const char *title, const char *options[], int option_count, int selected) {
    selected = clamp_int(selected, 0, option_count - 1);

    for (;;) {
        display.clearDisplay();
        draw_top_bar(title);

        int item_h = option_count <= 2 ? 14 : 12;
        int start_y = option_count <= 2 ? 21 : 17;

        display.setFont(&rismol57);
        for (int i = 0; i < option_count; ++i) {
            int y = start_y + (i * item_h);
            if (i == selected) {
                display.fillRect(12, y - 1, 104, 11, 1);
                display.setTextColor(0);
            } else {
                display.setTextColor(1);
            }
            display.setCursor(18, y + 1);
            display.print(options[i]);
        }

        draw_bottom_hint("UP/DN SEL  OK  BACK");
        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_UP) {
                    selected = selected == 0 ? option_count - 1 : selected - 1;
                } else if (e.bit.KEY == KEY_DOWN) {
                    selected = selected == option_count - 1 ? 0 : selected + 1;
                } else if (e.bit.KEY == KEY_BACK) {
                    return -1;
                } else if (e.bit.KEY == KEY_OK) {
                    return selected;
                }
            }
        }

        vTaskDelay(4);
    }
}

SettingItem sample_rate_item() {
    return {"SAMPLE RATE", "SAMPLE_RATE", SettingKind::Integer, &SAMP_RATE, nullptr,
            10800, 192000, 6000, true};
}

SettingItem engine_speed_item() {
    return {"ENGINE SPEED", "ENGINE_SPEED", SettingKind::Integer, &ENG_SPEED, nullptr,
            1, 240, 1, true};
}

SettingItem low_pass_item() {
    return {"LOW PASS", "LPF_CUTOFF", SettingKind::Integer, &LPF_CUTOFF, nullptr,
            1000, SAMP_RATE / 2, 1000, true};
}

SettingItem high_pass_item() {
    return {"HIGH PASS", "HPF_CUTOFF", SettingKind::Integer, &HPF_CUTOFF, nullptr,
            0, 1000, 10, true};
}

SettingItem finetune_item() {
    return {"FINETUNE", "BASE_FREQ_HZ", SettingKind::Integer, &BASE_FREQ_HZ, nullptr,
            400, 500, 1, false};
}

SettingItem over_sample_item() {
    return {"OVER SAMPLE", "OVER_SAMPLE", SettingKind::Integer, &OVER_SAMPLE, nullptr,
            1, 8, 1, false};
}

SettingItem volume_item() {
    return {"VOLUME", "VOLUME", SettingKind::Integer, &g_vol, nullptr,
            0, 64, 1, false};
}

SettingItem midi_out_item() {
    return {"MIDI OUT", "MIDI_OUT", SettingKind::Toggle, nullptr, &_midi_output,
            0, 1, 1, false};
}

SettingItem debug_print_item() {
    return {"DEBUG PRINT", "DEBUG_PRINT", SettingKind::Toggle, nullptr, &_debug_print,
            0, 1, 1, false};
}

SettingItem reset_config_item() {
    return {"RESET CONFIG", nullptr, SettingKind::Action, nullptr, nullptr,
            0, 0, 0, false};
}

bool edit_setting_item(const SettingItem &item) {
    if (item.kind == SettingKind::Integer) {
        return edit_integer_item(item);
    }

    if (item.kind == SettingKind::Toggle) {
        return edit_toggle_item(item);
    }

    erase_config_set();
    return false;
}

void save_config_with_popup() {
    drawPopupBox("SAVE CONFIG...");
    display.display();
    write_config(config_path);
}

} // namespace

void vol_set_page() {
    SettingItem item = volume_item();
    if (edit_integer_item(item)) {
        save_config_with_popup();
    }
}

void samp_rate_set() {
    edit_integer_item(sample_rate_item());
}

void eng_speed_set() {
    edit_integer_item(engine_speed_item());
}

void low_pass_set() {
    edit_integer_item(low_pass_item());
}

void high_pass_set() {
    edit_integer_item(high_pass_item());
}

void finetune_set() {
    edit_integer_item(finetune_item());
}

void over_sample_set() {
    edit_integer_item(over_sample_item());
}

void midi_out_set() {
    SettingItem item = midi_out_item();
    edit_toggle_item(item);
}

void debug_print_set() {
    SettingItem item = debug_print_item();
    edit_toggle_item(item);
}

void erase_config_set() {
    static const char *confirm_str[2] = {"NO", "YES"};
    int ret = select_option("ERASE CONFIG?", confirm_str, 2, 0);
    if (ret == 1) {
        display.setFont(&rismol57);
        drawPopupBox("ERASE CONFIG...");
        display.display();
        remove(config_path);
        esp_restart();
    }
}

void reboot_page() {
    static const char *reboot_str[3] = {"NORMAL", "StoreProhibit", "LoadProhibit"};
    int ret = select_option("REBOOT...", reboot_str, 3, 0);
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

void settings_page() {
    int selected = 0;
    int page_start = 0;
    bool config_dirty = false;

    for (;;) {
        SettingItem settings_items[10] = {
            sample_rate_item(),
            engine_speed_item(),
            low_pass_item(),
            high_pass_item(),
            finetune_item(),
            over_sample_item(),
            volume_item(),
            midi_out_item(),
            debug_print_item(),
            reset_config_item(),
        };

        constexpr int item_count = sizeof(settings_items) / sizeof(settings_items[0]);
        if (selected < page_start) {
            page_start = selected;
        } else if (selected >= page_start + kRowsPerPage) {
            page_start = selected - kRowsPerPage + 1;
        }

        draw_settings_list(settings_items, item_count, selected, page_start);

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_UP) {
                    selected = selected == 0 ? item_count - 1 : selected - 1;
                } else if (e.bit.KEY == KEY_DOWN) {
                    selected = selected == item_count - 1 ? 0 : selected + 1;
                } else if (e.bit.KEY == KEY_BACK) {
                    break;
                } else if (e.bit.KEY == KEY_OK) {
                    config_dirty |= edit_setting_item(settings_items[selected]);
                } else if (e.bit.KEY == KEY_L || e.bit.KEY == KEY_S ||
                           e.bit.KEY == KEY_R || e.bit.KEY == KEY_P) {
                    SettingItem &item = settings_items[selected];
                    if (item.kind == SettingKind::Integer) {
                        int before = value_of(item);
                        int direction = (e.bit.KEY == KEY_L || e.bit.KEY == KEY_S) ? -1 : 1;
                        apply_integer_delta(item, item.step * direction);
                        if (value_of(item) != before) {
                            update_config_for_item(item);
                            config_dirty = true;
                            if (item.needs_restart) {
                                setting_change = true;
                            }
                        }
                    } else if (item.kind == SettingKind::Toggle) {
                        config_dirty |= toggle_item(item);
                    }
                }
            }
        }

        vTaskDelay(4);
    }

    if (config_dirty) {
        save_config_with_popup();
    }

    if (setting_change) {
        drawPopupBox("REBOOT REQUIRED");
        display.display();
        vTaskDelay(1024);
        esp_restart();
    }

    display.setFont(&rismol35);
}
