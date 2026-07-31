#include "./ModBusTasks.hpp"

#include "./ModBusDataTypes.hpp"
#include "./ModBusSerialization.hpp"

extern "C"
{
#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"
}
#include <cstring>

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

void Clear_UART_Errors (UART_HandleTypeDef *huart) {
  __IO uint32_t tmpreg = 0x00;
  tmpreg = huart->Instance->SR;
  tmpreg = huart->Instance->DR;
  (void) tmpreg; // Prevent compiler warning
}

volatile uint16_t G_RECEIVE_SIZE = 0;
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
      if (status != osOK)
        continue;

      size_t package_size = m_uart_buffer_size;
      Serialize (request.request, mp_uart_tx_buffer, &package_size);

      // 1. Prepare Receive before or right after Transmit
      osThreadFlagsClear (0x04);
      __HAL_UART_CLEAR_IDLEFLAG(&huart2);

      // Start DMA RX To Idle before sending request
      if (HAL_UARTEx_ReceiveToIdle_DMA (&huart2, mp_uart_rx_buffer, m_uart_buffer_size) != HAL_OK) {
        Clear_UART_Errors (&huart2);
        HAL_UART_AbortReceive (&huart2);
        HAL_UARTEx_ReceiveToIdle_DMA (&huart2, mp_uart_rx_buffer, m_uart_buffer_size);
      }

      // 2. Transmit Stage
      osThreadFlagsClear (0x03);
      if (HAL_UART_Transmit_DMA (&huart2, mp_uart_tx_buffer, package_size) != HAL_OK) {
        HAL_UART_AbortReceive (&huart2);
        continue;
      }

      // Wait for TX Callback
      uint32_t result = osThreadFlagsWait (0x03, osFlagsWaitAny, 500);
      if ((result & osFlagsError) != 0) {
        HAL_UART_Abort (&huart2);
        continue;
      }

      // Wait for physical TC bit (Crucial for RS485 / Half-Duplex)
      while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) == RESET) {
        osDelay (1);
      }

      // 3. Receive Stage (Wait for RX Event Callback)
      result = osThreadFlagsWait (0x04, osFlagsWaitAny, 2000);
      if ((result & osFlagsError) != 0) {
        HAL_UART_AbortReceive (&huart2);
        continue;
      }

      // 4. Process received data
      uint8_t *op_data = request.request.data;
      if (op_data != nullptr) {
        *((uint16_t*) op_data) = G_RECEIVE_SIZE;
        op_data += 2;
        if (G_RECEIVE_SIZE > 0)
          memcpy (op_data, mp_uart_rx_buffer, G_RECEIVE_SIZE);
      }

      if (request.task_handle != nullptr) {
        osThreadFlagsSet (request.task_handle, 0x05);
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
    if (huart->Instance == USART2) {
      osThreadFlagsSet (s_task_handle, 0x03);
    }
  }

  void HAL_UARTEx_RxEventCallback (UART_HandleTypeDef *huart, uint16_t size) {
    if (huart->Instance == USART2) {
      G_RECEIVE_SIZE = size;
      osThreadFlagsSet (s_task_handle, 0x04);
    }
  }

  void HAL_UART_ErrorCallback (UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
      __IO uint32_t tmpreg = huart->Instance->SR;
      tmpreg = huart->Instance->DR;
      (void) tmpreg;

      // 2. Clear HAL Internal Error State
      huart->ErrorCode = HAL_UART_ERROR_NONE;
      huart->gState = HAL_UART_STATE_READY;
      huart->RxState = HAL_UART_STATE_READY;

      // 3. Unblock the RTOS task so it can abort/retry instead of timing out forever
      if (s_task_handle != nullptr) {
        osThreadFlagsSet (s_task_handle, 0x04);
      }
    }
  }
}
