#include "gui_instrument.h"
#include "gui_menu.h"      // for menu(), drawPinstripe
#include "gui_input.h"     // for set_channel_sel_pos()
#include "gui_tracker.h"   // for menu_navi()
#include "gui_channel.h"   // for channel_menu (if needed)
#include <time.h>

// A list of random names for quick naming of instruments
const char *random_name[177] = {
    "Brave", "Zephyr", "Glade", "Frost", "Echo", "Briar", "Thorn", "Alder",
    "Misty", "Flare", "Nimbus", "Gale", "Sylva", "Cliff", "Raven", "Ashen",
    "Vail", "Drift", "Skye", "Dawn", "Coral", "Storm", "Ember", "Warden",
    "Pine", "Flint", "Violet", "Cinder", "Onyx", "Blaze", "Thorny", "Horizon",
    "Quill", "Violet", "Noble", "Fjord", "Cobalt", "Ashby", "Peregrine", "Opal",
    "Rustle", "Falcon", "Vermil", "Ivy", "Haven", "Slate", "Talon", "Tempest",
    "Lynx", "Ridge", "Shade", "Astra", "Vesper", "Nash", "Sable", "Drake",
    "Rook", "Moss", "Emberly", "Hail", "Cedar", "Mist", "Birch", "Crimson",
    "Starla", "Dusk", "Wilder", "Ashwood", "Jade", "Ravenna", "Breeze", "Nira",
    "Omen", "Fable", "Vale", "Thistle", "Severn", "Glimmer", "Sablewood", "Wisp",
    "Bracken", "Emberstone", "Winterfell", "Dawnstone", "Blightmoor", "Ironbark", 
    "Frostbloom", "Shadowmere", "Nightfall", "Mooncrest", "Windhaven", "Stormwatch", 
    "Falconer", "Mistral", "Silverthorn", "Dragonfire", "Vortex", "Ravenclaw", 
    "Draegar", "Sablewind", "Grimwood", "Valkyrie", "Darkholme", "Stoneforge",
    "Seabrook", "Celestia", "Ashfall", "Hallowbrook", "Thundershade", "Winterhold",
    "Obsidian", "Stormbringer", "Nightshade", "Frostveil", "Ironfang", "Moonshadow",
    "Brightstone", "Griffon", "Halloway", "Redthorn", "Brimstone", "Wolfbane",
    "Oakencrest", "Ironveil", "Swiftwind", "Bluefire", "Crystal", "Duskwalker", 
    "Mistveil", "Goldleaf", "Ravenwood", "Briarwood", "Falconcrest", "Stormbringer", 
    "Thornfield", "Nighthawk", "Vandale", "Silverclaw", "Eldritch", "Tornel", 
    "Glimmerstone", "Wolfsbane", "Ebonfall", "Blightforge", "Nightwing", "Duskfall",
    "Ravencroft", "Thorncrest", "Falcontide", "Vampira", "Firestrike", "Glacier",
    "Snowcrest", "Havenglen", "Stoneheart", "Mistrider", "Darkwing", "Ashburn", 
    "Silverfang", "Draugr", "Emberfall", "Stormrage", "Moonfire", "Obsidian", 
    "Ironclad", "Mistvale", "Thunderpeak", "Dreadmoor", "Skyfire", "Blazewood", 
    "Ravenshade", "Brumel", "Shadewood", "Frostmoon", "Valkyr", "Wyrmstone", "Dragonhold"
};

static const char *instrument_type_name(uint8_t type) {
    switch (type) {
        case INST_VRC6: return "TYPE:VRC6";
        case INST_VRC7: return "TYPE:VRC7";
        case INST_FDS: return "TYPE:FDS";
        case INST_N163: return "TYPE:N163";
        default: return "TYPE:2A03";
    }
}

static const char *arp_setting_name(uint32_t setting) {
    switch (setting) {
        case ARP_SETTING_FIXED: return "FIX";
        case ARP_SETTING_RELATIVE: return "REL";
        default: return "ABS";
    }
}

static void arp_setting_menu(uint32_t *setting) {
    if (setting == NULL) return;
    static const char *mode_str[3] = {"ABSOLUTE", "FIXED", "RELATIVE"};
    int init = (*setting == ARP_SETTING_FIXED) ? 1 : ((*setting == ARP_SETTING_RELATIVE) ? 2 : 0);
    int ret = menu("ARP MODE", mode_str, 3, NULL, 68, 37, 0, 0, init);
    if (ret != -1) {
        *setting = ret;
    }
}

