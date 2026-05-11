#include "gui_instrument.h"
#include "gui_menu.h"
#include "gui_input.h"
#include "gui_tracker.h"
#include "gui_channel.h"
#include "gui_file.h"
#include "wav.hpp"

namespace {

constexpr int MAX_DPCM_SAMPLES = 64;
constexpr uint32_t MAX_DPCM_BYTES = 0x0FF1;
constexpr uint32_t MAX_DPCM_PCM_SAMPLES = MAX_DPCM_BYTES * 8;
constexpr uint32_t DPCM_IMPORT_RATE = 33144;

int sample_count() {
    return static_cast<int>(ftm.dpcm_samples.size());
}

uint8_t clamp_pcm7(int value) {
    if (value < 0) {
        return 0;
    }
    if (value > 127) {
        return 127;
    }
    return static_cast<uint8_t>(value);
}

uint32_t normalized_dpcm_size(uint32_t size) {
    if (size == 0) {
        return 0;
    }
    uint32_t padded = size + ((1U - size) & 0x0FU);
    return padded > MAX_DPCM_BYTES ? MAX_DPCM_BYTES : padded;
}

void refresh_sample_metadata() {
    ftm.dpcm_block.sample_num = static_cast<uint8_t>(sample_count());
    for (int i = 0; i < sample_count(); ++i) {
        ftm.dpcm_samples[i].index = i;
        ftm.dpcm_samples[i].name_len = strnlen(ftm.dpcm_samples[i].name, sizeof(ftm.dpcm_samples[i].name));
    }
}

void compact_sample_table() {
    if (ftm.dpcm_samples.empty()) {
        refresh_sample_metadata();
        return;
    }

    std::vector<int> index_map(ftm.dpcm_samples.size(), -1);
    std::vector<dpcm_sample_t> compacted;
    compacted.reserve(ftm.dpcm_samples.size());

    for (int i = 0; i < sample_count() && compacted.size() < MAX_DPCM_SAMPLES; ++i) {
        dpcm_sample_t &sample = ftm.dpcm_samples[i];
        if (sample.sample_size_byte == 0 || sample.dpcm_data.empty()) {
            continue;
        }
        index_map[i] = compacted.size();
        sample.index = compacted.size();
        compacted.push_back(sample);
    }

    for (int i = 0; i < ftm.inst_block.inst_num; ++i) {
        instrument_t *inst = ftm.get_inst(i);
        if (inst == NULL) {
            continue;
        }

        for (int note = 0; note < 96; ++note) {
            int old_index = inst->dpcm[note].index - 1;
            if (old_index < 0) {
                continue;
            }
            if (old_index >= static_cast<int>(index_map.size()) || index_map[old_index] < 0) {
                inst->dpcm[note].index = 0;
            } else {
                inst->dpcm[note].index = index_map[old_index] + 1;
            }
        }
    }

    ftm.dpcm_samples.swap(compacted);
    refresh_sample_metadata();
}

void decode_sample(dpcm_sample_t &sample) {
    sample.sample_size_byte = sample.dpcm_data.size();
    sample.pcm_data.resize(sample.sample_size_byte * 8);
    if (sample.sample_size_byte > 0) {
        decode_dpcm(sample.dpcm_data.data(), sample.sample_size_byte, sample.pcm_data.data());
    }
}

void set_sample_name_from_path(dpcm_sample_t &sample, const char *path) {
    const char *name = strrchr(path, '/');
    name = name ? name + 1 : path;

    snprintf(sample.name, sizeof(sample.name), "%s", name);
    char *dot = strrchr(sample.name, '.');
    if (dot != NULL && dot != sample.name) {
        *dot = '\0';
    }
    sample.name_len = strnlen(sample.name, sizeof(sample.name));
}

int sample_value_from_pcm(const uint8_t *src, int bits) {
    if (bits == 8) {
        return src[0] >> 1;
    }

    if (bits == 16) {
        int16_t v = static_cast<int16_t>(src[0] | (src[1] << 8));
        return (static_cast<int>(v) + 32768) >> 9;
    }

    int32_t v = static_cast<int32_t>(
        static_cast<uint32_t>(src[0]) |
        (static_cast<uint32_t>(src[1]) << 8) |
        (static_cast<uint32_t>(src[2]) << 16) |
        (static_cast<uint32_t>(src[3]) << 24));
    return static_cast<int>((static_cast<int64_t>(v) + 2147483648LL) >> 25);
}

bool import_dmc_file(const char *path, dpcm_sample_t &sample) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return false;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (file_size <= 0) {
        fclose(fp);
        return false;
    }

