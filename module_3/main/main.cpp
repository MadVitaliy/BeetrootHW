#include <cstdio>
#include <cinttypes>

// Standard esp-idf includes
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/mcpwm_prelude.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// User defined includes
#include "MovingAverageFilter.h"
#include "Utils.h"
#include "Joystick.h"

constexpr uint8_t G_RX_AXIS_PIN = 1;
constexpr uint8_t G_RY_AXIS_PIN = 2;
constexpr uint8_t G_LX_AXIS_PIN = 4; // yep, I've skipped 3 pin, when soldering... :(
constexpr uint8_t G_LY_AXIS_PIN = 5;
constexpr uint8_t G_R_JOYSTICK_BUTTON_PIN = 6;
constexpr uint8_t G_L_JOYSTICK_BUTTON_PIN = 7;

constexpr uint8_t G_PWM_1_PIN = 15;
constexpr uint8_t G_PWM_2_PIN = 16;
constexpr uint8_t G_PWM_3_PIN = 17;
constexpr uint8_t G_PWM_4_PIN = 18;

#include "esp_adc/adc_continuous.h"

constexpr uint16_t FRAME_SIZE = 256; // Number of samples per conversion frame
constexpr adc_unit_t ADC_UNIT = ADC_UNIT_1;
#define GET_UNIT(x) ((x >> 3) & 0x1)

static const char *TAG = "ADC_DMA";

static adc_channel_t G_ADC_CHANNELS[] = {
    ADC_CHANNEL_0, // GPIO 1
    ADC_CHANNEL_1, // GPIO 2
    ADC_CHANNEL_3, // GPIO 4
    ADC_CHANNEL_4  // GPIO 5
};

constexpr uint8_t CHANNEL_NUM = sizeof(G_ADC_CHANNELS) / sizeof(adc_channel_t);

static uint8_t G_PWM_PINS[] = {39, 40, 41, 42};

// MG996R Servo specifications: 900-2100μs pulse width, center at 1500μs
#define SERVO_MIN_PULSEWIDTH_US 544  // Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH_US 2400 // Maximum pulse width in microsecond
#define SERVO_PULSE_MIN_DEGREE 0     // Minimum servo angle (actual servo range)
#define SERVO_PULSE_MAX_DEGREE 180   // Maximum servo angle (actual servo range)

// Input angle ranges
#define ROTATION_MIN_DEGREE 0 // Rotation input: 0 to 180
#define ROTATION_MAX_DEGREE 180
#define ELEVATION_MIN_DEGREE 0 // Elevation input: 0 to 90 (maps to 0 to -90)
#define ELEVATION_MAX_DEGREE 90

static const int SERVO_PIN_1 = 15;
static const int SERVO_PIN_2 = 16;

#define SERVO_TIMEBASE_RESOLUTION_HZ 1000000 // 1MHz, 1us per tick
#define SERVO_TIMEBASE_PERIOD 10000          // 10000 ticks, 10ms (100Hz - updated servo update rate)

// Simple sweep motion configuration
// Velocity is in degrees/second; adjust to taste
#define SWEEP_VELOCITY_DEG_PER_SEC 45.0f // Sweep speed (same for both axes)
#define SERVO_UPDATE_RATE_HZ 500.0f      // Control loop rate (Hz)


constexpr uint32_t G_PWM_FREQ = 50; // 50Hz

static const char *TAG2 = "mcpwm_servo";

// Handles required by ESP-IDF v6.0
static mcpwm_timer_handle_t     timer = NULL;
static mcpwm_oper_handle_t      operator_handle = NULL;
static mcpwm_cmpr_handle_t      comparator_1 = NULL;
static mcpwm_cmpr_handle_t      comparator_2 = NULL;
static mcpwm_gen_handle_t       generator_1 = NULL;
static mcpwm_gen_handle_t       generator_2 = NULL;

void mcpwm_servo_init(void)
{
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
    ESP_ERROR_CHECK(mcpwm_new_comparator(operator_handle, &compare_config, &comparator_2));

    // 4. Allocate Generators (The physical outputs)
    mcpwm_generator_config_t gen_config_1{};
    gen_config_1.gen_gpio_num = SERVO_PIN_1;
    mcpwm_generator_config_t gen_config_2{};
    gen_config_2.gen_gpio_num = SERVO_PIN_2;

    ESP_ERROR_CHECK(mcpwm_new_generator(operator_handle, &gen_config_1, &generator_1));
    ESP_ERROR_CHECK(mcpwm_new_generator(operator_handle, &gen_config_2, &generator_2));

    // Set default starting duty cycle (e.g., 1500us center point)
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator_1, 1500));
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator_2, 1500));

    // 5. Configure Generator Actions
    // Generator 1 (Pin 39): Go HIGH on Timer Zero, Go LOW on Comparator 1 Match
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_1,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_1,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator_1, MCPWM_GEN_ACTION_LOW)));

    // Generator 2 (Pin 40): Go HIGH on Timer Zero, Go LOW on Comparator 2 Match
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator_2,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator_2,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator_2, MCPWM_GEN_ACTION_LOW)));

    // 6. Enable and Start the Timer
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));

    ESP_LOGI(TAG2, "MCPWM successfully started.");
}

Filters::MovingAverage<8> filter;

adc_oneshot_unit_handle_t adc1_handle;
Joystick G_JOYSTICK_LEFT(adc1_handle, ADC_CHANNEL_4, ADC_CHANNEL_3);
Joystick G_JOYSTICK_RIGHT(adc1_handle, ADC_CHANNEL_1, ADC_CHANNEL_0);

extern "C" void app_main(void)
{
    // pinMode(G_PWM_PINS[0], OUTPUT);

    // 1. Initialize the ADC Unit
    adc_oneshot_unit_init_cfg_t init_config1 = {};
    init_config1.unit_id = ADC_UNIT_1;
    init_config1.ulp_mode = ADC_ULP_MODE_DISABLE;

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
    ESP_LOGI(TAG, "ADC One-shot initialized successfully. Starting loop...");

    // Initialize joysticks only after adc initialization
    G_JOYSTICK_LEFT.Init(adc1_handle);
    G_JOYSTICK_RIGHT.Init(adc1_handle);

    mcpwm_servo_init();
    
    uint16_t prev_duty_x = 2000, prev_duty_y = 2000; // ms
    while (1)
    {
        if (G_JOYSTICK_LEFT.Update() && G_JOYSTICK_RIGHT.Update())
        {
            const int8_t lx = G_JOYSTICK_LEFT.GetXState();
            const int8_t ly = G_JOYSTICK_LEFT.GetYState();
            const int8_t rx = G_JOYSTICK_RIGHT.GetXState();
            const int8_t ry = G_JOYSTICK_RIGHT.GetYState();

            prev_duty_x += lx * 10u;
            prev_duty_y += ly * 10u;

            mcpwm_comparator_set_compare_value(comparator_1, prev_duty_x);
            mcpwm_comparator_set_compare_value(comparator_2, prev_duty_y);

            ESP_LOGI("", "prev_duty_x:%d, prev_duty_y:%d, rx:%d, ry:%d", prev_duty_x, prev_duty_y, rx, ry);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to update to joysticks");
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    // (Optional Clean up if you ever break the loop)
    // ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
}
