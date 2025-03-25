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

void instrument_option_page() {
    static const char *menu_str[4] = {"NEW", "RENAME", "RAND NAME", "REMOVE"};
    srand(time(NULL));
    int ret = menu("INSTRUMENT", menu_str, 4, NULL, 60, 45, 0, 0, 0);
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

            for (int f = 0; f < 5; f++) {
                if (i == inst_sel_pos)
                    display.drawBitmap(88 + (f * 8), itemYPos + 1, fami32_icon[f], 7, 7, !ftm.get_inst(i)->seq_index[f].enable);
                else
                    display.drawBitmap(88 + (f * 8), itemYPos + 1, fami32_icon[f], 7, 7, ftm.get_inst(i)->seq_index[f].enable);
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
            display.drawFastVLine(63 + (player.channel[4].get_samp_pos() * (64.0f / (float)player.channel[4].get_samp_len())), 58, 6, 1);
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

        if (touchKeypad.available()) {
            touchKeypadEvent e = touchKeypad.read();
            update_touchpad_note(NULL, NULL, e);
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

void sequence_editor(instrument_t *inst) {
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

        if (touchKeypad.available()) {
            touchKeypadEvent e = touchKeypad.read();
            update_touchpad_note(NULL, NULL, e);
        }

        vTaskDelay(4);
    }
}