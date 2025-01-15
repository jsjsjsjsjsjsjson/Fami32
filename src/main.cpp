#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Keypad.h>
// #include <esp_littlefs.h>
#include <esp_vfs_fat.h>
#include <driver/i2s_std.h>
#include "chipbox_pin.h"
#include "ftm_file.h"
#include "tracker.h"
#include "dirent.h"
#include "SerialTerminal.h"
#include "gui.h"

extern "C" {
#include "ls.h"
}

FAMI_PLAYER player;

TaskHandle_t SOUND_TASK_HD = NULL;

Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &SPI, DISPLAY_DC, DISPLAY_RESET, DISPLAY_CS);

Adafruit_Keypad keypad = Adafruit_Keypad(makeKeymap(KEYPAD_MAP),
                                        (byte[KEYPAD_ROWS]){KEYPAD_R0, KEYPAD_R1, KEYPAD_R2, KEYPAD_R3},
                                        (byte[KEYPAD_COLS]){KEYPAD_C0, KEYPAD_C1, KEYPAD_C2},
                                        KEYPAD_ROWS, KEYPAD_COLS);

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

void sound_task(void *arg) {
    player.init(&ftm);

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
        player.process_tick();
        i2s_channel_write(tx_handle, player.get_buf(), player.get_buf_size_byte(), &writed, portMAX_DELAY);
        // serial_audio(player.get_buf(), player.get_buf_size());
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

extern float BASE_FREQ_HZ;

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
    for (;;) {
        terminal.update();
        vTaskDelay(2);
    }
}

void keypad_task(void *arg) {
    keypad.begin();
    for (;;) {
        keypad.tick();
        vTaskDelay(4);
    }
}

void setup() {
    SPI.begin(DISPLAY_SCL, -1, DISPLAY_SDA);
    display.begin();
    display.display();
    esp_vfs_fat_mount_config_t fat_conf = {
        .format_if_mount_failed = true,
        .max_files = 4
    };
    wl_handle_t wl_handle;
    esp_err_t ret = esp_vfs_fat_spiflash_mount_rw_wl("/flash", "spiffs", &fat_conf, &wl_handle);
    printf("\nFATFS mount %d: %s\n", ret, esp_err_to_name(ret));
    vTaskDelay(64);
    xTaskCreate(shell, "SHELL", 4096, NULL, 4, NULL);
    xTaskCreate(sound_task, "SOUND TASK", 10240, NULL, 5, &SOUND_TASK_HD);
    xTaskCreate(keypad_task, "KEYPAD", 1024, NULL, 2, NULL);
    xTaskCreate(gui_task, "GUI", 10240, NULL, 3, NULL); // &OSC_TASK);
}

void loop() {
    vTaskDelete(NULL);
}