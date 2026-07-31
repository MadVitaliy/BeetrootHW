#include <cstdio>
#include <cinttypes>

// Standard esp-idf includes
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// User defined includes
#include "PowerManagement.h"
#include "SSD.h"

constexpr uint8_t G_RX_AXIS_PIN = 1;
constexpr uint8_t G_RY_AXIS_PIN = 2;
constexpr uint8_t G_LX_AXIS_PIN = 4; // yep, I've skipped 3 pin, when soldering... :(
constexpr uint8_t G_LY_AXIS_PIN = 5;
constexpr uint8_t G_R_JOYSTICK_BUTTON_PIN = 6;
constexpr uint8_t G_L_JOYSTICK_BUTTON_PIN = 7;

constexpr uint8_t G_PWM_1_PIN = 42;
constexpr uint8_t G_PWM_2_PIN = 41;
constexpr uint8_t G_PWM_3_PIN = 40;
constexpr uint8_t G_PWM_4_PIN = 39;

#include "esp_adc/adc_continuous.h"

constexpr uint16_t FRAME_SIZE = 256; // Number of samples per conversion frame
constexpr adc_unit_t ADC_UNIT = ADC_UNIT_1;
#define GET_UNIT(x) ((x >> 3) & 0x1)

static const char *TAG = "ADC_DMA";

static adc_channel_t channels[] = {
    ADC_CHANNEL_0, // GPIO 36 (ESP32)
    ADC_CHANNEL_1, // GPIO 39 (ESP32)
    ADC_CHANNEL_3, // GPIO 32 (ESP32)
    ADC_CHANNEL_4  // GPIO 33 (ESP32)
};
constexpr uint8_t CHANNEL_NUM = sizeof(channels) / sizeof(adc_channel_t);

// Helper function to register the factory calibration scheme
static bool init_adc_calibration(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    // Modern chips (ESP32-S3, C3, etc.) natively use the Curve Fitting calibration scheme

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = ADC_CHANNEL_0,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
    if (ret == ESP_OK)
    {
        calibrated = true;
    }

    *out_handle = handle;
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Calibration successfully initialized using eFuse metrics.");
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED)
    {
        ESP_LOGW(TAG, "Calibration scheme not supported (missing eFuse data). Using fallback.");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to initialize calibration handle due to an error.");
    }

    return calibrated;
}

extern "C" void app_main(void)
{
    // 1. Initialize the ADC Unit
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {};
    init_config1.unit_id = ADC_UNIT_1;
    // init_config1.clk_src = ADC_DIGI_CLK_SRC_DEFAULT; // Uses default clock for the SoC
    init_config1.ulp_mode = ADC_ULP_MODE_DISABLE;

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // 2. Configure the specific ADC Channel (Attenuation & Bitwidth)
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,         // 0 to 3.3V full-scale range
        .bitwidth = ADC_BITWIDTH_DEFAULT, // Default max width for the chip (typically 12-bit)
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config));

    adc_cali_handle_t cali_handle = NULL;
    bool do_calibration = init_adc_calibration(ADC_UNIT_1, ADC_ATTEN_DB_12, &cali_handle);

    ESP_LOGI(TAG, "ADC One-shot initialized successfully. Starting loop...");

    int raw_value = 0;
    ESP_LOGI("", "Raw ADC Value, Calclulated Voltage, Calibrated Voltage");
    while (1)
    {
        // 3. Trigger a single read on the channel
        esp_err_t ret = adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &raw_value);

        if (ret == ESP_OK)
        {
            const int calculated_voltage_mv = (3300 * raw_value) / 4096;
            int calibrated_voltage_mv;
            adc_cali_raw_to_voltage(cali_handle, raw_value, &calibrated_voltage_mv);
            ESP_LOGI("", "%d,%d,%d", raw_value, calculated_voltage_mv, calibrated_voltage_mv);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read ADC value!");
        }

        // Delay for 500ms
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // (Optional Clean up if you ever break the loop)
    // ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
}
