#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Keypad.h>
#include <MPR121_Keypad.h>
// #include <esp_littlefs.h>
#include <esp_vfs_fat.h>
#include <driver/i2s_std.h>
#include "chipbox_pin.h"
#include "ftm_file.h"
#include "tracker.h"
#include "dirent.h"
#include <easy_usb_midi.h>
#include "SerialTerminal.h"
#include "gui.h"

bool _debug_print = false;

extern "C" {
#include "ls.h"
#include "micro_config.h"
}

FAMI_PLAYER player;
i2s_chan_handle_t tx_handle;

TaskHandle_t SOUND_TASK_HD = NULL;

TaskHandle_t GUI_TASK = NULL;

EASY_USB_MIDI USB_MIDI;

Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &SPI, DISPLAY_DC, DISPLAY_RESET, DISPLAY_CS);

Adafruit_Keypad keypad = Adafruit_Keypad(makeKeymap(KEYPAD_MAP),
                                        (byte[KEYPAD_ROWS]){KEYPAD_R0, KEYPAD_R1, KEYPAD_R2, KEYPAD_R3},
                                        (byte[KEYPAD_COLS]){KEYPAD_C0, KEYPAD_C1, KEYPAD_C2},
                                        KEYPAD_ROWS, KEYPAD_COLS);

MPR121_Keypad touchKeypad(TOUCHPAD0_ADDRS, TOUCHPAD1_ADDRS);

void open_ftm_cmd(int argc, const char* argv[]) {
    if (argc < 2) {
        printf("%s <file>\n", argv[0]);
        return;
    }
    ftm.open_ftm(argv[1]);
}

void read_ftm_cmd(int argc, const char* argv[]) {
    ftm.read_ftm_all();
}

void print_frame_cmd(int argc, const char* argv[]) {
    if (argc < 2) {
        printf("%s <frame>\n", argv[0]);
        return;
    }
    ftm.print_frame_data(strtol(argv[1], NULL, 0));
}

void reboot_cmd(int argc, const char* argv[]) {
    printf("Rebooting...\n");
    esp_restart();
}

void serial_audio(int16_t *buf, size_t samp) {
    for (size_t i = 0; i < samp; i++) {
        Serial.write((int8_t)(buf[i] / 128));
    }
}

bool sound_task_stat;

void sound_task(void *arg) {
    player.init(&ftm);

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, &tx_handle, NULL);
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMP_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)PCM5102A_BCK,
            .ws = (gpio_num_t)PCM5102A_WS,
            .dout = (gpio_num_t)PCM5102A_DIN,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    i2s_channel_init_std_mode(tx_handle, &std_cfg);
    i2s_channel_enable(tx_handle);

    Serial.begin(921600);

    size_t writed;

    for (;;) {
        sound_task_stat = true;
        USB_MIDI.tick();
        player.process_tick();
        i2s_channel_write(tx_handle, player.get_buf(), player.get_buf_size_byte(), &writed, portMAX_DELAY);
        // serial_audio(player.get_buf(), player.get_buf_size());
        sound_task_stat = false;
        vTaskDelay(1);
    }
}

void start_fami_cmd(int argc, const char* argv[]) {
    player.start_play();
    printf("START SOUND\n");
}

void fast_test(int argc, const char* argv[]) {
    ftm.open_ftm("/flash/13 - Fearofdark - An Earth made of Molten Rock.ftm");
    ftm.read_ftm_all();
    start_fami_cmd(0, NULL);
}

void rm_cmd(int argc, const char* argv[]) {
    if (argc < 2) {
        printf("%s <filename>\n");
        return;
    }
    remove(argv[1]);
}

void format_fs(int argc, const char* argv[]) {
    printf("Formating...\n");
    esp_vfs_fat_spiflash_format_rw_wl("/flash", "spiffs");
}

extern int BASE_FREQ_HZ;

void set_base_freq_cmd(int argc, const char* argv[]) {
    if (argc < 2) {
        printf("%s <Hz>\n");
        return;
    }
    BASE_FREQ_HZ = strtol(argv[1], NULL, 0);
    printf("BASE FREQ SET TO -> %d\n", (int)BASE_FREQ_HZ);
}

extern bool _debug_print;

void set_debug_cmd(int argc, const char* argv[]) {
    _debug_print = !_debug_print;
}

void ssd1306_command_cmd(int argc, const char* argv[]) {
    if (argc < 2) {
        printf("%s <data>\n");
        return;
    }
    printf("SSD1306 COMMANDS: ");
    for (int i = 0; i < (argc - 1); i++) {
        printf("%02X ", strtol(argv[i + 1], NULL, 0));
    }
    printf("\n");
    printf("SEND...\n");
    display.ssd1306_command(0xE3); // NOP
    for (int i = 0; i < (argc - 1); i++) {
        display.ssd1306_command(strtol(argv[i + 1], NULL, 0));
    }
}

