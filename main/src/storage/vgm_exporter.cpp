#include "vgm_exporter.h"

#include <algorithm>
#include <math.h>
#include <new>
#include <string.h>
#include <vector>

#include "fami32_player.h"
#include "src_config.h"

namespace {

constexpr uint32_t VGM_SAMPLE_RATE = 44100;
constexpr uint32_t VGM_HEADER_SIZE = 0x100;
constexpr uint32_t VGM_VERSION = 0x00000171;
constexpr uint32_t NES_APU_CLOCK = 1789773;
constexpr uint32_t NES_DPCM_RAM_BASE = 0xC000;
constexpr uint32_t NES_DPCM_RAM_LIMIT = 0x10000;
constexpr uint32_t MAX_EXPORT_SECONDS = 600;

struct VgmWriter {
    FILE *fp = nullptr;
    uint32_t total_samples = 0;
    uint32_t gd3_offset = 0;
    uint32_t loop_offset = 0;
    uint32_t loop_samples = 0;

    bool open(const char *filename) {
        fp = fopen(filename, "wb+");
        if (fp == nullptr) {
            return false;
        }

        uint8_t header[VGM_HEADER_SIZE] = {};
        memcpy(header, "Vgm ", 4);
        put_u32(header, 0x08, VGM_VERSION);
        put_u32(header, 0x24, ENG_SPEED > 0 ? (uint32_t)ENG_SPEED : 60);
        put_u32(header, 0x34, VGM_HEADER_SIZE - 0x34);
        put_u32(header, 0x84, NES_APU_CLOCK);

        return fwrite(header, 1, sizeof(header), fp) == sizeof(header);
    }

    void close() {
        if (fp != nullptr) {
            fclose(fp);
            fp = nullptr;
        }
    }

    bool finish() {
        if (!write_u8(0x66)) {
            return false;
        }

        if (!write_gd3()) {
            return false;
        }

        long file_size = ftell(fp);
        if (file_size < 0) {
            return false;
        }

        return patch_u32(0x04, (uint32_t)file_size - 0x04)
            && patch_u32(0x14, gd3_offset ? gd3_offset - 0x14 : 0)
            && patch_u32(0x18, total_samples)
            && patch_u32(0x1C, loop_offset ? loop_offset - 0x1C : 0)
            && patch_u32(0x20, loop_samples);
    }

    bool write_apu(uint8_t reg, uint8_t value) {
        return write_u8(0xB4) && write_u8(reg) && write_u8(value);
    }

    uint32_t tell() {
        long offset = ftell(fp);
        return offset < 0 ? 0 : (uint32_t)offset;
    }

    void set_loop(uint32_t offset, uint32_t samples) {
        loop_offset = offset;
        loop_samples = samples;
    }

    bool wait_samples(uint32_t samples) {
        while (samples > 0) {
            if (samples == 735) {
                if (!write_u8(0x62)) return false;
                total_samples += 735;
                samples = 0;
            } else if (samples == 882) {
                if (!write_u8(0x63)) return false;
                total_samples += 882;
                samples = 0;
            } else {
                uint16_t chunk = samples > 65535 ? 65535 : (uint16_t)samples;
                if (!write_u8(0x61) || !write_u16(chunk)) return false;
                total_samples += chunk;
                samples -= chunk;
            }
        }
        return true;
    }

    bool write_dpcm_ram_block(const std::vector<uint8_t> &ram) {
        if (ram.empty()) {
            return true;
        }
        if (!write_u8(0x67) || !write_u8(0x66) || !write_u8(0xC2)) {
            return false;
        }
        if (!write_u32((uint32_t)ram.size() + 2) || !write_u16((uint16_t)NES_DPCM_RAM_BASE)) {
            return false;
        }
        return fwrite(ram.data(), 1, ram.size(), fp) == ram.size();
    }

    static void put_u32(uint8_t *data, uint32_t offset, uint32_t value) {
        data[offset + 0] = value & 0xff;
        data[offset + 1] = (value >> 8) & 0xff;
        data[offset + 2] = (value >> 16) & 0xff;
        data[offset + 3] = (value >> 24) & 0xff;
    }

