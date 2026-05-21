#include "PowerManagement.h"

#include "esp_task_wdt.h"

void PrintLastShutDownReason()
{
    const esp_reset_reason_t reason = esp_reset_reason();
    switch (reason)
    {
    case ESP_RST_UNKNOWN:
        printf("Reset reason can not be determined\n");
        break;
    case ESP_RST_POWERON:
        printf("Reset due to power-on event\n");
        break;
    case ESP_RST_EXT:
        printf("Reset by external pin (not applicable for ESP32)\n");
        break;
    case ESP_RST_SW:
        printf("Software reset via esp_restart\n");
        break;
    case ESP_RST_PANIC:
        printf("Software reset due to exception/panic\n");
        break;
    case ESP_RST_INT_WDT:
        printf("Reset (software or hardware) due to interrupt watchdog\n");
        break;
    case ESP_RST_TASK_WDT:
        printf("Reset due to task watchdog\n");
        break;
    case ESP_RST_WDT:
        printf("Reset due to other watchdogs\n");
        break;
    case ESP_RST_DEEPSLEEP:
        printf("Reset after exiting deep sleep mode\n");
        break;
    case ESP_RST_BROWNOUT:
        printf("Brownout reset (software or hardware)\n");
        break;
    case ESP_RST_SDIO:
        printf("Reset over SDIO\n");
        break;
    case ESP_RST_USB:
        printf("Reset by USB peripheral\n");
        break;
    case ESP_RST_JTAG:
        printf("Reset by JTAG\n");
        break;
    case ESP_RST_EFUSE:
        printf("Reset due to efuse error\n");
        break;
    case ESP_RST_PWR_GLITCH:
        printf("Reset due to power glitch detected\n");
        break;
    case ESP_RST_CPU_LOCKUP:
        printf("Reset due to CPU lock up (double exception)\n");
        break;
    default:
        break;
    }
}
