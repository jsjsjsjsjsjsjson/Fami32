#include "boot_check.h"
#include "fonts/rismol_3_5.h"
#include "fonts/rismol_5_7.h"
#include "../build/git_version.h"

#include <stdint.h>

const char* get_exc_cause_name(uint32_t exc_cause) {
    switch (exc_cause) {
        case 0: return "IllegalInstructionCause";
        case 1: return "SyscallCause";
        case 2: return "InstructionFetchErrorCause";
        case 3: return "LoadStoreErrorCause";
        case 4: return "Level1InterruptCause";
        case 5: return "AllocaCause";
        case 6: return "IntegerDivideByZeroCause";
        case 7: return "Reserved for Tensilica";
        case 8: return "PrivilegedCause";
        case 9: return "LoadStoreAlignmentCause";
        case 10:
        case 11: return "Reserved for Tensilica";
        case 12: return "InstrPIFDataErrorCause";
        case 13: return "LoadStorePIFDataErrorCause";
        case 14: return "InstrPIFAddrErrorCause";
        case 15: return "LoadStorePIFAddrErrorCause";
        case 16: return "InstTLBMissCause";
        case 17: return "InstTLBMultiHitCause";
        case 18: return "InstFetchPrivilegeCause";
        case 19: return "Reserved for Tensilica";
        case 20: return "InstFetchProhibitedCause";
        case 21:
        case 22:
        case 23: return "Reserved for Tensilica";
        case 24: return "LoadStoreTLBMissCause";
        case 25: return "LoadStoreTLBMultiHitCause";
        case 26: return "LoadStorePrivilegeCause";
        case 27: return "Reserved for Tensilica";
        case 28: return "LoadProhibitedCause";
        case 29: return "StoreProhibitedCause";
        case 30:
        case 31: return "Reserved for Tensilica";
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39: return "CoprocessornDisabled";
        default: return "Reserved";
    }
}

void show_check_info(Adafruit_SSD1306 *display, Adafruit_Keypad *keypad) {
    display->setFont(&rismol57);
    display->fillRect(0, 0, 128, 9, 1);
    display->setTextColor(0);
    display->setCursor(1, 1);
    display->printf("FAMI32 CRASH!");
    esp_core_dump_summary_t summary;
    esp_core_dump_get_summary(&summary);

    display->setFont(&rismol35);
    display->setTextColor(1);
    display->setCursor(0, 11);

    display->println("INFO:");
    display->printf("TASK \"%s\" @ 0x%08lX\n", summary.exc_task, summary.exc_pc);
    display->printf("EXCVADDR: 0x%08lX\n", summary.ex_info.exc_vaddr);
    display->printf("CAUSE: %s\n", get_exc_cause_name(summary.ex_info.exc_cause));

    display->setCursor(display->getCursorX(), display->getCursorY() + 2);

    if (summary.exc_bt_info.depth > 0) {
        display->printf("Backtrace:\n");
        for (int i = 0; i < summary.exc_bt_info.depth; i++) {
            display->printf("0x%08lX ", summary.exc_bt_info.bt[i]);
        }
    }

    display->setCursor(0, 58);
    display->printf("SHA: %s (FM32-%s)", summary.app_elf_sha256, get_version_string());
    display->display();

    keypad->begin();

    while (!keypad->available()) {
        keypad->tick();
        vTaskDelay(2);
    }
    display->clearDisplay();
    display->setFont(&rismol57);
    display->setCursor(0, 0);
    display->print("REBOOT...");
    display->display();
    esp_restart();
}

int boot_check() {
    esp_reset_reason_t reason = esp_reset_reason();

    switch (reason) {
        case ESP_RST_PANIC:
            return -1;
            break;
        case ESP_RST_SW:
        case ESP_RST_POWERON:
        case ESP_RST_BROWNOUT:
        case ESP_RST_DEEPSLEEP:
            return 0;
            break;
        default:
            return 1;
            break;
    }
}