    bool write_u8(uint8_t value) {
        return fputc(value, fp) != EOF;
    }

    bool write_u16(uint16_t value) {
        return write_u8(value & 0xff) && write_u8((value >> 8) & 0xff);
    }

    bool write_u32(uint32_t value) {
        return write_u8(value & 0xff)
            && write_u8((value >> 8) & 0xff)
            && write_u8((value >> 16) & 0xff)
            && write_u8((value >> 24) & 0xff);
    }

    bool patch_u32(uint32_t offset, uint32_t value) {
        if (fseek(fp, offset, SEEK_SET) != 0) {
            return false;
        }
        if (!write_u32(value)) {
            return false;
        }
        return fseek(fp, 0, SEEK_END) == 0;
    }

    bool write_gd3() {
        gd3_offset = (uint32_t)ftell(fp);
        if (gd3_offset == 0) {
            return false;
        }

        std::vector<uint8_t> body;
        if (!append_utf16_ascii(body, current_info->title, sizeof(current_info->title))
            || !append_utf16_literal(body, "")
            || !append_utf16_literal(body, "Fami32")
            || !append_utf16_literal(body, "")
            || !append_utf16_literal(body, "Nintendo Entertainment System")
            || !append_utf16_literal(body, "")
            || !append_utf16_ascii(body, current_info->author, sizeof(current_info->author))
            || !append_utf16_literal(body, "")
            || !append_utf16_literal(body, "")
            || !append_utf16_literal(body, "Fami32 NES VGM Exporter")
            || !append_utf16_ascii(body, current_info->copyright, sizeof(current_info->copyright))) {
            return false;
        }

        return fwrite("Gd3 ", 1, 4, fp) == 4
            && write_u32(0x00000100)
            && write_u32((uint32_t)body.size())
            && fwrite(body.data(), 1, body.size(), fp) == body.size();
    }

    INFO_BLOCK *current_info = nullptr;

    static bool append_utf16_ascii(std::vector<uint8_t> &out, const char *str, size_t max_len) {
        size_t len = 0;
        while (len < max_len && str[len] != '\0') {
            out.push_back((uint8_t)str[len]);
            out.push_back(0);
            len++;
        }
        out.push_back(0);
        out.push_back(0);
        return true;
    }

    static bool append_utf16_literal(std::vector<uint8_t> &out, const char *str) {
        return append_utf16_ascii(out, str, strlen(str));
    }
};

struct ApuRegisterCache {
    uint8_t value[0x18] = {};
    bool valid[0x18] = {};

    bool write_if_changed(VgmWriter &writer, uint8_t reg, uint8_t data) {
        if (reg >= sizeof(value)) {
            return false;
        }
        if (!valid[reg] || value[reg] != data) {
            if (!writer.write_apu(reg, data)) {
                return false;
            }
            value[reg] = data;
            valid[reg] = true;
        }
        return true;
    }
};

struct SongPositionTracker {
    struct Visit {
        bool visited = false;
        uint32_t file_offset = 0;
        uint32_t sample_offset = 0;
    };

    std::vector<Visit> visits;
    uint32_t rows = 0;

    void init(uint32_t frame_count, uint32_t row_count) {
        rows = std::max<uint32_t>(1, row_count);
        visits.assign(std::max<uint32_t>(1, frame_count) * rows, Visit());
    }

    bool mark_or_get_loop(int frame,
                          int row,
                          uint32_t file_offset,
                          uint32_t sample_offset,
                          uint32_t *loop_file_offset,
                          uint32_t *loop_sample_offset) {
        uint32_t index = position_index(frame, row);
        if (index >= visits.size()) {
            return false;
        }
        if (visits[index].visited) {
            if (loop_file_offset != nullptr) {
                *loop_file_offset = visits[index].file_offset;
            }
            if (loop_sample_offset != nullptr) {
                *loop_sample_offset = visits[index].sample_offset;
            }
            return true;
        }
        visits[index].visited = true;
        visits[index].file_offset = file_offset;
        visits[index].sample_offset = sample_offset;
        return false;
    }

private:
    uint32_t position_index(int frame, int row) const {
        if (frame < 0) frame = 0;
        if (row < 0) row = 0;
        return (uint32_t)frame * rows + (uint32_t)row;
    }
};

struct RenderPlayerOwner {
    FAMI_PLAYER *player = nullptr;

