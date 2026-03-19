#include "boot_router.h"
#include "usb_msc_mode.h"
#include "esp_system.h"

__NOINIT_ATTR uint32_t need_router;
__NOINIT_ATTR BOOT_MODE mode;

void boot_router_set_mode(BOOT_MODE m) {
    need_router = 0xFFFFFFFF;
    mode = m;
    esp_restart();
}

void boot_router() {
    if (need_router == 0xFFFFFFFF) {
        need_router = 0;
        printf("Need boot router, mode = %d.\n", mode);
        switch (mode)
        {
        case USB_MSC:
            msc_mode();
            break;
        
        case USB_AUDIO:
            
            break;

        default:
            printf("Unknow boot mode.\n");
            break;
        }
    }
}