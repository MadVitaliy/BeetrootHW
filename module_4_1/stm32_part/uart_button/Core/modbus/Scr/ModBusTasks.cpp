#include "./ModBusTasks.hpp"

#include "./ModBusDataTypes.hpp"
#include "./ModBusSerialization.hpp"

extern "C"
{
#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"
}

extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;

typedef StaticTask_t osStaticThreadDef_t;
osThreadId_t UartComunicatioHandle;
uint32_t UartComunicatioBuffer[512];
osStaticThreadDef_t UartComunicationControlBlock;
const osThreadAttr_t UartComunication_attributes =
  { .name = "UartComunicatio",                         //
      .cb_mem = &UartComunicationControlBlock,         //
      .cb_size = sizeof(UartComunicationControlBlock), //
      .stack_mem = &UartComunicatioBuffer[0],          //
      .stack_size = sizeof(UartComunicatioBuffer),     //
      .priority = (osPriority_t) osPriorityLow,        //
    };

osMessageQueueId_t h_request_queue;
static osThreadId_t s_task_handle = nullptr;

namespace Tasks::Communication
{
  using namespace Modbus;
  void ModbusTask (void *argument) {
    static constexpr size_t m_uart_buffer_size = 256;
    uint8_t mp_uart_tx_buffer[m_uart_buffer_size];
    uint8_t mp_uart_rx_buffer[m_uart_buffer_size];

    osMessageQueueId_t queue_handle = (osMessageQueueId_t) argument;
    s_task_handle = osThreadGetId ();
    RequestWithHandle request;

    for (;;) {
      const osStatus_t status = osMessageQueueGet (queue_handle, &request, NULL, osWaitForever);
      if (status == osOK) {
        // osThreadFlagsClear (0x01);
        size_t package_size = m_uart_buffer_size;
        Serialize (request.request, mp_uart_tx_buffer, &package_size);
        HAL_UART_Transmit_DMA (&huart2, mp_uart_tx_buffer, package_size);
      }
    }

  }

  osMessageQueueId_t Init () {
    h_request_queue = osMessageQueueNew (8, sizeof(RequestWithHandle), NULL);
    s_task_handle = osThreadNew (ModbusTask, (void*) h_request_queue, &UartComunication_attributes);
    return h_request_queue;
  }
}

extern "C"
{
  void HAL_UART_TxCpltCallback (UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) { // Match your UART instance
      // Signal UartTask that hardware has finished transmitting
      //osThreadFlagsSet (s_task_handle, 0x01);
    }
  }
}