    bool create() {
        player = new (std::nothrow) FAMI_PLAYER();
        return player != nullptr;
    }

    ~RenderPlayerOwner() {
        delete player;
    }

    FAMI_PLAYER *operator->() {
        return player;
    }

    FAMI_PLAYER &operator*() {
        return *player;
    }
};

uint8_t clamp4(int value) {
    if (value < 0) return 0;
    if (value > 15) return 15;
    return (uint8_t)value;
}

uint16_t clamp_period(float period) {
    long value = lroundf(period);
    if (value < 0) value = 0;
    if (value > 0x7ff) value = 0x7ff;
    return (uint16_t)value;
}

uint8_t pulse_duty(WAVE_TYPE mode) {
    switch (mode) {
        case PULSE_025: return 1;
        case PULSE_050: return 2;
        case PULSE_075: return 3;
        case PULSE_125:
        default:
            return 0;
    }
}

uint8_t scaled_volume(FAMI_CHANNEL &channel) {
    return clamp4((channel.get_rel_vol() + 7) / 15);
}

bool write_pulse_channel(VgmWriter &writer, ApuRegisterCache &regs, FAMI_CHANNEL &channel, uint8_t base_reg) {
    uint16_t period = clamp_period(channel.get_period_rel());
    uint8_t vol = scaled_volume(channel);
    uint8_t duty = pulse_duty(channel.get_mode());

    return regs.write_if_changed(writer, base_reg + 0, (duty << 6) | 0x30 | vol)
        && regs.write_if_changed(writer, base_reg + 1, 0x08)
        && regs.write_if_changed(writer, base_reg + 2, period & 0xff)
        && regs.write_if_changed(writer, base_reg + 3, (period >> 8) & 0x07);
}

bool write_triangle_channel(VgmWriter &writer, ApuRegisterCache &regs, FAMI_CHANNEL &channel) {
    uint16_t period = clamp_period(channel.get_period_rel());
    uint8_t linear = channel.get_rel_vol() ? 0xff : 0x80;

    return regs.write_if_changed(writer, 0x08, linear)
        && regs.write_if_changed(writer, 0x0A, period & 0xff)
        && regs.write_if_changed(writer, 0x0B, (period >> 8) & 0x07);
}

bool write_noise_channel(VgmWriter &writer, ApuRegisterCache &regs, FAMI_CHANNEL &channel) {
    uint8_t vol = scaled_volume(channel);
    uint8_t mode = channel.get_mode() == NOISE1 ? 0x80 : 0x00;
    uint8_t period = 15 - (channel.get_noise_rate() & 0x0f);

    return regs.write_if_changed(writer, 0x0C, 0x30 | vol)
        && regs.write_if_changed(writer, 0x0E, mode | period)
        && regs.write_if_changed(writer, 0x0F, 0x00);
}

uint8_t dpcm_length_register(uint32_t bytes) {
    if (bytes <= 1) {
        return 0;
    }
    uint32_t value = (bytes - 1) / 16;
    if (value > 0xff) {
        value = 0xff;
    }
    return (uint8_t)value;
}

bool write_dpcm_channel(VgmWriter &writer,
                        ApuRegisterCache &regs,
                        FAMI_CHANNEL &channel,
                        const std::vector<uint16_t> &sample_addr,
                        FTM_FILE *ftm_data,
                        bool &last_active,
                        int &last_sample,
                        int &last_pos) {
    bool active = channel.get_dpcm_sample_status();
    int sample = channel.get_dpcm_sample_num();
    int sample_pos = channel.get_samp_pos();

    if (!active) {
        last_active = false;
        last_sample = -1;
        last_pos = 0;
        return regs.write_if_changed(writer, 0x15, 0x0f);
    }

    if (sample < 0 || sample >= (int)sample_addr.size() || sample >= (int)ftm_data->dpcm_samples.size()) {
        return true;
    }

    if (!last_active || last_sample != sample || sample_pos < last_pos) {
        const dpcm_sample_t &dpcm = ftm_data->dpcm_samples[sample];
        uint32_t offset_bytes = (uint32_t)channel.get_dpcm_sample_start() * 64;
        if (offset_bytes >= dpcm.dpcm_data.size()) {
            return true;
        }

        uint32_t remaining = dpcm.dpcm_data.size() - offset_bytes;
        uint32_t addr = sample_addr[sample] + offset_bytes;
        if (addr < NES_DPCM_RAM_BASE || addr >= NES_DPCM_RAM_LIMIT) {
            return true;
        }

        uint8_t rate = channel.get_dpcm_pitch() & 0x0f;
        uint8_t loop = channel.get_dpcm_sample_loop() ? 0x40 : 0x00;
        uint8_t addr_reg = (uint8_t)((addr - NES_DPCM_RAM_BASE) / 64);
        uint8_t len_reg = dpcm_length_register(remaining);
        uint8_t delta = clamp4(channel.get_rel_vol() / 16) << 3;

        if (!regs.write_if_changed(writer, 0x15, 0x0f)
            || !regs.write_if_changed(writer, 0x10, loop | rate)
            || !regs.write_if_changed(writer, 0x11, delta)
            || !regs.write_if_changed(writer, 0x12, addr_reg)
            || !regs.write_if_changed(writer, 0x13, len_reg)
            || !regs.write_if_changed(writer, 0x15, 0x1f)) {
            return false;
        }
    }

    last_active = true;
    last_sample = sample;
    last_pos = sample_pos;
    return true;
}

bool write_apu_state(VgmWriter &writer,
                     ApuRegisterCache &regs,
                     FAMI_PLAYER &render_player,
                     const std::vector<uint16_t> &sample_addr,
                     FTM_FILE *ftm_data,
                     bool &last_dpcm_active,
                     int &last_dpcm_sample,
                     int &last_dpcm_pos) {
    return write_pulse_channel(writer, regs, render_player.channel[0], 0x00)
        && write_pulse_channel(writer, regs, render_player.channel[1], 0x04)
        && write_triangle_channel(writer, regs, render_player.channel[2])
        && write_noise_channel(writer, regs, render_player.channel[3])
        && write_dpcm_channel(writer, regs, render_player.channel[4], sample_addr, ftm_data, last_dpcm_active, last_dpcm_sample, last_dpcm_pos);
}

vgm_export_result_t build_dpcm_ram(FTM_FILE *ftm_data, std::vector<uint8_t> &ram, std::vector<uint16_t> &sample_addr) {
    sample_addr.assign(ftm_data->dpcm_samples.size(), 0);

    uint32_t cursor = 0;
    for (size_t i = 0; i < ftm_data->dpcm_samples.size(); ++i) {
        const std::vector<uint8_t> &data = ftm_data->dpcm_samples[i].dpcm_data;
        if (data.empty()) {
            continue;
        }

        cursor = (cursor + 63) & ~((uint32_t)63);
        if (NES_DPCM_RAM_BASE + cursor + data.size() > NES_DPCM_RAM_LIMIT) {
            return VGM_EXPORT_DPCM_TOO_LARGE;
        }

        if (ram.size() < cursor + data.size()) {
            ram.resize(cursor + data.size(), 0);
        }
        memcpy(ram.data() + cursor, data.data(), data.size());
        sample_addr[i] = (uint16_t)(NES_DPCM_RAM_BASE + cursor);
        cursor += data.size();
    }

    return VGM_EXPORT_OK;
}

uint32_t samples_for_tick(double &accumulator) {
    accumulator += (double)VGM_SAMPLE_RATE / (double)std::max(1, ENG_SPEED);
    uint32_t samples = (uint32_t)floor(accumulator);
    accumulator -= samples;
    return samples;
}

} // namespace

