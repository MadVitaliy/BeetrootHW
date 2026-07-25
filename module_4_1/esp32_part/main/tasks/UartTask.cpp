#include "UartTask.hpp"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <cstring>

static const char *TAG = "UART_TASK";

constexpr uart_port_t G_UART_PORT_NUM = UART_NUM_1;
constexpr gpio_num_t G_UART_TX_PIN = GPIO_NUM_16;
constexpr gpio_num_t G_UART_RX_PIN = GPIO_NUM_15;
constexpr size_t G_BUF_SIZE = 256;

static QueueHandle_t s_uart_queue;

static void init_uart(void) {
    uart_config_t uart_config = {};
    uart_config.baud_rate = 115200;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;

    ESP_ERROR_CHECK(uart_driver_install(G_UART_PORT_NUM, G_BUF_SIZE * 2, 0, 20, &s_uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(G_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(G_UART_PORT_NUM, G_UART_TX_PIN, G_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_rx_timeout(G_UART_PORT_NUM, 10));
}

static void send_command_task_entry(void *parameter) {
    const uint8_t command[] = {0x02, 0x01, 0x05, 0x05, 0x03}; // Fixed hex notation
    for (;;) {
        uint32_t thread_notification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (thread_notification > 0) {
            uart_write_bytes_with_break(G_UART_PORT_NUM, command, sizeof(command), 10);
            ESP_LOGI(TAG, "Command sent over UART");
        }
    }
}

static void read_command_task_entry(void *parameter) {
    TaskHandle_t led_task_handle = (TaskHandle_t)parameter;
    uint8_t rx_buffer[G_BUF_SIZE + 1]; // +1 for null terminator safety
    uart_event_t event;

    for (;;) {
        if (xQueueReceive(s_uart_queue, &event, portMAX_DELAY)) {
            switch (event.type) {
            case UART_DATA: {
                int len = uart_read_bytes(G_UART_PORT_NUM, rx_buffer, event.size, portMAX_DELAY);
                if (len <= 0) break;

                rx_buffer[len] = '\0'; // Safe now!
                ESP_LOGI(TAG, "Received %d bytes", len);
                ESP_LOG_BUFFER_HEXDUMP(TAG, rx_buffer, len, ESP_LOG_INFO);

                // Safe array index check
                if (len >= 3 && rx_buffer[2] == 5 && led_task_handle != NULL) {
                    xTaskNotifyGive(led_task_handle);
                }
                break;
            }
            case UART_FIFO_OVF:
            case UART_BUFFER_FULL:
                ESP_LOGW(TAG, "UART Buffer Overflow!");
                uart_flush_input(G_UART_PORT_NUM);
                xQueueReset(s_uart_queue);
                break;
            default:
                break;
            }
        }
    }
}

UartTaskHandles StartUartTasks(TaskHandle_t led_task_handle) {
    init_uart();
    UartTaskHandles handles;

    xTaskCreate(send_command_task_entry, "SendCmdTask", 2048, NULL, 1, &handles.send_task);
    xTaskCreate(read_command_task_entry, "ReadCmdTask", 4096, (void*)led_task_handle, 1, &handles.read_task);

    return handles;
}