#include "free_heap.h"

void shell(void *arg) {
    SerialTerminal terminal;
    terminal.begin(115200, "Fami32");
    terminal.addCommand("ls", ls_entry);
    terminal.addCommand("rm", rm_cmd);
    terminal.addCommand("free", free_command);
    terminal.addCommand("reboot", reboot_cmd);
    terminal.addCommand("exit", reboot_cmd);
    terminal.addCommand("open_ftm", open_ftm_cmd);
    terminal.addCommand("read_ftm", read_ftm_cmd);
    terminal.addCommand("print_frame", print_frame_cmd);
    terminal.addCommand("start_fami", start_fami_cmd);
    terminal.addCommand("fast_test", fast_test);
    terminal.addCommand("format_fs", format_fs);
    terminal.addCommand("set_debug", set_debug_cmd);
    terminal.addCommand("set_base_freq", set_base_freq_cmd);
    terminal.addCommand("ssd1306", ssd1306_command_cmd);
    for (;;) {
        terminal.update();
        vTaskDelay(8);
    }
}

void keypad_task(void *arg) {
    touchKeypad.begin();
    for (;;) {
        keypad.tick();
        touchKeypad.tick();
        vTaskDelay(1);
    }
}

void set_brigness(uint8_t var) {
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(var);
}

const uint8_t bayerMatrix[4][4] = {
    {  0, 128,  32, 160 },
    { 192,  64, 224,  96 },
    {  48, 176,  16, 144 },
    { 240, 112, 208,  80 }
};

void midi_callback(midi_event_packed_t e) {
    set_channel_sel_pos(e.ch);
    if (e.event == MIDI_CIN_NOTE_ON) {
        player.channel[channel_sel_pos].set_inst(inst_sel_pos);
        player.channel[channel_sel_pos].set_note(e.note);
        player.channel[channel_sel_pos].set_vol(e.vol >> 3);
        player.channel[channel_sel_pos].note_start();
    } else if (e.event == MIDI_CIN_NOTE_OFF) {
        player.channel[channel_sel_pos].note_end();
    } else if (e.event == MIDI_CIN_CONTROL_CHANGE) {
        if (e.note == 0x20) {
            uint8_t set_prog = e.vol;
            if (set_prog >= ftm.inst_block.inst_num) {
                set_prog = ftm.inst_block.inst_num - 1;
            }
            inst_sel_pos = set_prog;
        }
    } else {
        player.channel[channel_sel_pos].note_cut();
        printf("UNKNOW USB MIDI EVENT: 0x%X (%d) -> DATA=%02X %02X\n", e.event, e.event, e.note, e.vol);
    }
}

