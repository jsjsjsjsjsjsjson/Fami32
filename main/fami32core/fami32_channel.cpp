#include "fami32_channel.h"

void FAMI_CHANNEL::clear_all_fx_flag() {
    chl_vol = 15;
    rel_vol = 0;

    freq = 0;
    period = 0;
    pos_count = 0;

    if (mode < TRIANGULAR) {
        mode = PULSE_125;
        chl_mode = mode;
    } else if (mode > DPCM_SAMPLE) {
        mode = NOISE0;
        chl_mode = mode;
    }

    slide_up = 0;
    slide_down = 0;

    vol_slide_up = 0;
    vol_slide_down = 0;

    vol_count = 0;

    auto_port = 0;
    auto_port_source = 0.0f;
    auto_port_target = 0.0f;
    auto_port_finish = true;

    vib_spd = 0;
    vib_dep = 0;

    vib_pos = 0;

    tre_spd = 0;
    tre_dep = 0;

    tre_pos = 0;

    portup_target = 0.0f;
    portup_speed = 0;

    portdown_target = 0.0f;
    portdown_speed = 0;

    period_offset = 0;

    delay_cut_count = 0;
    delay_cut_status = false;

    arp_fx_n1 = 0;
    arp_fx_n2 = 0;

    arp_fx_pos = 0;

    sample_pos = 0;
    sample_fpos = 0;
    sample_var = 0;
    sample_num = 0;
    sample_start = 0;
    sample_status = 0;
    sample_len = 0;
    sample_pitch = 0;

    last_note = 255;
}

void FAMI_CHANNEL::init(FTM_FILE* data) {
    tick_length = SAMP_RATE / ENG_SPEED;
    ftm_data = data;
    inst_proc.init(data);
    tick_buf.resize(tick_length);
    DBG_PRINTF("INIT: ftm_data = %p, tick_length = (%d / %d) = %d\n", ftm_data, SAMP_RATE, ENG_SPEED, tick_length);
    hpf.setCutoffFrequency(HPF_CUTOFF, SAMP_RATE);
}

void FAMI_CHANNEL::set_slide_up(uint8_t n) {
    arp_fx_n1 = 0;
    arp_fx_n2 = 0;

    slide_up = n;
    slide_down = 0;
    auto_port = 0;
}

void FAMI_CHANNEL::set_slide_down(uint8_t n) {
    arp_fx_n1 = 0;
    arp_fx_n2 = 0;

    slide_down = n;
    slide_up = 0;
    auto_port = 0;
}

void FAMI_CHANNEL::set_vol_slide_up(uint8_t n) {
    vol_count = 0;
    if (n)
        vol_slide_up = roundf(8.0f / n);
    else
        vol_slide_up = 0;
}

void FAMI_CHANNEL::set_vol_slide_down(uint8_t n) {
    vol_count = 0;
    if (n)
        vol_slide_down = roundf(8.0f / n);
    else
        vol_slide_down = 0;
}

void FAMI_CHANNEL::set_auto_port(uint8_t n) {
    arp_fx_n1 = 0;
    arp_fx_n2 = 0;

    auto_port = n;
    if (last_note != 255) {
        auto_port_source = get_period();
        auto_port_finish = true;
    }
    slide_up = 0;
    slide_down = 0;
}

void FAMI_CHANNEL::set_vibrato(uint8_t spd, uint8_t dep) {
    vib_spd = spd;
    vib_dep = dep;

    vib_pos = 0;
}

void FAMI_CHANNEL::set_tremolo(uint8_t spd, uint8_t dep) {
    tre_spd = spd;
    tre_dep = dep;

    tre_pos = 0;
}

void FAMI_CHANNEL::set_period_offset(int8_t var) {
    int8_t per_off_last = period_offset;
    period_offset = var;
}

void FAMI_CHANNEL::set_port_up(uint8_t spd, uint8_t offset) {
    arp_fx_n1 = 0;
    arp_fx_n2 = 0;

    slide_up = 0;
    slide_down = 0;

    portdown_speed = 0;
    auto_port = 0;
    portup_speed = (spd * 2) + 1;
    portup_target = note2period(base_note + offset);
    base_note += offset;
}

void FAMI_CHANNEL::set_port_down(uint8_t spd, uint8_t offset) {
    arp_fx_n1 = 0;
    arp_fx_n2 = 0;

    slide_up = 0;
    slide_down = 0;

    portup_speed = 0;
    auto_port = 0;
    portdown_speed = (spd * 2) + 1;
    portdown_target = note2period(base_note - offset);
    base_note -= offset;
}

void FAMI_CHANNEL::set_delay_cut(uint8_t t) {
    delay_cut_count = t;
    delay_cut_status = true;
}

