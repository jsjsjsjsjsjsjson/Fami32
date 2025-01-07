#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <esp_littlefs.h>
#include <driver/i2s_std.h>
#include "chipbox_pin.h"
#include "ftm_file.h"
#include "tracker.h"
#include "dirent.h"
#include "SerialTerminal.h"

extern "C" {
#include "ls.h"
}

Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &SPI, DISPLAY_DC, DISPLAY_RESET, DISPLAY_CS);

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

FAMI_CHANNEL channel;

void sound(void *arg) {
    channel.init(&ftm);
    i2s_chan_handle_t tx_handle;
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

    size_t writed;

    for (;;) {
        channel.make_tick_sound();
        i2s_channel_write(tx_handle, channel.get_buf(), channel.get_buf_size(), &writed, portMAX_DELAY);
    }
}

void start_fami_cmd(int argc, const char* argv[]) {
    printf("START_SOUND: %d\n", xTaskCreate(sound, "SOUND", 8192, NULL, 5, NULL));
}

void start_note_cmd(int argc, const char* argv[]) {
    if (argc < 2) {
        printf("%s <note>\n", argv[0]);
        return;
    }
    channel.set_note(strtol(argv[1], NULL, 0));
    channel.note_start();
}

void end_note_cmd(int argc, const char* argv[]) {
    channel.note_end();
}

void set_inst_cmd(int argc, const char* argv[]) {
    if (argc < 2) {
        printf("%s <inst>\n", argv[0]);
        return;
    }
    channel.set_inst(strtol(argv[1], NULL, 0));
}

void shell(void *arg) {
    SerialTerminal terminal;
    terminal.begin(115200, "Fami32");
    terminal.addCommand("ls", ls_entry);
    terminal.addCommand("reboot", reboot_cmd);
    terminal.addCommand("exit", reboot_cmd);
    terminal.addCommand("open_ftm", open_ftm_cmd);
    terminal.addCommand("read_ftm", read_ftm_cmd);
    terminal.addCommand("print_frame", print_frame_cmd);
    terminal.addCommand("start_fami", start_fami_cmd);
    terminal.addCommand("set_inst", set_inst_cmd);
    terminal.addCommand("start_note", start_note_cmd);
    terminal.addCommand("end_note", end_note_cmd);
    for (;;) {
        terminal.update();
        vTaskDelay(1);
    }
}

void setup() {
    SPI.begin(DISPLAY_SCL, -1, DISPLAY_SDA);
    display.begin();
    display.display();
    esp_vfs_littlefs_conf_t littlefs_conf = {
        .base_path = "/flash",
        .partition_label = "spiffs",
        .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_littlefs_register(&littlefs_conf);
    if (ret == ESP_OK) {
        printf("\nLittleFS mounted!\n");
        display.printf("LittleFS mounted!\n");
        display.display();
    }
    xTaskCreate(shell, "SHELL", 10240, NULL, 3, NULL);
}

void loop() {
    for (uint8_t y = 0; y < 64; y++) {
        for (uint8_t x = 0; x < 128; x++) {
            display.drawPixel(x, y, rand() & 1);
        }
    }
    display.display();
    // vTaskDelay(1);
    // vTaskDelete(NULL);
}