    uint32_t read_size = static_cast<uint32_t>(file_size);
    if (read_size > MAX_DPCM_BYTES) {
        read_size = MAX_DPCM_BYTES;
    }
    uint32_t stored_size = normalized_dpcm_size(read_size);

    sample.dpcm_data.assign(stored_size, 0xAA);
    size_t got = fread(sample.dpcm_data.data(), 1, read_size, fp);
    fclose(fp);
    if (got == 0) {
        return false;
    }

    decode_sample(sample);
    return true;
}

bool import_wav_file(const char *path, dpcm_sample_t &sample) {
    WavReader reader;
    if (reader.init(path) != WavReader::OK) {
        return false;
    }

    if (reader.channels() == 0 || reader.bits_per_sample() == 0 || reader.sample_rate() == 0) {
        return false;
    }

    const int bytes_per_sample = reader.bits_per_sample() / 8;
    const int channels = reader.channels();
    uint32_t source_limit = static_cast<uint32_t>(
        ((static_cast<uint64_t>(MAX_DPCM_PCM_SAMPLES) * reader.sample_rate()) + DPCM_IMPORT_RATE - 1) / DPCM_IMPORT_RATE);
    if (source_limit == 0) {
        source_limit = MAX_DPCM_PCM_SAMPLES;
    } else if (source_limit > MAX_DPCM_PCM_SAMPLES * 6) {
        source_limit = MAX_DPCM_PCM_SAMPLES * 6;
    }
    std::vector<uint8_t> frame(bytes_per_sample * channels);
    std::vector<uint8_t> source_pcm;
    source_pcm.reserve(source_limit);

    while (source_pcm.size() < source_limit) {
        size_t got = 0;
        if (reader.read_frame(frame.data(), channels, &got) != WavReader::OK || got < static_cast<size_t>(channels)) {
            break;
        }

        int mixed = 0;
        for (int ch = 0; ch < channels; ++ch) {
            mixed += sample_value_from_pcm(&frame[ch * bytes_per_sample], reader.bits_per_sample());
        }
        mixed /= channels;
        source_pcm.push_back(clamp_pcm7(mixed));
    }

    if (source_pcm.empty()) {
        return false;
    }

    std::vector<uint8_t> import_pcm;
    uint32_t out_count = static_cast<uint32_t>((static_cast<uint64_t>(source_pcm.size()) * DPCM_IMPORT_RATE) / reader.sample_rate());
    if (out_count == 0) {
        out_count = source_pcm.size();
    }
    if (out_count > MAX_DPCM_PCM_SAMPLES) {
        out_count = MAX_DPCM_PCM_SAMPLES;
    }
    import_pcm.resize(out_count);

    for (uint32_t i = 0; i < out_count; ++i) {
        uint32_t src_index = static_cast<uint32_t>((static_cast<uint64_t>(i) * reader.sample_rate()) / DPCM_IMPORT_RATE);
        if (src_index >= source_pcm.size()) {
            src_index = source_pcm.size() - 1;
        }
        import_pcm[i] = source_pcm[src_index];
    }

    uint32_t encoded_capacity = (import_pcm.size() + 7) / 8;
    uint32_t stored_size = normalized_dpcm_size(encoded_capacity);
    sample.dpcm_data.assign(stored_size, 0xAA);
    int encoded_size = encode_dpcm(import_pcm.data(), import_pcm.size(), sample.dpcm_data.data());
    if (encoded_size <= 0) {
        return false;
    }
    for (uint32_t i = encoded_size; i < stored_size; ++i) {
        sample.dpcm_data[i] = 0xAA;
    }

    decode_sample(sample);
    return true;
}

