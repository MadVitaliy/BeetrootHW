#include <cstdio>
#include <cinttypes>
#include <cstring>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "esp_log.h"
#include "tasks/ButtonTask.hpp"
#include "tasks/LedTask.hpp"
#include "tasks/UartTask.hpp"

static const char *TAG = "MAIN";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Initializing system...");

    // 1. Start LED Task first (returns handle)
    TaskHandle_t led_handle = StartLedTask();

    // 2. Start UART Tasks, passing the LED handle to ReadCmdTask
    UartTaskHandles uart_handles = StartUartTasks(led_handle);

    // 3. Start Button Task, passing the SendCmd handle to ButtonTask
    StartButtonTask(uart_handles.send_task);

    ESP_LOGI(TAG, "System running!");
    // app_main returns naturally here; FreeRTOS scheduler handles tasks independently.
}