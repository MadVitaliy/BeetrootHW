#include <cstdio>
#include <cinttypes>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP-IDF Drivers for ADC and LEDC (PWM)
#include "esp_adc/adc_oneshot.h"
#include "driver/ledc.h"

#include "ButtonsArray.h"

constexpr uint8_t G_BUZZER_PIN = 15;
constexpr adc_channel_t G_ADC_CHANNEL = ADC_CHANNEL_0;

adc_oneshot_unit_handle_t adc1_handle;
ButtonsArray G_BUTTONS_ARRAY(adc1_handle, G_ADC_CHANNEL, 4);
static const char *TAG = "MAIN";

constexpr uint32_t NOTE_C = 262;
constexpr uint32_t NOTE_D = 294;
constexpr uint32_t NOTE_E = 330;
constexpr uint32_t NOTE_G = 392;

// LEDC configuration constants
constexpr ledc_mode_t LEDC_MODE = LEDC_LOW_SPEED_MODE;
constexpr ledc_channel_t LEDC_CHAN = LEDC_CHANNEL_0;
constexpr ledc_timer_t LEDC_TIM = LEDC_TIMER_0;
constexpr ledc_timer_bit_t LEDC_DUTY_RES = LEDC_TIMER_10_BIT;
constexpr uint32_t LEDC_MAX_DUTY = 1023; // 2^10 - 1

// Helper function to initialize the PWM (LEDC) peripheral
static void init_pwm(void)
{
    // Initialize the struct to zero first to safely catch all unlisted framework fields
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode       = LEDC_MODE;
    ledc_timer.duty_resolution  = LEDC_DUTY_RES;
    ledc_timer.timer_num        = LEDC_TIM;
    ledc_timer.freq_hz          = 4000;  // Initial placeholder frequency
    ledc_timer.clk_cfg          = LEDC_AUTO_CLK;
    
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Zero-initialize the channel struct to prevent missing field errors
    ledc_channel_config_t ledc_channel = {};
    ledc_channel.gpio_num       = G_BUZZER_PIN;
    ledc_channel.speed_mode     = LEDC_MODE;
    ledc_channel.channel        = LEDC_CHAN;
    ledc_channel.intr_type      = LEDC_INTR_DISABLE;
    ledc_channel.timer_sel      = LEDC_TIM;
    ledc_channel.duty           = 0; // Start silent
    ledc_channel.hpoint         = 0;
    
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

// Helper function to update the tone frequency and set duty cycle to 50%
static void play_tone(uint32_t frequency_hz)
{
    if (frequency_hz == 0)
    {
        // Stop sound by setting duty cycle to 0
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHAN, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHAN));
    }
    else
    {
        // Set target frequency, then set duty cycle to 50% for a square wave
        ESP_ERROR_CHECK(ledc_set_freq(LEDC_MODE, LEDC_TIM, frequency_hz));
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHAN, LEDC_MAX_DUTY / 2));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHAN));
    }
}

extern "C" void app_main(void)
{
    adc_oneshot_unit_init_cfg_t init_config1 = {};
    init_config1.unit_id = ADC_UNIT_1;
    init_config1.ulp_mode = ADC_ULP_MODE_DISABLE;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
    ESP_LOGI(TAG, "ADC One-shot initialized successfully.");
    G_BUTTONS_ARRAY.Init(adc1_handle);

    // Initialize the PWM driver
    init_pwm();
    ESP_LOGI(TAG, "System initialized. Waiting for button presses...");

    while (1)
    {
        const auto button_read = G_BUTTONS_ARRAY.ReadButton();
        if (!button_read)
        {
play_tone(0);
        }else{

            switch (button_read.value())
            {
            case 0:
                play_tone(NOTE_C);
                break;
                case 1:
                play_tone(NOTE_D);
                break;
                case 2:
                play_tone(NOTE_E);
                break;
                case 3:
                play_tone(NOTE_G);
                break;
            default:
                play_tone(0);
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    // (Optional Clean up if you ever break the loop)
    // ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
}