static int clamp_i(int value, int min_value, int max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static void draw_editor_header(const char *chip, const char *name) {
    display.fillRect(0, 0, 128, 9, 1);
    display.setTextColor(0);
    display.setCursor(1, 1);
    display.setFont(&rismol57);
    display.printf("%s %.12s", chip, name);
    display.setFont(&rismol35);
    display.setTextColor(1);
}

static void draw_tab_strip(const char *tabs[], int count, int active) {
    if (count <= 0) return;
    int tab_w = 128 / count;
    display.drawFastHLine(0, 9, 128, 1);
    for (int i = 0; i < count; ++i) {
        int x = i * tab_w;
        int w = (i == count - 1) ? (128 - x) : tab_w;
        if (i == active) {
            display.fillRect(x, 10, w, 8, 1);
            display.setTextColor(0);
        } else {
            display.setTextColor(1);
        }
        display.setCursor(x + 2, 12);
        display.print(tabs[i]);
    }
    display.setTextColor(1);
    display.drawFastHLine(0, 18, 128, 1);
}

static void edit_int_value(const char *title, int min_value, int max_value, int *value) {
    if (value == NULL) return;
    num_set_menu_int(title, min_value, max_value, 1, value, 0, 0, 84, 34);
    *value = clamp_i(*value, min_value, max_value);
}

static void instrument_type_page() {
    const char *type_str[5] = {"2A03", "VRC6", "VRC7", "FDS", "N163"};
    int init = 0;
    if (ftm.get_inst(inst_sel_pos)->type == INST_VRC6) init = 1;
    else if (ftm.get_inst(inst_sel_pos)->type == INST_VRC7) init = 2;
    else if (ftm.get_inst(inst_sel_pos)->type == INST_FDS) init = 3;
    else if (ftm.get_inst(inst_sel_pos)->type == INST_N163) init = 4;

    int ret = menu("INSTRUMENT TYPE", type_str, 5, NULL, 64, 53, 0, 0, init);
    if (ret == -1) return;

    if (ret == 1) {
        ftm.set_inst_type(inst_sel_pos, INST_VRC6);
        if (!ftm.vrc6_enabled()) {
            ftm.set_vrc6_enabled(true);
            player.reload();
        }
    } else if (ret == 2) {
        ftm.set_inst_type(inst_sel_pos, INST_VRC7);
        if (!ftm.vrc7_enabled()) {
            ftm.set_vrc7_enabled(true);
            player.reload();
        }
    } else if (ret == 3) {
        ftm.set_inst_type(inst_sel_pos, INST_FDS);
        if (!ftm.fds_enabled()) {
            ftm.set_fds_enabled(true);
            player.reload();
        }
    } else if (ret == 4) {
        ftm.set_inst_type(inst_sel_pos, INST_N163);
        if (!ftm.n163_enabled()) {
            ftm.set_n163_enabled(true);
            player.reload();
        }
    } else {
        ftm.set_inst_type(inst_sel_pos, INST_2A03);
    }
    player.channel[channel_sel_pos].set_inst(inst_sel_pos);
}

void instrument_option_page() {
    const char *menu_str[5] = {"NEW", "RENAME", "RAND NAME", instrument_type_name(ftm.get_inst(inst_sel_pos)->type), "REMOVE"};
    srand(time(NULL));
    int ret = menu("INSTRUMENT", menu_str, 5, NULL, 68, 45, 0, 0, 0);
    switch (ret)
    {
    case 0:
        ftm.create_new_inst();
        break;

    case 1:
        displayKeyboard("INSTRUMENT NAME", ftm.get_inst(inst_sel_pos)->name, 63);
        ftm.get_inst(inst_sel_pos)->name_len = strlen(ftm.get_inst(inst_sel_pos)->name);
        break;

    case 2:
        strcpy(ftm.get_inst(inst_sel_pos)->name, random_name[rand() % 177]);
        ftm.get_inst(inst_sel_pos)->name_len = strlen(ftm.get_inst(inst_sel_pos)->name);
        break;

    case 3:
        instrument_type_page();
        break;

    case 4:
        ftm.remove_inst(inst_sel_pos);
        break;
    
    default:
        break;
    }
    if (inst_sel_pos >= ftm.inst_block.inst_num) {
        inst_sel_pos = ftm.inst_block.inst_num - 1;
    }
}

void instrument_menu() {
    if (inst_sel_pos >= ftm.inst_block.inst_num) {
        inst_sel_pos = ftm.inst_block.inst_num - 1;
    }
    static int pageStart = 0;
    const int itemsPerPage = 6;

    for (;;) {
        display.clearDisplay();

        display.fillRect(0, 0, 128, 9, 1);
        display.setTextColor(0);
        display.setFont(&rismol57);
        display.setCursor(1, 1);
        display.print("INSTRUMENT ");
        display.setFont(&font4x6);
        display.printf("(%02X/%02lX)", inst_sel_pos, ftm.inst_block.inst_num - 1);

        display.drawFastHLine(0, 9, player.channel[channel_sel_pos].get_rel_vol() / 2, 1);

        display.setFont(&rismol35);
        display.setTextColor(1);

        if (inst_sel_pos < pageStart) {
            pageStart = inst_sel_pos;
        } else if (inst_sel_pos >= pageStart + itemsPerPage) {
            pageStart = inst_sel_pos - itemsPerPage + 1;
        }
        int pageEnd = (pageStart + itemsPerPage >= ftm.inst_block.inst_num) ? ftm.inst_block.inst_num : pageStart + itemsPerPage;

        for (uint8_t i = pageStart; i < pageEnd; i++) {
            int displayIndex = i - pageStart;
            int itemYPos = 9 + (displayIndex * 8);

            if (i == inst_sel_pos) {
                display.fillRect(0, itemYPos + 1, 128, 7, 1);
                display.setTextColor(0);
                display.setCursor(2, itemYPos + 2);
            } else {
                display.setTextColor(1);
                display.setCursor(2, itemYPos + 2);
            }

            if (ftm.get_inst(i)->name_len > 18) {
                display.printf("%02lX>%.15s...", ftm.get_inst(i)->index, ftm.get_inst(i)->name);
            } else {
                display.printf("%02lX>%s", ftm.get_inst(i)->index, ftm.get_inst(i)->name);
            }

            if (ftm.get_inst(i)->type == INST_VRC7) {
                display.setCursor(88, itemYPos + 2);
                display.printf("V7 P%02lX", ftm.get_inst(i)->vrc7_patch & 0x0F);
            } else if (ftm.get_inst(i)->type == INST_FDS) {
                display.setCursor(88, itemYPos + 2);
                display.printf("FDS W%02d", ftm.get_inst(i)->fds_wave[0] & 0x3F);
            } else if (ftm.get_inst(i)->type == INST_N163) {
                display.setCursor(88, itemYPos + 2);
                display.printf("N W%02ld", ftm.get_inst(i)->n163_wave_count);
            } else {
                for (int f = 0; f < 5; f++) {
                    if (i == inst_sel_pos)
                        display.drawBitmap(88 + (f * 8), itemYPos + 1, fami32_icon[f], 7, 7, !ftm.get_inst(i)->seq_index[f].enable);
                    else
                        display.drawBitmap(88 + (f * 8), itemYPos + 1, fami32_icon[f], 7, 7, ftm.get_inst(i)->seq_index[f].enable);
                }
            }
        }
        display.setTextColor(1);

        display.drawFastVLine(86, 10, 47, 1);
        display.drawFastHLine(0, 57, 128, 1);

        display.drawFastVLine(63, 58, 6, 1);
        display.setCursor(0, 59);
        display.printf("CH%d: %.10s", channel_sel_pos, player.channel[channel_sel_pos].get_inst()->name);

        if (channel_sel_pos == 3)
            display.drawFastVLine(63 + (player.channel[3].get_noise_rate() * 4), 58, 6, 1);
        else if (channel_sel_pos == 4) 
            display.drawFastVLine(63 + (player.channel[4].get_samp_len() ? player.channel[4].get_samp_pos() * (64.0f / (float)player.channel[4].get_samp_len()) : 0), 58, 6, 1);
        else if (ftm.is_vrc7_channel(channel_sel_pos))
            display.drawFastVLine(63 + roundf(period2note(player.channel[channel_sel_pos].get_period_rel()) * 0.5f), 58, 6, 1);
        else
            display.drawFastVLine(63 + roundf(period2note(player.channel[channel_sel_pos].get_period_rel()) * 0.5f), 58, 6, 1);

        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OK) {
                    sequence_editor(ftm.get_inst(inst_sel_pos));
                } else if (e.bit.KEY == KEY_NAVI) {
                    menu_navi();
                    break;
                } else if (e.bit.KEY == KEY_MENU) {
                    instrument_option_page();
                } else if (e.bit.KEY == KEY_BACK) {
                    // BACK
                } else if (e.bit.KEY == KEY_UP) {
                    inst_sel_pos--;
                    if (inst_sel_pos < 0) {
                        inst_sel_pos = ftm.inst_block.inst_num - 1;
                        pageStart = (ftm.inst_block.inst_num / itemsPerPage) * itemsPerPage;
                    }
                } else if (e.bit.KEY == KEY_DOWN) {
                    inst_sel_pos++;
                    if (inst_sel_pos >= ftm.inst_block.inst_num) {
                        inst_sel_pos = 0;
                        pageStart = 0;
                    }
                } else if (e.bit.KEY == KEY_L) {
                    set_channel_sel_pos(channel_sel_pos - 1);
                } else if (e.bit.KEY == KEY_R) {
                    set_channel_sel_pos(channel_sel_pos + 1);
                } else if (e.bit.KEY == KEY_OCTU) {
                    g_octv++;
                } else if (e.bit.KEY == KEY_OCTD) {
                    g_octv--;
                }
            }
        }

        touch_input_event_t touch_event;
        if (touch_input_pop_event(&touch_event)) {
            process_note_io_event(note_io_event_from_input(touch_event));
        }

        vTaskDelay(4);
    }
}

void quick_gen_sequ(instrument_t *inst, sequences_t *sequ, uint32_t type, uint32_t index) {
    int mode = 0;
    int style = 0;
    int direction = 0;
    int len = 1;
    int count = 1;
    int last_ret = 0;

    static const char *fristMenuStr[6] = {"MODE", "STYLE", "DIRECTION", "LENGTH", "COUNT", "OK!"};
    static const char *modeMenuStr[3] = {"NORMAL", "ARPEGGIO", "RANDOM"};
    static const char *dirMenuStr[2] = {"FORWARD", "REVERSE"};

    for (;;) {
        int ret = menu("QUICK GEN.", fristMenuStr, 6, NULL, 64, 53, 0, 0, last_ret);
        if (ret == 0) {
            int ret = menu("MODE", modeMenuStr, 3, NULL, 52, 37, 0, 0, mode);
            if (ret != -1) {
                if (ret != mode) style = 0;
                mode = ret;
            }
        } else if (ret == 1) {
            if (mode != 2) {
                int ret = menu("STYLE", mode ? chordNames : math_env_name, mode ? CHORD_ENV_NUM : MATH_ENV_NUM, NULL, 64, 53, 0, 0, style);
                if (ret != -1) {
                    style = ret;
                    if (mode == 1) len = count * chordLengths[style];
                }
            } else {
                num_set_menu_int("RAND MAX", 1, 255, 1, &style, 0, 0, 62, 32);
                if (style < 1) style = 1;
            }
        } else if (ret == 2) {
            if (mode != 2) {
                int ret = menu("DIRECTION", dirMenuStr, 2, NULL, 60, 29, 0, 0, direction);
                if (ret != -1) direction = ret;
            }
        } else if (ret == 3) {
            num_set_menu_int("LENGTH", 1, 255, 1, &len, 0, 0, 62, 32);
        } else if (ret == 4) {
            num_set_menu_int("COUNT", 1, 255, 1, &count, 0, 0, 62, 32);
        } else if (ret == 5) {
            break;
        } else {
            return;
        }
        last_ret = ret;
    }

    drawPopupBox("PLEASE WAIT...");
    ftm.resize_inst_sequ_len(inst, type, index, len);
    if (mode == 0) {
        for (int i = 0; i < len; i++) {
            if (direction) {
                sequ->data[len - 1 - i] = roundf(mathEnv[style](i, count, len) * 15.01f);
            } else {
                sequ->data[i] = roundf(mathEnv[style](i, count, len) * 15.01f);
            }
            if (sequ->data[i] > 15) sequ->data[i] = 15;
        }
    } else if (mode == 1) {
        for (int i = 0; i < len; i++) {
            if (direction) {
                sequ->data[len - 1 - i] = chords[style][(i / count) % chordLengths[style]];
            } else {
                sequ->data[i] = chords[style][(i / count) % chordLengths[style]];
            }
        }
    } else if (mode == 2) {
        int c = 0;
        int var = rand() % style;
        for (int i = 0; i < len; i++) {
            sequ->data[i] = var;
            c++;
            if (c >= count) {
                var = rand() % style;
                c = 0;
            }
        }
    }
}

