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
uint16_t CalculateCRC(const uint8_t *o_buffer, size_t i_size);
[[maybe_unused]] uint8_t *SerializeLittleEndian(uint8_t *io_buffer, uint16_t i_word);
[[maybe_unused]] uint8_t *SerializeBigEndian(uint8_t *io_buffer, uint16_t i_word);
[[maybe_unused]] uint8_t *SerializeWord(uint8_t *io_buffer, uint16_t i_word);

static void init_uart(void)
{
    uart_config_t uart_config = {};
    uart_config.baud_rate = 115200;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;

    ESP_ERROR_CHECK(uart_driver_install(G_UART_PORT_NUM, G_BUF_SIZE * 2, G_BUF_SIZE * 2, 20, &s_uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(G_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(G_UART_PORT_NUM, G_UART_TX_PIN, G_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_rx_timeout(G_UART_PORT_NUM, 10));
}

extern bool G_STM32_LED_STATUS; // Again, it's not what I am proud of....
static void send_command_task_entry(void *parameter)
{
    constexpr uint8_t request_size_without_crc = 5;
    constexpr uint8_t request_size = request_size_without_crc + 2;
    uint8_t response[] = {0x01, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00};
    // Response is: slave_address, function code, number of bytes (2 bytes for 1 register), 2 bytes register'value, 2 bytes CRC.
    for (;;)
    {
        const uint32_t thread_notification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (thread_notification > 0)
        {
            const uint16_t value_to_be_sent = G_STM32_LED_STATUS ? 0xFFFF : 0x0000;
            uint8_t *p_res_buf = response + 3; //
            p_res_buf = SerializeWord(p_res_buf, value_to_be_sent);
            SerializeLittleEndian(p_res_buf, CalculateCRC(response, request_size_without_crc));

            // uart_write_bytes_with_break(G_UART_PORT_NUM, response, request_size, 10);
            // ^ I lost a whole fucking day to the debugger because of this shit, 
            // | trying to figure out why the stm32 wasn't receiving data. 
            uart_write_bytes(G_UART_PORT_NUM, response, request_size);

            ESP_LOGI(TAG, "Sent %d bytes", request_size);
            ESP_LOG_BUFFER_HEXDUMP(TAG, response, request_size, ESP_LOG_INFO);
        }
    }
}

struct InputData
{
    TaskHandle_t led_task;
    TaskHandle_t send_task;
};

static void
read_command_task_entry(void *parameter)
{
    InputData *p_input_parame = (InputData *)parameter;

    TaskHandle_t led_task_handle = p_input_parame->led_task;
    TaskHandle_t send_cmd_task_handle = p_input_parame->send_task;

    uint8_t rx_buffer[G_BUF_SIZE + 1]; // +1 for null terminator safety
    uart_event_t event;
    uint8_t expected_write_request[] = {0x01, 0x06, 0x00, 0x01, 0x00, 0x03, 0x0b, 0x98};
    uint8_t expected_read_request[] = {0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0xca, 0xd5};

    for (;;)
    {
        if (xQueueReceive(s_uart_queue, &event, portMAX_DELAY))
        {
            switch (event.type)
            {
            case UART_DATA:
            {
                int len = uart_read_bytes(G_UART_PORT_NUM, rx_buffer, event.size, portMAX_DELAY);
                ESP_LOGI(TAG, "Received %d bytes", len);
                ESP_LOG_BUFFER_HEXDUMP(TAG, rx_buffer, len, ESP_LOG_INFO);

                if (memcmp(rx_buffer, expected_write_request, len) == 0)
                    xTaskNotifyGive(led_task_handle);
                else if (memcmp(rx_buffer, expected_read_request, len) == 0)
                {
                    vTaskDelay(pdMS_TO_TICKS(100));
                    xTaskNotifyGive(send_cmd_task_handle);
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

UartTaskHandles StartUartTasks(TaskHandle_t led_task_handle)
{
    init_uart();
    UartTaskHandles handles;

    xTaskCreate(send_command_task_entry, "SendCmdTask", 2048, NULL, 2, &handles.send_task);
    static InputData input_params{led_task_handle, handles.send_task};
    xTaskCreate(read_command_task_entry, "ReadCmdTask", 4096, (void *)(&input_params), 1, &handles.read_task);

    return handles;
}

uint16_t CalculateCRC(const uint8_t *o_buffer, size_t i_size)
{
    if (o_buffer == nullptr)
        return 0;

    uint16_t crc = 0xFFFF;
    constexpr uint16_t XOR_CONSTANT = 0xA001;
    while (i_size--)
    {
        crc ^= *o_buffer;
        ++o_buffer;
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ XOR_CONSTANT;
            else
                crc >>= 1;
        }
    }
    return crc;
}

uint8_t *SerializeLittleEndian(uint8_t *io_buffer, uint16_t i_word)
{
    io_buffer[1] = (uint8_t)i_word;
    io_buffer[0] = (uint8_t)(i_word >> 8);
    return io_buffer + 2;
}

uint8_t *SerializeBigEndian(uint8_t *io_buffer, uint16_t i_word)
{
    io_buffer[0] = (uint8_t)i_word;
    io_buffer[1] = (uint8_t)(i_word >> 8);
    return io_buffer + 2;
}

constexpr bool G_BIG_ENDIAN = false;
uint8_t *SerializeWord(uint8_t *io_buffer, uint16_t i_word)
{
    if constexpr (G_BIG_ENDIAN)
        return SerializeBigEndian(io_buffer, i_word);
    else
        return SerializeLittleEndian(io_buffer, i_word);
}