void FAMI_CHANNEL::make_tick_sound() {
    int8_t env_vol = inst_proc.get_sequ_var(VOL_SEQU);

    int8_t tre_var = 0;
    if (tre_spd && tre_dep) {
        tre_pos = (tre_pos + tre_spd) & 63;
        tre_var = (vib_table[tre_pos] * tre_dep) / 32;
    }

    rel_vol = env_vol * (chl_vol + tre_var);

    if (mode < 5) {
        period_rely = period - period_offset;
        freq = period2freq(period_rely);
        pos_count = (freq / SAMP_RATE) * 32;

        if (vib_spd && vib_dep) {
            vib_pos = (vib_pos + vib_spd) & 63;
            int8_t vib_var = (vib_table[vib_pos] * vib_dep) / 16;
            period_rely = get_period() + vib_var;
            pos_count = (period2freq(period_rely) / SAMP_RATE) * 32;
        }

        if (mode == TRIANGULAR) {
            rel_vol = rel_vol ? 225 : 0;
        }
        // DBG_PRINTF("(DEBUG: VOL) GET_ENV: %d->%d\n", inst_proc.get_pos(VOL_SEQU), inst_proc.get_sequ_var(VOL_SEQU));
        float rel_pos_count;
        if (mode == TRIANGULAR) rel_pos_count = pos_count / 2 / OVER_SAMPLE;

        else rel_pos_count = pos_count / OVER_SAMPLE;
        for (int i = 0; i < tick_length; i++) {
            if (!rel_vol) {
                tick_buf[i] = 0;
                continue;
            }
            int sum = 0;
            for (int j = 0; j < OVER_SAMPLE; j++) {
                i_pos = i_pos & 31;
                sum += wave_table[mode][i_pos] * rel_vol;
                f_pos += rel_pos_count;
                if (f_pos > 1.0f) {
                    i_pos += (int)f_pos;
                    f_pos -= (int)f_pos;
                }
            }
            tick_buf[i] = sum / OVER_SAMPLE;
        }
    } else if (mode > 5) {
        for (int i = 0; i < tick_length; i++) {
            tick_buf[i] = nes_noise_get_sample(mode - 6) * rel_vol;
        }
    } else if (mode == DPCM_SAMPLE) {
        float count = dpcm_pitch_table[sample_pitch] / SAMP_RATE;
        if (sample_num < ftm_data->dpcm_samples.size()) {
            uint8_t *sample = ftm_data->dpcm_samples[sample_num].pcm_data.data();
            for (int i = 0; i < tick_length; i++) {
                if (sample_status) {
                    sample_var = sample[sample_pos];
                    sample_fpos += count;
                    if (sample_fpos > 1.0f) {
                        sample_pos += (int)sample_fpos;
                        sample_fpos -= (int)sample_fpos;
                        if (sample_pos >= sample_len) {
                            if (sample_loop) {
                                sample_pos = 0;
                            } else {
                                sample_status = false;
                            }
                        }
                    }
                }
                // printf("SAMPLE_VAR: %d\n", sample_var);
                tick_buf[i] = hpf.process(sample_var * 64);
            }
        }
    } else {
        memset(tick_buf.data(), 0, tick_buf.size() * sizeof(int16_t));
    }
}