vgm_export_result_t export_nes_vgm(FTM_FILE *ftm_data,
                                   const char *filename,
                                   vgm_export_progress_cb progress_cb,
                                   void *progress_user) {
    if (ftm_data == nullptr || filename == nullptr) {
        return VGM_EXPORT_FILE_ERROR;
    }

    std::vector<uint8_t> dpcm_ram;
    std::vector<uint16_t> sample_addr;
    vgm_export_result_t layout_result = build_dpcm_ram(ftm_data, dpcm_ram, sample_addr);
    if (layout_result != VGM_EXPORT_OK) {
        return layout_result;
    }

    VgmWriter writer;
    writer.current_info = &ftm_data->nf_block;
    if (!writer.open(filename)) {
        writer.close();
        return VGM_EXPORT_FILE_ERROR;
    }

    if (!writer.write_dpcm_ram_block(dpcm_ram)) {
        writer.close();
        return VGM_EXPORT_FILE_ERROR;
    }

    ApuRegisterCache regs;
    bool last_dpcm_active = false;
    int last_dpcm_sample = -1;
    int last_dpcm_pos = 0;

    if (!writer.write_apu(0x15, 0x00)
        || !writer.write_apu(0x17, 0x40)
        || !writer.write_apu(0x15, 0x0f)) {
        writer.close();
        return VGM_EXPORT_FILE_ERROR;
    }

    RenderPlayerOwner render_player;
    if (!render_player.create()) {
        writer.close();
        return VGM_EXPORT_FILE_ERROR;
    }

    render_player->init(ftm_data);
    render_player->reload();
    render_player->set_row(0);
    render_player->jmp_to_frame(0);
    render_player->start_play();

    uint32_t ticks = 0;
    uint32_t last_row_event = render_player->get_row_event_counter();
    SongPositionTracker position_tracker;
    position_tracker.init(ftm_data->fr_block.frame_num, ftm_data->fr_block.pat_length);
    position_tracker.mark_or_get_loop(render_player->get_frame(),
                                      render_player->get_row(),
                                      writer.tell(),
                                      writer.total_samples,
                                      nullptr,
                                      nullptr);
    double sample_accumulator = 0.0;
    uint32_t max_samples = VGM_SAMPLE_RATE * MAX_EXPORT_SECONDS;

    while (render_player->get_play_status()) {
        render_player->process_tick();

        if (!write_apu_state(writer, regs, *render_player, sample_addr, ftm_data, last_dpcm_active, last_dpcm_sample, last_dpcm_pos)) {
            writer.close();
            return VGM_EXPORT_FILE_ERROR;
        }

        uint32_t wait = samples_for_tick(sample_accumulator);
        if (!writer.wait_samples(wait)) {
            writer.close();
            return VGM_EXPORT_FILE_ERROR;
        }

        ticks++;
        if (progress_cb != nullptr && (ticks & 0x0f) == 0) {
            if (!progress_cb(ticks, writer.total_samples, progress_user)) {
                writer.close();
                return VGM_EXPORT_FILE_ERROR;
            }
        }

        uint32_t row_event = render_player->get_row_event_counter();
        if (render_player->get_play_status() && row_event != last_row_event) {
            last_row_event = row_event;
            uint32_t loop_file_offset = 0;
            uint32_t loop_sample_offset = 0;
            if (position_tracker.mark_or_get_loop(render_player->get_frame(),
                                                  render_player->get_row(),
                                                  writer.tell(),
                                                  writer.total_samples,
                                                  &loop_file_offset,
                                                  &loop_sample_offset)) {
                writer.set_loop(loop_file_offset, writer.total_samples - loop_sample_offset);
                break;
            }
        }
        if (writer.total_samples > max_samples) {
            writer.finish();
            writer.close();
            return VGM_EXPORT_RENDER_LIMIT;
        }
    }

    render_player->stop_play();

    if (!writer.finish()) {
        writer.close();
        return VGM_EXPORT_FILE_ERROR;
    }

    writer.close();
    return VGM_EXPORT_OK;
}

const char *vgm_export_result_str(vgm_export_result_t result) {
    switch (result) {
        case VGM_EXPORT_OK:
            return "OK";
        case VGM_EXPORT_FILE_ERROR:
            return "FILE ERROR";
        case VGM_EXPORT_DPCM_TOO_LARGE:
            return "DPCM TOO LARGE";
        case VGM_EXPORT_RENDER_LIMIT:
            return "TIME LIMIT";
        default:
            return "UNKNOWN ERROR";
    }
}