void setup() {
    SPI.begin(DISPLAY_SCL, -1, DISPLAY_SDA);
    display.begin();
    display.clearDisplay();
    display.display();

    vTaskDelay(200);

    for (int i = 0; i < 32; i++) {
        display.fillScreen(1);
        display.drawBitmap(48, 0, fami32_logo, 32, 32, 0);
        display.setTextColor(0);
        display.setFont(&rismol57);
        display.setCursor(47, 33);
        display.print("FAMI32");
        display.setFont(&rismol35);
        display.setCursor(0, 0);
        display.printf("V%d.%d", FAMI32_VERSION, FAMI32_SUBVERSION);
        display.setCursor(0, 47);
        display.printf("libchara-dev\n%s %s", __DATE__, __TIME__);
        for (int x = 0; x < 128; x++) {
            for (int y = 0; y < 64; y++) {
                display.drawPixel(x, y, (display.getPixel(x, y) * (i * 8)) > bayerMatrix[y & 3][x & 3]);
            }
        }
        display.display();
    }

    esp_vfs_fat_mount_config_t fat_conf = {
        .format_if_mount_failed = true,
        .max_files = 1
    };
    wl_handle_t wl_handle;
    esp_err_t ret = esp_vfs_fat_spiflash_mount_rw_wl("/flash", "spiffs", &fat_conf, &wl_handle);
    printf("\nFATFS mount %d: %s\n", ret, esp_err_to_name(ret));

    if (read_config(config_path) != CONFIG_SUCCESS) {
        display.setCursor(0, 59);
        display.printf("Create new config...");
        display.display();
        printf("NO CONFIG FILE FOUND.\nCREATE IT...\n");
        if (set_config_value("SAMPLE_RATE", CONFIG_INT, &SAMP_RATE) == CONFIG_SUCCESS) {
            printf("Updated 'SAMPLE_RATE' to %d\n", SAMP_RATE);
        }
        if (set_config_value("ENGINE_SPEED", CONFIG_INT, &ENG_SPEED) == CONFIG_SUCCESS) {
            printf("Updated 'ENG_SPEED' to %d\n", ENG_SPEED);
        }
        if (set_config_value("LPF_CUTOFF", CONFIG_INT, &LPF_CUTOFF) == CONFIG_SUCCESS) {
            printf("Updated 'LPF_CUTOFF' to %d\n", LPF_CUTOFF);
        }
        if (set_config_value("HPF_CUTOFF", CONFIG_INT, &HPF_CUTOFF) == CONFIG_SUCCESS) {
            printf("Updated 'HPF_CUTOFF' to %d\n", HPF_CUTOFF);
        }
        if (set_config_value("BASE_FREQ_HZ", CONFIG_INT, &BASE_FREQ_HZ) == CONFIG_SUCCESS) {
            printf("Updated 'BASE_FREQ_HZ' to %d\n", BASE_FREQ_HZ);
        }
        if (set_config_value("OVER_SAMPLE", CONFIG_INT, &OVER_SAMPLE) == CONFIG_SUCCESS) {
            printf("Updated 'OVER_SAMPLE' to %d\n", OVER_SAMPLE);
        }
        if (set_config_value("VOLUME", CONFIG_INT, &g_vol) == CONFIG_SUCCESS) {
            printf("Updated 'VOLUME' to %d\n", g_vol);
        }
        if (write_config(config_path) != CONFIG_SUCCESS) {
            printf("Failed to write config file.\n");
        }
        esp_restart();
    }

    if (get_config_value("SAMPLE_RATE", CONFIG_INT, &SAMP_RATE) == CONFIG_SUCCESS) {
        printf("SAMPLE_RATE: %d\n", SAMP_RATE);
    }
    if (get_config_value("ENGINE_SPEED", CONFIG_INT, &ENG_SPEED) == CONFIG_SUCCESS) {
        printf("ENG_SPEED: %d\n", ENG_SPEED);
    }
    if (get_config_value("LPF_CUTOFF", CONFIG_INT, &LPF_CUTOFF) == CONFIG_SUCCESS) {
        printf("LPF_CUTOFF: %d\n", LPF_CUTOFF);
    }
    if (get_config_value("HPF_CUTOFF", CONFIG_INT, &HPF_CUTOFF) == CONFIG_SUCCESS) {
        printf("HPF_CUTOFF: %d\n", HPF_CUTOFF);
    }
    if (get_config_value("BASE_FREQ_HZ", CONFIG_INT, &BASE_FREQ_HZ) == CONFIG_SUCCESS) {
        printf("BASE_FREQ_HZ: %d\n", BASE_FREQ_HZ);
    }
    if (get_config_value("OVER_SAMPLE", CONFIG_INT, &OVER_SAMPLE) == CONFIG_SUCCESS) {
        printf("OVER_SAMPLE: %d\n", OVER_SAMPLE);
    }
    if (get_config_value("VOLUME", CONFIG_INT, &g_vol) == CONFIG_SUCCESS) {
        printf("VOLUME: %d\n", g_vol);
    }

    USB_MIDI.begin();
    USB_MIDI.onMidiEvent(midi_callback);

    display.setCursor(0, 59);
    display.printf("Press any key to continue...");
    display.display();

    keypad.begin();

    while (!keypad.available()) {
        keypad.tick();
        vTaskDelay(32);
    }
    keypad.read();

    xTaskCreate(sound_task, "SOUND TASK", 10240, NULL, 8, &SOUND_TASK_HD);
    xTaskCreate(keypad_task, "KEYPAD", 2048, NULL, 3, NULL);

    for (int i = 16; i > 0; i--) {
        display.fillScreen(1);
        display.drawBitmap(48, 0, fami32_logo, 32, 32, 0);
        display.setTextColor(0);
        display.setFont(&rismol57);
        display.setCursor(47, 33);
        display.print("FAMI32");
        display.setFont(&rismol35);
        display.setCursor(0, 0);
        display.printf("V%d.%d", FAMI32_VERSION, FAMI32_SUBVERSION);
        display.setCursor(0, 47);
        display.printf("libchara-dev\n%s %s", __DATE__, __TIME__);
        for (int x = 0; x < 128; x++) {
            for (int y = 0; y < 64; y++) {
                display.drawPixel(x, y, (display.getPixel(x, y) * (i * 16)) > bayerMatrix[y & 3][x & 3]);
            }
        }
        display.display();
    }

    display.setFont(&rismol35);
    display.setTextColor(1);

    xTaskCreate(gui_task, "GUI", 20480, NULL, 6, &GUI_TASK);
    xTaskCreate(shell, "SHELL", 8192, NULL, 10, NULL);
}

void loop() {
    vTaskDelete(NULL);
}