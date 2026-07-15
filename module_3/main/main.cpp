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

static mcpwm_cmpr_handle_t rotation_comparator = NULL;
static mcpwm_cmpr_handle_t elevation_comparator = NULL;

static inline uint32_t angle_to_compare(float angle)
{
    // Map 0-180 degree range to pulse width
    return (uint32_t)((angle - SERVO_PULSE_MIN_DEGREE) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) /
                          (SERVO_PULSE_MAX_DEGREE - SERVO_PULSE_MIN_DEGREE) +
                      SERVO_MIN_PULSEWIDTH_US);
}

static inline float map_rotation(float input_angle)
{
    // Map 0 to 180 input directly to 0 to 180 servo range (1:1 mapping)
    float clamped = input_angle;
    if (clamped < ROTATION_MIN_DEGREE)
        clamped = ROTATION_MIN_DEGREE;
    if (clamped > ROTATION_MAX_DEGREE)
        clamped = ROTATION_MAX_DEGREE;

    return clamped; // Direct 1:1 mapping
}

static inline float map_elevation(float input_angle)
{
    // Map 0 to 90 input range to 90 to 0 servo range (inverted)
    // Input 0 → Servo 90, Input 30 → Servo 60, Input 90 → Servo 0
    float clamped = input_angle;
    if (clamped < ELEVATION_MIN_DEGREE)
        clamped = ELEVATION_MIN_DEGREE;
    if (clamped > ELEVATION_MAX_DEGREE)
        clamped = ELEVATION_MAX_DEGREE;

    return 90.0f - clamped; // Invert: 0→90, 90→0
}

constexpr uint32_t G_PWM_FREQ = 50; // 50Hz

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

    // int raw_value = 0;
    ESP_LOGI("", "Raw ADC Value, Calclulated Voltage, Calibrated Voltage");

    ESP_LOGI(TAG, "Create timer and operator");
    mcpwm_timer_config_t timer_config{};
    timer_config.group_id = 0;
    timer_config.clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT;
    timer_config.resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ;
    timer_config.period_ticks = SERVO_TIMEBASE_PERIOD;
    timer_config.count_mode = MCPWM_TIMER_COUNT_MODE_UP;
    // timer_config.intr_priority = 1;
    // timer_config.flags.allow_pd = 0;

    mcpwm_timer_handle_t timer = NULL;
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    mcpwm_oper_handle_t oper = NULL;
    mcpwm_operator_config_t operator_config{};
    operator_config.group_id = 0; // operator must be in the same group to the timer

    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));

    ESP_LOGI(TAG, "Connect timer and operator");
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    ESP_LOGI(TAG, "Create comparators and generators from the operator");
    mcpwm_comparator_config_t comparator_config{};
    comparator_config.flags.update_cmp_on_tez = true;   // Keep your default update event
    comparator_config.flags.update_cmp_on_tep = false;  // <--- Fixed: Added missing struct members
    comparator_config.flags.update_cmp_on_sync = false; // <--- Fixed: Added missing struct members

    // Create comparator for rotation servo
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &rotation_comparator));

    // Create comparator for elevation servo
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &elevation_comparator));

    // Create generator for rotation servo
    mcpwm_gen_handle_t rotation_generator = NULL;
    mcpwm_generator_config_t rotation_gen_config{};
    rotation_gen_config.gen_gpio_num = SERVO_PIN_1;

    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &rotation_gen_config, &rotation_generator));

    // Create generator for elevation servo
    mcpwm_gen_handle_t elevation_generator = NULL;
    mcpwm_generator_config_t elevation_gen_config{};
    elevation_gen_config.gen_gpio_num = SERVO_PIN_2;

    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &elevation_gen_config, &elevation_generator));

    // Set initial compare values to center position for both servos
    // Rotation: 90° input → 90° servo, Elevation: 45° input → 45° servo (inverted to middle position)
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(rotation_comparator,
                                                       angle_to_compare(0))); // Start at 0°
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(elevation_comparator,
                                                       angle_to_compare(0))); // Start at 0°

    ESP_LOGI(TAG, "Set generator actions on timer and compare events");

    // Rotation servo: go high on counter empty
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(rotation_generator,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    // Rotation servo: go low on compare threshold
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(rotation_generator,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, rotation_comparator, MCPWM_GEN_ACTION_LOW)));

    // Elevation servo: go high on counter empty
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(elevation_generator,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    // Elevation servo: go low on compare threshold
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(elevation_generator,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, elevation_comparator, MCPWM_GEN_ACTION_LOW)));

    ESP_LOGI(TAG, "Enable and start timer");
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));

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

            mcpwm_comparator_set_compare_value(rotation_comparator, prev_duty_x);
            mcpwm_comparator_set_compare_value(elevation_comparator, prev_duty_y);

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
