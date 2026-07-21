#include <cstdio>
#include <cinttypes>

// Standard esp-idf includes
#include "driver/gpio.h"
#include "driver/mcpwm_prelude.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "Utils.h"
#include "Encoder.h"

constexpr uint8_t G_PWM_1_PIN = 15;
constexpr gpio_num_t G_ENC_BUTTON = gpio_num_t(6);
constexpr gpio_num_t G_ENC_CH_A = gpio_num_t(4);
constexpr gpio_num_t G_ENC_CH_B = gpio_num_t(5);

// MG996R Servo specifications: 900-2100μs pulse width, center at 1500μs
#define SERVO_MIN_PULSEWIDTH_US 544  // Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH_US 2400 // Maximum pulse width in microsecond

#define SERVO_TIMEBASE_RESOLUTION_HZ 1000000 // 1MHz, 1us per tick
#define SERVO_TIMEBASE_PERIOD 10000          // 10000 ticks, 10ms (100Hz - updated servo update rate)

constexpr uint32_t G_PWM_FREQ = 50; // 50Hz

// Handles required by ESP-IDF v6.0
static mcpwm_timer_handle_t timer = NULL;
static mcpwm_oper_handle_t operator_handle = NULL;
static mcpwm_cmpr_handle_t comparator_1 = NULL;
static mcpwm_gen_handle_t generator_1 = NULL;

void mcpwm_servo_init(void)
{
    static const char *TAG2 = "mcpwm_servo";
    ESP_LOGI(TAG2, "Initializing MCPWM for ESP-IDF v6.0...");

    // 1. Allocate the Timer
    mcpwm_timer_config_t timer_config{};
    timer_config.group_id = 0;
    timer_config.clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT;
    timer_config.resolution_hz = 1000000; // 1MHz = 1 microsecond per tick
    timer_config.count_mode = MCPWM_TIMER_COUNT_MODE_UP;
    timer_config.period_ticks = 20000; // 20,000 ticks = 20ms (50Hz)

    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    // 2. Allocate the Operator
    mcpwm_operator_config_t operator_config{};
    operator_config.group_id = 0; // Must match timer's group_id

    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &operator_handle));

    // Connect Operator to Timer
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operator_handle, timer));

    // 3. Allocate Comparators (These translate raw microsecond values to events)
    mcpwm_comparator_config_t compare_config{};
    compare_config.flags.update_cmp_on_tez = true;   // Keep your default update event
    compare_config.flags.update_cmp_on_tep = false;  // <--- Fixed: Added missing struct members
    compare_config.flags.update_cmp_on_sync = false; // <--- Fixed: Added missing struct members

    ESP_ERROR_CHECK(mcpwm_new_comparator(operator_handle, &compare_config, &comparator_1));

    // 4. Allocate Generators (The physical outputs)
    mcpwm_generator_config_t gen_config_1{};
    gen_config_1.gen_gpio_num = G_PWM_1_PIN;

    ESP_ERROR_CHECK(mcpwm_new_generator(operator_handle, &gen_config_1, &generator_1));

    // Set default starting duty cycle (e.g., 1500us center point)
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator_1, 1500));

    // 5. Configure Generator Actions
    // Generator 1 (Pin 39): Go HIGH on Timer Zero, Go LOW on Comparator 1 Match
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_1,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_1,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator_1, MCPWM_GEN_ACTION_LOW)));

    // 6. Enable and Start the Timer
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));

    ESP_LOGI(TAG2, "MCPWM successfully started.");
}

Encoder G_ENCODER(G_ENC_CH_A, G_ENC_CH_B, G_ENC_BUTTON);

extern "C" void app_main(void)
{
    static const char *TAG = "main";

    mcpwm_servo_init();

    G_ENCODER.Init();

    constexpr uint8_t pwm_step = 10;
    uint16_t pwm_ms = 2000;
    while (1)
    {
        G_ENCODER.Update();
        const int enc_dir = G_ENCODER.GetDir();
        if (enc_dir)
            ESP_LOGI(TAG, "Dir: %d steps", enc_dir);
        pwm_ms += enc_dir * pwm_step;
        if (pwm_ms < SERVO_MIN_PULSEWIDTH_US)
            pwm_ms = SERVO_MIN_PULSEWIDTH_US;
        else if (pwm_ms > SERVO_MAX_PULSEWIDTH_US)
            pwm_ms = SERVO_MAX_PULSEWIDTH_US;

        mcpwm_comparator_set_compare_value(comparator_1, pwm_ms);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
