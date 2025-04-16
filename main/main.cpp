#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Keypad.h>
#include <MPR121_Keypad.h>

#include <driver/i2s_std.h>
#include "fami32_pin.h"
#include "ftm_file.h"
#include "fami32_player.h"
#include <dirent.h>
#include <USBMIDI.h>
#include "gui/gui_common.h"
#include "gui/gui_input.h"
#include "tinyusb.h"
#include "tusb_msc_storage.h"
#include "boot_check.h"

bool _debug_print = false;
bool _midi_output = false;
bool edit_mode = false;

int g_vol = 16;

extern "C" {
#include "micro_config.h"
}

FAMI_PLAYER player;
i2s_chan_handle_t tx_handle;
TaskHandle_t SOUND_TASK_HD = NULL;
TaskHandle_t GUI_TASK = NULL;
USBMIDI MIDI;
Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &SPI, DISPLAY_DC, DISPLAY_RESET, DISPLAY_CS);
Adafruit_Keypad keypad = Adafruit_Keypad(makeKeymap(KEYPAD_MAP),
                                        (uint8_t[KEYPAD_ROWS]){KEYPAD_R0, KEYPAD_R1, KEYPAD_R2, KEYPAD_R3},
                                        (uint8_t[KEYPAD_COLS]){KEYPAD_C0, KEYPAD_C1, KEYPAD_C2},
                                        KEYPAD_ROWS, KEYPAD_COLS);
MPR121_Keypad touchKeypad(TOUCHPAD0_ADDRS, TOUCHPAD1_ADDRS);

bool sound_task_stat;

void sound_task(void *arg) {
    player.init(&ftm);

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, &tx_handle, NULL);
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG((uint32_t)SAMP_RATE),
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

    // Serial.begin(921600);

    size_t writed;

    for (;;) {
        sound_task_stat = true;
        player.process_tick();
        i2s_channel_write(tx_handle, player.get_buf(), player.get_buf_size_byte(), &writed, portMAX_DELAY);
        // serial_audio(player.get_buf(), player.get_buf_size());
        sound_task_stat = false;
        vTaskDelay(1);
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

void midi_callback(midiEventPacket_t packet) {
    static midiEventData_t e;
    memcpy(&e, &packet, 4);
    // ESP_LOGI("MIDI_CALLBACK", "%X%X %X%X %02X %02X", e.cn, e.cin, e.ch, e.event, e.note, e.vol);
    set_channel_sel_pos(e.ch);
    if (e.event == MIDI_CIN_NOTE_ON) {
        player.channel[channel_sel_pos].set_inst(inst_sel_pos);
        player.channel[channel_sel_pos].set_note(e.note);
        player.channel[channel_sel_pos].set_vol(e.vol >> 3);
        player.channel[channel_sel_pos].note_start();
        if (edit_mode) {
            unpk_item_t pt_tmp = ftm.get_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row());
            pt_tmp.note = (e.note % 12) + 1;
            pt_tmp.octave = (e.note / 12) - 2;
            pt_tmp.instrument = inst_sel_pos;
            pt_tmp.volume = e.vol >> 3;
            ftm.set_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row(), pt_tmp);
            if (!player.get_play_status()) {
                player.set_row(player.get_row() + 1);
            }
        }
    } else if (e.event == MIDI_CIN_NOTE_OFF) {
        player.channel[channel_sel_pos].note_end();
        if (edit_mode) {
            if (player.get_play_status()) {
                unpk_item_t pt_tmp = ftm.get_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row());
                pt_tmp.note = NOTE_END;
                pt_tmp.instrument = NO_INST;
                pt_tmp.octave = NO_OCT;
                ftm.set_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row(), pt_tmp);
            }
        }
    } else if (e.event == MIDI_CIN_CONTROL_CHANGE) {
        if (e.note == 0x20) {
            uint8_t set_prog = e.vol;
            if (set_prog >= ftm.inst_block.inst_num) {
                set_prog = ftm.inst_block.inst_num - 1;
            }
            inst_sel_pos = set_prog;
        } if (e.note == 0x7B) {
            player.channel[e.ch % 5].note_cut();
            printf("ALL NOTES OFF IN CHANNEL%d\n", e.ch % 5);
        } else {
            printf("UNKNOW CC: CMD=%02X, PARAM=%02X\n", e.note, e.vol);
        }
    } else if (e.event == MIDI_CIN_PITCH_BEND_CHANGE) {
        BASE_FREQ_HZ = 440 + (1024 - (((e.vol << 8) | e.note) / 16));
        printf("FINETUNE: %d, PITCH_BEND: 0x%04X\n", BASE_FREQ_HZ, (e.vol << 8) | e.note);
    } else {
        player.channel[channel_sel_pos].note_cut();
        printf("UNKNOW USB MIDI EVENT: 0x%X (%d) -> DATA=%02X %02X\n", e.event, e.event, e.note, e.vol);
    }
}

static esp_err_t storage_init_spiflash(wl_handle_t *wl_handle)
{
    ESP_LOGI("USB_MSC", "Initializing wear levelling");

    const esp_partition_t *data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, NULL);
    if (data_partition == NULL) {
        ESP_LOGE("USB_MSC", "Failed to find FATFS partition. Check the partition table.");
        return ESP_ERR_NOT_FOUND;
    }

    return wl_mount(data_partition, wl_handle);
}