void FAMI_CHANNEL::update_tick() {
    if (mode != DPCM_SAMPLE) {
        if (inst_proc.get_sequ_enable(PIT_SEQU) || inst_proc.get_sequ_enable(HPI_SEQU)) {
            if (mode < 5) {
                if (inst_proc.get_status(PIT_SEQU)) {
                    set_period(get_period() + (float)inst_proc.get_sequ_var(PIT_SEQU));
                }
                if (inst_proc.get_status(HPI_SEQU)) {
                    set_period(get_period() + (float)(inst_proc.get_sequ_var(HPI_SEQU) * 16));
                }
            } else if (mode > 5) {
                // DBG_PRINTF("PIT: %d->%d\n", inst_proc.get_pos(PIT_SEQU), inst_proc.get_sequ_var(PIT_SEQU));
                set_noise_rate((noise_rate_rel - inst_proc.get_sequ_var(PIT_SEQU)) & 15);
            }
        }

        if (inst_proc.get_sequ_enable(ARP_SEQU)) {
            if (inst_proc.get_status(ARP_SEQU)) {
                if (mode < 5) {
                    set_note_rely(base_note + inst_proc.get_sequ_var(ARP_SEQU));
                } else if (mode > 5) {
                    // DBG_PRINTF("ARP: %d->%d\n", inst_proc.get_pos(ARP_SEQU), inst_proc.get_sequ_var(ARP_SEQU));
                    set_noise_rate_rel((noise_rate + inst_proc.get_sequ_var(ARP_SEQU)) & 15);
                }
            }
        }

        if (inst_proc.get_sequ_enable(DTY_SEQU)) {
            if (mode < 4) {
                set_mode((WAVE_TYPE)inst_proc.get_sequ_var(DTY_SEQU));
            } else if (mode > 5) {
                set_mode((WAVE_TYPE)(inst_proc.get_sequ_var(DTY_SEQU) & 1));
            }
        } else {
            set_mode(chl_mode);
        }
    }

    if (delay_cut_status) {
        if (delay_cut_count) {
            delay_cut_count--;
        } else {
            note_cut();
            delay_cut_status = false;
        }
    }

    make_tick_sound();

    if (mode != DPCM_SAMPLE) {
        if (slide_up) {
            set_period(get_period() - slide_up);
        } else if (slide_down) {
            set_period(get_period() + slide_down);
        }

        if (vol_slide_up) {
            vol_count++;
            if (vol_count >= vol_slide_up) {
                set_vol(get_vol() + 1);
                vol_count = 0;
            }
        }

        if (vol_slide_down) {
            vol_count++;
            if (vol_count >= vol_slide_down) {
                set_vol(get_vol() - 1);
                vol_count = 0;
            }
        }

        if (auto_port && !auto_port_finish) {
            if (auto_port_source > auto_port_target) {
                set_period(get_period() - auto_port);
                if (get_period() < auto_port_target) {
                    rely_note = base_note;
                    set_period(auto_port_target);
                    auto_port_finish = true;
                }
            } else if (auto_port_source < auto_port_target) {
                set_period(get_period() + auto_port);
                if (get_period() > auto_port_target) {
                    rely_note = base_note;
                    set_period(auto_port_target);
                    auto_port_finish = true;
                }
            }
        }

        if (portup_speed) {
            set_period(get_period() - portup_speed);
            if (get_period() <= portup_target) {
                portup_speed = 0;
                set_period(portup_target);
            }
        } else if (portdown_speed) {
            set_period(get_period() + portdown_speed);
            if (get_period() >= portdown_target) {
                portdown_speed = 0;
                set_period(portdown_target);
            }
        }

        if (arp_fx_n1 || arp_fx_n2) {
            if (arp_fx_pos == 0) {
                set_note_rely(base_note);
            } else if (arp_fx_pos == 1) {
                set_note_rely(base_note + arp_fx_n1);
            } else if (arp_fx_pos == 2) {
                set_note_rely(base_note + arp_fx_n2);
            }
            arp_fx_pos++;
            if (arp_fx_n2) {
                if (arp_fx_pos > 2) {
                    arp_fx_pos = 0;
                }
            } else {
                if (arp_fx_pos > 1) {
                    arp_fx_pos = 0;
                }
            }
        }
    }

    inst_proc.update_tick();
}

void FAMI_CHANNEL::set_inst(int n) {
    // DBG_PRINTF("SET INST: %d\n", n);
    inst_proc.set_inst(n);
}

instrument_t *FAMI_CHANNEL::get_inst() {
    return inst_proc.get_inst();
}

void FAMI_CHANNEL::note_start() {
    if (mode == DPCM_SAMPLE) {
        sample_fpos = 0;
        sample_pos = sample_start * 512;
        sample_status = true;
    } else {
        inst_proc.start();
        portup_speed = 0;
        portdown_speed = 0;
        // i_pos = 8;
    }
}

void FAMI_CHANNEL::set_dpcm_offset(uint8_t n) {
    sample_start = n;
    sample_pos = n * 512;
}

void FAMI_CHANNEL::set_dpcm_var(uint8_t n) {
    sample_var = n;
}

void FAMI_CHANNEL::set_dpcm_pitch(uint8_t n) {
    sample_pitch = n & 15;
}

void FAMI_CHANNEL::set_arp_fx(uint8_t n1, uint8_t n2) {
    arp_fx_n1 = n1;
    arp_fx_n2 = n2;
    arp_fx_pos = 0;
}

void FAMI_CHANNEL::note_end() {
    if (mode == DPCM_SAMPLE) {
        sample_var = 0;
        sample_status = false;
    } else {
        inst_proc.end();
    }
}

void FAMI_CHANNEL::note_cut() {
    last_note = 255;
    if (mode == DPCM_SAMPLE) {
        sample_var = 0;
        sample_status = false;
    } else {
        inst_proc.cut();
        portup_speed = false;
        portdown_speed = false;
    }
}

void FAMI_CHANNEL::set_period(float period_ref) {
    if (mode > 5) {
        set_noise_rate((uint8_t)period_ref & 15);
    } else {
        if (period_ref < 12) period_ref = 12.0f;
        else if (period_ref > 2048) period_ref = 2048.0f;
        period = period_ref;
    }
}

void FAMI_CHANNEL::set_freq(float freq_ref) {
    freq = freq_ref;
    period = freq2period(freq);
    pos_count = (freq / SAMP_RATE) * 32;
}

