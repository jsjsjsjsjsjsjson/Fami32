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
        case INST_VRC7: return "TYPE:VRC7";
        case INST_FDS: return "TYPE:FDS";
        default: return "TYPE:2A03";
    }
}

static void instrument_type_page() {
    const char *type_str[3] = {"2A03", "VRC7", "FDS"};
    int init = 0;
    if (ftm.get_inst(inst_sel_pos)->type == INST_VRC7) init = 1;
    else if (ftm.get_inst(inst_sel_pos)->type == INST_FDS) init = 2;

    int ret = menu("INSTRUMENT TYPE", type_str, 3, NULL, 64, 37, 0, 0, init);
    if (ret == -1) return;

    if (ret == 1) {
        ftm.set_inst_type(inst_sel_pos, INST_VRC7);
        if (!ftm.vrc7_enabled()) {
            ftm.set_vrc7_enabled(true);
            player.reload();
        }
    } else if (ret == 2) {
        ftm.set_inst_type(inst_sel_pos, INST_FDS);
        if (!ftm.fds_enabled()) {
            ftm.set_fds_enabled(true);
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

void quick_gen_sequ(sequences_t *sequ, uint32_t type, uint32_t index) {
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
    ftm.resize_sequ_len(type, index, len);
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
    int sel = 0;

    for (;;) {
        display.clearDisplay();
        display.fillRect(0, 0, 128, 9, 1);
        display.setTextColor(0);
        display.setCursor(1, 1);
        display.setFont(&rismol57);
        display.printf("VRC7 %.12s", inst->name);

        display.setFont(&rismol35);
        display.setTextColor(1);
        display.drawFastHLine(0, 10, 128, 1);

        display.setCursor(2, 14);
        if (sel == 0) display.fillRect(0, 13, 64, 7, 1), display.setTextColor(0);
        display.printf("PATCH:%02lX", inst->vrc7_patch & 0x0F);
        display.setTextColor(1);

        for (uint8_t r = 0; r < 8; ++r) {
            uint8_t row = r / 2;
            uint8_t col = r & 1;
            uint8_t x = col ? 64 : 2;
            uint8_t y = 24 + row * 8;
            if (sel == r + 1) {
                display.fillRect(x - 2, y - 1, 60, 7, 1);
                display.setTextColor(0);
            } else {
                display.setTextColor(1);
            }
            display.setCursor(x, y);
            display.printf("R%d:%02X", r, inst->vrc7_regs[r]);
        }

        display.setTextColor(1);
        display.setCursor(0, 58);
        display.print("OK EDIT MENU TYPE");
        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_BACK) {
                    return;
                } else if (e.bit.KEY == KEY_OK) {
                    if (sel == 0) {
                        int value = inst->vrc7_patch & 0x0F;
                        num_set_menu_int("VRC7 PATCH", 0, 15, 1, &value, 0, 0, 72, 34);
                        inst->vrc7_patch = value & 0x0F;
                    } else {
                        int value = inst->vrc7_regs[sel - 1];
                        char title[24];
                        snprintf(title, sizeof(title), "VRC7 R%d", sel - 1);
                        num_set_menu_int(title, 0, 255, 1, &value, 0, 0, 72, 34);
                        inst->vrc7_regs[sel - 1] = value & 0xFF;
                    }
                    player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                } else if (e.bit.KEY == KEY_MENU) {
                    instrument_type_page();
                    if (inst->type != INST_VRC7) return;
                } else if (e.bit.KEY == KEY_L) {
                    sel--;
                    if (sel < 0) sel = 8;
                } else if (e.bit.KEY == KEY_R) {
                    sel++;
                    if (sel > 8) sel = 0;
                } else if (e.bit.KEY == KEY_UP) {
                    sel -= 2;
                    if (sel < 0) sel += 9;
                } else if (e.bit.KEY == KEY_DOWN) {
                    sel += 2;
                    if (sel > 8) sel -= 9;
                } else if (e.bit.KEY == KEY_P) {
                    if (sel == 0) {
                        inst->vrc7_patch = (inst->vrc7_patch + 1) & 0x0F;
                    } else {
                        inst->vrc7_regs[sel - 1]++;
                    }
                    player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                } else if (e.bit.KEY == KEY_S) {
                    if (sel == 0) {
                        inst->vrc7_patch = (inst->vrc7_patch - 1) & 0x0F;
                    } else {
                        inst->vrc7_regs[sel - 1]--;
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
    static const char *menu_str[3] = {"LENGTH", "LOOP", "RELEASE"};

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
                        uint8_t len = restore_len[sequ_type];
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
                        restore_len[sequ_type] = sequ->length;
                        sequ->length = 0;
                        sequ_sel_index = 0;
                        pageStart = 0;
                        pageEnd = 32;
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
                    int ret = menu("FDS SEQUENCE", menu_str, 3, NULL, 72, 37, 0, 0, 0);
                    fds_sequence_t &edit_seq = inst->fds_seq[sequ_type];
                    switch (ret) {
                    case 0: {
                        int len = edit_seq.length;
                        num_set_menu_int("SEQU LENGTH", 0, FAMI32_FDS_SEQUENCE_MAX, 1, &len, 0, 0, 100, 34);
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
                        } else if (sequ->data[sequ_sel_index] < 127) {
                            sequ->data[sequ_sel_index]++;
                        }
                    }
                } else if (e.bit.KEY == KEY_S) {
                    if (sequ != NULL) {
                        if (sequ_type == VOL_SEQU) {
                            if (sequ->data[sequ_sel_index] > 0) sequ->data[sequ_sel_index]--;
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
    static const char *menu_str[4] = {"WAVE TABLE", "MOD TABLE", "PARAMS", "SEQUENCES"};

    for (;;) {
        display.clearDisplay();
        display.fillRect(0, 0, 128, 9, 1);
        display.setTextColor(0);
        display.setCursor(1, 1);
        display.setFont(&rismol57);
        display.printf("FDS %.13s", inst->name);
        display.setFont(&rismol35);
        display.setTextColor(1);
        display.drawFastHLine(0, 10, 128, 1);

        for (int i = 0; i < FAMI32_FDS_WAVE_SIZE; ++i) {
            int h = (inst->fds_wave[i] & 0x3F) / 2;
            display.drawFastVLine(i * 2, 43 - h, h ? h : 1, 1);
        }
        if (section == 0) {
            invertRect(wave_index * 2, 44, 2, 5);
        }

        for (int i = 0; i < FAMI32_FDS_MOD_SIZE; ++i) {
            int x = i * 2;
            int y = 55 - (inst->fds_mod[i] & 7);
            display.drawFastVLine(x, y, 1 + (inst->fds_mod[i] & 7), 1);
        }
        if (section == 1) {
            invertRect(mod_index * 2, 56, 2, 5);
        }

        display.setCursor(0, 50);
        display.printf("W%02d:%02d M%02d:%d", wave_index, inst->fds_wave[wave_index] & 0x3F, mod_index, inst->fds_mod[mod_index] & 7);
        display.setCursor(64, 12);
        display.printf("SPD:%04lX", inst->fds_mod_speed & 0x0FFF);
        display.setCursor(64, 20);
        display.printf("DEP:%02lX", inst->fds_mod_depth & 0x3F);
        display.setCursor(64, 28);
        display.printf("DLY:%02lX", inst->fds_mod_delay & 0xFF);

        if (section == 2) {
            invertRect(63, 11 + (param_index * 8), 64, 8);
        } else if (section == 3) {
            invertRect(74, 57, 54, 7);
        }

        display.setCursor(0, 58);
        if (section == 0 || section == 1) {
            display.print("L/R X UP/DN Y");
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
                    if (section == 2) {
                        fds_edit_param(inst, param_index);
                    } else if (section == 3) {
                        fds_sequence_editor(inst);
                    }
                    player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                } else if (e.bit.KEY == KEY_MENU) {
                    int ret = menu("FDS EDIT", menu_str, 4, NULL, 72, 45, 0, 0, section);
                    if (ret != -1) {
                        section = ret;
                        if (section == 3) {
                            fds_sequence_editor(inst);
                            player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                        }
                    }
                } else if (e.bit.KEY == KEY_L) {
                    if (section == 1) {
                        mod_index--;
                        if (mod_index < 0) mod_index = FAMI32_FDS_MOD_SIZE - 1;
                    } else if (section == 2) {
                        param_index--;
                        if (param_index < 0) param_index = 2;
                    } else {
                        wave_index--;
                        if (wave_index < 0) wave_index = FAMI32_FDS_WAVE_SIZE - 1;
                    }
                } else if (e.bit.KEY == KEY_R) {
                    if (section == 1) {
                        mod_index++;
                        if (mod_index >= FAMI32_FDS_MOD_SIZE) mod_index = 0;
                    } else if (section == 2) {
                        param_index++;
                        if (param_index > 2) param_index = 0;
                    } else {
                        wave_index++;
                        if (wave_index >= FAMI32_FDS_WAVE_SIZE) wave_index = 0;
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

void sequence_editor(instrument_t *inst) {
    if (inst != NULL && inst->type == INST_VRC7) {
        vrc7_instrument_editor(inst);
        return;
    }
    if (inst != NULL && inst->type == INST_FDS) {
        fds_instrument_editor(inst);
        return;
    }

    int sequ_type = 0;

    int sequ_sel_index = 0;

    int pageStart = 0;
    int pageEnd = 32;

    static const char *sequ_name[5] = {"VOLUME", "ARPEGGIO", "PITCH", "HI-PITCH", "DUTY"};

    static const char *menu_str[5] = {"LENGTH", "LOOP", "RELEASE", "SEQU INDEX", "QUICK GEN."};

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

        sequences_t *sequ = ftm.get_sequ(sequ_type, inst->seq_index[sequ_type].seq_index);

        if (sequ != NULL) {
            int draw_w = (sequ->length <= 32) ? (128 / sequ->length) : 4;

            if (sequ->length <= 32) {
                pageEnd = sequ->length;
                drawChessboard(sequ->length * draw_w, 11, 128 - (sequ->length * draw_w), 40);
            }

            display.setFont(&rismol34);
            display.setCursor(112, 54);
            display.print("LOOP");
            display.setCursor(100, 59);
            display.print("RELEASE");
            display.setFont(&rismol35);

            if (pageEnd > sequ->loop) {
                int draw_start = sequ->loop - pageStart;
                invertRect(draw_start * draw_w, 54, 128 - (draw_start * draw_w), 4);
            }

            if (pageEnd > sequ->release) {
                int draw_start = sequ->release - pageStart;
                invertRect(draw_start * draw_w, 59, 128 - (draw_start * draw_w), 4);
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
                for (int i = pageStart; i < pageEnd; i++) {
                    int displayIndex = i - pageStart;
                    if ((player.channel[channel_sel_pos].get_inst() == inst) && (player.channel[channel_sel_pos].get_inst_pos(sequ_type) == i)) {
                        drawChessboard((displayIndex * draw_w) + 1, (51 - (sequ->data[i] * 10)) + 1, draw_w - 2, (sequ->data[i] * 10) - 2);
                    }
                    if (i == sequ_sel_index) {
                        display.drawFastHLine(displayIndex * draw_w, 12, draw_w, 1);
                        display.fillRect(displayIndex * draw_w, (51 - (sequ->data[i] * 10)), draw_w, (sequ->data[i] * 10), 1);
                    } else {
                        display.drawRect(displayIndex * draw_w, (51 - (sequ->data[i] * 10)), draw_w, (sequ->data[i] * 10), 1);
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
                        ftm.resize_sequ(sequ_type, inst->seq_index[sequ_type].seq_index + 1);
                        ftm.resize_sequ_len(sequ_type, inst->seq_index[sequ_type].seq_index, 1);
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
                    int ret = menu("SEQUENCE", menu_str, 5, NULL, 60, 45, 0, 0, 0);
                    int sequ_len;
                    int sequ_index_ref;
                    int loop_ref;
                    int release_ref;
                    if (sequ != NULL) {
                        sequ_len = sequ->length;
                        sequ_index_ref = inst->seq_index[sequ_type].seq_index;
                        loop_ref = sequ->loop;
                        release_ref = sequ->release;
                    }
                    switch (ret)
                    {
                    case 0:
                        if (sequ != NULL) {
                            num_set_menu_int("SEQU LENGTH", 1, 255, 1, &sequ_len, 0, 0, 100, 34);
                            ftm.resize_sequ_len(sequ_type, inst->seq_index[sequ_type].seq_index, sequ_len);
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
                            quick_gen_sequ(sequ, sequ_type, inst->seq_index[sequ_type].seq_index);
                            sequ_sel_index = sequ->length - 1;
                        } else {
                            drawPopupBox("CREATE THIS SEQUENCE FIRST!");
                            vTaskDelay(1024);
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
                        if (sequ_type == VOL_SEQU && sequ->data[sequ_sel_index] > 15) {
                            sequ->data[sequ_sel_index] = 15;
                        } if (sequ_type == DTY_SEQU && sequ->data[sequ_sel_index] > 3) {
                            sequ->data[sequ_sel_index] = 0;
                        }
                    }
                } else if (e.bit.KEY == KEY_S) {
                    if (sequ != NULL) {
                        sequ->data[sequ_sel_index]--;
                        if (sequ->data[sequ_sel_index] < 0) {
                            if (sequ_type == VOL_SEQU) {
                                sequ->data[sequ_sel_index] = 0;
                            } else if (sequ_type == DTY_SEQU) {
                                sequ->data[sequ_sel_index] = 3;
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
