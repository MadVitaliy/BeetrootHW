#include "LedTask.hpp"
#include "led_strip.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "LED_TASK";
constexpr gpio_num_t G_LED_STRIP_GPIO = GPIO_NUM_48;
constexpr size_t G_LEDS_QUANTITY = 1;

static led_strip_handle_t s_led_strip = NULL;

static void init_led_strip(void) {
    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = G_LED_STRIP_GPIO;
    strip_config.max_leds = G_LEDS_QUANTITY;
    strip_config.led_model = LED_MODEL_WS2812;
    strip_config.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB;

    led_strip_rmt_config_t rmt_config = {};
    rmt_config.clk_src = RMT_CLK_SRC_DEFAULT;
    rmt_config.resolution_hz = 10 * 1000 * 1000;

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &s_led_strip));
    led_strip_clear(s_led_strip);
}

static void turn_on_purple(void) {
    if (s_led_strip) {
        led_strip_set_pixel(s_led_strip, 0, 128, 0, 128);
        led_strip_refresh(s_led_strip);
    }
}

static void turn_off(void) {
    if (s_led_strip) {
        led_strip_clear(s_led_strip);
    }
}

static void led_task_entry(void *param) {
    bool led_is_shining = false;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (led_is_shining) {
            turn_off();
        } else {
            turn_on_purple();
        }
        led_is_shining = !led_is_shining;
    }
}

TaskHandle_t StartLedTask(void) {
    init_led_strip();
    TaskHandle_t handle = NULL;
    xTaskCreate(led_task_entry, "LedTask", 2048, NULL, 1, &handle);
    return handle;
}