void vrc7_instrument_editor(instrument_t *inst) {
    if (inst == NULL) return;
    int section = 0;
    int field = 0;
    static const char *tabs[3] = {"PATCH", "MOD", "CAR"};
    static const char *menu_str[4] = {"PATCH", "MODULATOR", "CARRIER", "TYPE"};
    static const uint8_t mod_regs[4] = {0, 2, 4, 6};
    static const uint8_t car_regs[4] = {1, 3, 5, 7};
    static const char *reg_labels[8] = {
        "R0 MUL", "R1 MUL", "R2 LVL", "R3 KSL",
        "R4 AD",  "R5 AD",  "R6 SR",  "R7 SR"
    };

    for (;;) {
        display.clearDisplay();
        draw_editor_header("VRC7", inst->name);
        draw_tab_strip(tabs, 3, section);

        if (section == 0) {
            display.fillRect(0, 22, 70, 10, 1);
            display.setTextColor(0);
            display.setCursor(3, 24);
            display.printf("PATCH:%02lX", (unsigned long)(inst->vrc7_patch & 0x0F));
            display.setTextColor(1);
            display.setCursor(76, 23);
            display.print((inst->vrc7_patch & 0x0F) == 0 ? "CUSTOM" : "PRESET");

            display.setCursor(2, 38);
            display.print("CUSTOM REGISTERS");
            for (uint8_t r = 0; r < 8; ++r) {
                int x = 2 + (r * 15);
                display.setCursor(x, 48);
                display.printf("%02X", inst->vrc7_regs[r]);
            }
        } else {
            const uint8_t *regs = (section == 1) ? mod_regs : car_regs;
            for (uint8_t i = 0; i < 4; ++i) {
                uint8_t r = regs[i];
                int y = 22 + (i * 8);
                if (field == i) {
                    display.fillRect(0, y - 1, 128, 8, 1);
                    display.setTextColor(0);
                } else {
                    display.setTextColor(1);
                }
                display.setCursor(2, y);
                display.printf("%s:%02X  HI:%X LO:%X", reg_labels[r], inst->vrc7_regs[r],
                               (inst->vrc7_regs[r] >> 4) & 0x0F, inst->vrc7_regs[r] & 0x0F);
            }
            display.setTextColor(1);
        }

        display.setTextColor(1);
        display.setCursor(0, 58);
        display.print(section == 0 ? "OK EDIT MENU PAGE" : "OK EDIT L/R SEL");
        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_BACK) {
                    return;
                } else if (e.bit.KEY == KEY_OK) {
                    if (section == 0) {
                        int value = inst->vrc7_patch & 0x0F;
                        edit_int_value("VRC7 PATCH", 0, 15, &value);
                        inst->vrc7_patch = value & 0x0F;
                    } else {
                        uint8_t reg = (section == 1) ? mod_regs[field] : car_regs[field];
                        int value = inst->vrc7_regs[reg];
                        char title[24];
                        snprintf(title, sizeof(title), "VRC7 R%d", reg);
                        edit_int_value(title, 0, 255, &value);
                        inst->vrc7_regs[reg] = value & 0xFF;
                    }
                    player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                } else if (e.bit.KEY == KEY_MENU) {
                    int ret = menu("VRC7 EDIT", menu_str, 4, NULL, 72, 45, 0, 0, section);
                    if (ret >= 0 && ret < 3) {
                        section = ret;
                        field = 0;
                    } else if (ret == 3) {
                        instrument_type_page();
                        if (inst->type != INST_VRC7) return;
                    }
                } else if (e.bit.KEY == KEY_NAVI) {
                    menu_navi();
                    return;
                } else if (e.bit.KEY == KEY_L) {
                    if (section > 0) {
                        field--;
                        if (field < 0) field = 3;
                    } else {
                        section = 2;
                        field = 0;
                    }
                } else if (e.bit.KEY == KEY_R) {
                    if (section > 0) {
                        field++;
                        if (field > 3) field = 0;
                    } else {
                        section = 1;
                        field = 0;
                    }
                } else if (e.bit.KEY == KEY_UP) {
                    if (section > 0) {
                        uint8_t reg = (section == 1) ? mod_regs[field] : car_regs[field];
                        inst->vrc7_regs[reg]++;
                        player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                    } else {
                        inst->vrc7_patch = (inst->vrc7_patch + 1) & 0x0F;
                        player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                    }
                } else if (e.bit.KEY == KEY_DOWN) {
                    if (section > 0) {
                        uint8_t reg = (section == 1) ? mod_regs[field] : car_regs[field];
                        inst->vrc7_regs[reg]--;
                        player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                    } else {
                        inst->vrc7_patch = (inst->vrc7_patch - 1) & 0x0F;
                        player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                    }
                } else if (e.bit.KEY == KEY_P) {
                    if (section == 0) {
                        inst->vrc7_patch = (inst->vrc7_patch + 1) & 0x0F;
                    } else {
                        uint8_t reg = (section == 1) ? mod_regs[field] : car_regs[field];
                        inst->vrc7_regs[reg]++;
                    }
                    player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                } else if (e.bit.KEY == KEY_S) {
                    if (section == 0) {
                        inst->vrc7_patch = (inst->vrc7_patch - 1) & 0x0F;
                    } else {
                        uint8_t reg = (section == 1) ? mod_regs[field] : car_regs[field];
                        inst->vrc7_regs[reg]--;
                    }
                    player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                }
            }
        }

        touch_input_event_t touch_event;
        if (touch_input_pop_event(&touch_event)) {
            process_note_io_event(note_io_event_from_input(touch_event));
        }

        vTaskDelay(4);
    }
}