bool import_sample(const char *path) {
    if (sample_count() >= MAX_DPCM_SAMPLES) {
        drawPopupBox("SAMPLE TABLE FULL");
        vTaskDelay(1024);
        return false;
    }

    dpcm_sample_t sample = {};
    set_sample_name_from_path(sample, path);

    const char *dot = strrchr(path, '.');
    bool ok = false;
    if (dot != NULL && (!strcasecmp(dot, ".wav"))) {
        ok = import_wav_file(path, sample);
    } else if (dot != NULL && (!strcasecmp(dot, ".dmc") || !strcasecmp(dot, ".dpcm"))) {
        ok = import_dmc_file(path, sample);
    }

    if (!ok) {
        drawPopupBox("IMPORT FAILED");
        vTaskDelay(1024);
        return false;
    }

    sample.index = sample_count();
    ftm.dpcm_samples.push_back(sample);
    refresh_sample_metadata();
    return true;
}

void remove_sample(int sample_index) {
    if (sample_index < 0 || sample_index >= sample_count()) {
        return;
    }

    ftm.dpcm_samples.erase(ftm.dpcm_samples.begin() + sample_index);

    for (int i = 0; i < ftm.inst_block.inst_num; ++i) {
        instrument_t *inst = ftm.get_inst(i);
        if (inst == NULL) {
            continue;
        }

        for (int note = 0; note < 96; ++note) {
            if (inst->dpcm[note].index == sample_index + 1) {
                inst->dpcm[note].index = 0;
            } else if (inst->dpcm[note].index > sample_index + 1) {
                inst->dpcm[note].index--;
            }
        }
    }

    refresh_sample_metadata();
}

void clear_note_assignment(instrument_t *inst, int note_index) {
    inst->dpcm[note_index].index = 0;
    inst->dpcm[note_index].pitch = 0;
    inst->dpcm[note_index].loop = 0;
    inst->dpcm[note_index].d_counte = 0xFF;
}

void draw_sample_meter(const dpcm_sample_t &sample) {
    if (sample.pcm_data.empty()) {
        return;
    }

    for (int x = 0; x < 64; ++x) {
        size_t pos = (static_cast<size_t>(x) * sample.pcm_data.size()) / 64;
        uint8_t v = sample.pcm_data[pos];
        int y = 58 - (v / 16);
        display.drawPixel(64 + x, y, 1);
    }
}

