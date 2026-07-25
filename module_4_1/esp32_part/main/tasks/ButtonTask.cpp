#include "ButtonTask.hpp"
#include "Button.h"
#include "esp_log.h"

static const char *TAG = "BUTTON_TASK";
constexpr gpio_num_t G_BUTTON_GPIO = GPIO_NUM_7;

static void button_task_entry(void *parameter) {
    TaskHandle_t send_cmd_task_handle = (TaskHandle_t)parameter;
    
    constexpr bool button_is_pulled_down = false;
    constexpr bool use_builtin_resistor = true;
    Button button(G_BUTTON_GPIO, button_is_pulled_down, use_builtin_resistor);
    button.Init();

    for (;;) {
        button.Update();
        if (button.WasPressed()) {
            if (send_cmd_task_handle != NULL) {
                xTaskNotifyGive(send_cmd_task_handle);
            }
            ESP_LOGI(TAG, "Button pressed");
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

void StartButtonTask(TaskHandle_t send_cmd_task_handle) {
    xTaskCreate(button_task_entry, "ButtonTask", 2048, (void*)send_cmd_task_handle, 1, NULL);
}