static void fds_sequence_editor(instrument_t *inst) {
    if (inst == NULL) return;

    int sequ_type = 0;
    int sequ_sel_index = 0;
    int pageStart = 0;
    int pageEnd = 32;
    int draw_base = 30;
    uint8_t restore_len[FAMI32_FDS_SEQUENCE_TYPES] = {1, 1, 1};

    static const char *sequ_name[FAMI32_FDS_SEQUENCE_TYPES] = {"VOLUME", "ARPEGGIO", "PITCH"};
    static const char *menu_str[4] = {"LENGTH", "LOOP", "RELEASE", "ARP MODE"};

    for (;;) {
        fds_sequence_t &seq = inst->fds_seq[sequ_type];
        fds_sequence_t *sequ = (seq.length > 0) ? &seq : NULL;

        if (sequ != NULL) {
            if (sequ_sel_index >= sequ->length) {
                sequ_sel_index = sequ->length - 1;
            }
            if (sequ_sel_index < 0) {
                sequ_sel_index = 0;
            }
            if (sequ_sel_index < pageStart) {
                pageStart = sequ_sel_index;
                pageEnd = pageStart + 32;
            } else if (sequ_sel_index >= pageEnd) {
                pageEnd = sequ_sel_index + 1;
                pageStart = pageEnd - 32;
            }
            if (pageStart < 0) pageStart = 0;
            if (pageEnd > sequ->length) pageEnd = sequ->length;
        } else {
            sequ_sel_index = 0;
            pageStart = 0;
            pageEnd = 32;
        }

        display.clearDisplay();
        display.fillRect(0, 0, 128, 9, 1);
        display.setTextColor(0);
        display.setCursor(1, 1);
        display.setFont(&rismol57);
        display.printf("FDS %s", sequ_name[sequ_type]);
        display.setFont(&rismol35);
        display.setTextColor(1);
        display.drawFastHLine(0, 9, 128, 1);

        if (sequ != NULL) {
            int draw_w = (sequ->length <= 32) ? (128 / sequ->length) : 4;
            if (draw_w < 1) draw_w = 1;

            if (sequ->length <= 32) {
                pageStart = 0;
                pageEnd = sequ->length;
                drawChessboard(sequ->length * draw_w, 11, 128 - (sequ->length * draw_w), 40);
            }

            display.setFont(&rismol34);
            display.setCursor(112, 54);
            display.print("LOOP");
            display.setCursor(100, 59);
            display.print("RELEASE");
            display.setFont(&rismol35);

            if (sequ->loop != SEQ_FEAT_DISABLE && pageEnd > (int)sequ->loop) {
                int draw_start = (int)sequ->loop - pageStart;
                if (draw_start >= 0) invertRect(draw_start * draw_w, 54, 128 - (draw_start * draw_w), 4);
            }

            if (sequ->release != SEQ_FEAT_DISABLE && pageEnd > (int)sequ->release) {
                int draw_start = (int)sequ->release - pageStart;
                if (draw_start >= 0) invertRect(draw_start * draw_w, 59, 128 - (draw_start * draw_w), 4);
            }

            if (sequ_type == VOL_SEQU) {
                drawChessboard(0, 20, 128, 1);
                for (int i = pageStart; i < pageEnd; i++) {
                    int displayIndex = i - pageStart;
                    int value = sequ->data[i];
                    if (value < 0) value = 0;
                    if (value > 31) value = 31;
                    int h = value;
                    if (i == sequ_sel_index) {
                        display.drawFastHLine(displayIndex * draw_w, 12, draw_w, 1);
                        display.fillRect(displayIndex * draw_w, 51 - h, draw_w, h ? h : 1, 1);
                    } else {
                        display.drawRect(displayIndex * draw_w, 51 - h, draw_w, h ? h : 1, 1);
                    }
                }
                display.setCursor(0, 12);
            } else if (sequ_type == ARP_SEQU) {
                drawChessboard(0, draw_base, 128, 1);
                for (int i = pageStart; i < pageEnd; i++) {
                    int displayIndex = i - pageStart;
                    if (i == sequ_sel_index) {
                        display.drawFastHLine(displayIndex * draw_w, 12, draw_w, 1);
                        display.fillRect(displayIndex * draw_w, (draw_base - sequ->data[i]) - 1, draw_w + 1, 3, 1);
                    } else {
                        display.drawRect(displayIndex * draw_w, (draw_base - sequ->data[i]) - 1, draw_w + 1, 3, 1);
                    }
                }
                display.setCursor(0, 45);
            } else {
                drawChessboard(0, draw_base, 128, 1);
                for (int i = pageStart; i < pageEnd; i++) {
                    int displayIndex = i - pageStart;
                    if (i == sequ_sel_index) {
                        display.drawFastHLine(displayIndex * draw_w, 12, draw_w, 1);
                        fillRectWithNegativeSize(displayIndex * draw_w, draw_base, draw_w, -sequ->data[i], 1);
                    } else {
                        drawRectWithNegativeSize(displayIndex * draw_w, draw_base, draw_w, -sequ->data[i], 1);
                    }
                }
                display.setCursor(0, 45);
            }

            display.printf("(%d,%d)", sequ_sel_index, sequ->data[sequ_sel_index]);
            if (sequ_type == ARP_SEQU) {
                display.setCursor(94, 45);
                display.print(arp_setting_name(sequ->setting));
            }
        } else {
            display.setCursor(0, 14);
            display.printf("NULL %s SEQUENCE:\nPRESS OK TO CREATE IT...", sequ_name[sequ_type]);
        }

        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OK) {
                    if (sequ == NULL) {
                        int len = restore_len[sequ_type];
                        if (len < 1) len = 1;
                        if (len > FAMI32_FDS_SEQUENCE_MAX) len = FAMI32_FDS_SEQUENCE_MAX;
                        seq.length = len;
                        if (seq.loop >= seq.length) seq.loop = SEQ_FEAT_DISABLE;
                        if (seq.release >= seq.length) seq.release = SEQ_FEAT_DISABLE;
                        for (int i = 0; i < seq.length; ++i) {
                            if (sequ_type == VOL_SEQU) {
                                if (seq.data[i] < 0 || seq.data[i] > 31) seq.data[i] = 31;
                            }
                        }
                    } else {
                        int value = sequ->data[sequ_sel_index];
                        if (sequ_type == VOL_SEQU) {
                            edit_int_value("FDS VOLUME", 0, 31, &value);
                        } else if (sequ_type == ARP_SEQU && sequ->setting == ARP_SETTING_FIXED) {
                            edit_int_value("FDS ARP", 0, 95, &value);
                        } else if (sequ_type == ARP_SEQU) {
                            edit_int_value("FDS ARP", -128, 127, &value);
                        } else {
                            edit_int_value("FDS PITCH", -128, 127, &value);
                        }
                        sequ->data[sequ_sel_index] = value;
                    }
                } else if (e.bit.KEY == KEY_BACK) {
                    return;
                } else if (e.bit.KEY == KEY_MENU) {
                    int ret = menu("SELECT SEQUENCE", sequ_name, FAMI32_FDS_SEQUENCE_TYPES, NULL, 68, 37, 0, 0, sequ_type);
                    if (ret != -1) {
                        sequ_sel_index = 0;
                        pageStart = 0;
                        pageEnd = 32;
                        sequ_type = ret;
                    }
                } else if (e.bit.KEY == KEY_NAVI) {
                    int ret = menu("FDS SEQUENCE", menu_str, sequ_type == ARP_SEQU ? 4 : 3, NULL, 72, 37, 0, 0, 0);
                    fds_sequence_t &edit_seq = inst->fds_seq[sequ_type];
                    switch (ret) {
                    case 0: {
                        int len = edit_seq.length;
                        num_set_menu_int("SEQU LENGTH", 0, FAMI32_FDS_SEQUENCE_MAX, 1, &len, 0, 0, 100, 34);
                        if (len == 0 && edit_seq.length > 0) {
                            restore_len[sequ_type] = edit_seq.length;
                        }
                        if (len > edit_seq.length) {
                            for (int i = edit_seq.length; i < len; ++i) {
                                edit_seq.data[i] = (sequ_type == VOL_SEQU) ? 31 : 0;
                            }
                        }
                        edit_seq.length = len;
                        if (edit_seq.length > 0) {
                            sequ_sel_index = edit_seq.length - 1;
                            if (edit_seq.loop >= edit_seq.length) edit_seq.loop = SEQ_FEAT_DISABLE;
                            if (edit_seq.release >= edit_seq.length) edit_seq.release = SEQ_FEAT_DISABLE;
                        } else {
                            sequ_sel_index = 0;
                        }
                        break;
                    }
                    case 1: {
                        int loop_ref = edit_seq.loop == SEQ_FEAT_DISABLE ? -1 : (int)edit_seq.loop;
                        int maxp = edit_seq.length > 0 ? edit_seq.length - 1 : 0;
                        num_set_menu_int("SEQU LOOP", -1, maxp, 1, &loop_ref, 0, 0, 126, 34);
                        edit_seq.loop = loop_ref < 0 ? SEQ_FEAT_DISABLE : (uint32_t)loop_ref;
                        break;
                    }
                    case 2: {
                        int release_ref = edit_seq.release == SEQ_FEAT_DISABLE ? -1 : (int)edit_seq.release;
                        int maxp = edit_seq.length > 0 ? edit_seq.length - 1 : 0;
                        num_set_menu_int("SEQU RELEASE", -1, maxp, 1, &release_ref, 0, 0, 126, 34);
                        edit_seq.release = release_ref < 0 ? SEQ_FEAT_DISABLE : (uint32_t)release_ref;
                        break;
                    }
                    case 3:
                        if (sequ_type == ARP_SEQU) {
                            uint32_t setting = edit_seq.setting;
                            arp_setting_menu(&setting);
                            edit_seq.setting = setting;
                        }
                        break;
                    default:
                        break;
                    }
                } else if (e.bit.KEY == KEY_L) {
                    if (sequ != NULL) {
                        sequ_sel_index--;
                        if (sequ_sel_index < 0) {
                            sequ_sel_index = sequ->length - 1;
                            if (sequ->length > 32) {
                                pageStart = sequ->length - 32;
                                pageEnd = sequ->length;
                            }
                        } else if (sequ_sel_index < pageStart) {
                            pageStart = sequ_sel_index;
                            pageEnd = pageStart + 32;
                        }
                    }
                } else if (e.bit.KEY == KEY_R) {
                    if (sequ != NULL) {
                        sequ_sel_index++;
                        if (sequ_sel_index >= sequ->length) {
                            sequ_sel_index = 0;
                            pageStart = 0;
                            pageEnd = 32;
                        } else if (sequ_sel_index >= pageEnd) {
                            pageEnd = sequ_sel_index + 1;
                            pageStart = pageEnd - 32;
                        }
                    }
                } else if (e.bit.KEY == KEY_UP) {
                    draw_base++;
                } else if (e.bit.KEY == KEY_DOWN) {
                    draw_base--;
                } else if (e.bit.KEY == KEY_P) {
                    if (sequ != NULL) {
                        if (sequ_type == VOL_SEQU) {
                            if (sequ->data[sequ_sel_index] < 31) sequ->data[sequ_sel_index]++;
                        } else if (sequ_type == ARP_SEQU && sequ->setting == ARP_SETTING_FIXED) {
                            sequ->data[sequ_sel_index]++;
                            if (sequ->data[sequ_sel_index] > 95) sequ->data[sequ_sel_index] = 0;
                        } else if (sequ->data[sequ_sel_index] < 127) {
                            sequ->data[sequ_sel_index]++;
                        }
                    }
                } else if (e.bit.KEY == KEY_S) {
                    if (sequ != NULL) {
                        if (sequ_type == VOL_SEQU) {
                            if (sequ->data[sequ_sel_index] > 0) sequ->data[sequ_sel_index]--;
                        } else if (sequ_type == ARP_SEQU && sequ->setting == ARP_SETTING_FIXED) {
                            sequ->data[sequ_sel_index]--;
                            if (sequ->data[sequ_sel_index] < 0) sequ->data[sequ_sel_index] = 95;
                        } else if (sequ->data[sequ_sel_index] > -128) {
                            sequ->data[sequ_sel_index]--;
                        }
                    }
                } else if (e.bit.KEY == KEY_OCTU) {
                    g_octv++;
                } else if (e.bit.KEY == KEY_OCTD) {
                    g_octv--;
                }
            }
        }

        touch_input_event_t touch_event;
        if (touch_input_pop_event(&touch_event)) {
            process_note_io_event(note_io_event_from_input(touch_event));
        }

        vTaskDelay(4);
    }
}

