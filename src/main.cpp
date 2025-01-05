#include <Arduino.h>
#include "chipbox_pin.h"
#include <Adafruit_SSD1306.h>
#include <esp_littlefs.h>
#include "ftm_file.h"
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

void shell(void *arg) {
    SerialTerminal terminal;
    terminal.begin(115200, "Fami32");
    terminal.addCommand("ls", ls_entry);
    terminal.addCommand("reboot", reboot_cmd);
    terminal.addCommand("exit", reboot_cmd);
    terminal.addCommand("open_ftm", open_ftm_cmd);
    terminal.addCommand("read_ftm", read_ftm_cmd);
    terminal.addCommand("print_frame", print_frame_cmd);
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
    xTaskCreate(shell, "SHELL", 10240, NULL, 4, NULL);
}

void loop() {
    screenSaver(display);
    // vTaskDelete(NULL);
}