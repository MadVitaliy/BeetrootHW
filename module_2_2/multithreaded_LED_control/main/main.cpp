#include <cstdio>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Settings
constexpr uint32_t SLEEP_TIME_MS = 1000;

#define ADC_PIN ADC_CHANNEL_5        // Channel 7 - Check ESP32 Pinout for the GPIO Number
#define ADC_UNIT ADC_UNIT_1          // ADC1
#define ADC_BITWIDTH ADC_BITWIDTH_12 // 12-bit resolution (0-4095)
#define ADC_ATTEN ADC_ATTEN_DB_12    // ~3.3V full-scale voltage

constexpr gpio_num_t G_LED_PIN = GPIO_NUM_4;
constexpr ledc_mode_t G_PWM_MODE = LEDC_LOW_SPEED_MODE;
constexpr ledc_channel_t G_PWM_CHANNEL = LEDC_CHANNEL_0;
constexpr uint8_t G_PWM_RESOLUTION = 12; // bits (1–20 on ESP32)
constexpr uint32_t G_PWM_MAX_VALUE = (1 << G_PWM_RESOLUTION) - 1;
constexpr uint32_t G_PWM_MIN_VALUE = G_PWM_MAX_VALUE * 0.3; //

constexpr uint32_t G_PWM_FREQ = 5000;
constexpr int G_HPOINT = 0;

TaskHandle_t G_LED_TASK = NULL;
TaskHandle_t G_POTENTIOMETER_TASK = NULL;

QueueHandle_t G_ADC_VALUES_QUEUE = NULL;
constexpr uint8_t QUEUE_SIZE = 10;

void pwm_init(void)
{
    ledc_timer_config_t timer = {};

    timer.speed_mode = G_PWM_MODE;
    timer.duty_resolution = LEDC_TIMER_12_BIT;
    timer.timer_num = LEDC_TIMER_0;
    timer.freq_hz = G_PWM_FREQ;
    timer.clk_cfg = LEDC_AUTO_CLK;
    timer.deconfigure = false;
    ESP_ERROR_CHECK(ledc_timer_config(&timer));
}

void pwm_channel_init(void)
{
    ledc_channel_config_t channel;
    channel.gpio_num = G_LED_PIN;
    channel.speed_mode = G_PWM_MODE;
    channel.channel = G_PWM_CHANNEL;
    channel.intr_type = LEDC_INTR_DISABLE;
    channel.timer_sel = LEDC_TIMER_0;
    channel.duty = 0; // initial dut
    channel.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
    channel.hpoint = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&channel));
}

void pwm_set_duty(uint32_t duty)
{
    ledc_set_duty(G_PWM_MODE, G_PWM_CHANNEL, duty);
    ledc_update_duty(G_PWM_MODE, G_PWM_CHANNEL);
}

void PotentiometerTask(void *ip_parameters)
{
    static adc_oneshot_unit_handle_t adc_handle;

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE};

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12};

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_PIN, &config));

    static int adc_value = 0;
    for (;;)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_PIN, &adc_value));
        xQueueSend(G_ADC_VALUES_QUEUE, &adc_value, portMAX_DELAY); // Send to queue
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void LedTask(void *ip_parameters)
{
    pwm_init();
    pwm_channel_init();

    static int adc_sample = 0;
    for (;;)
    {
        if (xQueueReceive(G_ADC_VALUES_QUEUE, &adc_sample, portMAX_DELAY))
        {
            const uint32_t duty = (adc_sample * G_PWM_MAX_VALUE) / 4095;
            pwm_set_duty(duty);
            ESP_LOGI("ADC", "adc=%d duty=%lu", adc_sample, duty);
        }
    }
}

extern "C" void app_main(void)
{
    G_ADC_VALUES_QUEUE = xQueueCreate(QUEUE_SIZE, sizeof(int));
    if (G_ADC_VALUES_QUEUE == NULL)
    {
        ESP_LOGI("Init", "Failed to create queue!");
        while (1)
            ;
    }

    xTaskCreatePinnedToCore(
        PotentiometerTask,     // Task function
        "PotentiometerTask",   // Task name
        10000,                 // Stack size (bytes)
        NULL,                  // Parameters
        1,                     // Priority
        &G_POTENTIOMETER_TASK, // Priority (the lowwest)
        0                      // Core 0
    );

    xTaskCreatePinnedToCore(
        LedTask,     // Task function
        "LedTask",   // Task name
        10000,       // Stack size (bytes)
        NULL,        // Parameters
        1,           // Priority (the lowwest)
        &G_LED_TASK, // Task handle
        1            // Core 1
    );
}