void dpcm_assignment_editor(int sample_index) {
    if (sample_index < 0 || sample_index >= sample_count()) {
        return;
    }

    instrument_t *inst = ftm.get_inst(inst_sel_pos);
    if (inst == NULL) {
        return;
    }

    static int note_index = 0;
    static const char *menu_str[5] = {"PITCH", "LOOP", "DELTA", "CLEAR NOTE", "CLEAR INST"};

    for (;;) {
        dpcm_t *assign = &inst->dpcm[note_index];
        int note = (note_index % 12) + 1;
        int octave = note_index / 12;

        display.clearDisplay();
        display.fillRect(0, 0, 128, 9, 1);
        display.setTextColor(0);
        display.setFont(&rismol57);
        display.setCursor(1, 1);
        display.print("DPCM MAP ");
        display.setFont(&font4x6);
        display.printf("I%02X S%02X", inst_sel_pos, sample_index);

        display.setTextColor(1);
        display.setFont(&rismol57);
        display.setCursor(0, 13);
        display.printf("%s%d", note2str[note], octave);
        display.setFont(&rismol35);
        display.setCursor(38, 13);
        display.printf("SMP:%02X", assign->index ? assign->index - 1 : 0);
        if (assign->index == 0) {
            display.print(" --");
        } else if (assign->index - 1 == sample_index) {
            display.print(" *");
        }

        display.setCursor(0, 24);
        display.printf("PITCH:%02X LOOP:%s", assign->pitch, assign->loop ? "ON" : "OFF");
        display.setCursor(0, 32);
        if (assign->d_counte == 0xFF) {
            display.print("DELTA:--");
        } else {
            display.printf("DELTA:%02X", assign->d_counte);
        }

        dpcm_sample_t &sample = ftm.dpcm_samples[sample_index];
        display.drawFastHLine(0, 42, 128, 1);
        display.setCursor(0, 45);
        display.printf("%02X>%s", sample_index, sample.name);
        display.setCursor(0, 55);
        display.printf("%luB OK=SET", (unsigned long)sample.sample_size_byte);
        draw_sample_meter(sample);

        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OK) {
                    if (assign->index == sample_index + 1) {
                        clear_note_assignment(inst, note_index);
                    } else {
                        assign->index = sample_index + 1;
                        assign->pitch = 15;
                        assign->loop = 0;
                        assign->d_counte = 0xFF;
                    }
                } else if (e.bit.KEY == KEY_BACK) {
                    return;
                } else if (e.bit.KEY == KEY_MENU) {
                    int ret = menu("DPCM NOTE", menu_str, 5, NULL, 66, 53, 0, 0, 0);
                    int pitch = assign->pitch;
                    int loop = assign->loop ? 1 : 0;
                    int delta = assign->d_counte == 0xFF ? -1 : assign->d_counte;
                    if (ret == 0) {
                        num_set_menu_int("PITCH", 0, 15, 1, &pitch, 0, 0, 60, 34);
                        assign->pitch = pitch;
                    } else if (ret == 1) {
                        num_set_menu_int("LOOP", 0, 1, 1, &loop, 0, 0, 52, 34);
                        assign->loop = loop ? 8 : 0;
                    } else if (ret == 2) {
                        num_set_menu_int("DELTA", -1, 127, 1, &delta, 0, 0, 72, 34);
                        assign->d_counte = delta < 0 ? 0xFF : delta;
                    } else if (ret == 3) {
                        clear_note_assignment(inst, note_index);
                    } else if (ret == 4) {
                        for (int i = 0; i < 96; ++i) {
                            clear_note_assignment(inst, i);
                        }
                    }
                } else if (e.bit.KEY == KEY_NAVI) {
                    menu_navi();
                    return;
                } else if (e.bit.KEY == KEY_L) {
                    note_index = (note_index + 95) % 96;
                } else if (e.bit.KEY == KEY_R) {
                    note_index = (note_index + 1) % 96;
                } else if (e.bit.KEY == KEY_UP) {
                    note_index = (note_index + 84) % 96;
                } else if (e.bit.KEY == KEY_DOWN) {
                    note_index = (note_index + 12) % 96;
                } else if (e.bit.KEY == KEY_P) {
                    if (assign->pitch < 15) {
                        assign->pitch++;
                    }
                } else if (e.bit.KEY == KEY_S) {
                    if (assign->pitch > 0) {
                        assign->pitch--;
                    }
                }
            }
        }

        touch_input_event_t touch_event;
        if (touch_input_pop_event(&touch_event)) {
            note_io_result_t note_result = process_note_io_event(note_io_event_from_input(touch_event));
            if (note_result.has_note) {
                int midi_note = item2note(note_result.note, note_result.octave);
                if (midi_note >= 24 && midi_note < 120) {
                    note_index = midi_note - 24;
                }
            }
        }

        vTaskDelay(4);
    }
}

