#ifndef BOOT_ROUTER_H
#define BOOT_ROUTER_H

#include "esp_system.h"

__NOINIT_ATTR enum BOOT_MODE {
    USB_MSC,
    USB_AUDIO,
};

void boot_router_set_mode(BOOT_MODE m);
void boot_router();

#endif