static void fds_edit_param(instrument_t *inst, int param_index) {
    if (inst == NULL) return;
    if (param_index == 0) {
        int v = inst->fds_mod_speed & 0x0FFF;
        num_set_menu_int("FDS SPEED", 0, 4095, 1, &v, 0, 0, 88, 34);
        inst->fds_mod_speed = v & 0x0FFF;
    } else if (param_index == 1) {
        int v = inst->fds_mod_depth & 0x3F;
        num_set_menu_int("FDS DEPTH", 0, 63, 1, &v, 0, 0, 72, 34);
        inst->fds_mod_depth = v & 0x3F;
    } else {
        int v = inst->fds_mod_delay & 0xFF;
        num_set_menu_int("FDS DELAY", 0, 255, 1, &v, 0, 0, 72, 34);
        inst->fds_mod_delay = v & 0xFF;
    }
}

static void fds_adjust_param(instrument_t *inst, int param_index, int delta) {
    if (inst == NULL) return;
    if (param_index == 0) {
        int v = (int)(inst->fds_mod_speed & 0x0FFF) + delta;
        if (v < 0) v = 0;
        if (v > 4095) v = 4095;
        inst->fds_mod_speed = v;
    } else if (param_index == 1) {
        int v = (int)(inst->fds_mod_depth & 0x3F) + delta;
        if (v < 0) v = 0;
        if (v > 63) v = 63;
        inst->fds_mod_depth = v;
    } else {
        int v = (int)(inst->fds_mod_delay & 0xFF) + delta;
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        inst->fds_mod_delay = v;
    }
}

void fds_instrument_editor(instrument_t *inst) {
    if (inst == NULL) return;
    int section = 0;
    int wave_index = 0;
    int mod_index = 0;
    int param_index = 0;
    static const char *tabs[4] = {"WAVE", "MOD", "PARM", "SEQ"};
    static const char *menu_str[5] = {"WAVE TABLE", "MOD TABLE", "PARAMS", "SEQUENCES", "TYPE"};
    static const char *seq_name[FAMI32_FDS_SEQUENCE_TYPES] = {"VOL", "ARP", "PIT"};

    for (;;) {
        display.clearDisplay();
        draw_editor_header("FDS", inst->name);
        draw_tab_strip(tabs, 4, section);

        if (section == 0) {
            display.drawRect(0, 20, 128, 28, 1);
            for (int i = 0; i < FAMI32_FDS_WAVE_SIZE; ++i) {
                int h = ((inst->fds_wave[i] & 0x3F) * 25) / 63;
                int x = i * 2;
                if (i == wave_index) {
                    display.fillRect(x, 47 - h, 2, h ? h : 1, 1);
                    display.drawFastHLine(x, 19, 2, 1);
                } else {
                    display.drawFastVLine(x, 47 - h, h ? h : 1, 1);
                }
            }
            display.setCursor(0, 51);
            display.printf("WAVE %02d/%02d VAL:%02d", wave_index, FAMI32_FDS_WAVE_SIZE - 1,
                           inst->fds_wave[wave_index] & 0x3F);
        } else if (section == 1) {
            display.drawRect(0, 22, 128, 24, 1);
            for (int i = 0; i < FAMI32_FDS_MOD_SIZE; ++i) {
                int x = i * 4;
                int h = ((inst->fds_mod[i] & 7) * 20) / 7;
                if (i == mod_index) {
                    display.fillRect(x, 45 - h, 3, h ? h : 1, 1);
                    display.drawFastHLine(x, 20, 3, 1);
                } else {
                    display.drawRect(x, 45 - h, 3, h ? h : 1, 1);
                }
            }
            display.setCursor(0, 51);
            display.printf("MOD %02d/%02d VAL:%d", mod_index, FAMI32_FDS_MOD_SIZE - 1,
                           inst->fds_mod[mod_index] & 7);
        } else if (section == 2) {
            const char *labels[3] = {"SPEED", "DEPTH", "DELAY"};
            uint32_t values[3] = {inst->fds_mod_speed & 0x0FFF, inst->fds_mod_depth & 0x3F, inst->fds_mod_delay & 0xFF};
            for (int i = 0; i < 3; ++i) {
                int y = 23 + i * 10;
                if (i == param_index) {
                    display.fillRect(0, y - 1, 128, 9, 1);
                    display.setTextColor(0);
                } else {
                    display.setTextColor(1);
                }
                display.setCursor(2, y);
                display.printf("%s:%04lX", labels[i], (unsigned long)values[i]);
            }
            display.setTextColor(1);
        } else {
            display.setCursor(2, 22);
            display.print("FDS LOCAL SEQUENCES");
            for (int i = 0; i < FAMI32_FDS_SEQUENCE_TYPES; ++i) {
                fds_sequence_t &seq = inst->fds_seq[i];
                int y = 33 + i * 8;
                display.setCursor(4, y);
                if (seq.length > 0) {
                    display.printf("%s LEN:%03d L:%s R:%s", seq_name[i], seq.length,
                                   seq.loop == SEQ_FEAT_DISABLE ? "--" : "ON",
                                   seq.release == SEQ_FEAT_DISABLE ? "--" : "ON");
                } else {
                    display.printf("%s OFF", seq_name[i]);
                }
            }
            invertRect(88, 57, 40, 7);
        }

        display.setCursor(0, 58);
        if (section == 0 || section == 1) {
            display.print("OK EDIT L/R X +/- Y");
        } else if (section == 2) {
            display.print("OK EDIT L/R SEL");
        } else {
            display.print("OK SEQUENCES");
        }
        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_BACK) {
                    return;
                } else if (e.bit.KEY == KEY_OK) {
                    if (section == 0) {
                        int value = inst->fds_wave[wave_index] & 0x3F;
                        edit_int_value("FDS WAVE", 0, 63, &value);
                        inst->fds_wave[wave_index] = value & 0x3F;
                    } else if (section == 1) {
                        int value = inst->fds_mod[mod_index] & 7;
                        edit_int_value("FDS MOD", 0, 7, &value);
                        inst->fds_mod[mod_index] = value & 7;
                    } else if (section == 2) {
                        fds_edit_param(inst, param_index);
                    } else if (section == 3) {
                        fds_sequence_editor(inst);
                    }
                    player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                } else if (e.bit.KEY == KEY_MENU) {
                    int ret = menu("FDS EDIT", menu_str, 5, NULL, 72, 53, 0, 0, section);
                    if (ret >= 0 && ret < 4) {
                        section = ret;
                    } else if (ret == 4) {
                        instrument_type_page();
                        if (inst->type != INST_FDS) return;
                    }
                } else if (e.bit.KEY == KEY_L) {
                    if (section == 1) {
                        mod_index--;
                        if (mod_index < 0) mod_index = FAMI32_FDS_MOD_SIZE - 1;
                    } else if (section == 2) {
                        param_index--;
                        if (param_index < 0) param_index = 2;
                    } else if (section == 0) {
                        wave_index--;
                        if (wave_index < 0) wave_index = FAMI32_FDS_WAVE_SIZE - 1;
                    } else {
                        section = 2;
                    }
                } else if (e.bit.KEY == KEY_R) {
                    if (section == 1) {
                        mod_index++;
                        if (mod_index >= FAMI32_FDS_MOD_SIZE) mod_index = 0;
                    } else if (section == 2) {
                        param_index++;
                        if (param_index > 2) param_index = 0;
                    } else if (section == 0) {
                        wave_index++;
                        if (wave_index >= FAMI32_FDS_WAVE_SIZE) wave_index = 0;
                    } else {
                        section = 0;
                    }
                } else if (e.bit.KEY == KEY_UP || e.bit.KEY == KEY_P) {
                    if (section == 0) {
                        if (inst->fds_wave[wave_index] < 63) inst->fds_wave[wave_index]++;
                    } else if (section == 1) {
                        if (inst->fds_mod[mod_index] < 7) inst->fds_mod[mod_index]++;
                    } else if (section == 2) {
                        fds_adjust_param(inst, param_index, 1);
                    }
                    player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                } else if (e.bit.KEY == KEY_DOWN || e.bit.KEY == KEY_S) {
                    if (section == 0) {
                        if (inst->fds_wave[wave_index] > 0) inst->fds_wave[wave_index]--;
                    } else if (section == 1) {
                        if (inst->fds_mod[mod_index] > 0) inst->fds_mod[mod_index]--;
                    } else if (section == 2) {
                        fds_adjust_param(inst, param_index, -1);
                    }
                    player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                } else if (e.bit.KEY == KEY_NAVI) {
                    menu_navi();
                    return;
                } else if (e.bit.KEY == KEY_OCTU) {
                    section++;
                    if (section > 3) section = 0;
                } else if (e.bit.KEY == KEY_OCTD) {
                    section--;
                    if (section < 0) section = 3;
                }
            }
        }

        touch_input_event_t touch_event;
        if (touch_input_pop_event(&touch_event)) {
            process_note_io_event(note_io_event_from_input(touch_event));
        }

        vTaskDelay(4);
    }
}

