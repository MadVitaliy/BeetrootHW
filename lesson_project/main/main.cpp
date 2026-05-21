#include <cstdio>
#include <cinttypes>

// Standard esp-idf includes
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_task_wdt.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// User defined includes
#include "SSD.h"

QueueHandle_t G_ADC_VALUES_QUEUE = NULL;
constexpr uint8_t QUEUE_SIZE = 1;

QueueHandle_t G_BUTTONS_QUEUE = nullptr;

void PrintLastShutDownReason()
{
    const esp_reset_reason_t reason = esp_reset_reason();
    switch (reason)
    {
    case ESP_RST_UNKNOWN:
        printf("Reset reason can not be determined\n");
        break;
    case ESP_RST_POWERON:
        printf("Reset due to power-on event\n");
        break;
    case ESP_RST_EXT:
        printf("Reset by external pin (not applicable for ESP32)\n");
        break;
    case ESP_RST_SW:
        printf("Software reset via esp_restart\n");
        break;
    case ESP_RST_PANIC:
        printf("Software reset due to exception/panic\n");
        break;
    case ESP_RST_INT_WDT:
        printf("Reset (software or hardware) due to interrupt watchdog\n");
        break;
    case ESP_RST_TASK_WDT:
        printf("Reset due to task watchdog\n");
        break;
    case ESP_RST_WDT:
        printf("Reset due to other watchdogs\n");
        break;
    case ESP_RST_DEEPSLEEP:
        printf("Reset after exiting deep sleep mode\n");
        break;
    case ESP_RST_BROWNOUT:
        printf("Brownout reset (software or hardware)\n");
        break;
    case ESP_RST_SDIO:
        printf("Reset over SDIO\n");
        break;
    case ESP_RST_USB:
        printf("Reset by USB peripheral\n");
        break;
    case ESP_RST_JTAG:
        printf("Reset by JTAG\n");
        break;
    case ESP_RST_EFUSE:
        printf("Reset due to efuse error\n");
        break;
    case ESP_RST_PWR_GLITCH:
        printf("Reset due to power glitch detected\n");
        break;
    case ESP_RST_CPU_LOCKUP:
        printf("Reset due to CPU lock up (double exception)\n");
        break;
    default:
        break;
    }
}

esp_timer_handle_t GH_DEBOUNCE_TIMER;
TaskHandle_t GH_DISPLAY_TIME = nullptr;
TaskHandle_t GH_DEBOUNCE_TASK = nullptr;

#define DEBOUNCE_DELAY_US 200000ULL // Debounce delay in microseconds (200 ms)
static volatile uint64_t last_isr_tick = 0;
static volatile uint32_t counter = 0;

constexpr gpio_num_t G_BUTTON_PIN = GPIO_NUM_4;

esp_timer_handle_t GH_ONE_SHOT_TIMER = NULL;

void DebounceTask(void *arg)
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        esp_timer_start_once(GH_ONE_SHOT_TIMER, 20000);
    }
}

volatile int64_t G_CURRENT_TIMESTAMP = 0;
IRAM_ATTR void button_isr(void *arg)
{
    G_CURRENT_TIMESTAMP = esp_timer_get_time();
    gpio_intr_disable(G_BUTTON_PIN);

    BaseType_t higher_woken = pdFALSE;
    vTaskNotifyGiveFromISR(GH_DEBOUNCE_TASK, &higher_woken);

    portYIELD_FROM_ISR(higher_woken);
}

constexpr gpio_num_t G_SHR_OUT_PIN = GPIO_NUM_5;
constexpr gpio_num_t G_SHR_CLOCK_PIN = GPIO_NUM_6;
constexpr gpio_num_t G_SHR_CLEAR_PIN = GPIO_NUM_7;
SSD ssd(false, G_SHR_OUT_PIN, G_SHR_CLOCK_PIN, G_SHR_CLEAR_PIN);

// Callback function to be executed when the timer expires
volatile bool G_IS_TIME_RUNNING = false;
static void ButtonDebounce(void *arg)
{
    const int64_t time_since_isr = esp_timer_get_time() - G_CURRENT_TIMESTAMP;
    const int level = gpio_get_level(G_BUTTON_PIN);
    if (level != 0)
    {
        if (!G_IS_TIME_RUNNING)
        {
            G_IS_TIME_RUNNING = true;
            vTaskResume(GH_DISPLAY_TIME);
        }
        else
        {
            G_IS_TIME_RUNNING = false;
            vTaskSuspend(GH_DISPLAY_TIME);
        }
    } // else {ignore: must have been noise}.
    gpio_intr_enable(G_BUTTON_PIN);
    ESP_LOGI("ButtonDebounce", "One-shot timer triggered! Mills from button's ISR: %lld", time_since_isr);
}

void DisplayTimer(void *ip_parameters)
{
    while (true)
    {
        // TODO: read timer and convert to seconds.
        static uint16_t dummy_time = 0;
        ++dummy_time;
        const uint8_t h = dummy_time / 100;
        const uint8_t l = dummy_time % 100;
        const uint8_t dec = l / 10;
        const uint8_t d = l % 10;
        ssd.Clear();
        ssd.Put(d);
        ssd.Put(dec);
        ssd.Put(h);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

extern "C" void app_main(void)
{
    PrintLastShutDownReason();

    // Display configuration
    ssd.Init();

    // Button configuration
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << G_BUTTON_PIN);

    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    G_BUTTONS_QUEUE = xQueueCreate(10, sizeof(uint32_t));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(G_BUTTON_PIN, button_isr, NULL);

    esp_timer_create_args_t one_shot_timer_args = {};
    one_shot_timer_args.callback = &ButtonDebounce;
    one_shot_timer_args.name = "ButtonDebounceCallback";
    ESP_ERROR_CHECK(esp_timer_create(&one_shot_timer_args, &GH_ONE_SHOT_TIMER));

    xTaskCreate(DisplayTimer, "DisplayTimer", 4096, nullptr, 1, &GH_DISPLAY_TIME);
    vTaskSuspend(GH_DISPLAY_TIME);
    G_IS_TIME_RUNNING = false;

    xTaskCreate(DebounceTask, "DebounceTask", 4096, nullptr, 2, &GH_DEBOUNCE_TASK);

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
