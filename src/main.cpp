#include <Arduino.h>
#include <Adafruit_SSD1306.h>
// #include <esp_littlefs.h>
#include <esp_vfs_fat.h>
#include <driver/i2s_std.h>
#include "chipbox_pin.h"
#include "ftm_file.h"
#include "tracker.h"
#include "dirent.h"
#include "SerialTerminal.h"

extern "C" {
#include "ls.h"
}

FAMI_CHANNEL channel;
FAMI_PLAYER player;

TaskHandle_t SOUND_TASK_HD = NULL;
TaskHandle_t OSC_TASK = NULL;

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

void serial_audio(int16_t *buf, size_t samp) {
    for (size_t i = 0; i < samp; i++) {
        Serial.write((int8_t)(buf[i] / 128));
    }
}

void sound_task(void *arg) {
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

    vTaskSuspend(NULL);
    size_t writed;
    player.init(&ftm);
    vTaskResume(OSC_TASK);
    Serial.begin(921600);

    for (;;) {
        player.process_tick();
        i2s_channel_write(tx_handle, player.get_buf(), player.get_buf_size_byte(), &writed, portMAX_DELAY);
        serial_audio(player.get_buf(), player.get_buf_size());
        vTaskDelay(1);
    }
}

void start_fami_cmd(int argc, const char* argv[]) {
    vTaskResume(SOUND_TASK_HD);
    printf("START SOUND\n");
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

void fast_test(int argc, const char* argv[]) {
    ftm.open_ftm("/flash/11 - gyms - Magmo's Magical Ingot.ftm");
    ftm.read_ftm_all();
    start_fami_cmd(0, NULL);
    // srand(time(NULL));
}

void osc_task(void *arg) {
    display.fillScreen(1);
    display.setTextSize(2);
    display.setTextColor(0);
    display.setCursor(1, 1);
    display.printf("Fami32");
    display.setCursor(1, 18);
    display.setTextSize(1);
    display.printf("%s %s\n READY.", __DATE__, __TIME__);
    display.setTextColor(1);
    display.display();
    vTaskSuspend(NULL);
    for (;;) {
        display.clearDisplay();
        for (uint8_t i = 0; i < 4; i++) {
            display.fillRect(0, i * 16, player.get_chl_vol(i)*2, 8, 1);
            display.setCursor(64, i * 16);
            if (i == 3)
                display.printf("%02d   0%X\n", player.get_chl_vol(i), (int)player.channel[i].get_noise_rate());
            else
                display.printf("%02d  %03X\n", player.get_chl_vol(i), (int)player.channel[i].get_period());
            
            display.fillRect(0, (i * 16) + 8, player.get_chl_env_vol(i)*2, 8, 1);
            display.setCursor(64, (i * 16) + 8);
            if (i == 3)
                display.printf("%02d   %s\n", player.get_chl_env_vol(i), player.channel[i].get_mode() == NOISE1 ? "N1" : "N0");
            else
                display.printf("%02d   %02d\n", player.get_chl_env_vol(i), player.channel[i].get_mode());
        }
        for (uint8_t x = 0; x < 128; x++) {
            display.drawPixel(x, (player.get_buf()[x * 8] / 128) + 31, 1);
        }
        display.display();
        vTaskDelay(2);
    }
}

void osc_cmd(int argc, const char* argv[]) {
    vTaskResume(OSC_TASK);
    // Serial.read();
    // for (;;) {
    //     display.clearDisplay();
    //     for (uint8_t x = 0; x < 128; x++) {
    //         display.drawPixel(x, (channel.get_buf()[x * 8] / 512) + 31, 1);
    //     }
    //     display.display();
    //     vTaskDelay(4);
    //     if (Serial.available()) {
    //         printf("\n");
    //         Serial.read();
    //         break;
    //     }
    // }
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

#include "free_heap.h"

void shell(void *arg) {
    SerialTerminal terminal;
    terminal.begin(115200, "Fami32");
    terminal.addCommand("ls", ls_entry);
    terminal.addCommand("rm", rm_cmd);
    terminal.addCommand("free", free_command);
    terminal.addCommand("reboot", reboot_cmd);
    terminal.addCommand("exit", reboot_cmd);
    terminal.addCommand("osc", osc_cmd);
    terminal.addCommand("open_ftm", open_ftm_cmd);
    terminal.addCommand("read_ftm", read_ftm_cmd);
    terminal.addCommand("print_frame", print_frame_cmd);
    terminal.addCommand("start_fami", start_fami_cmd);
    terminal.addCommand("set_inst", set_inst_cmd);
    terminal.addCommand("start_note", start_note_cmd);
    terminal.addCommand("end_note", end_note_cmd);
    terminal.addCommand("fast_test", fast_test);
    terminal.addCommand("format_fs", format_fs);
    for (;;) {
        terminal.update();
        vTaskDelay(1);
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
    esp_err_t ret = esp_vfs_fat_spiflash_mount("/flash", "spiffs", &fat_conf, &wl_handle);
    printf("\nFATFS mount %d: %s\n", ret, esp_err_to_name(ret));
    vTaskDelay(64);
    xTaskCreate(sound_task, "SOUND TASK", 10240, NULL, 5, &SOUND_TASK_HD);
    xTaskCreate(shell, "SHELL", 4096, NULL, 3, NULL);
    xTaskCreate(osc_task, "OSC", 2048, NULL, 4, &OSC_TASK);
}

void loop() {
    vTaskDelete(NULL);
}