static bool n163_sequence_editor_request = false;

static void n163_adjust_param(instrument_t *inst, int param_index, int delta) {
    if (inst == NULL) return;
    if (param_index == 0) {
        int v = (int)inst->n163_wave_size + delta;
        if (v < 1) v = 1;
        if (v > FAMI32_N163_WAVE_SIZE) v = FAMI32_N163_WAVE_SIZE;
        inst->n163_wave_size = v;
    } else if (param_index == 1) {
        int v = (int)inst->n163_wave_count + delta;
        if (v < 1) v = 1;
        if (v > FAMI32_N163_WAVE_MAX) v = FAMI32_N163_WAVE_MAX;
        inst->n163_wave_count = v;
    } else {
        int v = (int)inst->n163_wave_pos + delta;
        if (v < 0) v = 0;
        if (v > 127) v = 127;
        inst->n163_wave_pos = v;
    }
}

static void n163_edit_param(instrument_t *inst, int param_index) {
    if (inst == NULL) return;
    if (param_index == 0) {
        int v = inst->n163_wave_size;
        num_set_menu_int("N163 SIZE", 1, FAMI32_N163_WAVE_SIZE, 1, &v, 0, 0, 72, 34);
        inst->n163_wave_size = v;
    } else if (param_index == 1) {
        int v = inst->n163_wave_count;
        num_set_menu_int("N163 COUNT", 1, FAMI32_N163_WAVE_MAX, 1, &v, 0, 0, 80, 34);
        inst->n163_wave_count = v;
    } else {
        int v = inst->n163_wave_pos;
        num_set_menu_int("N163 POS", 0, 127, 1, &v, 0, 0, 72, 34);
        inst->n163_wave_pos = v;
    }
}

static void n163_instrument_editor(instrument_t *inst) {
    if (inst == NULL) return;
    int section = 0;
    int wave = 0;
    int sample = 0;
    int param = 0;
    static const char *tabs[3] = {"WAVE", "PARM", "SEQ"};
    static const char *menu_str[4] = {"WAVE TABLE", "PARAMS", "SEQUENCES", "TYPE"};

    for (;;) {
        if (inst->n163_wave_size < 1 || inst->n163_wave_size > FAMI32_N163_WAVE_SIZE) inst->n163_wave_size = FAMI32_N163_WAVE_SIZE;
        if (inst->n163_wave_count < 1 || inst->n163_wave_count > FAMI32_N163_WAVE_MAX) inst->n163_wave_count = 1;
        if (wave >= (int)inst->n163_wave_count) wave = inst->n163_wave_count - 1;
        if (sample >= (int)inst->n163_wave_size) sample = inst->n163_wave_size - 1;

        display.clearDisplay();
        draw_editor_header("N163", inst->name);
        draw_tab_strip(tabs, 3, section);

        if (section == 0) {
            display.drawRect(0, 21, 128, 27, 1);
            int draw_w = 128 / inst->n163_wave_size;
            if (draw_w < 1) draw_w = 1;
            for (int i = 0; i < (int)inst->n163_wave_size; ++i) {
                int h = ((inst->n163_wave[wave][i] & 0x0F) * 24) / 15;
                int x = i * draw_w;
                if (i == sample) {
                    display.fillRect(x, 47 - h, draw_w, h ? h : 1, 1);
                    display.drawFastHLine(x, 19, draw_w, 1);
                } else {
                    display.drawRect(x, 47 - h, draw_w, h ? h : 1, 1);
                }
            }
            display.setCursor(0, 51);
            display.printf("W%02d/%02lu S%02d:%02d", wave, (unsigned long)inst->n163_wave_count,
                           sample, inst->n163_wave[wave][sample] & 0x0F);
            display.setCursor(72, 51);
            display.printf("SZ:%02lu", (unsigned long)inst->n163_wave_size);
        } else if (section == 1) {
            const char *labels[3] = {"SIZE", "COUNT", "POS"};
            uint32_t values[3] = {inst->n163_wave_size, inst->n163_wave_count, inst->n163_wave_pos};
            for (int i = 0; i < 3; ++i) {
                int y = 23 + i * 10;
                if (i == param) {
                    display.fillRect(0, y - 1, 128, 9, 1);
                    display.setTextColor(0);
                } else {
                    display.setTextColor(1);
                }
                display.setCursor(2, y);
                display.printf("%s:%03lu", labels[i], (unsigned long)values[i]);
            }
            display.setTextColor(1);
        } else {
            display.setCursor(2, 22);
            display.print("N163 SEQUENCES");
            for (int i = 0; i < 5; ++i) {
                int y = 34 + (i / 2) * 8;
                int x = (i & 1) ? 64 : 4;
                display.setCursor(x, y);
                display.drawBitmap(x, y - 1, fami32_icon[i], 7, 7, inst->seq_index[i].enable);
                display.setCursor(x + 10, y);
                display.printf("#%02d %s", inst->seq_index[i].seq_index,
                               inst->seq_index[i].enable ? "ON" : "OFF");
            }
            invertRect(88, 57, 40, 7);
        }

        display.setCursor(0, 58);
        if (section == 0) display.print("OK EDIT L/R X OCT W");
        else if (section == 1) display.print("OK EDIT L/R SEL");
        else display.print("OK SEQUENCES");
        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_BACK) {
                    return;
                } else if (e.bit.KEY == KEY_OK) {
                    if (section == 0) {
                        int value = inst->n163_wave[wave][sample] & 0x0F;
                        edit_int_value("N163 SAMPLE", 0, 15, &value);
                        inst->n163_wave[wave][sample] = value & 0x0F;
                    } else if (section == 1) {
                        n163_edit_param(inst, param);
                    } else if (section == 2) {
                        n163_sequence_editor_request = true;
                        sequence_editor(inst);
                    }
                    player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                } else if (e.bit.KEY == KEY_MENU) {
                    int ret = menu("N163 EDIT", menu_str, 4, NULL, 72, 45, 0, 0, section);
                    if (ret >= 0 && ret < 3) {
                        section = ret;
                    } else if (ret == 3) {
                        instrument_type_page();
                        if (inst->type != INST_N163) return;
                    }
                } else if (e.bit.KEY == KEY_NAVI) {
                    menu_navi();
                    return;
                } else if (e.bit.KEY == KEY_L) {
                    if (section == 0) {
                        sample--;
                        if (sample < 0) sample = inst->n163_wave_size - 1;
                    } else if (section == 1) {
                        param--;
                        if (param < 0) param = 2;
                    }
                } else if (e.bit.KEY == KEY_R) {
                    if (section == 0) {
                        sample++;
                        if (sample >= (int)inst->n163_wave_size) sample = 0;
                    } else if (section == 1) {
                        param++;
                        if (param > 2) param = 0;
                    }
                } else if (e.bit.KEY == KEY_UP || e.bit.KEY == KEY_P) {
                    if (section == 0) {
                        if (inst->n163_wave[wave][sample] < 15) inst->n163_wave[wave][sample]++;
                    } else if (section == 1) {
                        n163_adjust_param(inst, param, 1);
                    }
                    player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                } else if (e.bit.KEY == KEY_DOWN || e.bit.KEY == KEY_S) {
                    if (section == 0) {
                        if (inst->n163_wave[wave][sample] > 0) inst->n163_wave[wave][sample]--;
                    } else if (section == 1) {
                        n163_adjust_param(inst, param, -1);
                    }
                    player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                } else if (e.bit.KEY == KEY_OCTU) {
                    if (section == 0) {
                        wave++;
                        if (wave >= (int)inst->n163_wave_count) wave = 0;
                    } else {
                        section++;
                        if (section > 2) section = 0;
                    }
                } else if (e.bit.KEY == KEY_OCTD) {
                    if (section == 0) {
                        wave--;
                        if (wave < 0) wave = inst->n163_wave_count - 1;
                    } else {
                        section--;
                        if (section < 0) section = 2;
                    }
                }
            }
        }

        touch_input_event_t touch_event;
        if (touch_input_pop_event(&touch_event)) {
            process_note_io_event(note_io_event_from_input(touch_event));
        }

        vTaskDelay(4);
    }
}