// String descriptors
const char *s_str_desc[5] = {
    (char[]){0x09, 0x04},  // Supported language
    "nyakoLab",            // Manufacturer
    "Fami32",              // Product
    "000721",              // Serial number
    "Fami32 Data and MIDI",
};

// Configuration descriptor
const uint8_t usb_cfg_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, 2, 0, TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN + TUD_MIDI_DESC_LEN, 0, 100),
    TUD_MSC_DESCRIPTOR(0, 0, 0x01, 0x81, 64), // EP1 OUT, EP1 IN
    TUD_MIDI_DESCRIPTOR(1, 0, 0x02, 0x82, 64), // EP2 OUT, EP2 IN
};

bool init_tinyusb() {
    // Initialize TinyUSB
    tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = s_str_desc,
        .string_descriptor_count = sizeof(s_str_desc) / sizeof(s_str_desc[0]),
        .external_phy = false,
        .configuration_descriptor = usb_cfg_desc,
    };
    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE("USB INIT", "Failed to initialize TinyUSB: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

void app_main_cpp() {
    SPI.begin((gpio_num_t)DISPLAY_SCL, (gpio_num_t)-1, (gpio_num_t)DISPLAY_SDA);
    display.begin();
    display.clearDisplay();

    if (boot_check()) {
        show_check_info(&display, &keypad, FAMI32_VERSION, FAMI32_SUBVERSION);
    }

    vTaskDelay(128);

    for (int i = 0; i < 32; i++) {
        display.fillScreen(1);
        display.drawBitmap(48, 1, fami32_logo, 32, 32, 0);
        display.setTextColor(0);
        display.setFont(&rismol57);
        display.setCursor(47, 33);
        display.print("FAMI32");
        display.setFont(&rismol35);
        display.setCursor(0, 0);
        display.printf("V%d.%d-IDF", FAMI32_VERSION, FAMI32_SUBVERSION);
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
    wl_handle_t wl_flash_handle;
    esp_err_t ret = esp_vfs_fat_spiflash_mount_rw_wl("/flash", "flash", &fat_conf, &wl_flash_handle);
    printf("\nFatFS mount %d: %s\n", ret, esp_err_to_name(ret));

    if (read_config(config_path) != CONFIG_SUCCESS) {
        display.setCursor(0, 59);
        display.printf("Create new config...");
        display.display();
        printf("NO CONFIG FILE FOUND.\nCREATE IT...\n");
        set_config_value("SAMPLE_RATE", CONFIG_INT, &SAMP_RATE);
        set_config_value("ENGINE_SPEED", CONFIG_INT, &ENG_SPEED);
        set_config_value("LPF_CUTOFF", CONFIG_INT, &LPF_CUTOFF);
        set_config_value("HPF_CUTOFF", CONFIG_INT, &HPF_CUTOFF);
        set_config_value("BASE_FREQ_HZ", CONFIG_INT, &BASE_FREQ_HZ);
        set_config_value("OVER_SAMPLE", CONFIG_INT, &OVER_SAMPLE);
        set_config_value("VOLUME", CONFIG_INT, &g_vol);
        if (write_config(config_path) != CONFIG_SUCCESS) {
            printf("Failed to write config file.\n");
        }
        esp_restart();
    }

    get_config_value("SAMPLE_RATE", CONFIG_INT, &SAMP_RATE);
    get_config_value("ENGINE_SPEED", CONFIG_INT, &ENG_SPEED);
    get_config_value("LPF_CUTOFF", CONFIG_INT, &LPF_CUTOFF);
    get_config_value("HPF_CUTOFF", CONFIG_INT, &HPF_CUTOFF);
    get_config_value("BASE_FREQ_HZ", CONFIG_INT, &BASE_FREQ_HZ);
    get_config_value("OVER_SAMPLE", CONFIG_INT, &OVER_SAMPLE);
    get_config_value("VOLUME", CONFIG_INT, &g_vol);

    wl_handle_t wl_usbmsc_handle;
    ESP_ERROR_CHECK(storage_init_spiflash(&wl_usbmsc_handle));
    const tinyusb_msc_spiflash_config_t config_spi = {
        .wl_handle = wl_usbmsc_handle,
        .mount_config = {.max_files = 5}
    };
    ESP_ERROR_CHECK(tinyusb_msc_storage_init_spiflash(&config_spi));
    init_tinyusb();
    MIDI.begin();

    display.setCursor(0, 59);
    display.printf("Press any key to continue...");
    display.display();

    keypad.begin();

    while (!keypad.available()) {
        keypad.tick();
        vTaskDelay(32);
    }
    keypad.read();

    xTaskCreatePinnedToCore(sound_task, "SOUND TASK", 8192, NULL, 8, &SOUND_TASK_HD, 0);
    xTaskCreatePinnedToCore(keypad_task, "KEYPAD", 4096, NULL, 3, NULL, 1);

    MIDI.setCallback(midi_callback);

    for (int i = 16; i > 0; i--) {
        display.fillScreen(1);
        display.drawBitmap(48, 1, fami32_logo, 32, 32, 0);
        display.setTextColor(0);
        display.setFont(&rismol57);
        display.setCursor(47, 33);
        display.print("FAMI32");
        display.setFont(&rismol35);
        display.setCursor(0, 0);
        display.printf("V%d.%d-IDF", FAMI32_VERSION, FAMI32_SUBVERSION);
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

    xTaskCreatePinnedToCore(gui_task, "GUI", 20480, NULL, 7, &GUI_TASK, 1);
}

extern "C" void app_main(void) { app_main_cpp(); }