void sample_option_page(int *selected_sample) {
    static const char *menu_str[4] = {"IMPORT", "RENAME", "REMOVE", "ASSIGN"};
    int ret = menu("DPCM SAMPLE", menu_str, 4, NULL, 72, 45, 0, 0, 0);

    if (ret == 0) {
        const char *path = file_select("/flash");
        if (path != NULL) {
            drawPopupBox("IMPORTING...");
            if (import_sample(path)) {
                *selected_sample = sample_count() - 1;
            }
        }
    } else if (ret == 1) {
        if (sample_count() > 0) {
            displayKeyboard("SAMPLE NAME", ftm.dpcm_samples[*selected_sample].name, 63);
            refresh_sample_metadata();
        }
    } else if (ret == 2) {
        if (sample_count() > 0) {
            remove_sample(*selected_sample);
            if (*selected_sample >= sample_count()) {
                *selected_sample = sample_count() - 1;
            }
            if (*selected_sample < 0) {
                *selected_sample = 0;
            }
        }
    } else if (ret == 3) {
        if (sample_count() > 0) {
            dpcm_assignment_editor(*selected_sample);
        }
    }
}

} // namespace

void sample_editor_menu() {
    static int selected_sample = 0;
    static int pageStart = 0;
    const int itemsPerPage = 6;

    compact_sample_table();

    for (;;) {
        int count = sample_count();
        if (count == 0) {
            selected_sample = 0;
            pageStart = 0;
        } else {
            if (selected_sample < 0) {
                selected_sample = count - 1;
            } else if (selected_sample >= count) {
                selected_sample = 0;
            }
        }

        display.clearDisplay();

        display.fillRect(0, 0, 128, 9, 1);
        display.setTextColor(0);
        display.setFont(&rismol57);
        display.setCursor(1, 1);
        display.print("SAMPLE EDITOR ");
        display.setFont(&font4x6);
        if (count > 0) {
            display.printf("(%02X/%02X)", selected_sample, count - 1);
        } else {
            display.print("(EMPTY)");
        }

        display.drawFastHLine(0, 9, player.channel[channel_sel_pos].get_rel_vol() / 2, 1);
        display.drawFastHLine(0, 63, 128, 1);

        display.setFont(&rismol35);
        display.setTextColor(1);

        if (count == 0) {
            display.setCursor(0, 12);
            display.print("NO DPCM SAMPLE\nPRESS MENU TO IMPORT\n.WAV .DMC .DPCM");
        } else {
            if (selected_sample < pageStart) {
                pageStart = selected_sample;
            } else if (selected_sample >= pageStart + itemsPerPage) {
                pageStart = selected_sample - itemsPerPage + 1;
            }
            int pageEnd = (pageStart + itemsPerPage >= count) ? count : pageStart + itemsPerPage;

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

                if (dpcm_sample->name_len > 14) {
                    display.printf("%02X>%.11s.. %luB", dpcm_sample->index, dpcm_sample->name, (unsigned long)dpcm_sample->sample_size_byte);
                } else {
                    display.printf("%02X>%s", dpcm_sample->index, dpcm_sample->name);
                }
            }

            display.setTextColor(1);
            display.setCursor(0, 57);
            display.printf("OK MAP I%02X  MENU EDIT", inst_sel_pos);
        }

        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OK) {
                    if (count > 0) {
                        dpcm_assignment_editor(selected_sample);
                    }
                } else if (e.bit.KEY == KEY_NAVI) {
                    menu_navi();
                    break;
                } else if (e.bit.KEY == KEY_MENU) {
                    sample_option_page(&selected_sample);
                    refresh_sample_metadata();
                } else if (e.bit.KEY == KEY_BACK) {
                    return;
                } else if (e.bit.KEY == KEY_UP) {
                    if (count > 0) {
                        selected_sample--;
                        if (selected_sample < 0) {
                            selected_sample = count - 1;
                            pageStart = ((count - 1) / itemsPerPage) * itemsPerPage;
                        }
                    }
                } else if (e.bit.KEY == KEY_DOWN) {
                    if (count > 0) {
                        selected_sample++;
                        if (selected_sample >= count) {
                            selected_sample = 0;
                            pageStart = 0;
                        }
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