void FAMI_CHANNEL::set_note_rely(uint8_t note) {
    if (mode > 5) {
        set_noise_rate(note2noise(note));
    } else if (mode < 5) {
        rely_note = note;
        period = note2period(note);
        pos_count = (freq / SAMP_RATE) * 32;
    } else if (mode == DPCM_SAMPLE) {
        sample_num = inst_proc.get_inst()->dpcm[note - 24].index - 1;
        if (sample_num < 0) {
            DBG_PRINTF("DPCM_CHANNEL: WARN -> NO_SAMPLE\n");
            return;
        }
        sample_loop = inst_proc.get_inst()->dpcm[note - 24].loop;
        sample_pitch = inst_proc.get_inst()->dpcm[note - 24].pitch;
        sample_len = ftm_data->dpcm_samples[sample_num].sample_size_byte * 8;
        DBG_PRINTF("DPCM_CHANNEL: SET NUM->%d, PITCH->%d, LOOP->%d, SMP_LEN->%d\n", sample_num, sample_pitch, sample_loop, sample_len);
        sample_pos = sample_start * 256;
    }
}

float FAMI_CHANNEL::get_freq() {
    return freq;
}

float FAMI_CHANNEL::get_period() {
    return period - period_offset;
}

float FAMI_CHANNEL::get_period_rel() {
    if (mode < DPCM_SAMPLE)
        return period_rely;
    else if (mode > DPCM_SAMPLE)
        return get_noise_rate();
    else
        return sample_num;
}

void FAMI_CHANNEL::set_noise_rate(uint8_t rate) {
    noise_rate = rate;
    set_noise_rate_rel(rate);
}

void FAMI_CHANNEL::set_noise_rate_rel(uint8_t rate) {
    noise_rate_rel = rate;
    nes_noise_set_period(noise_rate_rel);
}

uint8_t FAMI_CHANNEL::get_noise_rate() {
    return noise_rate_rel;
}

uint8_t FAMI_CHANNEL::get_dpcm_pitch() {
    return sample_pitch;
}

void FAMI_CHANNEL::set_mode(WAVE_TYPE m) {
    if (mode > DPCM_SAMPLE) {
        mode = (WAVE_TYPE)((m&1) + NOISE0);
        // DBG_PRINTF("SET_NOISE_MODE: %d\n", mode);
    } else if (mode != TRIANGULAR) {
        mode = m;
    }
    // DBG_PRINTF("SET MODE: %d\n", mode);
}

void FAMI_CHANNEL::set_chl_mode(WAVE_TYPE m) {
    if (mode > DPCM_SAMPLE) {
        chl_mode = (WAVE_TYPE)((m&1) + NOISE0);
        DBG_PRINTF("SET_NOISE_MODE(CHL): %d\n", chl_mode);
    } else if (mode != TRIANGULAR) {
        chl_mode = (WAVE_TYPE)(m & 3);
    }
}

WAVE_TYPE FAMI_CHANNEL::get_mode() {
    return mode;
}

void FAMI_CHANNEL::set_note(uint8_t note) {
    if (auto_port) {
        auto_port_target = note2period(note);
        if (last_note == 255) {
            set_note_rely(note);
            auto_port_source = get_period();
            auto_port_source = auto_port_target;
        } else {
            auto_port_source = get_period();
        }
        auto_port_finish = false;
        // printf("AUTO_PORT (P=%d): %f -> %f\n", last_note, auto_port_source, auto_port_target);
    } else {
        set_note_rely(note);
    }
    last_note = base_note;
    base_note = note;
    // DBG_PRINTF("SET_NOTE: P=%f, F=%f, C=%f\n", period, freq, pos_count);
}

void FAMI_CHANNEL::set_vol(int8_t vol) {
    if (vol < 0)
        vol = 0;
    else if (vol > 15)
        vol = 15;

    chl_vol = vol;
}

int8_t FAMI_CHANNEL::get_vol() {
    return chl_vol;
}

int8_t FAMI_CHANNEL::get_env_vol() {
    return inst_proc.get_sequ_var(VOL_SEQU);
}

uint8_t FAMI_CHANNEL::get_rel_vol() {
    if (mode == DPCM_SAMPLE) {
        return sample_var * 2;
    } else {
        return rel_vol;
    }
}

int16_t* FAMI_CHANNEL::get_buf() {
    return tick_buf.data();
}

size_t FAMI_CHANNEL::get_buf_size_byte() {
    return tick_buf.size() * sizeof(int16_t);
}

size_t FAMI_CHANNEL::get_buf_size() {
    return tick_buf.size();
}

int FAMI_CHANNEL::get_samp_pos() {
    return sample_pos;
}

size_t FAMI_CHANNEL::get_samp_len() {
    return sample_len;
}

uint32_t FAMI_CHANNEL::get_inst_pos(int sequ_type) {
    return inst_proc.get_pos(sequ_type);
}