void sequence_editor(instrument_t *inst) {
    if (inst != NULL && inst->type == INST_VRC7) {
        vrc7_instrument_editor(inst);
        return;
    }
    if (inst != NULL && inst->type == INST_FDS) {
        fds_instrument_editor(inst);
        return;
    }
    if (inst != NULL && inst->type == INST_N163 && !n163_sequence_editor_request) {
        n163_instrument_editor(inst);
        return;
    }
    n163_sequence_editor_request = false;

    int sequ_type = 0;

    int sequ_sel_index = 0;

    int pageStart = 0;
    int pageEnd = 32;

    static const char *sequ_name[5] = {"VOLUME", "ARPEGGIO", "PITCH", "HI-PITCH", "DUTY"};

    static const char *menu_str[6] = {"LENGTH", "LOOP", "RELEASE", "SEQU INDEX", "QUICK GEN.", "ARP MODE"};

    int draw_base = 30;

    if (inst == NULL) {
        return;
    }

    for (;;) {
        display.clearDisplay();

        display.fillRect(0, 0, 128, 9, 1);
        display.setTextColor(0);
        display.setCursor(1, 1);
        display.setFont(&rismol57);
        display.print(inst->name);

        display.drawBitmap(120, 1, fami32_icon[sequ_type], 7, 7, 0);
        if (!inst->seq_index[sequ_type].enable) {
            invertRect(119, 0, 9, 9);
        }

        // display.drawFastHLine(0, 9, player.channel[channel_sel_pos].get_rel_vol() / 2, 1);

        // display.drawRect(0, 10, 128, 42, 1);
        display.drawFastHLine(0, 10, 128, 1);
        display.drawFastHLine(0, 51, 128, 1);

        display.setFont(&rismol35);
        display.setTextColor(1);

        sequences_t *sequ_slot = ftm.get_inst_sequ(inst, sequ_type, inst->seq_index[sequ_type].seq_index);
        sequences_t *sequ = sequ_slot;
        if (sequ != NULL && (sequ->length == 0 || sequ->data.empty())) {
            sequ = NULL;
        }

        if (sequ != NULL) {
            if (sequ_sel_index >= sequ->length) {
                sequ_sel_index = sequ->length - 1;
            }
            if (sequ_sel_index < 0) {
                sequ_sel_index = 0;
            }
            if (sequ_sel_index < pageStart) {
                pageStart = sequ_sel_index;
                pageEnd = pageStart + 32;
            } else if (sequ_sel_index >= pageEnd) {
                pageEnd = sequ_sel_index + 1;
                pageStart = pageEnd - 32;
            }
            if (pageStart < 0) pageStart = 0;
            if (pageEnd > sequ->length) pageEnd = sequ->length;

            int draw_w = (sequ->length <= 32) ? (128 / sequ->length) : 4;
            if (draw_w < 1) draw_w = 1;

            if (sequ->length <= 32) {
                pageStart = 0;
                pageEnd = sequ->length;
                drawChessboard(sequ->length * draw_w, 11, 128 - (sequ->length * draw_w), 40);
            }

            display.setFont(&rismol34);
            display.setCursor(112, 54);
            display.print("LOOP");
            display.setCursor(100, 59);
            display.print("RELEASE");
            display.setFont(&rismol35);

            if (sequ->loop != SEQ_FEAT_DISABLE && pageEnd > (int)sequ->loop) {
                int draw_start = sequ->loop - pageStart;
                if (draw_start >= 0) invertRect(draw_start * draw_w, 54, 128 - (draw_start * draw_w), 4);
            }

            if (sequ->release != SEQ_FEAT_DISABLE && pageEnd > (int)sequ->release) {
                int draw_start = sequ->release - pageStart;
                if (draw_start >= 0) invertRect(draw_start * draw_w, 59, 128 - (draw_start * draw_w), 4);
            }

            if (sequ_type == VOL_SEQU) {
                drawChessboard(0, 20, 128, 1);
                for (int i = pageStart; i < pageEnd; i++) {
                    int displayIndex = i - pageStart;
                    if ((player.channel[channel_sel_pos].get_inst() == inst) && (player.channel[channel_sel_pos].get_inst_pos(sequ_type) == i)) {
                        drawChessboard((displayIndex * draw_w) + 1, (51 - (sequ->data[i] * 2)) + 1, draw_w - 2, (sequ->data[i] * 2) - 2);
                    }
                    if (i == sequ_sel_index) {
                        display.drawFastHLine(displayIndex * draw_w, 12, draw_w, 1);
                        display.fillRect(displayIndex * draw_w, (51 - (sequ->data[i] * 2)), draw_w, sequ->data[i] * 2, 1);
                    } else {
                        display.drawRect(displayIndex * draw_w, (51 - (sequ->data[i] * 2)), draw_w, sequ->data[i] * 2, 1);
                    }
                }
                display.setCursor(0, 12);
            } else if (sequ_type == ARP_SEQU) {
                drawChessboard(0, draw_base, 128, 1);
                for (int i = pageStart; i < pageEnd; i++) {
                    int displayIndex = i - pageStart;
                    // display.drawRect(i * draw_w, (51 - (sequ->data[i] * 2)), draw_w, sequ->data[i] * 2, 1);
                    if ((player.channel[channel_sel_pos].get_inst() == inst) && (player.channel[channel_sel_pos].get_inst_pos(sequ_type) == i)) {
                        drawChessboard((displayIndex * draw_w) + 1, (draw_base - sequ->data[i]), draw_w - 1, 1);
                    }
                    if (i == sequ_sel_index) {
                        display.drawFastHLine(displayIndex * draw_w, 12, draw_w, 1);
                        display.fillRect(displayIndex * draw_w, (draw_base - sequ->data[i]) - 1, draw_w + 1, 3, 1);
                    } else {
                        display.drawRect(displayIndex * draw_w, (draw_base - sequ->data[i]) - 1, draw_w + 1, 3, 1);
                    }
                }
                display.setCursor(0, 45);
            } else if (sequ_type == DTY_SEQU) {
                drawChessboard(0, 20, 128, 1);
                int duty_scale = (inst->type == INST_VRC6) ? 5 : ((inst->type == INST_N163) ? 3 : 10);
                for (int i = pageStart; i < pageEnd; i++) {
                    int displayIndex = i - pageStart;
                    if ((player.channel[channel_sel_pos].get_inst() == inst) && (player.channel[channel_sel_pos].get_inst_pos(sequ_type) == i)) {
                        drawChessboard((displayIndex * draw_w) + 1, (51 - (sequ->data[i] * duty_scale)) + 1, draw_w - 2, (sequ->data[i] * duty_scale) - 2);
                    }
                    if (i == sequ_sel_index) {
                        display.drawFastHLine(displayIndex * draw_w, 12, draw_w, 1);
                        display.fillRect(displayIndex * draw_w, (51 - (sequ->data[i] * duty_scale)), draw_w, (sequ->data[i] * duty_scale), 1);
                    } else {
                        display.drawRect(displayIndex * draw_w, (51 - (sequ->data[i] * duty_scale)), draw_w, (sequ->data[i] * duty_scale), 1);
                    }
                }
                display.setCursor(0, 12);
            } else {
                drawChessboard(0, draw_base, 128, 1);
                for (int i = pageStart; i < pageEnd; i++) {
                    int displayIndex = i - pageStart;
                    if ((player.channel[channel_sel_pos].get_inst() == inst) && (player.channel[channel_sel_pos].get_inst_pos(sequ_type) == i)) {
                        drawChessboardWithNegativeSize((displayIndex * draw_w) + 1, draw_base, draw_w - 2, -sequ->data[i]);
                    }
                    if (i == sequ_sel_index) {
                        display.drawFastHLine(displayIndex * draw_w, 12, draw_w, 1);
                        fillRectWithNegativeSize(displayIndex * draw_w, draw_base, draw_w, -sequ->data[i], 1);
                    } else {
                        drawRectWithNegativeSize(displayIndex * draw_w, draw_base, draw_w, -sequ->data[i], 1);
                    }
                }
                display.setCursor(0, 45);
            }

            display.printf("(%d,%d)", sequ_sel_index, sequ->data[sequ_sel_index]);
            if (sequ_type == ARP_SEQU) {
                display.setCursor(94, 45);
                display.print(arp_setting_name(sequ->setting));
            }
        } else {
            display.setCursor(0, 14);
            display.printf("NULL %s SEQUENCE: #%d\nPRESS OK TO CREATE IT...", sequ_name[sequ_type], inst->seq_index[sequ_type].seq_index);
        }
        
        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OK) {
                    inst->seq_index[sequ_type].enable = !inst->seq_index[sequ_type].enable;
                    if (sequ == NULL) {
                        if (sequ_slot == NULL) {
                            ftm.resize_inst_sequ(inst, sequ_type, inst->seq_index[sequ_type].seq_index + 1);
                        }
                        ftm.resize_inst_sequ_len(inst, sequ_type, inst->seq_index[sequ_type].seq_index, 1);
                    }
                } else if (e.bit.KEY == KEY_BACK) {
                    return;
                } else if (e.bit.KEY == KEY_MENU) {
                    int ret = menu("SELECT SEQUENCE", sequ_name, 5, NULL, 68, 53, 0, 0, sequ_type);
                    if (ret != -1) {
                        sequ_sel_index = 0;
                        pageEnd = 32;
                        pageStart = 0;
                        sequ_type = ret;
                    }
                } else if (e.bit.KEY == KEY_NAVI) {
                    int ret = menu("SEQUENCE", menu_str, sequ_type == ARP_SEQU ? 6 : 5, NULL, 60, 45, 0, 0, 0);
                    int sequ_len;
                    int sequ_index_ref = inst->seq_index[sequ_type].seq_index;
                    int loop_ref;
                    int release_ref;
                    if (sequ != NULL) {
                        sequ_len = sequ->length;
                        loop_ref = sequ->loop;
                        release_ref = sequ->release;
                    }
                    switch (ret)
                    {
                    case 0:
                        if (sequ != NULL) {
                            num_set_menu_int("SEQU LENGTH", 1, 255, 1, &sequ_len, 0, 0, 100, 34);
                            ftm.resize_inst_sequ_len(inst, sequ_type, inst->seq_index[sequ_type].seq_index, sequ_len);
                            sequ_sel_index = sequ_len - 1;
                        }
                        break;

                    case 1:
                        if (sequ != NULL) {
                            num_set_menu_int("SEQU LOOP", -1, sequ_len - 1, 1, &loop_ref, 0, 0, 126, 34);
                            sequ->loop = loop_ref;
                        }
                        break;

                    case 2:
                        if (sequ != NULL) {
                            num_set_menu_int("SEQU RELEASE", -1, sequ_len - 1, 1, &release_ref, 0, 0, 126, 34);
                            sequ->release = release_ref;
                        }
                        break;

                    case 3:
                        num_set_menu_int("SEQU INDEX", 0, 255, 1, &sequ_index_ref, 0, 0, 64, 34);
                        inst->seq_index[sequ_type].seq_index = sequ_index_ref;
                        break;

                    case 4:
                        if (sequ != NULL) {
                            quick_gen_sequ(inst, sequ, sequ_type, inst->seq_index[sequ_type].seq_index);
                            sequ_sel_index = sequ->length - 1;
                        } else {
                            drawPopupBox("CREATE THIS SEQUENCE FIRST!");
                            vTaskDelay(1024);
                        }
                        break;

                    case 5:
                        if (sequ != NULL && sequ_type == ARP_SEQU) {
                            arp_setting_menu(&sequ->setting);
                        }
                        break;

                    default:
                        break;
                    }
                } else if (e.bit.KEY == KEY_L) {
                    if (sequ != NULL) {
                        sequ_sel_index--;
                        if (sequ_sel_index < 0) {
                            sequ_sel_index = sequ->length - 1;
                            if (sequ->length > 32) {
                                pageStart = sequ->length - 32;
                                pageEnd = sequ->length;
                            }
                        } else if (sequ_sel_index < pageStart) {
                            pageStart = sequ_sel_index;
                            pageEnd = pageStart + 32;
                        }
                    }
                } else if (e.bit.KEY == KEY_R) {
                    if (sequ != NULL) {
                        sequ_sel_index++;
                        if (sequ_sel_index >= sequ->length) {
                            sequ_sel_index = 0;
                            pageStart = 0;
                            pageEnd = 32;
                        } else if (sequ_sel_index >= pageEnd) {
                            pageEnd = sequ_sel_index + 1;
                            pageStart = pageEnd - 32;
                        }
                    }
                } else if (e.bit.KEY == KEY_UP) {
                    draw_base++;
                } else if (e.bit.KEY == KEY_DOWN) {
                    draw_base--;
                } else if (e.bit.KEY == KEY_P) {
                    if (sequ != NULL) {
                        sequ->data[sequ_sel_index]++;
                        int duty_max = (inst->type == INST_VRC6) ? 7 : ((inst->type == INST_N163) ? (int)inst->n163_wave_count - 1 : 3);
                        if (duty_max < 0) duty_max = 0;
                        if (sequ_type == VOL_SEQU && sequ->data[sequ_sel_index] > 15) {
                            sequ->data[sequ_sel_index] = 15;
                        } else if (sequ_type == ARP_SEQU && sequ->setting == ARP_SETTING_FIXED && sequ->data[sequ_sel_index] > 95) {
                            sequ->data[sequ_sel_index] = 0;
                        } else if (sequ_type == DTY_SEQU && sequ->data[sequ_sel_index] > duty_max) {
                            sequ->data[sequ_sel_index] = 0;
                        }
                    }
                } else if (e.bit.KEY == KEY_S) {
                    if (sequ != NULL) {
                        sequ->data[sequ_sel_index]--;
                        if (sequ->data[sequ_sel_index] < 0) {
                            if (sequ_type == VOL_SEQU) {
                                sequ->data[sequ_sel_index] = 0;
                            } else if (sequ_type == ARP_SEQU && sequ->setting == ARP_SETTING_FIXED) {
                                sequ->data[sequ_sel_index] = 95;
                            } else if (sequ_type == DTY_SEQU) {
                                int duty_max = (inst->type == INST_VRC6) ? 7 : ((inst->type == INST_N163) ? (int)inst->n163_wave_count - 1 : 3);
                                if (duty_max < 0) duty_max = 0;
                                sequ->data[sequ_sel_index] = duty_max;
                            }
                        }
                    }
                } else if (e.bit.KEY == KEY_OCTU) {
                    g_octv++;
                } else if (e.bit.KEY == KEY_OCTD) {
                    g_octv--;
                }
            }
        }

        touch_input_event_t touch_event;
        if (touch_input_pop_event(&touch_event)) {
            process_note_io_event(note_io_event_from_input(touch_event));
        }

        vTaskDelay(4);
    }
}
