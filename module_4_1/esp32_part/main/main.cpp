#include <cstdio>
#include <cinttypes>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "driver/uart.h"

#include "Button.h"

static const char *TAG = "MAIN";

constexpr gpio_num_t G_BUTTON_GPIO = gpio_num_t(7);
constexpr gpio_num_t G_LED_STRIP_GPIO = gpio_num_t(48);
constexpr size_t G_LEDS_QUANTITY = 1;

static led_strip_handle_t G_LED_STRIP = NULL;
static QueueHandle_t G_UART_QUEUE;

// Helper to initialize the LED strip driver (RMT backend)
static void init_led_strip(void)
{
    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = G_LED_STRIP_GPIO;
    strip_config.max_leds = G_LEDS_QUANTITY;
    strip_config.led_model = LED_MODEL_WS2812;
    strip_config.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB;
    strip_config.flags.invert_out = false;

    led_strip_rmt_config_t rmt_config = {};
    rmt_config.clk_src = RMT_CLK_SRC_DEFAULT;
    rmt_config.resolution_hz = 10 * 1000 * 1000; // 10MHz
    rmt_config.flags.with_dma = false;

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &G_LED_STRIP));
    led_strip_clear(G_LED_STRIP); // Ensure LED starts turned off
}

// Function 1: Turn on the LED in Purple
void turn_on_purple(void)
{
    if (G_LED_STRIP == NULL)
        return;
    // Set RGB values: Red = 128, Green = 0, Blue = 128 (Purple)
    // Scale values down (e.g., max 128) to avoid excessive glare/brightness
    led_strip_set_pixel(G_LED_STRIP, 0, 128, 0, 128);
    led_strip_refresh(G_LED_STRIP);
}

// Function 2: Turn off the LED
void turn_off(void)
{
    if (G_LED_STRIP == NULL)
        return;
    led_strip_clear(G_LED_STRIP);
}

constexpr uart_port_t G_UART_PORT_NUM = UART_NUM_1;
constexpr gpio_num_t G_UART_TX_PIN = GPIO_NUM_16;
constexpr gpio_num_t G_UART_RX_PIN = GPIO_NUM_15;
constexpr size_t G_BUF_SIZE = 256;

static void init_uart(void)
{
    uart_config_t uart_config = {};
    memset(&uart_config, 0, sizeof(uart_config_t));
    uart_config.baud_rate = 115200;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;

    // Install driver with an RX Queue so events (like RX Idle Timeout) are captured
    ESP_ERROR_CHECK(uart_driver_install(G_UART_PORT_NUM, G_BUF_SIZE * 2, 0, 20, &G_UART_QUEUE, 0));
    ESP_ERROR_CHECK(uart_param_config(G_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(G_UART_PORT_NUM, G_UART_TX_PIN, G_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Configure RX Idle Timeout (fires after 10 idle character durations)
    ESP_ERROR_CHECK(uart_set_rx_timeout(G_UART_PORT_NUM, 10));
}

TaskHandle_t GH_BUTTON_TASK = NULL;
TaskHandle_t GH_LED_TASK = NULL;

TaskHandle_t GH_SEND_COMMAND_TASK = NULL;
TaskHandle_t GH_READ_COMMAND_TASK = NULL;

void ButtonTask(void *parameter)
{
    constexpr bool button_is_bulled_down = false;
    constexpr bool use_build_in_resistor = true;
    Button button(G_BUTTON_GPIO, button_is_bulled_down, use_build_in_resistor);
    button.Init();
    for (;;)
    {
        button.Update();
        if (button.WasPressed())
        {
            xTaskNotifyGive(GH_SEND_COMMAND_TASK);
            ESP_LOGI("ButtonTask", "pressed");
        }

        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

void SendCommandTask(void *parameter)
{
    const uint8_t command[] = {02, 01, 05, 05, 03};
    for (;;)
    {
        const uint32_t thread_notification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (thread_notification <= 0)
            continue;
        uart_write_bytes_with_break(G_UART_PORT_NUM, command, 5, 10);
        ESP_LOGI("SendCommandTask", "sent");
    }
}

void ReadCommandTask(void *parameter)
{
    const char *TAG = "ReadCommandTask";
    uint8_t rx_buffer[G_BUF_SIZE];
    uart_event_t event;
    for (;;)
    {
        // Goes true if buffer overflows or uart line idles
        if (xQueueReceive(G_UART_QUEUE, (void *)&event, portMAX_DELAY))
        {
            switch (event.type)
            {
            case UART_DATA:
            {
                // Read available bytes in buffer
                const int len = uart_read_bytes(G_UART_PORT_NUM, rx_buffer, event.size, portMAX_DELAY);
                if (len <= 0)
                    break;

                ESP_LOGI(TAG, "Hex Dump (%d bytes):", len);
                ESP_LOG_BUFFER_HEXDUMP(TAG, rx_buffer, len, ESP_LOG_INFO);

                rx_buffer[len] = '\0'; // Null-terminate received string
                ESP_LOGI(TAG, "Received %d bytes (Till Idle): %s", len, rx_buffer);

                if (rx_buffer[2] == 5)
                    xTaskNotifyGive(GH_LED_TASK);

                break;
            }
            case UART_FIFO_OVF:
            case UART_BUFFER_FULL:
                ESP_LOGW(TAG, "UART Buffer Overflow!");
                uart_flush_input(G_UART_PORT_NUM);
                xQueueReset(G_UART_QUEUE);
                break;
            default:
                break;
            }
        }
    }
}

void LedTask(void *parameter)
{

    bool led_is_shining = false;
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (led_is_shining)
            turn_off();
        else
            turn_on_purple();
        led_is_shining = !led_is_shining;
    }
}

extern "C" void app_main(void)
{
    init_led_strip();
    init_uart();
    ESP_LOGI(TAG, "UART Listening... Send commands 'on' or 'off'");

    xTaskCreate(
        ButtonTask,
        "ButtonTask",
        1024,
        NULL,
        1,
        &GH_BUTTON_TASK);
    xTaskCreate(
        SendCommandTask,
        "SendCommandTask",
        1024,
        NULL,
        1,
        &GH_SEND_COMMAND_TASK);
    xTaskCreate(
        ReadCommandTask,
        "ReadCommandTask",
        4096,
        NULL,
        1,
        &GH_READ_COMMAND_TASK);
    xTaskCreate(
        LedTask,
        "LedTask",
        1024,
        NULL,
        1,
        &GH_LED_TASK);

    // Should not be reachable
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
