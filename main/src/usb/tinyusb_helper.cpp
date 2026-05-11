#include "tinyusb_helper.h"
#include "tinyusb_default_config.h"
#include "esp_log.h"

// String descriptors
static const char *usb_str_desc[] = {
    (const char[]){0x09, 0x04},
    "nyakoLab",
    "Fami32",
    "000721",
    "Fami32 USB Device",
};

bool init_tinyusb(const uint8_t *cfg_desc, size_t desc_len)
{
    (void)desc_len;

    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();

    tusb_cfg.descriptor.string = usb_str_desc;
    tusb_cfg.descriptor.string_count = sizeof(usb_str_desc) / sizeof(usb_str_desc[0]);
    tusb_cfg.descriptor.full_speed_config = cfg_desc;

    esp_err_t ret = tinyusb_driver_install(&tusb_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE("USB INIT", "TinyUSB init failed: %s", esp_err_to_name(ret));
        return false;
    }

    return true;
}