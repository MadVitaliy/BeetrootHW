#include "ButtonTask.hpp"
#include "Button.h"
#include "esp_log.h"

static const char *TAG = "BUTTON_TASK";
constexpr gpio_num_t G_BUTTON_GPIO = GPIO_NUM_7;

bool G_STM32_LED_STATUS = false;

static void button_task_entry(void *parameter) {
    TaskHandle_t send_cmd_task_handle = (TaskHandle_t)parameter;
    
    constexpr bool button_is_pulled_down = false;
    constexpr bool use_builtin_resistor = true;
    Button button(G_BUTTON_GPIO, button_is_pulled_down, use_builtin_resistor);
    button.Init();

    for (;;) {
        button.Update();
        if (button.WasPressed()) {
            ESP_LOGI(TAG, "Button pressed");
            G_STM32_LED_STATUS = !G_STM32_LED_STATUS;
            /*
              It used to be:
                if (send_cmd_task_handle != NULL) {
                    xTaskNotifyGive(send_cmd_task_handle);
                }
                ESP_LOGI(TAG, "Button pressed");
              But since now ESP32 is a modbus slave davice and can't initiate communication.
              A master device periodically request the value of G_STM32_LED_STATUS, instead.
            */
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

void StartButtonTask(TaskHandle_t send_cmd_task_handle) {
    xTaskCreate(button_task_entry, "ButtonTask", 2048, (void*)send_cmd_task_handle, 1, NULL);
}
