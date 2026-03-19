#include "usb_msc_mode.h"
#include <gfx_oled_ssd1306.h>
#include <Adafruit_Keypad.h>
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "wear_levelling.h"

#include "fami32_icon.h"
#include "fonts/rismol_5_7.h"
#include "fonts/rismol_3_5.h"

extern GfxOledSSD1306 display;

__NOINIT_ATTR uint32_t boot_mode;

#define CONFIG_TOTAL_LEN_MSC  (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

TU_ATTR_ALIGNED(4)
static const uint8_t usb_cfg_desc_msc[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN_MSC, 0x80, 100),
    TUD_MSC_DESCRIPTOR(0, 4, 0x01, 0x81, 64),
};

static tinyusb_msc_storage_handle_t s_msc_storage = NULL;

static esp_err_t storage_init_spiflash(wl_handle_t *wl_handle) {
    ESP_LOGI("USB_MSC", "Initializing wear levelling");

    const esp_partition_t *data_partition =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                 ESP_PARTITION_SUBTYPE_DATA_FAT,
                                 NULL);

    if (data_partition == NULL) {
        ESP_LOGE("USB_MSC", "Failed to find FATFS partition. Check the partition table.");
        return ESP_ERR_NOT_FOUND;
    }

    return wl_mount(data_partition, wl_handle);
}

extern Adafruit_Keypad keypad;

void msc_mode(void) {
    wl_handle_t wl_usbmsc_handle;
    ESP_ERROR_CHECK(storage_init_spiflash(&wl_usbmsc_handle));

    const tinyusb_msc_storage_config_t storage_cfg = {
        .medium = {
            .wl_handle = wl_usbmsc_handle,
        },
    };

    ESP_ERROR_CHECK(
        tinyusb_msc_new_storage_spiflash(&storage_cfg, &s_msc_storage)
    );

    if (!init_tinyusb(usb_cfg_desc_msc, sizeof(usb_cfg_desc_msc))) {
        ESP_LOGE("USB_MSC", "Failed to initialize TinyUSB");
        return;
    }

    uint8_t t = 0;
    uint8_t dot = 0;
    display.setTextColor(1);
    while (!keypad.available()) {
        display.clearDisplay();
        display.drawBitmap(19, 18, usb_msc_icon[t], 90, 28, 1);
        t++;
        if (t >= 7) {
            t = 0;
            dot = (dot + 1) & 3;
        }
        display.setFont(&rismol57);
        display.setCursor(0, 0);
        display.print("USB MSC MODE");
        for (int i = 0; i < dot; i++) {
            display.print(".");
        }
        display.setFont(&rismol35);
        display.setCursor(0, 59);
        display.print("Press any key to exit...");
        display.display();

        keypad.tick();
        vTaskDelay(80);
    }

    display.clearDisplay();
    display.setFont(&rismol57);
    display.setCursor(0, 0);
    display.print("REBOOT...");
    display.display();
    esp_restart();
}